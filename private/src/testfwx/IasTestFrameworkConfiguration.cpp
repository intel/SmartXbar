/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file   IasTestFrameworkConfiguration.cpp
 * @date   2017
 * @brief  Configuration database of test framework.
 */

#include "testfwx/IasTestFrameworkConfiguration.hpp"
#include "testfwx/IasTestFrameworkRoutingZone.hpp"
#include "testfwx/IasTestFrameworkWaveFile.hpp"
#include "model/IasPipeline.hpp"

namespace IasAudio {

static const std::string cClassName = "IasTestFrameworkConfiguration::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"


IasTestFrameworkConfiguration::IasTestFrameworkConfiguration()
  :IasConfiguration()
  ,mInputWaveFileMap()
  ,mOutputWaveFileMap()
  ,mTestFrameworkRoutingZone(nullptr)
  ,mTestFrameworkPipeline(nullptr)
  ,mLog(IasAudioLogging::registerDltContext("TFC", "Test Framework Configuration"))
{
}


IasTestFrameworkConfiguration::~IasTestFrameworkConfiguration()
{
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX);
  mInputWaveFileMap.clear();
  mOutputWaveFileMap.clear();
  deleteTestFrameworkRoutingZone();
  deleteTestFrameworkPipeline();
}


IasTestFrameworkConfiguration::IasResult IasTestFrameworkConfiguration::addWaveFile(IasAudioPinPtr audioPin, IasTestFrameworkWaveFilePtr waveFile)
{
  if (audioPin == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: audioPin is nullptr");
    return eIasNullPointer;
  }
  if (waveFile == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: waveFile is nullptr");
    return eIasNullPointer;
  }

  if (mInputWaveFileMap.count(audioPin) > 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Pin", audioPin->getParameters()->name, "is already linked with input file:",
                mInputWaveFileMap.at(audioPin)->getFileName());
    return eIasObjectAlreadyExists;
  }
  if (mOutputWaveFileMap.count(audioPin) > 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Pin", audioPin->getParameters()->name, "is already linked with output file:",
                mOutputWaveFileMap.at(audioPin)->getFileName());
    return eIasObjectAlreadyExists;
  }

  std::string waveFileName = waveFile->getFileName();

  for(auto& it : mOutputWaveFileMap)
  {
    std::string existingOutputFileName = it.second->getFileName();
    if(existingOutputFileName.compare(waveFileName) == 0)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "File", waveFileName, "is already linked with output pin", it.first->getParameters()->name);
      return eIasObjectAlreadyExists;
    }
  }

  IasAudioPin::IasPinDirection pinDirection = audioPin->getDirection();
  switch (pinDirection)
  {
    case IasAudioPin::eIasPinDirectionPipelineInput:
    {
      mInputWaveFileMap[audioPin] = waveFile;
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "File", waveFileName, "added for input pin:", audioPin->getParameters()->name);
      break;
    }
    case IasAudioPin::eIasPinDirectionPipelineOutput:
    {
      for(auto& it : mInputWaveFileMap)
      {
        std::string existingInputFileName = it.second->getFileName();
        if(existingInputFileName.compare(waveFileName) == 0)
        {
          DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "File", waveFileName, "is already linked with input pin", it.first->getParameters()->name);
          return eIasObjectAlreadyExists;
        }
      }

      mOutputWaveFileMap[audioPin] = waveFile;
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "File", waveFileName, "added for output pin:", audioPin->getParameters()->name);
      break;
    }
    default:
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid direction for pin:", audioPin->getParameters()->name);
      return eIasFailed;
    }
  }
  return eIasOk;
}


IasTestFrameworkConfiguration::IasResult IasTestFrameworkConfiguration::getWaveFile(IasAudioPinPtr audioPin, IasTestFrameworkWaveFilePtr *waveFile)
{
  if (audioPin == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: audioPin is nullptr");
    return eIasNullPointer;
  }
  if (waveFile == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: waveFile is nullptr");
    return eIasNullPointer;
  }

  IasAudioPin::IasPinDirection pinDirection = audioPin->getDirection();
  switch (pinDirection)
  {
    case IasAudioPin::eIasPinDirectionPipelineInput:
    {
      if (mInputWaveFileMap.count(audioPin) > 0)
      {
        *waveFile = mInputWaveFileMap.at(audioPin);
      }
      else
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Wave file not found for input audio pin:", audioPin->getParameters()->name);
        return eIasObjectNotFound;
      }
      break;
    }
    case IasAudioPin::eIasPinDirectionPipelineOutput:
    {
      if (mOutputWaveFileMap.count(audioPin) > 0)
      {
        *waveFile = mOutputWaveFileMap.at(audioPin);
      }
      else
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Wave file not found for output audio pin:", audioPin->getParameters()->name);
        return eIasObjectNotFound;
      }
      break;
    }
    default:
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid direction for pin:", audioPin->getParameters()->name);
      return eIasFailed;
    }
   }
  return eIasOk;
}


void IasTestFrameworkConfiguration::deleteWaveFile(IasAudioPinPtr audioPin)
{
  if (audioPin == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: audioPin is nullptr");
    return;
  }

  IasAudioPin::IasPinDirection pinDirection = audioPin->getDirection();
  switch (pinDirection)
  {
    case IasAudioPin::eIasPinDirectionPipelineInput:
    {
      if (mInputWaveFileMap.count(audioPin) > 0)
      {
        mInputWaveFileMap.erase(audioPin);
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Wave file erased for input audio pin:", audioPin->getParameters()->name);
      }
      else
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Wave file not found, nothing to delete for input audio pin:", audioPin->getParameters()->name);
      }
      break;
    }
    case IasAudioPin::eIasPinDirectionPipelineOutput:
    {
      if (mOutputWaveFileMap.count(audioPin) > 0)
      {
        mOutputWaveFileMap.erase(audioPin);
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Wave file erased for output audio pin:", audioPin->getParameters()->name);
      }
      else
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Wave file not found, nothing to delete for output audio pin:", audioPin->getParameters()->name);
      }
      break;
    }
    default:
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid direction for pin:", audioPin->getParameters()->name);
      break;
    }
  }
}


IasTestFrameworkConfiguration::IasResult IasTestFrameworkConfiguration::addTestFrameworkRoutingZone(IasTestFrameworkRoutingZonePtr routingZone)
{
  if (routingZone == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: routingZone is nullptr");
    return eIasNullPointer;
  }
  if (mTestFrameworkRoutingZone != nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Test framework routing zone already exists:", mTestFrameworkRoutingZone->getParameters()->name);
    return eIasObjectAlreadyExists;
  }

  mTestFrameworkRoutingZone = routingZone;
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Test framework routing zone added:", routingZone->getParameters()->name);

  return eIasOk;
}


void IasTestFrameworkConfiguration::deleteTestFrameworkRoutingZone()
{
  if (mTestFrameworkRoutingZone != nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Deleting test framework routing zone:", mTestFrameworkRoutingZone->getParameters()->name);
    mTestFrameworkRoutingZone.reset();
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Test framework routing zone not added, nothing to delete");
  }
}


IasTestFrameworkConfiguration::IasResult IasTestFrameworkConfiguration::addTestFrameworkPipeline(IasPipelinePtr pipeline)
{
  if (pipeline == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: pipeline is nullptr");
    return eIasNullPointer;
  }
  if (mTestFrameworkPipeline != nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Pipeline already exists:", mTestFrameworkPipeline->getParameters()->name);
    return eIasObjectAlreadyExists;
  }

  mTestFrameworkPipeline = pipeline;
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Pipeline added:", pipeline->getParameters()->name);

  return eIasOk;
}


void IasTestFrameworkConfiguration::deleteTestFrameworkPipeline()
{
  if (mTestFrameworkPipeline != nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Deleting pipeline:", mTestFrameworkPipeline->getParameters()->name);
    mTestFrameworkPipeline.reset();
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Pipeline not added, nothing to delete");
  }
}


} // namespace IasAudio
