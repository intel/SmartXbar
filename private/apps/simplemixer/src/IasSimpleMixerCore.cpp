/*
 * Copyright (C) 2019 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file  IasSimpleMixerCore.cpp
 * @date  2016
 * @brief The core implementation of the simple mixer module
 */

#include <malloc.h>
#include <iostream>
#include <math.h>
#include <stdio.h>
#include <sstream>
#include <string.h>
#include "audio/smartx/rtprocessingfwx/IasModuleEvent.hpp"
#include "audio/smartx/IasEventProvider.hpp"
#include "audio/smartx/rtprocessingfwx/IasIGenericAudioCompConfig.hpp"
#include "audio/smartx/rtprocessingfwx/IasAudioStream.hpp"
#include "audio/smartx/rtprocessingfwx/IasSimpleAudioStream.hpp"
#include "audio/smartx/IasProperties.hpp"
#include "audio/simplemixer/IasSimpleMixerCore.hpp"
#include "audio/simplemixer/IasSimpleMixerCmd.hpp"


DLT_IMPORT_CONTEXT(logCtxSimpleMixer);

#define LOG_ON 0

namespace IasAudio {

static const std::string cClassName = "IasSimpleMixerCore::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"

IasSimpleMixerCore::IasSimpleMixerCore(const IasIGenericAudioCompConfig *config, const std::string &componentName)
  :IasGenericAudioCompCore(config, componentName)
  ,mNumStreams(0)
  ,mDefaultGain(1.0f)
  ,mXMixerGainMap(nullptr)
  ,mInplaceGainMap(nullptr)
  ,mTypeName("")
  ,mInstanceName("")
{
}

IasSimpleMixerCore::~IasSimpleMixerCore()
{
#if LOG_ON
  DLT_LOG_CXX(logCtxSimpleMixer, DLT_LOG_INFO, LOG_PREFIX, "Deleted");
#endif
  delete mXMixerGainMap;
  delete mInplaceGainMap;
}


IasAudioProcessingResult IasSimpleMixerCore::reset()
{
#if LOG_ON
  DLT_LOG_CXX(logCtxSimpleMixer, DLT_LOG_INFO, LOG_PREFIX, "called");
#endif
  return eIasAudioProcOK;
}


IasAudioProcessingResult IasSimpleMixerCore::init()
{
#if LOG_ON
  DLT_LOG_CXX(logCtxSimpleMixer, DLT_LOG_INFO, LOG_PREFIX, "called");
#endif

  mXMixerGainMap  = new IasXMixerGainMap();
  mInplaceGainMap = new IasInplaceGainMap();

  mXMixerGainMap->clear();
  mInplaceGainMap->clear();

  // Get the configuration properties.
  const IasProperties &configurationProperties = mConfig->getProperties();

  // Retrieve typeName and instanceName from the configuration properties.
  IasProperties::IasResult result;
  result = configurationProperties.get<std::string>("typeName", &mTypeName);
  if (result != IasProperties::eIasOk)
  {
#if LOG_ON
    DLT_LOG_CXX(logCtxSimpleMixer, DLT_LOG_ERROR, LOG_PREFIX,
                "Error: cannot find key 'typeName' in module configuration properties");
#endif
    return eIasAudioProcInitializationFailed;
  }

  result = configurationProperties.get<std::string>("instanceName",  &mInstanceName);
  if (result != IasProperties::eIasOk)
  {
#if LOG_ON
    DLT_LOG_CXX(logCtxSimpleMixer, DLT_LOG_ERROR, LOG_PREFIX,
                "Error: cannot find key 'instanceName' in module configuration properties");
#endif
    return eIasAudioProcInitializationFailed;
  }

  // Retrieve defaultGain from the configuration properties.
  int32_t defaultGain;
  result = configurationProperties.get<int32_t>("defaultGain", &defaultGain);
  if (result != IasProperties::eIasOk)
  {
#if LOG_ON
    DLT_LOG_CXX(logCtxSimpleMixer, DLT_LOG_ERROR, LOG_PREFIX,
                "Error: cannot find key 'defaultGain' in module configuration properties");
#endif
    return eIasAudioProcInitializationFailed;
  }
  // Convert from 1/10 dB into linear
  if (defaultGain > -1440)
  {
    mDefaultGain = powf(10.0f, static_cast<float>(defaultGain)/200.0f);
  }
  else
  {
    mDefaultGain = 0.0f;
  }
#if LOG_ON
  DLT_LOG_CXX(logCtxSimpleMixer, DLT_LOG_INFO, LOG_PREFIX, "Setting defaultGain (linear) to", mDefaultGain);
#endif

  // Iterate over all streams with in-place processing.
  const IasStreamPointerList& streams = mConfig->getStreams();
  mNumStreams = static_cast<uint32_t>(streams.size());
#if LOG_ON
  DLT_LOG_CXX(logCtxSimpleMixer, DLT_LOG_INFO, LOG_PREFIX, "mNumStreams:", mNumStreams);
#endif
  for (IasStreamPointerList::const_iterator streamIt = streams.begin(); streamIt != streams.end(); ++streamIt)
  {
#if LOG_ON
    DLT_LOG_CXX(logCtxSimpleMixer, DLT_LOG_INFO, LOG_PREFIX,
                "in-place processing of stream:", (*streamIt)->getName(),
                "ID =", (*streamIt)->getId());
#endif

    IasInplaceGainPair *inplaceGainPair = new IasInplaceGainPair((*streamIt)->getId(), mDefaultGain);
    auto status = mInplaceGainMap->insert(*inplaceGainPair);
    if (!status.second)
    {
#if LOG_ON
      DLT_LOG_CXX(logCtxSimpleMixer, DLT_LOG_ERROR, LOG_PREFIX,
                  "Error: mapping already exists in mInplaceGainMap.");
#endif
      return eIasAudioProcInitializationFailed;
    }
  }

  // Iterate over all stream mappings.
  const IasStreamMap& streamMap = mConfig->getStreamMapping();
  for (IasStreamMap::const_iterator mapIt = streamMap.begin(); mapIt != streamMap.end(); ++mapIt)
  {
    IasAudioStream* outputStream = mapIt->first;

    // Iterate over all input streams that are mapped to this output stream.
    const IasStreamPointerList& inputStreamPointerList = mapIt->second;
    for (IasAudioStream* inputStream :inputStreamPointerList)
    {
#if LOG_ON
      DLT_LOG_CXX(logCtxSimpleMixer, DLT_LOG_INFO, LOG_PREFIX,
                  "processing of stream mapping from input stream", inputStream->getName(),
                  "ID =", inputStream->getId(),
                  "to output stream", outputStream->getName(),
                  "ID =", outputStream->getId());
#endif

      IasAudioStreamMapping streamMapping = IasAudioStreamMapping(inputStream->getId(), outputStream->getId());
      IasXMixerGainPair xMixerGainPair = IasXMixerGainPair(streamMapping, mDefaultGain);
      auto status = mXMixerGainMap->insert(xMixerGainPair);
      if (!status.second)
      {
#if LOG_ON
        DLT_LOG_CXX(logCtxSimpleMixer, DLT_LOG_ERROR, LOG_PREFIX,
                    "Error: mapping already exists in mXMixerGainMap.");
#endif
        return eIasAudioProcInitializationFailed;
      }
    }
  }

  return eIasAudioProcOK;
}



/**
 * @brief Set the new gain value.
 */
void IasSimpleMixerCore::setInPlaceGain(int32_t   streamId,
                                        float newGain)
{
#if LOG_ON
  DLT_LOG_CXX(logCtxSimpleMixer, DLT_LOG_INFO, LOG_PREFIX,
              "streamId =", streamId,
              "newGain =",  newGain);
#endif
  IasMsgQueueEntry msgQueueEntry;
  msgQueueEntry.msgId = IasMsgIds::eIasSetInPlaceGain;
  msgQueueEntry.streamId1 = streamId;
  msgQueueEntry.gain      = newGain;
  mMsgQueue.push(msgQueueEntry);
}

/**
 * @brief Set the new gain value.
 */
void IasSimpleMixerCore::setXMixerGain(int32_t    inputStreamId,
                                        int32_t   outputStreamId,
                                        float newGain)
{
#if LOG_ON
  DLT_LOG_CXX(logCtxSimpleMixer, DLT_LOG_INFO, LOG_PREFIX,
              "inputStreamId =",  inputStreamId,
              "outputStreamId =", outputStreamId,
              "newGain =",        newGain);
#endif
  IasMsgQueueEntry msgQueueEntry;
  msgQueueEntry.msgId = IasMsgIds::eIasSetXMixerGain;
  msgQueueEntry.streamId1 = inputStreamId;
  msgQueueEntry.streamId2 = outputStreamId;
  msgQueueEntry.gain      = newGain;
  mMsgQueue.push(msgQueueEntry);
}




IasAudioProcessingResult IasSimpleMixerCore::processChild()
{
  IasMsgQueueEntry msgQueueEntry;
  while (mMsgQueue.try_pop(msgQueueEntry))
  {
    switch(msgQueueEntry.msgId)
    {
      case eIasSetInPlaceGain:
      {
        auto it = mInplaceGainMap->find(msgQueueEntry.streamId1);
        if (it == mInplaceGainMap->end())
        {
#if LOG_ON
          DLT_LOG_CXX(logCtxSimpleMixer, DLT_LOG_ERROR, LOG_PREFIX,
                      "Error: streamId", msgQueueEntry.streamId1,
                      "does not exist!");
#endif
          return eIasAudioProcInvalidParam;
        }

        it->second = msgQueueEntry.gain;
        sendSimpleMixerEvent(msgQueueEntry.streamId1, msgQueueEntry.streamId1, msgQueueEntry.gain);
        break;
      }
      case eIasSetXMixerGain:
      {
        IasAudioStreamMapping streamMapping = IasAudioStreamMapping(msgQueueEntry.streamId1, msgQueueEntry.streamId2);
        auto it = mXMixerGainMap->find(streamMapping);
        if (it == mXMixerGainMap->end())
        {
#if LOG_ON
          DLT_LOG_CXX(logCtxSimpleMixer, DLT_LOG_ERROR, LOG_PREFIX,
                      "Error: mapping from streamId", msgQueueEntry.streamId1,
                      "to streamId", msgQueueEntry.streamId2,
                      "does not exist!");
#endif
          return eIasAudioProcInvalidParam;
        }

#if LOG_ON
        DLT_LOG_CXX(logCtxSimpleMixer, DLT_LOG_INFO, LOG_PREFIX,
                    "Setting gain to", msgQueueEntry.gain,
                    "for x-mixer from streamId", msgQueueEntry.streamId1,
                    "to streamId", msgQueueEntry.streamId2);
#endif

        it->second = msgQueueEntry.gain;
        sendSimpleMixerEvent(msgQueueEntry.streamId1, msgQueueEntry.streamId2, msgQueueEntry.gain);
        break;
      }
      default:
        break;
    }
  }

  // Iterate over all streams we have to process in-place.
  const IasStreamPointerList& streams = mConfig->getStreams();
  for (IasStreamPointerList::const_iterator streamIt = streams.begin(); streamIt != streams.end(); ++streamIt)
  {
#if LOG_ON
    DLT_LOG_CXX(logCtxSimpleMixer, DLT_LOG_VERBOSE, LOG_PREFIX,
                "in-place processing of stream", (*streamIt)->getName(),
                "ID =", (*streamIt)->getId());
#endif

    IasSimpleAudioStream* nonInterleaved = (*streamIt)->asNonInterleavedStream();
    IAS_ASSERT(nonInterleaved != nullptr);
    const IasAudioFrame& buffers = nonInterleaved->getAudioBuffers();

    float gain = 0.0f;
    auto it = mInplaceGainMap->find((*streamIt)->getId());
    if (it == mInplaceGainMap->end())
    {
#if LOG_ON
      DLT_LOG_CXX(logCtxSimpleMixer, DLT_LOG_ERROR, LOG_PREFIX,
                  "Error: streamId", (*streamIt)->getId(),
                  "does not exist. Gain will be set to 0.0");
#endif
      gain = 0.0f;
    }
    else
    {
      gain = it->second;
    }


    for (uint32_t chanIdx = 0; chanIdx < nonInterleaved->getNumberChannels(); ++chanIdx)
    {
      float *channel = buffers[chanIdx];
      IAS_ASSERT(channel != nullptr);
      for (uint32_t frameIdx = 0; frameIdx < mFrameLength; ++frameIdx)
      {
        *channel = *channel * gain;
        channel++;
      }
    }
  }

  // Iterate over all stream mappings.
  const IasStreamMap& streamMap = mConfig->getStreamMapping();
  for (IasStreamMap::const_iterator mapIt = streamMap.begin(); mapIt != streamMap.end(); ++mapIt)
  {
    IasAudioStream* outputStream = mapIt->first;
    IasSimpleAudioStream *nonInterleavedOutput = outputStream->asNonInterleavedStream();
    const IasAudioFrame &outputBuffers = nonInterleavedOutput->getAudioBuffers();

    // Iterate over all input streams that are mapped to this output stream.
    const IasStreamPointerList& inputStreamPointerList = mapIt->second;
    bool firstInputStream = true;
    for (IasAudioStream* inputStream :inputStreamPointerList)
    {
      float gain = 0.0f;
      IasAudioStreamMapping streamMapping = IasAudioStreamMapping(inputStream->getId(), outputStream->getId());
      auto it = mXMixerGainMap->find(streamMapping);
      if (it == mXMixerGainMap->end())
      {
#if LOG_ON
        DLT_LOG_CXX(logCtxSimpleMixer, DLT_LOG_ERROR, LOG_PREFIX,
                    "Error: mapping from streamId", inputStream->getId(),
                    "to streamId", outputStream->getId(),
                    "does not exist. Gain will be set to 0.0");
#endif
        gain = 0.0f;
      }
      else
      {
        gain = it->second;
      }

#if LOG_ON
      DLT_LOG_CXX(logCtxSimpleMixer, DLT_LOG_VERBOSE, LOG_PREFIX,
                  "processing of stream mapping from input stream", inputStream->getName(),
                  "ID =", inputStream->getId(),
                  "to output stream", outputStream->getName(),
                  "ID =", outputStream->getId(),
                  "with gain =", gain);
#endif

      IasSimpleAudioStream *nonInterleavedInput = inputStream->asNonInterleavedStream();
      const IasAudioFrame &inputBuffers = nonInterleavedInput->getAudioBuffers();
      for (uint32_t chanIdx = 0; chanIdx < nonInterleavedInput->getNumberChannels(); ++chanIdx)
      {
        float *inputChannel  = inputBuffers[chanIdx];
        float *outputChannel = outputBuffers[chanIdx];
        for (uint32_t frameIdx = 0; frameIdx < mFrameLength; ++frameIdx)
        {
          if (firstInputStream)
          {
            // Copy with gain from input stream to output stream, if this is the first input stream.
            *outputChannel = *inputChannel * gain;
          }
          else
          {
            // Overlap-add all other input streams to this output stream.
            *outputChannel = *outputChannel + *inputChannel * gain;
          }
          inputChannel++;
          outputChannel++;
        }
      }
      firstInputStream = false;
    }
  }
  return eIasAudioProcOK;
}


void IasSimpleMixerCore::sendSimpleMixerEvent(int32_t inputStreamId, int32_t outputStreamId, float currentGain)
{
#if LOG_ON
  DLT_LOG_CXX(logCtxSimpleMixer, DLT_LOG_INFO, LOG_PREFIX,
              "inputStreamId =",  inputStreamId,
              "outputStreamId =", outputStreamId,
              "currentGain =",    currentGain);
#endif

  IasProperties properties;
  properties.set<int32_t>("eventType", IasSimpleMixer::eIasGainUpdateFinished);
  properties.set<std::string>("typeName",      mTypeName);
  properties.set<std::string>("instanceName",  mInstanceName);
  std::string inputPinName;
  std::string outputPinName;
  mConfig->getPinName(inputStreamId,  inputPinName);
  mConfig->getPinName(outputStreamId, outputPinName);
  properties.set("input_pin",  inputPinName);
  properties.set("output_pin", outputPinName);
  properties.set("gain", currentGain);
  IasModuleEventPtr event = IasEventProvider::getInstance()->createModuleEvent();
  event->setProperties(properties);
  IasEventProvider::getInstance()->send(event);
}


} // namespace IasAudio
