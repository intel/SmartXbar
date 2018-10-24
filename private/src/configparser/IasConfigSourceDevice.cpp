/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file   IasConfigSourceDevice.cpp
 * @date   2017
 * @brief  The XML Parser.
 */

#include "configparser/IasConfigSourceDevice.hpp"
#include "configparser/IasParseHelper.hpp"
#include "configparser/IasSmartXTypeConversion.hpp"
#include "model/IasAudioDevice.hpp"

namespace IasAudio {

static const std::string cClassName = "IasConfigSourceDevice::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"

IasConfigSourceDevice::IasConfigSourceDevice(std::int32_t numPorts)
  :mNumSourcePorts(numPorts)
  ,mSourcePortCounter(0)
  ,mLog(IasAudioLogging::registerDltContext("XMLP", "XML PARSER IASCONFIGSOURCEDEVICE"))
{
}

IasConfigSourceDevice::~IasConfigSourceDevice()
{
}

IasConfigSourceDevice::IasResult   IasConfigSourceDevice::setSourceParams(xmlNodePtr sourceDeviceNode)
{
  if (sourceDeviceNode == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid condition: sourceDeviceNode == nullptr");
    return IasResult::eIasFailed;
  }

  mSourceParams.name = getXmlAttributePtr(sourceDeviceNode, "name");
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Source device name: ", mSourceParams.name);

  //check if smartx plugin
  if (mSourceParams.name.find("smartx:") != std::string::npos)
  {
    size_t pos = mSourceParams.name.find("smartx:");
    mSourceParams.name = mSourceParams.name.substr(pos+7);
  }

  std::string clock_type = getXmlAttributePtr(sourceDeviceNode, "clock_type");
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Source device clockType : ", clock_type);
  mSourceParams.clockType = toClockType(clock_type);

  std::string data_format = getXmlAttributePtr(sourceDeviceNode, "data_format");
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Source device data_format : ", data_format);
  mSourceParams.dataFormat = toDataFormat(data_format);

  try
  {
    mSourceParams.numPeriodsAsrcBuffer = stoui(getXmlAttributePtr(sourceDeviceNode, "asrc_period_count"));
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Source device numPeriodsAsrcBuffer : ", mSourceParams.numPeriodsAsrcBuffer);
    if ( mSourceParams.numPeriodsAsrcBuffer < 4)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid condition: mSourceParams.numPeriodsAsrcBuffer < 4");
      return IasResult::eIasFailed;
    }
    mSourceParams.numChannels = stoui(getXmlAttributePtr(sourceDeviceNode, "num_channels"));
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Source device num_channels : ", mSourceParams.numChannels);
    mSourceParams.numPeriods = stoui(getXmlAttributePtr(sourceDeviceNode, "num_period"));
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Source device num_period : ", mSourceParams.numPeriods);
    mSourceParams.periodSize = stoui(getXmlAttributePtr(sourceDeviceNode, "period_size"));
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Source device period_size : ", mSourceParams.periodSize);
    mSourceParams.samplerate = stoui(getXmlAttributePtr(sourceDeviceNode, "sample_rate"));
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Source device sample_rate : ", mSourceParams.samplerate);
  }
  catch(...)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error in setSourceParams");
    return  IasResult::eIasFailed;
  }

  return IasResult::eIasOk;
}

IasConfigSourceDevice::IasResult   IasConfigSourceDevice::setSourcePortParams(IasISetup *setup, xmlNodePtr sourceDevicePortNode)
{
  if (sourceDevicePortNode == nullptr || setup == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid condition: sourceDevicePortNode == nullptr || setup == nullptr");
    return IasResult::eIasFailed;
  }

  IasISetup::IasResult result;

  mSourcePortParams.name = getXmlAttributePtr(sourceDevicePortNode, "name");
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Source Device Port name: ", mSourcePortParams.name);

  try
  {
    mSourcePortParams.numChannels = stoui(getXmlAttributePtr(sourceDevicePortNode, "channel_count"));
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Source Device Port channel_count: ", mSourcePortParams.numChannels);
    mSourcePortParams.id = stoui(getXmlAttributePtr(sourceDevicePortNode, "id"));
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Source Device Port id: ", mSourcePortParams.id);
    mSourcePortParams.direction = IasAudio::eIasPortDirectionOutput;
    mSourcePortParams.index = stoui(getXmlAttributePtr(sourceDevicePortNode, "channel_index"));
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Source Device Port channel_index: ", mSourcePortParams.index);
  }
  catch(...)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error in setSourcePortParams");
    return  IasResult::eIasFailed;
  }

  IasAudioPortPtr sourcePort = nullptr;
  mSourcePortVec.push_back(sourcePort);

  //Create source port
  result = setup->createAudioPort(mSourcePortParams, &(mSourcePortVec.at(mSourcePortCounter)));
  if (result != IasISetup::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error creating source audio port: ", mSourcePortParams.name, ": ",  toString(result));
    return IasResult::eIasFailed;
  }

  //Add source port
  result = setup->addAudioOutputPort(mSourceDevicePtr, mSourcePortVec.at(mSourcePortCounter));
  if (result != IasISetup::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error adding source output port: ", mSourcePortParams.name, ": ",  toString(result));
    return IasResult::eIasFailed;
  }

  return IasResult::eIasOk;
}


IasAudioDeviceParams IasConfigSourceDevice::getSourceDeviceParams() const
{
  return mSourceParams;
}

void IasConfigSourceDevice::incrementSourcePortCounter()
{
  ++mSourcePortCounter;
}


void IasConfigSourceDevice::setSourceDevicePtr(IasAudioSourceDevicePtr sourceDevicePtr)
{
  mSourceDevicePtr = sourceDevicePtr;
}

}
