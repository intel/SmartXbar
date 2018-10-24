/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file   IasConfigProcessingModule.cpp
 * @date   2017
 * @brief  The XML Parser.
 */

#include "configparser/IasConfigPipeline.hpp"
#include "configparser/IasConfigProcessingModule.hpp"
#include "configparser/IasParseHelper.hpp"

using namespace std;

namespace IasAudio {

static const std::string cClassName = "IasConfigProcessingModule::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"

IasConfigProcessingModule::IasConfigProcessingModule()
  :mModule(nullptr),
   mLog(IasAudioLogging::registerDltContext("XMLP", "XML PARSER IASCONFIGPROCESSINGMODULE"))
{

}

IasConfigProcessingModule::~IasConfigProcessingModule()
{

}

std::vector<IasAudioPinParams> IasConfigProcessingModule::getModulePinParams()
{
  return mModulePinParams;
}

std::vector<IasAudioPinPtr> IasConfigProcessingModule::getModulePins()
{
  return mModulePins;
}

IasConfigProcessingModule::IasResult IasConfigProcessingModule::setVectorProperty(const std::string& vectorPropertyType, const std::string& vectorPropertyDataFormat, xmlNodePtr cur_vec_node)
{
  if (vectorPropertyType == "" || vectorPropertyDataFormat == "" || cur_vec_node == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: setVectorProperty");
    return IasResult::eIasFailed;
  }

  if (vectorPropertyDataFormat == "Int32")
  {
    IasInt32Vector vectorProperty;
    std::int32_t tempValue;
    xmlNodePtr cur_val_node = nullptr;

    for (cur_val_node = cur_vec_node->children; cur_val_node != nullptr ; cur_val_node = cur_val_node->next)
    {
      try
      {
        tempValue = stoi(reinterpret_cast<const char *>(xmlNodeGetContent(cur_val_node)));
      }
      catch(...)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error converting vector property, String to Int32");
        return IasResult::eIasFailed;
      }
      vectorProperty.push_back(tempValue);
    }

    mModuleProperties.set<IasInt32Vector>(vectorPropertyType, vectorProperty);
  }

  else if (vectorPropertyDataFormat == "Int64")
  {
    IasInt64Vector vectorProperty;
    std::int64_t tempValue;
    xmlNodePtr cur_val_node = nullptr;

    for (cur_val_node = cur_vec_node->children; cur_val_node != nullptr ; cur_val_node = cur_val_node->next)
    {
      try
      {
        tempValue = stoll(reinterpret_cast<const char *>(xmlNodeGetContent(cur_val_node)));
      }
      catch(...)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error converting vector property, String to Int64");
        return IasResult::eIasFailed;
      }
      vectorProperty.push_back(tempValue);
    }

    mModuleProperties.set<IasInt64Vector>(vectorPropertyType, vectorProperty);
  }

  else if (vectorPropertyDataFormat == "Float32")
  {
    IasFloat32Vector vectorProperty;
    float tempValue;
    xmlNodePtr cur_val_node = nullptr;

    for (cur_val_node = cur_vec_node->children; cur_val_node != nullptr ; cur_val_node = cur_val_node->next)
    {
      try
      {
        tempValue = stof(reinterpret_cast<const char *>(xmlNodeGetContent(cur_val_node)));
      }
      catch(...)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error converting vector property, String to Float32");
        return IasResult::eIasFailed;
      }
      vectorProperty.push_back(tempValue);
    }

    mModuleProperties.set<IasFloat32Vector>(vectorPropertyType, vectorProperty);
  }

  else if (vectorPropertyDataFormat == "Float64")
  {
    IasFloat64Vector vectorProperty;
    double tempValue;
    xmlNodePtr cur_val_node = nullptr;

    for (cur_val_node = cur_vec_node->children; cur_val_node != nullptr ; cur_val_node = cur_val_node->next)
    {
      try
      {
        tempValue = stod(reinterpret_cast<const char *>(xmlNodeGetContent(cur_val_node)));
      }
      catch(...)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error converting vector property, String to Float64");
        return IasResult::eIasFailed;
      }
      vectorProperty.push_back(tempValue);
    }

    mModuleProperties.set<IasFloat64Vector>(vectorPropertyType, vectorProperty);
  }

  else if (vectorPropertyDataFormat == "String")
  {
    IasStringVector vectorProperty;
    std::string tempValue;
    xmlNodePtr cur_val_node = nullptr;

    for (cur_val_node = cur_vec_node->children; cur_val_node != nullptr ; cur_val_node = cur_val_node->next)
    {
      tempValue = reinterpret_cast<const char *>(xmlNodeGetContent(cur_val_node));
      vectorProperty.push_back(tempValue);
    }

    mModuleProperties.set<IasStringVector>(vectorPropertyType, vectorProperty);
  }
  else
  {
    return IasResult::eIasFailed;
  }
  return IasResult::eIasOk;
}

IasConfigProcessingModule::IasResult IasConfigProcessingModule::setScalerProperty(const std::string& scalerPropertyType, const std::string& dataType, const std::string& scalerPropertyContent)
{
  if (dataType == "" || scalerPropertyContent == "")
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: empty string");
    return IasResult::eIasFailed;
  }

  try
  {
    if(dataType == "Int32")
    {
      mModuleProperties.set<std::int32_t>(scalerPropertyType, stoi(scalerPropertyContent));
      return IasResult::eIasOk;
    }
    else if(dataType == "Int64")
    {
      mModuleProperties.set<std::int64_t>(scalerPropertyType, stoll(scalerPropertyContent));
      return IasResult::eIasOk;
    }
    else if(dataType == "Float32")
    {
      mModuleProperties.set<float>(scalerPropertyType, stof(scalerPropertyContent));
      return IasResult::eIasOk;
    }
    else if(dataType == "Float64")
    {
      mModuleProperties.set<double>(scalerPropertyType, stod(scalerPropertyContent));
      return IasResult::eIasOk;
    }
    else if(dataType == "String")
    {
      mModuleProperties.set<std::string>(scalerPropertyType, (scalerPropertyContent));
      return IasResult::eIasOk;
    }
    else
    {
      return IasResult::eIasFailed;
    }
  }
  catch(const std::invalid_argument&)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error converting", scalerPropertyContent, "to ", dataType);
    return IasResult::eIasFailed;
  }
  catch(const std::out_of_range&)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error ", scalerPropertyContent, "is out of range");
    return IasResult::eIasFailed;
  }
}

IasConfigProcessingModule::IasResult IasConfigProcessingModule::initProcessingModule(IasISetup *setup, IasPipelinePtr pipeline, xmlNodePtr processingModuleNode)
{
  if (processingModuleNode == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: processingModuleNode== nullptr");
    return IasResult::eIasFailed;
  }

  mModuleParams.typeName = getXmlAttributePtr(processingModuleNode, "type");
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Processing module typeName: ", mModuleParams.typeName);
  mModuleParams.instanceName = getXmlAttributePtr(processingModuleNode, "name");
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Processing module instanceName: ", mModuleParams.instanceName);


  //create processing module
  IasISetup::IasResult res = setup->createProcessingModule(mModuleParams, &mModule);
  if(res != IasISetup::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error creating processing module");
    return IasResult::eIasFailed;
  }

  //Traverse all pins
  xmlNodePtr processingModulePinsNode = getXmlChildNodePtr(processingModuleNode, "Pins");
  if (processingModulePinsNode == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: processingModule Pins node == nullptr");
    return IasResult::eIasFailed;
  }

  std::int32_t num_mod_pins = getXmlChildNodeCount(processingModulePinsNode);
  IasAudioPinPtr modulePins[num_mod_pins];
  IasAudioPinParams modulePinParams[num_mod_pins];
  std::int32_t pin_counter = 0;

  xmlNodePtr cur_pin_node = nullptr;
  for (cur_pin_node = processingModulePinsNode->children; cur_pin_node != nullptr ; cur_pin_node = cur_pin_node->next)
  {
    modulePinParams[pin_counter].name = getXmlAttributePtr(cur_pin_node, "name");
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Processing Module pin name: ", modulePinParams[pin_counter].name);

    try
    {
      modulePinParams[pin_counter].numChannels = stoui(getXmlAttributePtr(cur_pin_node, "channel_count"));
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "channel_count: ", modulePinParams[pin_counter].numChannels);
    }
    catch(...)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error in channel_count for processing module pin: ", modulePinParams[pin_counter].name);
      return IasResult::eIasFailed;
    }

    res = setup->createAudioPin(modulePinParams[pin_counter],&modulePins[pin_counter]);
    if(res != IasISetup::eIasOk)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error creating processing module pin: ", modulePinParams[pin_counter].name);
      return IasResult::eIasFailed;
    }

    IAS_ASSERT(modulePins[num_mod_pins] != nullptr)

    mModulePinParams.push_back(modulePinParams[pin_counter]);
    mModulePins.push_back(modulePins[pin_counter]);
    mActivePins.push_back(modulePinParams[pin_counter].name);

    pin_counter++;
  }//end traversing all pins

  //Processing module properties
  xmlNodePtr processingModulePropertiesNode = getXmlChildNodePtr(processingModuleNode, "Properties");
  if (processingModulePropertiesNode == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: processingModule properties node == nullptr");
    return IasResult::eIasFailed;
  }

  xmlNodePtr cur_property_node = nullptr;
  for (cur_property_node = processingModulePropertiesNode->children; cur_property_node != nullptr ; cur_property_node = cur_property_node->next)
  {
    if (!xmlStrcmp(cur_property_node->name, (const xmlChar *)"ScalarProperty"))
    {
      std::string scalerPropertyKey = getXmlAttributePtr(cur_property_node, "key");
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "scalerProperty Key: ", scalerPropertyKey);
      std::string scalerPropertyDataFormat = getXmlAttributePtr(cur_property_node, "type");
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "scalerProperty Type: ", scalerPropertyDataFormat);
      std::string scalerPropertyContent(reinterpret_cast<const char *>(xmlNodeGetContent(cur_property_node)));
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "scalerProperty Content: ", scalerPropertyContent);

      IasResult res = setScalerProperty(scalerPropertyKey, scalerPropertyDataFormat, scalerPropertyContent);

      if(res != IasConfigProcessingModule::eIasOk)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error setting scaler parameters: ", scalerPropertyKey);
        return IasResult::eIasFailed;
      }
    }
    else if (!xmlStrcmp(cur_property_node->name, (const xmlChar *)"VectorProperty"))
    {
      std::string vectorPropertyKey = getXmlAttributePtr(cur_property_node, "key");
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "vectorProperty Key: ", vectorPropertyKey);
      std::string vectorPropertyDataFormat = getXmlAttributePtr(cur_property_node, "type");
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "vectorProperty type: ", vectorPropertyDataFormat);

      IasResult res = setVectorProperty(vectorPropertyKey, vectorPropertyDataFormat, cur_property_node);
      if(res != IasConfigProcessingModule::eIasOk)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error setting vector parameters: ", vectorPropertyKey);
        return IasResult::eIasFailed;
      }
    }
    else
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error unknown property tag");
      return IasResult::eIasFailed;
    }
  }

  //set all properties
  setup->setProperties(mModule,mModuleProperties);

  //Add module to pipeline
  IasISetup::IasResult result = setup->addProcessingModule(pipeline, mModule);
  if(result != IasISetup::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error adding processing module to pipeline");
    return IasResult::eIasFailed;
  }


  //Pin mapping
  xmlNodePtr pinMappings = getXmlChildNodePtr(processingModuleNode, "PinMappings");
  if (pinMappings == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: PinMappings node == nullptr");
    return IasResult::eIasFailed;
  }

  xmlNodePtr cur_pin_map = pinMappings->children;
  //condition for in-place buffer
  if (cur_pin_map == nullptr)
  {
    //add all pins defined for this processing module
    for (int i = 0; i < pin_counter; i++)
    {
      res = setup->addAudioInOutPin(mModule, mModulePins.at(i));
      if(res != IasISetup::eIasOk)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error adding pin to the module");
        return IasResult::eIasFailed;
      }
    }//end adding all the pins
  }
  //condition for out-of-place buffer
  else
  {
    for (; cur_pin_map != nullptr ; cur_pin_map = cur_pin_map->next)
    {
      //Get input pin name
      std::string inputPinName = getXmlAttributePtr(cur_pin_map, "input_pin");

      //Get output pin name
      std::string outputPinName = getXmlAttributePtr(cur_pin_map, "output_pin");

      int inputIndex = 0;
      int outputIndex = 0;
      bool input_link_found = false;
      bool output_link_found = false;

      for(int i = 0; i < pin_counter; i++)
      {
        if (inputPinName == mModulePinParams.at(i).name)
        {
          inputIndex = i;
          input_link_found = true;
        }
        if (outputPinName == mModulePinParams.at(i).name)
        {
          outputIndex = i;
          output_link_found = true;
        }
      }

      if( input_link_found == false || output_link_found == false)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Processing Module pin mapping Error: ", inputPinName,  ", ",  outputPinName);
        return IasResult::eIasFailed;
      }
      else if (input_link_found == true && output_link_found == true)
      {
        res = setup->addAudioPinMapping(mModule, mModulePins.at(inputIndex), mModulePins.at(outputIndex));
        if(res != IasISetup::eIasOk)
        {
          DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error adding module pin mapping: ", mModuleParams.instanceName, "Input pin: ", mModulePinParams.at(inputIndex).name, "to Output in: ", mModulePinParams.at(outputIndex).name);
          return IasResult::eIasFailed;
        }
      }
      else
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error adding pin mapping: ", mModuleParams.instanceName, "Input pin: ", mModulePinParams.at(inputIndex).name, "to Output in: ", mModulePinParams.at(outputIndex).name);
        return IasResult::eIasFailed;
      }
    }//end mapping all out of place buffer pins
  }

  return IasResult::eIasOk;

}
} //end namespace IASAUDIO
