/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 *  @file   IasMixerCore.cpp
 *  @date   August 2016
 *  @brief  The mixer core consists of one or several elementary mixers.
 *
 *  Therefore, we need only one mixer core within an IVI system, which
 *  is able to generate all required output streams (output zones).
 */

#include <cstring>

#include "mixer/IasMixerCore.hpp"
#include "mixer/IasMixerElementary.hpp"
#include "audio/smartx/rtprocessingfwx/IasAudioStream.hpp"
#include "audio/smartx/rtprocessingfwx/IasIGenericAudioCompConfig.hpp"
#include "rtprocessingfwx/IasBundleAssignment.hpp"
#include "rtprocessingfwx/IasAudioChannelBundle.hpp"
#include "audio/smartx/rtprocessingfwx/IasProcessingTypes.hpp"

namespace IasAudio {

static const std::string cClassName = "IasMixerCore::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"

IasMixerCore::IasMixerCore(const IasIGenericAudioCompConfig* config, const std::string& componentName)
  :IasGenericAudioCompCore(config, componentName)
  ,mElementaryMixers{}
  ,mCoreStreamMap{}
  ,mOutputBundlesList{}
  ,mLogContext{IasAudioLogging::getDltContext("_MIX")}
{
}

IasMixerCore::~IasMixerCore()
{
  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "Deleted");
}

IasAudioProcessingResult IasMixerCore::reset()
{
  return eIasAudioProcOK;
}

IasAudioProcessingResult IasMixerCore::init()
{
  // Loop over all stream mappings. Since each stream mapping corresponds
  // to one output stream (output zone), we create an own elementary mixer
  // for each stream mapping (i.e., for each output zone).

  const IasStreamMap& streamMap = mConfig->getStreamMapping();

  mElementaryMixers.reserve(streamMap.size());

  for(const auto& streamMapIt : streamMap)
  {
    // Create the elementary mixer for this output stream.
    auto newElementaryMixer = std::make_shared<IasMixerElementary>(mConfig, streamMapIt, mFrameLength, mSampleRate);

    if(newElementaryMixer->init())
    {
      DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "error while calling IasMixerElementary::init()");
      return eIasAudioProcInitializationFailed;
    }
    if(newElementaryMixer->setupModeMain())
    {
      DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "error while calling IasMixerElementary::setupModeMain()");
      return eIasAudioProcInitializationFailed;
    }
    mElementaryMixers.push_back(newElementaryMixer);

    // Loop over all input streams that contribute to this output stream.
    IasStreamPointerList streamList = streamMapIt.second;
    for(IasStreamPointerList::iterator streamIt = streamList.begin(); streamIt!= streamList.end();++streamIt)
    {
      // Insert the input stream ID and the handle of the elementary mixer
      // into the mCoreStreamMap. Therefore, the mCoreStreamMap specifies for
      // each input stream, which elementary mixer it is associated with.
      int32_t id = (*streamIt)->getId();

      mCoreStreamMap.emplace(id, newElementaryMixer);
      int32_t outputId = streamMapIt.first->getId();
      DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "Successfully added input stream with ID", id, "to new elementary mixer for output stream with ID", outputId);
    }

    // Add all bundles that belong to this output stream to the mOutputBundlesList.
    const IasBundledAudioStream* outputStream = streamMapIt.first->asBundledStream();
    const auto& bundleAssignmentVector = outputStream->getBundleAssignments();

    for(const auto& bundleAssignment: bundleAssignmentVector)
    {
      mOutputBundlesList.push_back(bundleAssignment.getBundle());
    }
  }

  // Sort the list of all output bundles and remove all bundles that occur more than once.
  mOutputBundlesList.sort();
  mOutputBundlesList.unique();

  return eIasAudioProcOK;
}


IasAudioProcessingResult IasMixerCore::processChild()
{
  // ======================================================================
  // The mixer expects that all output bundles have been cleared by the
  // rtprocessingfw, before the audio chain is executed. This is essential,
  // because all elementary mixers only add their new samples to the content
  // of the existing output buffers. This is due to the bundle-based
  // processing of the elementary mixers.
  //
  // In an earlier release, the mixer itself has cleared the output bundles,
  // but this now conflicts with the downmixer, if both modules (mixer and
  // downmixer) jointly operate on a shared output bundle.
  // ======================================================================

  // This is required to convert the streams to the bundled format
  const IasStreamMap& streamMap = mConfig->getStreamMapping();
  IasStreamMap::const_iterator streamMapIt;
  for (streamMapIt = streamMap.begin(); streamMapIt != streamMap.end(); ++streamMapIt)
  {
    const IasStreamPointerList &inputStreams = (*streamMapIt).second;
    IasStreamPointerList::const_iterator streamIt;
    for (streamIt = inputStreams.begin(); streamIt != inputStreams.end(); ++streamIt)
    {
      // We don't need the pointer in this component, but just to trigger the conversion
      (void)(*streamIt)->asBundledStream();
    }
    // Take care that the output stream is in 'bundled' representation.
    // This is essential for output zones that do not have any processing
    // modules after the mixer, e.g., in case of the BT UlProcessed stream.
    (void)(*streamMapIt).first->asBundledStream();
  }

  // Execute all elementary mixers (one elementary mixer for each output stream).
  for(auto& elementaryMixer : mElementaryMixers)
  {
    elementaryMixer->run();
  }

  return eIasAudioProcOK;
}

IasAudioProcessingResult IasMixerCore::setBalance(int32_t streamId, float balanceLeft, float balanceRight)
{
  IasAudioProcessingResult result = eIasAudioProcOK;
  auto iterator = mCoreStreamMap.equal_range(streamId);
  if(iterator.first == iterator.second)
  {
    // Invalid streamId
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "invalid streamId: streamId=", streamId);
    return eIasAudioProcInvalidParam;
  }

  for (auto& entry=iterator.first; entry!=iterator.second; ++entry)
  {
    auto mixer = entry->second;
    result = mixer->setBalance(streamId, balanceLeft, balanceRight);
    if(result != eIasAudioProcOK)
    {
      DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "error while calling IasMixerElementary::setBalance()");      break;
    }
  }

  return result;
}

IasAudioProcessingResult IasMixerCore::setFader(int32_t streamId, float faderFront, float faderRear)
{
  IasAudioProcessingResult result = eIasAudioProcOK;
  auto iterator = mCoreStreamMap.equal_range(streamId);
  if(iterator.first == iterator.second)
  {
    // Invalid streamId
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "invalid streamId: streamId=", streamId);
    return eIasAudioProcInvalidParam;
  }

  for(auto& entry=iterator.first; entry!=iterator.second; ++entry)
  {
    auto mixer = entry->second;
    result = mixer->setFader(streamId, faderFront, faderRear);
    if(result != eIasAudioProcOK)
    {
      DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "error while calling IasMixerElementary::setFader()");
      break;
    }
  }

  return result;
}

IasAudioProcessingResult IasMixerCore::setInputGainOffset(int32_t streamId, float gain)
{
  IasAudioProcessingResult result = eIasAudioProcOK;
  auto iterator = mCoreStreamMap.equal_range(streamId);
  if(iterator.first == iterator.second)
  {
    // Invalid streamId
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "invalid streamId: streamId=", streamId);
    return eIasAudioProcInvalidParam;
  }

  for(auto& entry=iterator.first; entry!=iterator.second; ++entry)
  {
    auto mixer = entry->second;
    result = mixer->setInputGainOffset(streamId, gain);
    if(result != eIasAudioProcOK)
    {
      DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "error while calling IasMixerElementary::setInputGainOffset()");
      break;
    }
  }
  return result;
}


} // namespace IasAudio
