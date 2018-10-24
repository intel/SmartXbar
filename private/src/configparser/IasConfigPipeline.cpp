/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file   IasConfigPipeline.cpp
 * @date   2017
 * @brief  The XML Parser.
 */

#include "configparser/IasConfigPipeline.hpp"
#include "configparser/IasParseHelper.hpp"
#include "configparser/IasSmartXTypeConversion.hpp"

using namespace std;

namespace IasAudio {

static const std::string cClassName = "IasConfigPipeline::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"

IasConfigPipeline::IasConfigPipeline()
  :mPipeline(nullptr),
   mLog(IasAudioLogging::registerDltContext("XMLP", "XML PARSER IASCONFIGPIPELINE"))
{

}

IasConfigPipeline::~IasConfigPipeline()
{

}

IasPipelinePtr IasConfigPipeline::getPipelinePtr() const
{
  return mPipeline;
}

std::vector<IasAudioPinParams> IasConfigPipeline::getPipelineInputPinParams()
{
  return mPipelineInputPinParams;
}

std::vector<IasAudioPinParams> IasConfigPipeline::getPipelineOutputPinParams()
{
  return mPipelineOutputPinParams;
}

std::vector<IasAudioPinPtr> IasConfigPipeline::getPipelineInputPins()
{
  return mPipelineInputPins;
}

std::vector<IasAudioPinPtr> IasConfigPipeline::getPipelineOutputPins()
{
  return mPipelineOutputPins;
}

IasConfigPipeline::IasResult IasConfigPipeline::initPipeline(IasISetup *setup, xmlNodePtr processingPipelineNode)
{
  if (processingPipelineNode == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: processingPipelineNode== nullptr");
    return IasResult::eIasFailed;
  }

  mPipelineParams.name = getXmlAttributePtr(processingPipelineNode, "name");
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Processing Pipeline name: ", mPipelineParams.name);
  try
  {
    mPipelineParams.periodSize = stoui(getXmlAttributePtr(processingPipelineNode, "period_size"));
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "period_size: ", mPipelineParams.periodSize);
    mPipelineParams.samplerate = stoui(getXmlAttributePtr(processingPipelineNode, "sample_rate"));
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "sample_rate: ", mPipelineParams.samplerate);
  }
  catch(...)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error in setting period size or sampler ate of the pipeline: ", mPipelineParams.name);
    return IasResult::eIasFailed;
  }

  //create pipeline
  IasISetup::IasResult res = setup->createPipeline(mPipelineParams, &mPipeline);
  if(res != IasISetup::eIasOk)
  {
    // Report error and exit the program, because many of the following commands will result
    // in a segmentation fault if the pipeline has not been created pipeline appropriately.
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error creating example pipeline (please review DLT output for details)");
    return IasResult::eIasFailed;
  }

  //parse input pins
  xmlNodePtr processingPipelinePinsNode = getXmlChildNodePtr(processingPipelineNode, "InputPins");
  if (processingPipelinePinsNode == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Pipeline has no input pins defined");
    return IasResult::eIasFailed;
  }

  std::int32_t num_input_pins = getXmlChildNodeCount(processingPipelinePinsNode);
  IasAudioPinPtr pipelineInputPin[num_input_pins];
  IasAudioPinParams pipelineInputPinParams[num_input_pins];
  std::int32_t input_pin_counter = 0;

  //Traverse if processing input pins defined
  xmlNodePtr cur_pin_node = nullptr;
  for (cur_pin_node = processingPipelinePinsNode->children; cur_pin_node != nullptr ; cur_pin_node = cur_pin_node->next)
  {
    pipelineInputPinParams[input_pin_counter].name = getXmlAttributePtr(cur_pin_node, "name");
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Pipeline Input Pin Name: ", pipelineInputPinParams[input_pin_counter].name);

    try
    {
      pipelineInputPinParams[input_pin_counter].numChannels = stoui(getXmlAttributePtr(cur_pin_node, "channel_count"));
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "channel_count: ", pipelineInputPinParams[input_pin_counter].numChannels);
    }

    catch(...)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error in channel_count for pipeline Input Pin: ", pipelineInputPinParams[input_pin_counter].name);
      return IasResult::eIasFailed;
    }

    res = setup->createAudioPin(pipelineInputPinParams[input_pin_counter], &pipelineInputPin[input_pin_counter]);
    if(res != IasISetup::eIasOk)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error creating pipeline pin: ", pipelineInputPinParams[input_pin_counter].name);
      return IasResult::eIasFailed;
    }

    res = setup->addAudioInputPin(mPipeline, pipelineInputPin[input_pin_counter]);
    if(res != IasISetup::eIasOk)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error adding pipeline Input pin: ",pipelineInputPinParams[input_pin_counter].name);
      return IasResult::eIasFailed;
    }

    IAS_ASSERT(pipelineInputPin[input_pin_counter] != nullptr)

    mPipelineInputPinParams.push_back(pipelineInputPinParams[input_pin_counter]);
    mPipelineInputPins.push_back(pipelineInputPin[input_pin_counter]);

    input_pin_counter++;
  }

  //Parse output pins
  processingPipelinePinsNode = getXmlChildNodePtr(processingPipelineNode, "OutputPins");
  if (processingPipelinePinsNode == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Pipeline has no output pins defined");
    return IasResult::eIasFailed;
  }

  std::int32_t num_output_pins = getXmlChildNodeCount(processingPipelinePinsNode);
  IasAudioPinPtr pipelineOutputPin[num_output_pins];
  IasAudioPinParams pipelineOutputPinParams[num_output_pins];
  std::int32_t output_pin_counter = 0;

  //Traverse if processing output pins defined
  cur_pin_node = nullptr;
  for (cur_pin_node = processingPipelinePinsNode->children; cur_pin_node != nullptr ; cur_pin_node = cur_pin_node->next)
  {
    //mPipelineOutputPins = nullptr;

    //mPipelineOutputPinParams;
    pipelineOutputPinParams[output_pin_counter].name = getXmlAttributePtr(cur_pin_node, "name");
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Pipeline Output Pin Name: ", pipelineOutputPinParams[output_pin_counter].name);

    try
    {
      pipelineOutputPinParams[output_pin_counter].numChannels = stoui(getXmlAttributePtr(cur_pin_node, "channel_count"));
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "channel_count: ", pipelineOutputPinParams[output_pin_counter].numChannels);
    }

    catch(...)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error in channel_count for pipeline Output Pin: ", pipelineOutputPinParams[output_pin_counter].name);
      return IasResult::eIasFailed;
    }

    res = setup->createAudioPin(pipelineOutputPinParams[output_pin_counter], &pipelineOutputPin[output_pin_counter]);
    if(res != IasISetup::eIasOk)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error creating pipeline pin: ", pipelineOutputPinParams[output_pin_counter].name);
      return IasResult::eIasFailed;
    }

    res = setup->addAudioOutputPin(mPipeline, pipelineOutputPin[output_pin_counter]);
    if(res != IasISetup::eIasOk)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error adding pipeline output pin: ", pipelineOutputPinParams[output_pin_counter].name);
      return IasResult::eIasFailed;
    }

    IAS_ASSERT(pipelineOutputPin[output_pin_counter] != nullptr)
    mPipelineOutputPins.push_back(pipelineOutputPin[output_pin_counter]);
    mPipelineOutputPinParams.push_back(pipelineOutputPinParams[output_pin_counter]);

    output_pin_counter++;
  }

  //Traverse all processing modules defined inside the pipeline
  xmlNodePtr processingModulesNode = getXmlChildNodePtr(processingPipelineNode, "ProcessingModules");
  if (processingModulesNode == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "No processing modules defined in the pipeline");
    return IasResult::eIasFailed;
  }

  std::int32_t numProcessingModules = getXmlChildNodeCount(processingModulesNode);
  IasConfigProcessingModule processingModules[numProcessingModules];
  std::int32_t module_counter = 0;

  xmlNodePtr cur_mod_node = nullptr;
  for (cur_mod_node = processingModulesNode->children; cur_mod_node != nullptr ; cur_mod_node = cur_mod_node->next)
  {
    IasConfigProcessingModule::IasResult res = processingModules[module_counter].initProcessingModule(setup, mPipeline, cur_mod_node);
    if(res != IasConfigProcessingModule::eIasOk)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error initiating processing module");
      return IasResult::eIasFailed;
    }

    module_counter++;
  }//end creating all the processing module defined in the pipeline

  /*Setting all links inside pipeline
   *
   * Pipeline pins -> processing module pins
   * Processing module pins -> processing module pins
   * Processing module pins -> pipeline pins
   */
  xmlNodePtr processingLinks = getXmlChildNodePtr(processingPipelineNode, "ProcessingLinks");
  if (processingLinks == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "No processing links defined for the pipeline");
    return IasResult::eIasFailed;
  }

  xmlNodePtr cur_link_node = nullptr;
  for (cur_link_node = processingLinks->children; cur_link_node != nullptr ; cur_link_node = cur_link_node->next)
  {
    std::string linkType = getXmlAttributePtr(cur_link_node, "type");
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Link Type: ", linkType);
    std::string link_fromPin = getXmlAttributePtr(cur_link_node, "from_pin");
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Link fromPin:  ", link_fromPin);
    std::string link_toPin = getXmlAttributePtr(cur_link_node, "to_pin");
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Link toPin:  ", link_toPin);

    bool input_link_found = false;
    bool output_link_found = false;

    IasAudioPinPtr fromPin;
    IasAudioPinPtr toPin;

    //Traverse all pipeline input ports
    for (int i = 0; i < input_pin_counter; i++)
    {
      if (link_fromPin == pipelineInputPinParams[i].name)
      {
        input_link_found = true;
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Found from link: ", pipelineInputPinParams[i].name);
        fromPin = pipelineInputPin[i];
        if (fromPin == nullptr)
        {
          DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "fromPin was always empty");
        }
        break;
      }
    }

    //Traverse all modules pins
    for (int i = 0; i < numProcessingModules; i++)
    {
      std::vector<IasAudioPinParams> modulePinParams = processingModules[i].getModulePinParams();
      std::vector<IasAudioPinParams>::iterator it_pinParams = modulePinParams.begin();

      std::vector<IasAudioPinPtr> modulePins = processingModules[i].getModulePins();
      std::vector<IasAudioPinPtr>::iterator it_pinPtr = modulePins.begin();

      for(; it_pinParams != modulePinParams.end(), it_pinPtr != modulePins.end(); it_pinParams++, it_pinPtr++)
      {
        if (input_link_found == true)//look only for toPin
        {
          if (link_toPin == (*it_pinParams).name)
          {
            DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Found to link: ", (*it_pinParams).name);
            toPin = (*it_pinPtr);
            DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Linking: ", (*it_pinParams).name, "->", link_fromPin);
            res = setup->link(fromPin, toPin, toAudioPinLinkType(linkType));
            if(res != IasISetup::eIasOk)
            {
              DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error linking pin output: ",  link_fromPin, "to pin input: ", link_toPin);
              return IasResult::eIasFailed;
            }
            output_link_found = true;
            break;
          }
        }
        else // look for fromPin
        {
          if (link_fromPin == (*it_pinParams).name)
          {
            DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Found from link: ", (*it_pinParams).name);
            input_link_found = true;
            fromPin = (*it_pinPtr);
          }
        }
      }
      if (input_link_found == true && output_link_found == true)
      {
        break;
      }
    }
    if (input_link_found == true && output_link_found == true)
    {
      //move to next link connection
      continue;
    }
    //Traverse all output pins of pipeline
    for (int i = 0; i < output_pin_counter; i++)
    {
      if (input_link_found == true)
      {
        if (link_toPin == pipelineOutputPinParams[i].name)
        {
          DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Found to link: ", pipelineOutputPinParams[i].name);
          output_link_found = true;
          toPin = pipelineOutputPin[i];
          DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Linking: ", pipelineOutputPinParams[i].name, "->", link_fromPin);
          res = setup->link(fromPin, toPin, toAudioPinLinkType(linkType));
          if(res != IasISetup::eIasOk)
          {
            DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error linking pin output: ",  link_fromPin, "to pin input: ", link_toPin);
            return IasResult::eIasFailed;
          }
          break;
        }
      }
      else
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Can not link as no from pin found for the pipeline");
        return IasResult::eIasFailed;
      }
    }//end traversing all output pins of pipeline
  }//end traversing all processing links
  return IasResult::eIasOk;
}
} //end namespace IASAUDIO
