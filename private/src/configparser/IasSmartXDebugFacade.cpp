/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasSmartXDebugFacade.cpp
 * @date   2017
 * @brief
 */

#include "audio/configparser/IasSmartXDebugFacade.hpp"
#include "avbaudiomodules/internal/audio/common/IasAudioLogging.hpp"
#include "audio/smartx/IasSetupHelper.hpp"
#include "audio/smartx/IasIRouting.hpp"
#include "audio/smartx/IasIProcessing.hpp"
#include "audio/smartx/rtprocessingfwx/IasIModuleId.hpp"
#include "smartx/IasSetupImpl.hpp"
#include "model/IasAudioSinkDevice.hpp"
#include "model/IasAudioSourceDevice.hpp"
#include "model/IasAudioPort.hpp"
#include "model/IasAudioPin.hpp"
#include "model/IasAudioPortOwner.hpp"
#include "model/IasRoutingZone.hpp"
#include "model/IasPipeline.hpp"
#include "model/IasProcessingModule.hpp"
#include "audio/smartx/IasProperties.hpp"
#include "smartx/IasConfiguration.hpp"
#include "configparser/IasSmartXTypeConversion.hpp"

#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
#include <libxml2/libxml/parser.h>

#include <iostream>
#include <algorithm>

#include <iostream>
#include "diagnostic/IasDiagnostic.hpp"

#define MY_ENCODING "UTF-8"

using namespace std;

namespace IasAudio {

static const std::string cClassName = "IasSmartXDebugFacade::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"

IasSmartXDebugFacade::IasSmartXDebugFacade(IasSmartX *&smartx)
  :mSmartx{smartx}
  ,mLog(IasAudioLogging::registerDltContext("SDF", "SmartXDebugFacade"))
{
}

IasSmartXDebugFacade::~IasSmartXDebugFacade()
{

}

IasSmartXDebugFacade::IasResult IasSmartXDebugFacade::getSmartxSources(xmlNodePtr root_node, IasISetup *setup)
{
  //create Sources node
  xmlNodePtr sources = xmlNewNode(NULL,BAD_CAST"Sources");
  if (sources == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't create a new node: xmlNewNode returned nullptr");
    return eIasFailed;
  }
  xmlNodePtr res = xmlAddChild(root_node,sources);
  if (res == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't add child node: xmlAddChild returned nullptr");
    return eIasFailed;
  }

  IasAudioSourceDevicePtr sourceDevicePtr;
  IasAudioSourceDeviceVector sourceDevices = setup->getAudioSourceDevices();

  for (auto it = sourceDevices.begin(); it != sourceDevices.end(); it++)
  {
    sourceDevicePtr = *it;
    //create source node
    xmlNodePtr source = xmlNewNode(NULL,BAD_CAST"Source");
    if (source == nullptr)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't create a new node: xmlNewNode returned nullptr");
      return eIasFailed;
    }
    std::string sourceName = IasSetupHelper::getAudioSourceDeviceName(sourceDevicePtr);
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Source Device Name: ", sourceName);
    //add attribute
    if( sourceDevicePtr->isAlsaHandler())
    {
      xmlSetProp(source, BAD_CAST"name", reinterpret_cast<const unsigned char *>(sourceName.c_str()));
    }
    else
    {
      sourceName.insert(0, "smartx:");
      xmlSetProp(source, BAD_CAST"name", reinterpret_cast<const unsigned char *>(sourceName.c_str()));
    }
    std::int32_t numChannels = IasSetupHelper::getDeviceNumChannels(sourceDevicePtr);
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Source Device Num Channels: ", numChannels);
    //add attribute
    xmlSetProp(source, BAD_CAST"num_channels", reinterpret_cast<const unsigned char *>(to_string(numChannels).c_str()));

    std::int32_t sampleRate = IasSetupHelper::getDeviceSampleRate(sourceDevicePtr);
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Source Device Sample Rate: ", sampleRate);

    //add attribute
    xmlSetProp(source, BAD_CAST"sample_rate", reinterpret_cast<const unsigned char *>(to_string(sampleRate).c_str()));

    std::string dataFormat = IasSetupHelper::getDeviceDataFormat(sourceDevicePtr);
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Source Device Data Format: ", dataFormat );
    //add attribute
    xmlSetProp(source, BAD_CAST"data_format", reinterpret_cast<const unsigned char *>(toXmlDataFormat(dataFormat).c_str()));

    std::string clockType = IasSetupHelper::getDeviceClockType(sourceDevicePtr);
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Source Device clock Type: ", clockType);

    //add attribute
    xmlSetProp(source, BAD_CAST"clock_type", reinterpret_cast<const unsigned char *>(toXmlClockType(clockType).c_str()));

    std::int32_t periodSize= IasSetupHelper::getDevicePeriodSize(sourceDevicePtr);
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Source Device period size: ", periodSize );
    //add attribute
    xmlSetProp(source, BAD_CAST"period_size", reinterpret_cast<const unsigned char *>(to_string(periodSize).c_str()));

    std::int32_t numPeriod= IasSetupHelper::getDeviceNumPeriod(sourceDevicePtr);
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Source Device num period: ", numPeriod);
    //add attribute
    xmlSetProp(source, BAD_CAST"num_period", reinterpret_cast<const unsigned char *>(to_string(numPeriod).c_str()));

    std::int32_t asrcPeriodCount= IasSetupHelper::getDevicePeriodsAsrcBuffer(sourceDevicePtr);
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Source Device asrc period count: ", asrcPeriodCount);
    //add attribute
    xmlSetProp(source, BAD_CAST"asrc_period_count", reinterpret_cast<const unsigned char *>(to_string(asrcPeriodCount).c_str()));

    //add child source node
    xmlAddChild(sources,source);

    //Extract port parameters
    auto ports = setup->getAudioPorts();
    for(auto port : ports)
    {
      IasAudioPortOwnerPtr owner;
      port->getOwner(&owner);
      if (owner == sourceDevicePtr)
      {
        //create port node
        xmlNodePtr sourcePort = xmlNewNode(NULL,BAD_CAST"Port");
        if (sourcePort == nullptr)
        {
          DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't create a new node: xmlNewNode returned nullptr");
          return eIasFailed;
        }

        // If a port exists, it for sure has parameters
        IAS_ASSERT(port != nullptr);
        IasAudioPortParamsPtr audioPortParams = port->getParameters();
        IAS_ASSERT(audioPortParams != nullptr);
        std::string portName = audioPortParams->name;
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Source Port name: ", portName);
        //add attribute
        xmlSetProp(sourcePort, BAD_CAST"name", reinterpret_cast<const unsigned char *>(portName.c_str()));

        std::int32_t portId = audioPortParams->id;
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Source Port Id: ", portId);
        //add attribute
        xmlSetProp(sourcePort, BAD_CAST"id", reinterpret_cast<const unsigned char *>(to_string(portId).c_str()));

        std::int32_t portNumChannels = audioPortParams->numChannels;
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Source Port numChannels: ", portNumChannels);
        //add attribute
        xmlSetProp(sourcePort, BAD_CAST"channel_count", reinterpret_cast<const unsigned char *>(to_string(portNumChannels).c_str()));

        std::int32_t portIndex = audioPortParams->index;
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Source Port Index: ", portIndex);
        //add attribute
        xmlSetProp(sourcePort, BAD_CAST"channel_index", reinterpret_cast<const unsigned char *>(to_string(portIndex).c_str()));

        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Source Port Direction: ", "eIasPortDirectionOutput");

        //add port to source node
        res = xmlAddChild(source, sourcePort);
        if (res == nullptr)
        {
          DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't add child node: xmlAddChild returned nullptr");
          return eIasFailed;
        }
      }//else not a source port
    }//end traversing all ports
  }//end traversing all sources

  return eIasOk;
}

IasSmartXDebugFacade::IasResult IasSmartXDebugFacade::getSmartxSinks(xmlNodePtr root_node, IasISetup *setup)
{
  IasAudioSinkDevicePtr sinkDevicePtr;
  IasAudioSinkDeviceVector sinkDevices = setup->getAudioSinkDevices();

  //create Sinks node
  xmlNodePtr sinks = xmlNewNode(NULL,BAD_CAST"Sinks");
  if (sinks == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't create a new node: xmlNewNode returned nullptr");
    return eIasFailed;
  }
  xmlNodePtr res = xmlAddChild(root_node,sinks);
  if (res == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't add child node: xmlAddChild returned nullptr");
    return eIasFailed;
  }

  for (auto it = sinkDevices.begin(); it != sinkDevices.end(); it++)
  {
    sinkDevicePtr = *it;
    //create sink node
    xmlNodePtr sink = xmlNewNode(NULL,BAD_CAST"Sink");
    if (sink == nullptr)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't create a new node: xmlNewNode returned nullptr");
      return eIasFailed;
    }

    std::string sinkName = IasSetupHelper::getAudioSinkDeviceName(sinkDevicePtr);
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Sink Device Name: ", sinkName);
    //add attribute
    if(sinkDevicePtr->isAlsaHandler())
    {
      xmlSetProp(sink, BAD_CAST"name", reinterpret_cast<const unsigned char *>(sinkName.c_str()));
    }
    else
    {
      sinkName.insert(0, "smartx:");
      xmlSetProp(sink, BAD_CAST"name", reinterpret_cast<const unsigned char *>(sinkName.c_str()));
    }
    std::int32_t numChannels = IasSetupHelper::getDeviceNumChannels(sinkDevicePtr);
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Sink Device Num Channels: ", numChannels);
    //add attribute
    xmlSetProp(sink, BAD_CAST"num_channels", reinterpret_cast<const unsigned char *>(to_string(numChannels).c_str()));

    std::int32_t sampleRate = IasSetupHelper::getDeviceSampleRate(sinkDevicePtr);
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Sink Device Sample Rate: ", sampleRate);
    //add attribute
    xmlSetProp(sink, BAD_CAST"sample_rate", reinterpret_cast<const unsigned char *>(to_string(sampleRate).c_str()));

    std::string dataFormat = IasSetupHelper::getDeviceDataFormat(sinkDevicePtr);
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Sink Device Data Format: ", dataFormat);
    //add attribute
    xmlSetProp(sink, BAD_CAST"data_format", reinterpret_cast<const unsigned char *>(toXmlDataFormat(dataFormat).c_str()));

    std::string clockType = IasSetupHelper::getDeviceClockType(sinkDevicePtr);
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Sink Device clock Type: ", clockType);

    //add attribute
    xmlSetProp(sink, BAD_CAST"clock_type", reinterpret_cast<const unsigned char *>(toXmlClockType(clockType).c_str()));

    std::int32_t periodSize= IasSetupHelper::getDevicePeriodSize(sinkDevicePtr);
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Sink Device period size: ", periodSize);
    //add attribute
    xmlSetProp(sink, BAD_CAST"period_size", reinterpret_cast<const unsigned char *>(to_string(periodSize).c_str()));

    std::int32_t numPeriod= IasSetupHelper::getDeviceNumPeriod(sinkDevicePtr);
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Sink Device num period: ", numPeriod);
    //add attribute
    xmlSetProp(sink, BAD_CAST"num_period", reinterpret_cast<const unsigned char *>(to_string(numPeriod).c_str()));

    std::int32_t asrcPeriodCount= IasSetupHelper::getDevicePeriodsAsrcBuffer(sinkDevicePtr);
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Sink Device asrc period count: ", asrcPeriodCount);
    //add attribute
    xmlSetProp(sink, BAD_CAST"asrc_period_count", reinterpret_cast<const unsigned char *>(to_string(asrcPeriodCount).c_str()));

    //add child sink node
    res = xmlAddChild(sinks,sink);
    if (res == nullptr)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't add child node: xmlAddChild returned nullptr");
      return eIasFailed;
    }

    //Extract port parameters
    auto ports = setup->getAudioPorts();
    for(auto port : ports)
    {
      IasAudioPortOwnerPtr owner;
      port->getOwner(&owner);
      if (owner == sinkDevicePtr)
      {
        //create port node
        xmlNodePtr sinkPort = xmlNewNode(NULL,BAD_CAST"Port");
        if (sinkPort == nullptr)
        {
          DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't create a new node: xmlNewNode returned nullptr");
          return eIasFailed;
        }

        // If a port exists, it for sure has parameters
        IAS_ASSERT(port != nullptr);
        IasAudioPortParamsPtr audioPortParams = port->getParameters();
        IAS_ASSERT(audioPortParams != nullptr);
        std::string portName = audioPortParams->name;
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Sink Port name: ", portName);
        //add attribute
        xmlSetProp(sinkPort, BAD_CAST"name", reinterpret_cast<const unsigned char *>(portName.c_str()));

        std::int32_t portId = audioPortParams->id;
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Sink Port Id: ", portId);
        //add attribute
        xmlSetProp(sinkPort, BAD_CAST"id", reinterpret_cast<const unsigned char *>(to_string(portId).c_str()));

        std::int32_t portNumChannels = audioPortParams->numChannels;
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Sink Port numChannels: ", portNumChannels);
        //add attribute
        xmlSetProp(sinkPort, BAD_CAST"channel_count", reinterpret_cast<const unsigned char *>(to_string(portNumChannels).c_str()));

        std::int32_t portIndex = audioPortParams->index;
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Sink Port Index: ", portIndex);
        //add attribute
        xmlSetProp(sinkPort, BAD_CAST"channel_index", reinterpret_cast<const unsigned char *>(to_string(portIndex).c_str()));

        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Sink Port Direction: ", "eIasPortDirectionOutput");

        //add port to sink node
        res = xmlAddChild(sink, sinkPort);
        if (res == nullptr)
        {
          DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't add child node: xmlAddChild returned nullptr");
          return eIasFailed;
        }
      }//else not a sink port
    }//end traversing all ports
  }//end traversing all sinks

  return eIasOk;
}

IasSmartXDebugFacade::IasResult IasSmartXDebugFacade::getSmartxProcessingModuleProperties(xmlNodePtr propertiesNode, IasProcessingModulePtr module, IasISetup *setup)
{
  //Extract Properties
  IasProperties mModuleProperties;
  const IasPropertiesPtr properties = setup->getProperties(module);

  xmlNodePtr res = nullptr;

  const IasKeyList &keyList = properties->getPropertyKeys();
  for (auto property: keyList)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Property type: ", property);

    std::string key = property;
    std::string dataType;
    IasProperties::IasResult result = properties->getKeyDataType(key, dataType);
    if (result != IasProperties::eIasOk)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error in function getKeyDataType: ", toString(result));
      return eIasFailed;
    }

    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "DataKeyResult: ", toString(result),"Key data type: ", dataType);
    xmlNodePtr vectorPropertyNode = nullptr;
    xmlNodePtr scalarPropertyNode = nullptr;

    if (dataType.find("Vector") != std::string::npos)
    {
      //create vector property node
      vectorPropertyNode = xmlNewNode(NULL,BAD_CAST"VectorProperty");
      if (vectorPropertyNode == nullptr)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't create a new node: xmlNewNode returned nullptr");
        return eIasFailed;
      }

      //add attribute
      xmlSetProp(vectorPropertyNode, BAD_CAST"key", reinterpret_cast<const unsigned char *>(key.c_str()));

      //add attribute
      dataType = dataType.substr(3, dataType.size() - 9); //This is to remove Ias and Vector part in the string
      xmlSetProp(vectorPropertyNode, BAD_CAST"type", reinterpret_cast<const unsigned char *>(dataType.c_str()));

      //add vector property node to properties node
      res = xmlAddChild(propertiesNode, vectorPropertyNode);
      if (res == nullptr)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't add child node: xmlAddChild returned nullptr");
        return eIasFailed;
      }

      if (dataType == "Int32")
      {
        IasInt32Vector value;
        result = properties->get(key, &value);
        for (auto it: value)
        {
          //create value node
          xmlNodePtr valueNode = xmlNewNode(NULL,BAD_CAST"Value");
          if (valueNode == nullptr)
          {
            DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't create a new node: xmlNewNode returned nullptr");
            return eIasFailed;
          }
          DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "value: ", it);

          //add value node to vector property node
          res = xmlAddChild(vectorPropertyNode, valueNode);
          if (res == nullptr)
          {
            DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't add child node: xmlAddChild returned nullptr");
            return eIasFailed;
          }
          xmlNodeSetContent(valueNode, reinterpret_cast<const unsigned char *>(to_string(it).c_str()));
        }
      }
      else if (dataType == "Int64")
      {
        IasInt64Vector value;
        result = properties->get(key, &value);
        for (auto it: value)
        {
          //create value node
          xmlNodePtr valueNode = xmlNewNode(NULL,BAD_CAST"Value");
          if (valueNode == nullptr)
          {
            DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't create a new node: xmlNewNode returned nullptr");
            return eIasFailed;
          }
          DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "value: ", it);

          //add value node to vector property node
          res = xmlAddChild(vectorPropertyNode, valueNode);
          if (res == nullptr)
          {
            DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't add child node: xmlAddChild returned nullptr");
            return eIasFailed;
          }
          xmlNodeSetContent(valueNode, reinterpret_cast<const unsigned char *>(to_string(it).c_str()));
        }
      }
      else if (dataType == "Float32")
      {
        IasFloat32Vector value;
        result = properties->get(key, &value);
        for (auto it: value)
        {
          //create value node
          xmlNodePtr valueNode = xmlNewNode(NULL,BAD_CAST"Value");
          if (valueNode == nullptr)
          {
            DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't create a new node: xmlNewNode returned nullptr");
            return eIasFailed;
          }
          DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "value: ", it);

          //add value node to vector property node
          res = xmlAddChild(vectorPropertyNode, valueNode);
          if (res == nullptr)
          {
            DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't add child node: xmlAddChild returned nullptr");
            return eIasFailed;
          }
          xmlNodeSetContent(valueNode, reinterpret_cast<const unsigned char *>(to_string(it).c_str()));
        }
      }
      else if (dataType == "Float64")
      {
        IasFloat64Vector value;
        result = properties->get(key, &value);
        for (auto it: value)
        {
          //create value node
          xmlNodePtr valueNode = xmlNewNode(NULL,BAD_CAST"Value");
          if (valueNode == nullptr)
          {
            DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't create a new node: xmlNewNode returned nullptr");
            return eIasFailed;
          }
          DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "value: ", it);

          //add value node to vector property node
          res = xmlAddChild(vectorPropertyNode, valueNode);
          if (res == nullptr)
          {
            DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't add child node: xmlAddChild returned nullptr");
            return eIasFailed;
          }
          xmlNodeSetContent(valueNode, reinterpret_cast<const unsigned char *>(to_string(it).c_str()));
        }
      }
      else if (dataType == "String")
      {
        IasStringVector value;
        result = properties->get(key, &value);
        for (auto it: value)
        {
          //create value node
          xmlNodePtr valueNode = xmlNewNode(NULL,BAD_CAST"Value");
          if (valueNode == nullptr)
          {
            DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't create a new node: xmlNewNode returned nullptr");
            return eIasFailed;
          }
          DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "value: ", it);

          //add value node to vector property node
          res = xmlAddChild(vectorPropertyNode, valueNode);
          if (res == nullptr)
          {
            DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't add child node: xmlAddChild returned nullptr");
            return eIasFailed;
          }
          xmlNodeSetContent(valueNode, reinterpret_cast<const unsigned char *>(it.c_str()));
        }
      }
      else
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "No Data type matched");
      }
    }
    else
    {
      //create scalar property node
      scalarPropertyNode = xmlNewNode(NULL,BAD_CAST"ScalarProperty");
      if (scalarPropertyNode == nullptr)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't create a new node: xmlNewNode returned nullptr");
        return eIasFailed;
      }

      //add attribute
      xmlSetProp(scalarPropertyNode, BAD_CAST"key", reinterpret_cast<const unsigned char *>(key.c_str()));

      //add attribute
      xmlSetProp(scalarPropertyNode, BAD_CAST"type", reinterpret_cast<const unsigned char *>(dataType.c_str()));

      //add vector property node to properties node
      res = xmlAddChild(propertiesNode, scalarPropertyNode);
      if (res == nullptr)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't add child node: xmlAddChild returned nullptr");
        return eIasFailed;
      }

      if (dataType == "Int32")
      {
        std::int32_t value;
        result = properties->get(key, &value);
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "value: ", value);
        xmlNodeSetContent(scalarPropertyNode, reinterpret_cast<const unsigned char *>(to_string(value).c_str()));
      }
      else if (dataType == "Int64")
      {
        double value;
        result = properties->get(key, &value);
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "value: ", value);
        xmlNodeSetContent(scalarPropertyNode, reinterpret_cast<const unsigned char *>(to_string(value).c_str()));
      }
      else if (dataType == "Float32")
      {
        float value;
        result = properties->get(key, &value);
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "value: ", value);
        xmlNodeSetContent(scalarPropertyNode, reinterpret_cast<const unsigned char *>(to_string(value).c_str()));
      }
      else if (dataType == "Float64")
      {
        double value;
        result = properties->get(key, &value);
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "value: ", value);
        xmlNodeSetContent(scalarPropertyNode, reinterpret_cast<const unsigned char *>(to_string(value).c_str()));
      }
      else if (dataType == "String")
      {
        std::string value;
        result = properties->get(key, &value);
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "value: ", value);
        xmlNodeSetContent(scalarPropertyNode, reinterpret_cast<const unsigned char *>(value.c_str()));
      }
      else
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "No Data type matched");
      }
    }
  }
  return eIasOk;
}

IasSmartXDebugFacade::IasResult IasSmartXDebugFacade::getSmartxProcessingModules(xmlNodePtr processingPipeline, IasPipelinePtr rzPipeline, IasISetup *setup)
{
  IasPipeline::IasProcessingModuleVector processingModules;

  //create processingmodules node
  xmlNodePtr processingModulesNode= xmlNewNode(NULL,BAD_CAST"ProcessingModules");
  if (processingModulesNode == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't create a new node: xmlNewNode returned nullptr");
    return eIasFailed;
  }

  //add processing modules node to processingpipeline
  xmlNodePtr res = xmlAddChild(processingPipeline, processingModulesNode);
  if (res == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't add child node: xmlAddChild returned nullptr");
    return eIasFailed;
  }

  rzPipeline->getProcessingModules(&processingModules);
  for (IasProcessingModulePtr module :processingModules)
  {
    //create processing module node
    xmlNodePtr processingModuleNode= xmlNewNode(NULL,BAD_CAST"ProcessingModule");
    if (processingModuleNode == nullptr)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't create a new node: xmlNewNode returned nullptr");
      return eIasFailed;
    }

    IasProcessingModuleParamsPtr processingModuleParam = module->getParameters();
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Processing module type: ", processingModuleParam->typeName);
    //add attribute
    xmlSetProp(processingModuleNode, BAD_CAST"type", reinterpret_cast<const unsigned char *>(processingModuleParam->typeName.c_str()));

    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Processing module name: ", processingModuleParam->instanceName);
    //add attribute
    xmlSetProp(processingModuleNode, BAD_CAST"name", reinterpret_cast<const unsigned char *>(processingModuleParam->instanceName.c_str()));

    //add processing module node to processingmodules node
    res = xmlAddChild(processingModulesNode, processingModuleNode);
    if (res == nullptr)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't add child node: xmlAddChild returned nullptr");
      return eIasFailed;
    }

    //create processing module pins node
    xmlNodePtr processingModulePinsNode = xmlNewNode(NULL,BAD_CAST"Pins");
    if (processingModulePinsNode == nullptr)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't create a new node: xmlNewNode returned nullptr");
      return eIasFailed;
    }

    //add pins node to processing module node
    res = xmlAddChild(processingModuleNode, processingModulePinsNode);
    if (res == nullptr)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't add child node: xmlAddChild returned nullptr");
      return eIasFailed;
    }

    //Extract module pins
    IasAudioPinSetPtr processingModulePins = module->getAudioPinSet();
    for (const IasAudioPinPtr modulePin :(*processingModulePins))
    {
      //create processing module pin node
      xmlNodePtr processingModulePinNode= xmlNewNode(NULL,BAD_CAST"Pin");
      if (processingModulePinNode == nullptr)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't create a new node: xmlNewNode returned nullptr");
        return eIasFailed;
      }

      IasAudioPinParamsPtr pinParam = modulePin->getParameters();
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Processing module Pin name: ", pinParam->name);
      //add attribute
      xmlSetProp(processingModulePinNode, BAD_CAST"name", reinterpret_cast<const unsigned char *>(pinParam->name.c_str()));

      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Processing module Pin channels: ", pinParam->numChannels );
      //add attribute
      xmlSetProp(processingModulePinNode, BAD_CAST"channel_count", reinterpret_cast<const unsigned char *>(to_string(pinParam->numChannels).c_str()));

      //add pin node to pins
      res = xmlAddChild(processingModulePinsNode, processingModulePinNode);
      if (res == nullptr)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't add child node: xmlAddChild returned nullptr");
        return eIasFailed;
      }
    }//end traversing all module pins

    //create pin mapping node
    xmlNodePtr pinMappingsNode= xmlNewNode(NULL,BAD_CAST"PinMappings");
    if (pinMappingsNode == nullptr)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't create a new node: xmlNewNode returned nullptr");
      return eIasFailed;
    }

    //add pin mappings node to processing module node
    res = xmlAddChild(processingModuleNode, pinMappingsNode);
    if (res == nullptr)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't add child node: xmlAddChild returned nullptr");
      return eIasFailed;
    }

    //Extract pin mapping
    IasAudioPinMappingSetPtr modulePinMappingSet = module->getAudioPinMappingSet();
    for (const IasAudioPinMapping pinMapping :(*modulePinMappingSet))
    {
      //create pin mapping node
      xmlNodePtr pinMappingNode= xmlNewNode(NULL,BAD_CAST"PinMapping");
      if (pinMappingNode == nullptr)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't create a new node: xmlNewNode returned nullptr");
        return eIasFailed;
      }

      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Module input pin: ", pinMapping.inputPin->getParameters()->name);
      //add attribute
      xmlSetProp(pinMappingNode, BAD_CAST"input_pin", reinterpret_cast<const unsigned char *>(pinMapping.inputPin->getParameters()->name.c_str()));

      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Module output pin: ", pinMapping.outputPin->getParameters()->name);
      //add attribute
      xmlSetProp(pinMappingNode, BAD_CAST"output_pin", reinterpret_cast<const unsigned char *>(pinMapping.outputPin->getParameters()->name.c_str()));

      //add pin mapping node to pin mappings node
      res = xmlAddChild(pinMappingsNode, pinMappingNode);
      if (res == nullptr)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't add child node: xmlAddChild returned nullptr");
        return eIasFailed;
      }
    }

    IasAudioPinSetPtr moduleInputOutputPins = module->getAudioInputOutputPinSet();
    for (const IasAudioPinPtr pinMapping :(*moduleInputOutputPins))
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Module Input/Output pin: ", pinMapping->getParameters()->name);
    }

    //create properties node
    xmlNodePtr propertiesNode= xmlNewNode(NULL,BAD_CAST"Properties");
    if (propertiesNode == nullptr)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't create a new node: xmlNewNode returned nullptr");
      return eIasFailed;
    }

    //add properties node to processingmodule node
    res = xmlAddChild(processingModuleNode, propertiesNode);
    if (res == nullptr)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't add child node: xmlAddChild returned nullptr");
      return eIasFailed;
    }

    //Extract Module Properties
    IasSmartXDebugFacade::IasResult result = getSmartxProcessingModuleProperties(propertiesNode, module, setup);
    if (result != eIasOk)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error in getSmartxProcessingModuleProperties");
      return  IasResult::eIasFailed;
    }
  }//end traversing all modules

  return eIasOk;
}


IasSmartXDebugFacade::IasResult IasSmartXDebugFacade::getSmartxProcessingLinks(xmlNodePtr processingLinksNode, IasPipelinePtr rzPipeline)
{
  //Extract connections inside pipeline
  IasAudioPinVector pipelinePins;
  rzPipeline->getAudioPins(&pipelinePins);

  std::vector<std::string> audioPinMap;
  std::vector<std::string> pinMapList;

  for(auto pin :pipelinePins)
  {
    // If a pin exists, it for sure has parameters
    IAS_ASSERT(pin != nullptr);
    const IasAudioPinParamsPtr pinParam = pin->getParameters();
    IAS_ASSERT(pinParam != nullptr);
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Pipeline Pin: ", pinParam->name);
    rzPipeline->getAudioPinMap(pin, audioPinMap);

    if(audioPinMap.size() < 2)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Pin connections not found");
    }

    auto it = find(pinMapList.begin(), pinMapList.end(), pinParam->name);
    if(it == pinMapList.end())
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "New Connection: ", audioPinMap[0], " ", audioPinMap[1]);

      //create processinglink
      xmlNodePtr processingLinkNode = xmlNewNode(NULL,BAD_CAST"ProcessingLink");
      if (processingLinkNode == nullptr)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't create a new node: xmlNewNode returned nullptr");
        return eIasFailed;
      }

      if (audioPinMap[0].find("sink") != std::string::npos)
      {
        //add attribute
        xmlSetProp(processingLinkNode, BAD_CAST"from_pin", reinterpret_cast<const unsigned char *>(pinParam->name.c_str()));
        //add attribute
        xmlSetProp(processingLinkNode, BAD_CAST"to_pin", reinterpret_cast<const unsigned char *>(audioPinMap[0].substr(audioPinMap[0].find(":") + 1).c_str()));
        //add attribute
        xmlSetProp(processingLinkNode, BAD_CAST"type", reinterpret_cast<const unsigned char *>(audioPinMap[1].c_str()));
      }
      else
      {
        //add attribute
        xmlSetProp(processingLinkNode, BAD_CAST"from_pin", reinterpret_cast<const unsigned char *>(audioPinMap[0].substr(audioPinMap[0].find(":") + 1).c_str()));
        //add attribute
        xmlSetProp(processingLinkNode, BAD_CAST"to_pin", reinterpret_cast<const unsigned char *>(pinParam->name.c_str()));
        //add attribute
        xmlSetProp(processingLinkNode, BAD_CAST"type", reinterpret_cast<const unsigned char *>(audioPinMap[1].c_str()));
      }

      //add processinglink node to processinglinksnode
      xmlNodePtr res = xmlAddChild(processingLinksNode, processingLinkNode);
      if (res == nullptr)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't add child node: xmlAddChild returned nullptr");
        return eIasFailed;
      }

      pinMapList.push_back(pinParam->name);
      pinMapList.push_back(audioPinMap[0].substr(audioPinMap[0].find(":") + 1));
    }
    audioPinMap.clear();
  }//end traversing all pins

  return eIasOk;
}


IasSmartXDebugFacade::IasResult IasSmartXDebugFacade::getSmartxPipeline(IasPipelinePtr rzPipeline, xmlNodePtr routingzone, IasISetup *setup)
{
  // Already checked in getSmartxRoutingZones
  IAS_ASSERT(rzPipeline != nullptr);
  //create processing pipeline node
  xmlNodePtr processingPipeline = xmlNewNode(NULL,BAD_CAST"ProcessingPipeline");
  if (processingPipeline == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't create a new node: xmlNewNode returned nullptr");
    return eIasFailed;
  }
  IasPipelineParamsPtr pipelineParam = rzPipeline->getParameters();
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Pipeline name: ", pipelineParam->name);
  //add attribute
  xmlSetProp(processingPipeline, BAD_CAST"name", reinterpret_cast<const unsigned char *>(pipelineParam->name.c_str()));

  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Pipeline sampleRate: ", pipelineParam->samplerate);
  //add attribute
  xmlSetProp(processingPipeline, BAD_CAST"sample_rate", reinterpret_cast<const unsigned char *>(to_string(pipelineParam->samplerate).c_str()));

  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Pipeline period size: ", pipelineParam->periodSize);
  //add attribute
  xmlSetProp(processingPipeline, BAD_CAST"period_size", reinterpret_cast<const unsigned char *>(to_string(pipelineParam->periodSize).c_str()));

  //add processingpipeline to routing zone node
  xmlNodePtr res = xmlAddChild(routingzone, processingPipeline);
  if (res == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't add child node: xmlAddChild returned nullptr");
    return eIasFailed;
  }

  //Input and output pipeline pins
  IasAudioPinVector pipelineInputPins = rzPipeline->getPipelineInputPins();

  //create pipeline input pins node
  xmlNodePtr pipelineInputPin = xmlNewNode(NULL,BAD_CAST"InputPins");
  if (pipelineInputPin == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't create a new node: xmlNewNode returned nullptr");
    return eIasFailed;
  }

  //add pipeline input pins to processingpipeline node
  res = xmlAddChild(processingPipeline, pipelineInputPin);
  if (res == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't add child node: xmlAddChild returned nullptr");
    return eIasFailed;
  }

  for(auto pin :pipelineInputPins)
  {
    //create pipeline input pin node
    xmlNodePtr pipelinePin = xmlNewNode(NULL,BAD_CAST"Pin");
    if (pipelinePin == nullptr)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't create a new node: xmlNewNode returned nullptr");
      return eIasFailed;
    }

    // If a pin exists, it for sure has parameters
    IAS_ASSERT(pin != nullptr);
    const IasAudioPinParamsPtr pinParam = pin->getParameters();
    IAS_ASSERT(pinParam != nullptr);
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Pipeline Pin name: ", pinParam->name);
    //add attribute
    xmlSetProp(pipelinePin, BAD_CAST"name", reinterpret_cast<const unsigned char *>(pinParam->name.c_str()));

    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Pipeline Pin channels: ", pinParam->numChannels);
    //add attribute
    xmlSetProp(pipelinePin, BAD_CAST"channel_count", reinterpret_cast<const unsigned char *>(to_string(pinParam->numChannels).c_str()));

    //add input port to pipeline
    res = xmlAddChild(pipelineInputPin, pipelinePin);
    if (res == nullptr)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't add child node: xmlAddChild returned nullptr");
      return eIasFailed;
    }
  }

  //create pipeline output pins node
  xmlNodePtr pipelineOutputPin = xmlNewNode(NULL,BAD_CAST"OutputPins");
  if (pipelineOutputPin == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't create a new node: xmlNewNode returned nullptr");
    return eIasFailed;
  }

  //add pipeline output pins to processingpipeline node
  res = xmlAddChild(processingPipeline, pipelineOutputPin);
  if (res == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't add child node: xmlAddChild returned nullptr");
    return eIasFailed;
  }

  IasAudioPinVector pipelineOutputPins = rzPipeline->getPipelineOutputPins();
  for(auto pin :pipelineOutputPins)
  {
    //create pipeline output pin node
    xmlNodePtr pipelinePin = xmlNewNode(NULL,BAD_CAST"Pin");
    if (pipelinePin == nullptr)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't create a new node: xmlNewNode returned nullptr");
      return eIasFailed;
    }

    const IasAudioPinParamsPtr pinParam = pin->getParameters();
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Pipeline Pin name: ", pinParam->name);
    //add attribute
    xmlSetProp(pipelinePin, BAD_CAST"name", reinterpret_cast<const unsigned char *>(pinParam->name.c_str()));

    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Pipeline Pin channels: ", pinParam->numChannels);
    //add attribute
    xmlSetProp(pipelinePin, BAD_CAST"channel_count", reinterpret_cast<const unsigned char *>(to_string(pinParam->numChannels).c_str()));

    //add output port to pipeline
    res = xmlAddChild(pipelineOutputPin, pipelinePin);
    if (res == nullptr)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't add child node: xmlAddChild returned nullptr");
      return eIasFailed;
    }
  }

  //Extract processing modules
  IasSmartXDebugFacade::IasResult result = getSmartxProcessingModules(processingPipeline, rzPipeline, setup);
  if (result != eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "getSmartxProcessingModules");
    return  IasResult::eIasFailed;
  }

  //create processinglinks
  xmlNodePtr processingLinksNode = xmlNewNode(NULL,BAD_CAST"ProcessingLinks");
  if (processingLinksNode == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't create a new node: xmlNewNode returned nullptr");
    return eIasFailed;
  }

  //add processinglinks node to processingpipeline
  xmlAddChild(processingPipeline, processingLinksNode);
  if (res == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't add child node: xmlAddChild returned nullptr");
    return eIasFailed;
  }

  //Extract processing Links
  result = getSmartxProcessingLinks(processingLinksNode, rzPipeline);
  if (result != eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "getSmartxProcessingModules");
    return  IasResult::eIasFailed;
  }

  return eIasOk;
}


IasSmartXDebugFacade::IasResult IasSmartXDebugFacade::getSmartxRoutingZones(xmlNodePtr root_node, IasISetup *setup)
{
  //create Routing zones node
  xmlNodePtr routingzones = xmlNewNode(NULL,BAD_CAST"RoutingZones");
  if (routingzones == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't create a new node: xmlNewNode returned nullptr");
    return eIasFailed;
  }

  xmlNodePtr res = xmlAddChild(root_node, routingzones);
  if (res == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't add child node: xmlAddChild returned nullptr");
    return eIasFailed;
  }

  //Extract Routing zones
  IasRoutingZonePtr routingzonePtr;
  auto routingZones = setup->getRoutingZones();

  for(auto routingZone : routingZones)
  {
    //create routing zone node
    xmlNodePtr routingzone = xmlNewNode(NULL,BAD_CAST"RoutingZone");
    if (routingzone == nullptr)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't create a new node: xmlNewNode returned nullptr");
      return eIasFailed;
    }

    IasRoutingZoneParamsPtr routingZoneParam = routingZone->getRoutingZoneParams();

    std::string zoneName = routingZoneParam->name;
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Routingzone name: ", zoneName);
    xmlSetProp(routingzone, BAD_CAST"name", reinterpret_cast<const unsigned char *>(zoneName.c_str()));

    IasAudioSinkDevicePtr rzSinkDevicePtr = routingZone->getAudioSinkDevice();
    if (rzSinkDevicePtr == nullptr)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "No sink device linked to routing zone", zoneName);
      return eIasFailed;
    }
    std::string rzSinkDeviceName = IasSetupHelper::getAudioSinkDeviceName(rzSinkDevicePtr);
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "RoutingZone Sink device name: ", rzSinkDeviceName);

    if(rzSinkDevicePtr->isAlsaHandler())
    {
      xmlSetProp(routingzone, BAD_CAST"sink", reinterpret_cast<const unsigned char *>(rzSinkDeviceName.c_str()));
    }
    else
    {
      rzSinkDeviceName.insert(0, "smartx:");
      xmlSetProp(routingzone, BAD_CAST"sink", reinterpret_cast<const unsigned char *>(rzSinkDeviceName.c_str()));
    }

    if (routingZone->isDerivedZone())
    {
      IasRoutingZonePtr baseRz = routingZone->getBaseZone();
      std::string baseRzName =  baseRz->getRoutingZoneParams()->name;
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Base routing zone name: ", baseRzName);
      xmlSetProp(routingzone, BAD_CAST"derived_from", reinterpret_cast<const unsigned char *>(baseRzName.c_str()));
    }

    //add child routingzone to routingZones
    res = xmlAddChild(routingzones, routingzone);
    if (res == nullptr)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't add child node: xmlAddChild returned nullptr");
      return eIasFailed;
    }

    //Extract port parameters
    auto ports = setup->getAudioPorts();
    for(auto port : ports)
    {
      IasAudioPortOwnerPtr owner;
      port->getOwner(&owner);

      IasRoutingZonePtr rZone = owner->getRoutingZone();
      if (rZone != nullptr)
      {
        IasRoutingZoneParamsPtr rzPortParam = rZone->getRoutingZoneParams();
        if (rzPortParam->name == zoneName && owner->isRoutingZone())
        {
          //create routing zone port node
          xmlNodePtr rzPort = xmlNewNode(NULL,BAD_CAST"Port");
          if (rzPort == nullptr)
          {
            DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't create a new node: xmlNewNode returned nullptr");
            return eIasFailed;
          }

          // If a port exists, it for sure has parameters
          IAS_ASSERT(port != nullptr);
          IasAudioPortParamsPtr portParam = port->getParameters();
          IAS_ASSERT(portParam != nullptr);
          std::string rzPortName = portParam->name;
          DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Routing zone Port name: ", rzPortName);
          //add attribute
          xmlSetProp(rzPort, BAD_CAST"name", reinterpret_cast<const unsigned char *>(rzPortName.c_str()));

          std::int32_t rzPortId = portParam->id;
          DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Routing zone port Id: ", rzPortId);
          //add attribute
          xmlSetProp(rzPort, BAD_CAST"id", reinterpret_cast<const unsigned char *>(to_string(rzPortId).c_str()));

          std::int32_t rzPortNumChannels = portParam->numChannels;
          DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Routing zone Port numChannels: ", rzPortNumChannels);
          //add attribute
          xmlSetProp(rzPort, BAD_CAST"channel_count", reinterpret_cast<const unsigned char *>(to_string(rzPortNumChannels).c_str()));

          std::int32_t rzPortIndex = portParam->index;
          DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Routing zone port Index: ", rzPortIndex);
          //add attribute
          xmlSetProp(rzPort, BAD_CAST"channel_index", reinterpret_cast<const unsigned char *>(to_string(rzPortIndex).c_str()));

          //add port to routing zone node
          res = xmlAddChild(routingzone, rzPort);
          if (res == nullptr)
          {
            DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't add child node: xmlAddChild returned nullptr");
            return eIasFailed;
          }
        }
      }//not owned by routing zone
    }//end traversing all routing zone ports

    //Extract pipeline if pipeline exists
    IasPipelinePtr rzPipeline = routingZone->getPipeline();
    if (rzPipeline != nullptr)
    {
      IasSmartXDebugFacade::IasResult result = getSmartxPipeline(rzPipeline, routingzone, setup);
      if (result != eIasOk)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error in getSmartxPipeline");
        return  IasResult::eIasFailed;
      }
    }
  }//end traversing all routing zones

  return eIasOk;
}

IasSmartXDebugFacade::IasResult IasSmartXDebugFacade::getSmartxLinks(xmlNodePtr root_node, IasISetup *setup)
{
  //create links node
  xmlNodePtr linksNode = xmlNewNode(NULL,BAD_CAST"Links");
  if (linksNode == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't create a new node: xmlNewNode returned nullptr");
    return eIasFailed;
  }

  //add links node to root node
  xmlNodePtr res = xmlAddChild(root_node, linksNode);
  if (res == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't add child node: xmlAddChild returned nullptr");
    return eIasFailed;
  }

  //create setuplinks node
  xmlNodePtr setupLinksNode = xmlNewNode(NULL,BAD_CAST"SetupLinks");
  if (setupLinksNode == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't create a new node: xmlNewNode returned nullptr");
    return eIasFailed;
  }

  //add setuplinks node to root node
  res = xmlAddChild(linksNode, setupLinksNode);
  if (res == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't add child node: xmlAddChild returned nullptr");
    return eIasFailed;
  }

  //Extracting routingZone <-> sink device port links
  auto ports = setup->getAudioPorts();
  for(auto port : ports)
  {
    IasAudioPortOwnerPtr owner;
    port->getOwner(&owner);

    IasRoutingZonePtr rZone = owner->getRoutingZone();
    if (rZone != nullptr && owner->isRoutingZone())
    {
      IasAudioPortPtr sinkDeviceInputPort;
      rZone->getLinkedSinkPort(port, &sinkDeviceInputPort);
      if (sinkDeviceInputPort != nullptr)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "The Routing Zone Port: ", port->getParameters()->name, "Is linked to Sink Device: ", sinkDeviceInputPort->getParameters()->name);
        //create setuplink node
        xmlNodePtr setupLinkNode = xmlNewNode(NULL,BAD_CAST"SetupLink");
        if (setupLinkNode == nullptr)
        {
          DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't create a new node: xmlNewNode returned nullptr");
          return eIasFailed;
        }
        //add attribute
        xmlSetProp(setupLinkNode, BAD_CAST"rz_input_port", reinterpret_cast<const unsigned char *>(port->getParameters()->name.c_str()));

        //add attribute
        xmlSetProp(setupLinkNode, BAD_CAST"sink_input_port", reinterpret_cast<const unsigned char *>(sinkDeviceInputPort->getParameters()->name.c_str()));

        //add setuplink node to setuplinks node
        res = xmlAddChild(setupLinksNode, setupLinkNode);
        if (res == nullptr)
        {
          DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't add child node: xmlAddChild returned nullptr");
          return eIasFailed;
        }
        continue; //found connection!
      }
    }
  }//end traversing all ports

  //create routinglinks node
  xmlNodePtr routingLinksNode = xmlNewNode(NULL,BAD_CAST"RoutingLinks");
  if (routingLinksNode == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't create a new node: xmlNewNode returned nullptr");
    return eIasFailed;
  }

  //add routinglinks node to links node
  res = xmlAddChild(linksNode, routingLinksNode);
  if (res == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't add child node: xmlAddChild returned nullptr");
    return eIasFailed;
  }

  //Get all the dynamic active connections
  IasAudio::IasIRouting *routing = mSmartx->routing();

  IasConnectionVector activeConnections = routing->getActiveConnections();
  IasConnectionPortPair tempPair;

  for (auto mapIt=activeConnections.begin(); mapIt!=activeConnections.end(); mapIt++)
  {
    //create routinglink node
    xmlNodePtr routingLinkNode = xmlNewNode(NULL,BAD_CAST"RoutingLink");
    if (routingLinkNode == nullptr)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't create a new node: xmlNewNode returned nullptr");
      return eIasFailed;
    }

    tempPair = *mapIt;
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "output port id: ", tempPair.first->getParameters()->id);
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "input port id: ", tempPair.second->getParameters()->id);

    //add attribute
    xmlSetProp(routingLinkNode, BAD_CAST"output_port_id", reinterpret_cast<const unsigned char *>(to_string(tempPair.first->getParameters()->id).c_str()));

    //add attribute
    xmlSetProp(routingLinkNode, BAD_CAST"rz_input_port_id", reinterpret_cast<const unsigned char *>(to_string(tempPair.second->getParameters()->id).c_str()));

    //add routinglink node to routinglinks node
    res = xmlAddChild(routingLinksNode, routingLinkNode);
    if (res == nullptr)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't add child node: xmlAddChild returned nullptr");
      return eIasFailed;
    }
  }

  //create processingpipelinelinks node
  xmlNodePtr processingPipelineLinksNode = xmlNewNode(NULL,BAD_CAST"ProcessingPipelineLinks");
  if (processingPipelineLinksNode == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't create a new node: xmlNewNode returned nullptr");
    return eIasFailed;
  }

  //add processingpipelinelinks node to links node
  res = xmlAddChild(linksNode, processingPipelineLinksNode);
  if (res == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't add child node: xmlAddChild returned nullptr");
    return eIasFailed;
  }

  //Extract routingZone <-> pipeline port links
  //        pipeline    <-> sink device port links
  ports = setup->getAudioPorts();
  for(auto port : ports)
  {
    IasAudioPortOwnerPtr owner;
    port->getOwner(&owner);

    IasRoutingZonePtr rZone = owner->getRoutingZone();
    if (rZone != nullptr)
    {
      IasPipelinePtr pipeline = rZone->getPipeline();
      if (pipeline != nullptr) //routingzone has pipeline
      {
        //need to check against all the pipeline pins
        IasAudioPinVector pipelineAudioPins;
        pipeline->getAudioPins(&pipelineAudioPins);
        for(auto pin :pipelineAudioPins)
        {
          DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Pipeline has: ", pin->getParameters()->name);
          const IasAudioPinParamsPtr pinParam = pin->getParameters();

          IasAudioPortPtr inputPort = nullptr;

          IasPipeline::IasResult result = pipeline->getPipelinePinConnectionLink(pin, inputPort);
          if (result != IasPipeline::eIasOk)
          {
            DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error while calling IasPipeline::getPipelinePinConnectionLink:", toString(result));
            return eIasFailed;
          }
          if (inputPort != nullptr && inputPort->getParameters()->name == port->getParameters()->name)
          {
            /*
             * IasAudioPin::IasPinDirection direction = pin->getDirection();
             * the direction can be used to tell if it's an input to pipeline or output
             * direction -> eIasPinDirectionPipelineOutput -> pipeline output to sink
             * direction -> eIasPinDirectionPipelineInput -> routingzone input to pipeline input
             */

            //pipeline output to sink input
            DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Port: ", inputPort->getParameters()->name, "Port: ", pin->getParameters()->name, " Matched");
            //DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "PipelinePin Direction: ", pin->getDirection());

            //create processingpipelinelink node
            xmlNodePtr processingPipelineLinkNode = xmlNewNode(NULL,BAD_CAST"ProcessingPipelineLink");
            if (processingPipelineLinkNode == nullptr)
            {
              DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't create a new node: xmlNewNode returned nullptr");
              return eIasFailed;
            }

            //add attribute
            xmlSetProp(processingPipelineLinkNode, BAD_CAST"input_port", reinterpret_cast<const unsigned char *>(inputPort->getParameters()->name.c_str()));

            //add attribute
            xmlSetProp(processingPipelineLinkNode, BAD_CAST"pin", reinterpret_cast<const unsigned char *>(pin->getParameters()->name.c_str()));

            //add processingpipelinelink node to processingpipelinelinks node
            xmlAddChild(processingPipelineLinksNode, processingPipelineLinkNode);
            if (res == nullptr)
            {
              DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't add child node: xmlAddChild returned nullptr");
              return eIasFailed;
            }
            break;
          }
        }//traverse all pipeline ports
      }
    }//port is not owned by a routing zone
  }//traversed all ports

  return eIasOk;
}

IasSmartXDebugFacade::IasResult IasSmartXDebugFacade::getSmartxTopology(std::string &xmlTopologyString)
{
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "getSmartxTopology Function");
  //Create XML Topology
  xmlDocPtr doc;
  xmlNodePtr root_node;
  //new document instance
  doc = xmlNewDoc(BAD_CAST"1.0");
  //set root node
  root_node = xmlNewNode(NULL,BAD_CAST"SmartXBarSetup");
  if (root_node == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can't create a new node: xmlNewNode returned nullptr");
    xmlFreeDoc(doc);
    return eIasFailed;
  }
  xmlSetProp(root_node, BAD_CAST"version", BAD_CAST"1.0");
  xmlDocSetRootElement(doc,root_node);
  //Current Smartx instance
  IasAudio::IasISetup *setup = mSmartx->setup();
  //Fill all Sources
  IasSmartXDebugFacade::IasResult result = getSmartxSources(root_node, setup);
  if (result != eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error in getSmartxSources");
    xmlFreeDoc(doc);
    return eIasFailed;
  }
  //Fill all Sinks
  result = getSmartxSinks(root_node, setup);
  if (result != eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error in getSmartxSinks");
    xmlFreeDoc(doc);
    return eIasFailed;
  }
  //Fill all Routing zones
  result = getSmartxRoutingZones(root_node, setup);
  if (result != eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error in getSmartxRoutingZones");
    xmlFreeDoc(doc);
    return eIasFailed;
  }
  //Fill all links
  result = getSmartxLinks(root_node, setup);
  if (result != eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error in getSmartxRoutingZones");
    xmlFreeDoc(doc);
    return eIasFailed;
  }

  //save the document
  //TODO: Only for testing, remove it later
  int nRel = xmlSaveFormatFileEnc("topology.xml",doc, "UTF-8", 1);

  if (nRel != -1)
  {
    xmlChar *docstr;
    int len;
    xmlDocDumpMemory(doc, &docstr, &len);
  }

  xmlChar *buffer;
  int size;
  xmlDocDumpMemory(doc, &buffer, &size);
    xmlTopologyString = reinterpret_cast<const char *>(buffer);

  //memory free
  xmlFree(buffer);
  xmlFreeDoc(doc);

  return eIasOk;
}

const std::string IasSmartXDebugFacade::getVersion()
{
  return mSmartx->getVersion();
}

IasSmartXDebugFacade::IasResult IasSmartXDebugFacade::getParameters(const std::string &moduleInstanceName, IasProperties &parameters)
{
  IasProperties requestProperties;
  IasProperties returnProperties;
  requestProperties.set("cmd", cGetParametersCmdId);

  IasAudio::IasIProcessing *processing = mSmartx->processing();
  IasAudio::IasIProcessing::IasResult res = processing->sendCmd(moduleInstanceName, requestProperties, returnProperties);

  if (res == IasAudio::IasIProcessing::IasResult::eIasOk)
  {
    parameters = returnProperties;
    return eIasOk;
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error occurred while retrieving module properties");
    return eIasFailed;
  }
}

IasSmartXDebugFacade::IasResult IasSmartXDebugFacade::setParameters(const std::string &moduleInstanceName, const IasProperties &parameters)
{
  IasProperties requestProperties = parameters;
  IasProperties returnProperties;
  requestProperties.set("cmd", cSetParametersCmdId);

  IasAudio::IasIProcessing *processing = mSmartx->processing();
  IasAudio::IasIProcessing::IasResult res = processing->sendCmd(moduleInstanceName, requestProperties, returnProperties);

  if (res == IasAudio::IasIProcessing::IasResult::eIasOk)
  {
    return eIasOk;
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error occurred while setting module properties");
    return eIasFailed;
  }
}
} //namespace IasAudio
