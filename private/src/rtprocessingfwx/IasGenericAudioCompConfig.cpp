/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasGenericAudioCompConfig.cpp
 * @date   2012
 * @brief  This is the implementation of the IasGenericAudioCompConfig class.
 */

#include <cstdarg>

#include "audio/smartx/rtprocessingfwx/IasAudioStream.hpp"
#include "rtprocessingfwx/IasGenericAudioCompConfig.hpp"
#include "rtprocessingfwx/IasAudioChannelBundle.hpp"
#include "rtprocessingfwx/IasBundleAssignment.hpp"
#include "rtprocessingfwx/IasStreamParams.hpp"
#include "audio/smartx/rtprocessingfwx/IasBundledAudioStream.hpp"

namespace IasAudio {

static const std::string cClassName = "IasGenericAudioCompConfig::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"

IasGenericAudioCompConfig::IasGenericAudioCompConfig()
  :mActiveStreamPointers()
  ,mActiveBundlePointers()
  ,mActiveAudioFrame()
  ,mStreamMap()
  ,mStreamParams()
  ,mProperties()
  ,mModuleId(-1)
  ,mPinNameStreamIdMap()
  ,mLogContext(IasAudioLogging::getDltContext("PFW"))
{
}

IasGenericAudioCompConfig::~IasGenericAudioCompConfig()
{
  // The following vectors only need to be cleared because the pointers contained are managed somewhere else.
  mActiveAudioFrame.clear();
  mActiveBundlePointers.clear();
  mActiveStreamPointers.clear();
  mStreamMap.clear();

  IasStreamParamsMultimap::iterator streamIt;
  for (streamIt=mStreamParams.begin(); streamIt!=mStreamParams.end(); ++streamIt)
  {
    delete (*streamIt).second;
  }
  mStreamParams.clear();
  mPinNameStreamIdMap.clear();
}

void IasGenericAudioCompConfig::addStreamToProcess(IasAudioStream *streamToProcess, const std::string &pinName)
{
  IasStreamPointerList::iterator streamIt;
  bool found = false;
  for (streamIt=mActiveStreamPointers.begin(); streamIt!=mActiveStreamPointers.end(); ++streamIt)
  {
    if (*streamIt == streamToProcess)
    {
      found = true;
      DLT_LOG_CXX(*mLogContext, DLT_LOG_WARN, LOG_PREFIX, "Stream", streamToProcess->getName().c_str(),
                      "already added");
      break;
    }
  }
  if (!found)
  {
    const IasBundledAudioStream *bundledStream = streamToProcess->asBundledStream();
    if (bundledStream == nullptr)
    {
      DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "Stream", streamToProcess->getName().c_str(), "couldn't be added due to incorrect configuration. See previous error logs for details.");
      return;
    }
    // Stream was not added yet
    mActiveStreamPointers.push_back(streamToProcess);
    mPinNameStreamIdMap[pinName] = streamToProcess->getId();
    mStreamIdPinNameMap[streamToProcess->getId()] = pinName;

    IasBundleAssignmentVector::const_iterator bundleIterator;
    const IasBundleAssignmentVector &bundleAssignmentVector = bundledStream->getBundleAssignments();
    for (auto bundleAssignment: bundleAssignmentVector)
    {
      IasAudioChannelBundle *bundle = bundleAssignment.getBundle();
      IasBundlePointerList::iterator activeBundleIt;
      found = false;
      uint32_t bundleIndex = 0;
      for (activeBundleIt=mActiveBundlePointers.begin(); activeBundleIt!=mActiveBundlePointers.end(); ++activeBundleIt)
      {
        if (*activeBundleIt == bundle)
        {
          found = true;
          break;
        }
        bundleIndex++;
      }
      if (!found)
      {
        mActiveBundlePointers.push_back(bundle);
        mActiveAudioFrame.push_back(bundle->getAudioDataPointer());
      }
      uint32_t channelIndex = bundleAssignment.getIndex();
      uint32_t numberChannels = bundleAssignment.getNumberChannels();
      IAS_ASSERT(bundledStream != NULL); // to keep Klocwork happy, already asserted some lines above.
      int32_t streamId = bundledStream->getId();
      IasStreamParams *newStreamParams = new IasStreamParams(bundleIndex, channelIndex, numberChannels);
      IAS_ASSERT(newStreamParams != nullptr);
      mStreamParams.insert(IasStreamParamsPair(streamId, newStreamParams));
    }
    DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "Added stream", streamToProcess->getName().c_str());
  }
}

void IasGenericAudioCompConfig::addStreamMapping(IasAudioStream *inputStream, const std::string &inputPinName,
                                                 IasAudioStream *outputStream, const std::string &outputPinName)
{
  bool found = false;
  IasStreamMap::iterator outStreamIt = mStreamMap.find(outputStream);
  if (outStreamIt == mStreamMap.end())
  {
    // Output stream is not available in the list yet
    IasStreamPointerList newStreamList;
    newStreamList.push_back(inputStream);
    IasStreamPair newStreamPair(outputStream, newStreamList);
    mStreamMap.insert(newStreamPair);
    mPinNameStreamIdMap[outputPinName] = outputStream->getId();
    mStreamIdPinNameMap[outputStream->getId()] = outputPinName;
    mPinNameStreamIdMap[inputPinName] = inputStream->getId();
    mStreamIdPinNameMap[inputStream->getId()] = inputPinName;
    DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "New stream mapping",
                    inputStream->getName().c_str(), "->", outputStream->getName().c_str(),
                    "added");
  }
  else
  {
    // There is already a mapping for the output stream available
    IasStreamPointerList *streamList = &(*outStreamIt).second;
    IasStreamPointerList::iterator streamIt;
    found = false;
    for (streamIt=streamList->begin(); streamIt!=streamList->end(); ++streamIt)
    {
      if (*streamIt == inputStream)
      {
        DLT_LOG_CXX(*mLogContext, DLT_LOG_WARN, LOG_PREFIX, "Stream mapping",
                        inputStream->getName().c_str(), "->", outputStream->getName().c_str(),
                        "already exists");
        found = true;
        break;
      }
    }
    if (!found)
    {
      streamList->push_back(inputStream);
      mPinNameStreamIdMap[inputPinName] = inputStream->getId();
      mStreamIdPinNameMap[inputStream->getId()] = inputPinName;
      DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "Added stream mapping",
                      inputStream->getName().c_str(), "->", outputStream->getName().c_str(),
                      "to existing output stream mapping");
    }
  }
}

IasAudioProcessingResult IasGenericAudioCompConfig::getStreamId(const std::string &pinName, int32_t &streamId) const
{
  auto result = mPinNameStreamIdMap.find(pinName);
  if (result != mPinNameStreamIdMap.end())
  {
    streamId = result->second;
    DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "Got stream ID", result->second, "for pin", pinName);
    return eIasAudioProcOK;
  }
  else
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "Stream ID for pin", pinName, "not found");
    return eIasAudioProcInvalidParam;
  }
}

IasAudioProcessingResult IasGenericAudioCompConfig::getPinName(int32_t streamId, std::string &pinName) const
{
  auto result = mStreamIdPinNameMap.find(streamId);
  if (result != mStreamIdPinNameMap.end())
  {
    pinName = result->second;
    DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "Got pin name", result->second, "for stream ID", streamId);
    return eIasAudioProcOK;
  }
  else
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "Pin name for stream ID", streamId, "not found");
    return eIasAudioProcInvalidParam;
  }
}

IasAudioProcessingResult IasGenericAudioCompConfig::getStreamById(IasAudioStream** stream, int32_t id) const
{
  IasAudioProcessingResult res = eIasAudioProcOK;
  if (stream == nullptr)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "null pointer provided as parameter");
    return eIasAudioProcInvalidParam;
  }
  *stream = nullptr;
  for( auto &entry : mActiveStreamPointers)
  {
    if (entry->getId() == id)
    {
      *stream = entry;
      break;
    }
  }
  if (*stream == nullptr)
  {
    for (auto &entry : mStreamMap)
    {
      DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "mStreamMap entry first:", entry.first->getId());
      if (entry.first->getId() == id)
      {
        *stream = entry.first;
        break;
      }
      else
      {
        for (auto &li : entry.second)
        {
          DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "mStreamMap entry second:", li->getId());
          if (li->getId() == id)
          {
            *stream = li;
            break;
          }
        }
        if (*stream != nullptr)
        {
          break;
        }
      }
    }
  }
  if(*stream == nullptr)
  {
    res = eIasAudioProcInvalidStream;
  }
 return res;
}

} // namespace IasAudio
