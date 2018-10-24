/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file   IasConfigSinkDevice.cpp
 * @date   2017
 * @brief  The XML Parser.
 */

#include "configparser/IasConfigSinkDevice.hpp"
#include "configparser/IasParseHelper.hpp"
#include "configparser/IasSmartXTypeConversion.hpp"
#include "model/IasAudioDevice.hpp"

using namespace std;

namespace IasAudio {

static const std::string cClassName = "IasConfigSinkDevice::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"


IasConfigSinkDevice::IasConfigSinkDevice(std::int32_t numPorts)
  :mNumSinkPorts(numPorts)
  ,mSinkPortCounter(0)
  ,mRoutingZone(nullptr)
  ,mNumZonePorts(numPorts)
  ,mZonePortCounter(0)
  ,mLog(IasAudioLogging::registerDltContext("XMLP", "XML PARSER IASCONFIGSINKDEVICE"))
{

}

IasConfigSinkDevice::~IasConfigSinkDevice()
{
}

IasConfigSinkDevice::IasResult IasConfigSinkDevice::setSinkParams(xmlNodePtr sinkDeviceNode)
{
  if (sinkDeviceNode == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: sinkDeviceNode == nullptr");
    return IasResult::eIasFailed;
  }
  mSinkParams.name = getXmlAttributePtr(sinkDeviceNode, "name");
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Sink device name: ", mSinkParams.name);

  //check if smartx plugin
  if (mSinkParams.name.find("smartx:") != std::string::npos)
  {
    size_t pos = mSinkParams.name.find("smartx:");
    mSinkParams.name = mSinkParams.name.substr(pos+7);
  }


  std::string clock_type = getXmlAttributePtr(sinkDeviceNode, "clock_type");
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "clockType: ", clock_type);
  mSinkParams.clockType = toClockType(clock_type);

  std::string data_format = getXmlAttributePtr(sinkDeviceNode, "data_format");
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "data_format: ", data_format);
  mSinkParams.dataFormat = toDataFormat(data_format);

  try
  {
    mSinkParams.numPeriodsAsrcBuffer = stoi(getXmlAttributePtr(sinkDeviceNode, "asrc_period_count"));
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "numPeriodAsrcBuff: ", mSinkParams.numPeriodsAsrcBuffer);
    if (mSinkParams.numPeriodsAsrcBuffer  < 4)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid condition: mSinkParams.numPeriodsAsrcBuffer < 4");
      return IasResult::eIasFailed;
    }

    mSinkParams.numChannels = stoi(getXmlAttributePtr(sinkDeviceNode, "num_channels"));
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "num_channels: ", mSinkParams.numChannels);
    mSinkParams.numPeriods = stoi(getXmlAttributePtr(sinkDeviceNode, "num_period"));
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "num_period: ", mSinkParams.numPeriods);
    mSinkParams.periodSize = stoi(getXmlAttributePtr(sinkDeviceNode, "period_size"));
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "period_size: ", mSinkParams.periodSize);
    mSinkParams.samplerate = stoi(getXmlAttributePtr(sinkDeviceNode, "sample_rate"));
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "sample_rate: ", mSinkParams.samplerate);
  }
  catch(...)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error in setSinkParams");
    return IasResult::eIasFailed;
  }
  return IasResult::eIasOk;
}



IasConfigSinkDevice::IasResult   IasConfigSinkDevice::setSinkPortParams(IasISetup *setup, xmlNodePtr sinkDevicePortNode)
{
  if (sinkDevicePortNode == nullptr || setup == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: sinkDevicePortNode == nullptr || setup == nullptr");
    return IasResult::eIasFailed;
  }

  IasAudioPortParams sinkPortParam;
  IasISetup::IasResult result;

  sinkPortParam.name = getXmlAttributePtr(sinkDevicePortNode, "name");
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Sink device port name: ", sinkPortParam.name);

  try
  {
    sinkPortParam.numChannels = stoi(getXmlAttributePtr(sinkDevicePortNode, "channel_count"));
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "channel_count: ", sinkPortParam.numChannels);
    sinkPortParam.id = stoi(getXmlAttributePtr(sinkDevicePortNode, "id"));
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "id : ", sinkPortParam.id);
    sinkPortParam.direction = IasAudio::eIasPortDirectionInput;
    sinkPortParam.index = stoi(getXmlAttributePtr(sinkDevicePortNode, "channel_index"));
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "channel_index : ", sinkPortParam.index);
  }
  catch(...)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error in setSinkPortParams");
    return IasResult::eIasFailed;
  }

  IasAudioPortPtr port = nullptr;
  mSinkPortParams.push_back(sinkPortParam);
  mSinkPort.push_back(port);

  //Create sink port
  result = setup->createAudioPort(sinkPortParam, &(mSinkPort.at(mSinkPortCounter))); //
  if (result != IasISetup::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error creating sink audio port: ", sinkPortParam.name, ": ",  toString(result));
    return IasResult::eIasFailed;
  }

  //Add sink port
  result = setup->addAudioInputPort(mSinkDevicePtr, mSinkPort.at(mSinkPortCounter));
  if (result != IasISetup::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error adding sink audio port: ", sinkPortParam.name, ": ",  toString(result));
    return IasResult::eIasFailed;
  }

  return IasResult::eIasOk;
}


IasConfigSinkDevice::IasResult IasConfigSinkDevice::createLinkRoutingZone(IasISetup *setup)
{
  if (setup == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "setup == nullptr");
    return IasResult::eIasFailed;
  }
  //Create routing zone
  IasISetup::IasResult result = setup->createRoutingZone(mRoutingZoneParams, &mRoutingZone);
  if (result != IasISetup::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error creating routing zone: ", mRoutingZoneParams.name, ": ",  toString(result));
    return IasResult::eIasFailed;
  }

  // Link the routing zone with the sink
  result = setup->link(mRoutingZone, mSinkDevicePtr);
  if (result != IasISetup::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error linking routing zone to sink: ", mRoutingZoneParams.name, ": ",  toString(result));
    return IasResult::eIasFailed;
  }

  return IasResult::eIasOk;
}


IasConfigSinkDevice::IasResult IasConfigSinkDevice::setZonePortParams(IasISetup *setup, xmlNodePtr routingZonePortNode, xmlNodePtr setupLinksNode)
{
  if (setup == nullptr || routingZonePortNode == nullptr || setupLinksNode == nullptr )
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "setup == nullptr || routingZonePortNode == nullptr || setupLinksNode == nullptr");
    return IasResult::eIasFailed;
  }

  IasAudioPortParams zonePortParam;
  zonePortParam.name = getXmlAttributePtr(routingZonePortNode, "name");
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "ZonePort name : ", zonePortParam.name);

  try
  {
    zonePortParam.numChannels = static_cast<unsigned int>(stoi(getXmlAttributePtr(routingZonePortNode, "channel_count")));
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "ZonePort channel_count : ", zonePortParam.numChannels);
    zonePortParam.id = stoi(getXmlAttributePtr(routingZonePortNode, "id"));
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "ZonePort id : ", zonePortParam.id);
    zonePortParam.direction = IasAudio::eIasPortDirectionInput;
    zonePortParam.index = static_cast<unsigned int>(stoi(getXmlAttributePtr(routingZonePortNode, "channel_index")));
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "channel_index id : ", zonePortParam.index);
  }
  catch(...)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error in setZonePortParams");
    return IasResult::eIasFailed;
  }

  IasAudioPortPtr zonePort = nullptr;
  mZonePorts.push_back(zonePort);
  mZonePortParams.push_back(zonePortParam);


  //Create zone port
  IasISetup::IasResult result = setup->createAudioPort(zonePortParam, &(mZonePorts.at(mZonePortCounter)));
  if (result != IasISetup::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error creating routing zone audio port: ", mRoutingZoneParams.name, ": ",  toString(result));
    return IasResult::eIasFailed;
  }

  //Add zone port
  result = setup->addAudioInputPort(mRoutingZone, mZonePorts.at(mZonePortCounter));
  if (result != IasISetup::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error adding zone port: ", mRoutingZoneParams.name, ": ",  toString(result));
    return IasResult::eIasFailed;
  }

  //Link zone port to sink device port
  xmlNodePtr cur_setup_port = nullptr;
  for (cur_setup_port = setupLinksNode->children; cur_setup_port != nullptr ; cur_setup_port = cur_setup_port->next)
  {
    if (!xmlStrcmp(cur_setup_port->name, (const xmlChar *)"SetupLink"))
    {
      if (getXmlAttributePtr(cur_setup_port, "rz_input_port") == getXmlAttributePtr(routingZonePortNode, "name"))
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Link Setup rz_input_port: ", getXmlAttributePtr(cur_setup_port, "rz_input_port"));

        //for loop sink ports to match the name
        for (int i=0; i < mNumSinkPorts; i++)
        {
          // Added in method setSinkPortParams, which is always called before calling this method
          IAS_ASSERT(mSinkPort.at(i) != nullptr);
          IasAudioPortParamsPtr portParams = mSinkPort.at(i)->getParameters();
          // Sink ports are created in method setSinkPortParams. If that method fails, we don't reach
          // this code section
          IAS_ASSERT(portParams != nullptr);

          if(getXmlAttributePtr(cur_setup_port, "sink_input_port") == portParams->name)
          {
            DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Matched Sink Port Name : ", portParams->name);
            // Link the routing zone port with the sink port
            result = setup->link(mZonePorts.at(mZonePortCounter), mSinkPort.at(i));
            if (result != IasISetup::eIasOk)
            {
              DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Linking routing zone port to sink port Failed");
              return IasResult::eIasFailed;
            }

            return IasResult::eIasOk;
          }
        }//sink input port not matched. Move to next node...
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "No sink port matched to the routing zone");
        return  IasResult::eIasFailed;
      }
      //routing zone not matched. Move to next node...
    }
    else
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "SetupLinks does not contain SetupLink tag");
      return  IasResult::eIasFailed;
    }
  }//traversed all link zone ports

  return IasResult::eIasOk;
}



IasAudioDeviceParams IasConfigSinkDevice::getSinkDeviceParams() const
{
  return mSinkParams;
}

std::vector<IasAudioPortParams>  IasConfigSinkDevice::getSinkPortParams()
{
  return mSinkPortParams;
}

void IasConfigSinkDevice::setSinkDevicePtr(IasAudioSinkDevicePtr sinkDevicePtr)
{
  mSinkDevicePtr = sinkDevicePtr;
}

IasRoutingZoneParams IasConfigSinkDevice::getRoutingZoneParams() const
{
  return mRoutingZoneParams;
}

IasRoutingZonePtr IasConfigSinkDevice::getMyRoutingZone() const
{
  return mRoutingZone;
}

std::vector<IasAudioPortPtr> IasConfigSinkDevice::getMyRoutingZonePorts()
{
  return mZonePorts;
}

std::vector<IasAudioPortPtr> IasConfigSinkDevice::getSinkPorts()
{
  return mSinkPort;
}

void IasConfigSinkDevice::incrementSinkPortCounter()
{
  ++mSinkPortCounter;
}

void IasConfigSinkDevice::incrementZonePortCounter()
{
  ++mZonePortCounter;
}

void IasConfigSinkDevice::setRoutingZoneParamsName(std::string name)
{
  mRoutingZoneParams.name = name;
}

std::vector<IasAudioPortParams> IasConfigSinkDevice::getMyRoutingZonePortParams()
{
  return mZonePortParams;
}

}//end namespace IASAUDIO





