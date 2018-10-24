/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file   IasConfigParser.cpp
 * @date   2017
 * @brief  The XML Parser.
 */

#include <stdio.h>
#include <vector>
#include "configparser/IasConfigParser.hpp"
#include "configparser/IasParseHelper.hpp"
#include "configparser/IasConfigPipeline.hpp"
#include "configparser/IasSmartXTypeConversion.hpp"
#include "configparser/IasConfigSinkDevice.hpp"
#include "configparser/IasConfigSourceDevice.hpp"
#include "audio/configparser/IasSmartXconfigParser.hpp"
#include "audio/smartx/IasIRouting.hpp"
#include "audio/common/audiobuffer/IasMemoryAllocator.hpp"


namespace IasAudio {

static const std::string fFunction = "XMLPARSER::";
#define LOG_PREFIX fFunction +  __func__ + "(" + std::to_string(__LINE__) + "):"

IasConfigParserResult parseSinkDevices(IasISetup *setup, const xmlNodePtr rootNode)
{
  DltContext *mLog(IasAudioLogging::registerDltContext("PAR", "XML SINKPARSER"));

  if (setup == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "setup == nullptr");
    return IasConfigParserResult::eIasNullPointer;
  }

  xmlNodePtr cur_node = nullptr;
  xmlNodePtr Links = nullptr;
  xmlNodePtr setupLinksNode = nullptr;

  //Get sinks node
  cur_node = getXmlChildNodePtr(rootNode, "Sinks");
  if (cur_node == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Sinks node not found");
    return IasConfigParserResult::eIasNodeNotFound;
  }

  //Get Links node
  Links = getXmlChildNodePtr(rootNode, "Links");
  if (Links == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Links node not found");
    return IasConfigParserResult::eIasNodeNotFound;
  }

  //Get SetupLinks node
  setupLinksNode = getXmlChildNodePtr(Links, "SetupLinks");
  if (setupLinksNode == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "SetupLinks node not found");
    return IasConfigParserResult::eIasNodeNotFound;
  }

  /* Top reference created for sink devices, that contain routing zones and may
   * contain pipelines. This reference is required to match and link ports/pins
   * based on the connections in the xml.
   */
  std::vector<IasConfigSinkDevice> sinkDeviceList;
  std::int32_t numSinkDevices = 0;

  std::vector<IasConfigPipeline> pipelines;
  std::int32_t numPipelines = 0;

  //Traversing all sink devices
  for (cur_node = cur_node->children; cur_node != nullptr ; cur_node = cur_node->next)
  {
    if (!xmlStrcmp(cur_node->name, (const xmlChar *)"Sink"))
    {
      std::int32_t numPorts = getXmlChildNodeCount(cur_node);
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Number of ports to the sink device: ", numPorts);

      sinkDeviceList.emplace_back(numPorts);
      IasConfigSinkDevice::IasResult result = sinkDeviceList.at(numSinkDevices).setSinkParams(cur_node);

      if (result != IasConfigSinkDevice::eIasOk)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "setSinkParams Failed");
        return IasConfigParserResult::eIasFailed;
      }

      IasAudioSinkDevicePtr sinkDevicePtr = nullptr;
      IasISetup::IasResult setupRes = setup->createAudioSinkDevice(sinkDeviceList.at(numSinkDevices).getSinkDeviceParams(), &sinkDevicePtr);
      if (setupRes != IasISetup::eIasOk)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error creating : ", sinkDeviceList.at(numSinkDevices).getSinkDeviceParams().name, ": ",  toString(setupRes));
        return IasConfigParserResult::eIasFailed;
      }
      IAS_ASSERT(sinkDevicePtr != nullptr);
      sinkDeviceList.at(numSinkDevices).setSinkDevicePtr(sinkDevicePtr);

      //Parse sink device ports
      xmlNodePtr cur_node_port = nullptr;
      for (cur_node_port = cur_node->children; cur_node_port != nullptr ; cur_node_port = cur_node_port->next)
      {
        if (!xmlStrcmp(cur_node_port->name, (const xmlChar *)"Port"))
        {
          IasConfigSinkDevice::IasResult result = sinkDeviceList.at(numSinkDevices).setSinkPortParams(setup, cur_node_port);
          if(result != IasConfigSinkDevice::eIasOk)
          {
            DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "setSinkPortParams Failed");
            return IasConfigParserResult::eIasFailed;
          }
          sinkDeviceList.at(numSinkDevices).incrementSinkPortCounter();
        }
        else
        {
          DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Sink Port Node contain no Port tag");
          return IasConfigParserResult::eIasNodeNotFound;
        }
       }


       //Get the routing zone for the sink
       xmlNodePtr sink_rz_node = getXmlChildNodePtr(rootNode, "RoutingZones");
       if (sink_rz_node == nullptr)
       {
         DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Routing zones node not found");
         return IasConfigParserResult::eIasNodeNotFound;
       }

       for (sink_rz_node = sink_rz_node->children; sink_rz_node != nullptr ; sink_rz_node = sink_rz_node->next)
       {
         if (!xmlStrcmp(sink_rz_node->name, (const xmlChar *)"RoutingZone"))
         {
           if (getXmlAttributePtr(cur_node, "name") == getXmlAttributePtr(sink_rz_node,"sink"))
           {
             DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Sink Routing Zone: ", getXmlAttributePtr(sink_rz_node, "sink"), "Matched");
             sinkDeviceList.at(numSinkDevices).setRoutingZoneParamsName(getXmlAttributePtr(sink_rz_node,"name"));

             //Create and link routing zone
             IasConfigSinkDevice::IasResult result = sinkDeviceList.at(numSinkDevices).createLinkRoutingZone(setup);
             if (result != IasConfigSinkDevice::eIasOk)
             {
               DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "creatLinkRoutingZone Failed");
               return IasConfigParserResult::eIasFailed;
             }

             //Parse routing zone ports
             xmlNodePtr cur_rz_port = nullptr;
             for (cur_rz_port = sink_rz_node->children; cur_rz_port != nullptr ; cur_rz_port = cur_rz_port->next)
             {
               if (!xmlStrcmp(cur_rz_port->name, (const xmlChar *)"Port"))
               {
                 //Set routing zone port parameters
                 IasConfigSinkDevice::IasResult result = sinkDeviceList.at(numSinkDevices).setZonePortParams(setup, cur_rz_port, setupLinksNode);
                 if (result != IasConfigSinkDevice::eIasOk)
                 {
                   DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "setZonePortParams Failed");
                   return IasConfigParserResult::eIasFailed;
                 }
                 sinkDeviceList.at(numSinkDevices).incrementZonePortCounter();

               }
               else
               {
                 DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Routing zone node has no port tag");
               }
              }//Condition: Parsed all Port tags

              //Check if derived routing zone
              if (getXmlAttributePtr(sink_rz_node,"derived_from") != "")
              {
                bool derivedZoneFlag = false;
                for (int i = 0; i < numSinkDevices; i++)
                {
                  if (sinkDeviceList.at(i).getRoutingZoneParams().name ==  getXmlAttributePtr(sink_rz_node,"derived_from"))
                  {
                    IasISetup::IasResult setupRes =setup->addDerivedZone(sinkDeviceList.at(i).getMyRoutingZone(), sinkDeviceList.at(numSinkDevices).getMyRoutingZone());

                    if (setupRes != IasISetup::eIasOk)
                    {
                      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error adding derived zone to base zone:  ", toString(setupRes));
                      return IasConfigParserResult::eIasFailed;
                    }
                    derivedZoneFlag = true;
                  }
                  //not matched. move to next node...
                }
                if (derivedZoneFlag == false)
                {
                  DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Couldn't find derived zone");
                  return IasConfigParserResult::eIasFailed;
                }
              }//Condition: Routing zone not derived

              //Check if any pipeline defined for the routing zone
              xmlNodePtr rz_pip_node = getXmlChildNodePtr(sink_rz_node, "ProcessingPipeline");
              if (rz_pip_node != nullptr)
              {
                //create pipeline
                IasConfigPipeline pipeline;
                IasConfigPipeline::IasResult res = pipeline.initPipeline(setup, rz_pip_node);
                if(res != IasConfigPipeline::eIasOk)
                {
                  // Report error and exit the program, because many of the following commands will result
                  // in a segmentation fault if the pipeline has not been created pipeline appropriately.
                  DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error creating pipeline (please review DLT output for details");
                  return IasConfigParserResult::eIasFailed;
                }

                //initialize the pipeline chain
                IasISetup::IasResult result = setup->initPipelineAudioChain(pipeline.getPipelinePtr());
                if(result != IasISetup::eIasOk)
                {
                  DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error init pipeline chain - Please check DLT output for additional info.");
                  return IasConfigParserResult::eIasFailed;
                }

                //Add pipeline to the routing zone
                result = setup->addPipeline(sinkDeviceList.at(numSinkDevices).getMyRoutingZone(), pipeline.getPipelinePtr());
                if(result != IasISetup::eIasOk)
                {
                  DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error adding pipeline to zone");
                  return IasConfigParserResult::eIasFailed;
                }

                pipelines.push_back(pipeline);
                numPipelines++;
              }

            }//sink name didn't match. next node...
         }
         else
         {
           DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Routing Zones node does not Routing Zone tag");
           return IasConfigParserResult::eIasOk;
         }
       }//end traversing all sink routing nodes
    }
    else
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Sinks node does not contain Sink tag");
      return IasConfigParserResult::eIasOk;
    }

    ++numSinkDevices;

  }//end traversing all sink devices

  /*Connect all Processing pipeline links
   *
   * Routing zone ports -> pipeline input pins
   * Pipeline output pins -> sink input ports
   */

  //Get ProcessingPipelineLinks node
  xmlNodePtr ProcessingPipelineLinks= getXmlChildNodePtr(Links, "ProcessingPipelineLinks");
  if (ProcessingPipelineLinks != nullptr)
  {
    xmlNodePtr cur_ppLink_node;
    for (cur_ppLink_node = ProcessingPipelineLinks->children; cur_ppLink_node != nullptr ; cur_ppLink_node = cur_ppLink_node->next)
    {
      std::string output_port = getXmlAttributePtr(cur_ppLink_node, "pin"); //only pipeline
      std::string input_port = getXmlAttributePtr(cur_ppLink_node, "input_port"); //routing zone or sink device

      bool output_port_found = false;
      bool output_pin_found = false;
      bool linked = false;

      IasAudioPortPtr output_port_link; //routing zone port
      IasAudioPinPtr output_pin_link;   //pipeline output pin
      IasAudioPortPtr input_port_link;  //sink port
      IasAudioPinPtr input_pin_link;    //pipeline input pin

      //Possible output ports: routing zone/pipeline output ports
      for (int i = 0; i < numSinkDevices; i++) //traverse all routing zone ports
      {
        std::vector<IasAudioPortParams> routingzonePortParams = sinkDeviceList.at(i).getMyRoutingZonePortParams();
        std::vector<IasAudioPortParams>::iterator it_rzPortParams = routingzonePortParams.begin();

        std::vector<IasAudioPortPtr> routingzonePortPtrs = sinkDeviceList.at(i).getMyRoutingZonePorts();
        std::vector<IasAudioPortPtr>::iterator it_rzPortPtrs = routingzonePortPtrs.begin();

        for (; it_rzPortParams != routingzonePortParams.end(), it_rzPortPtrs != routingzonePortPtrs.end(); it_rzPortParams++, it_rzPortPtrs++)
        {
          if ((*it_rzPortParams).name == input_port)
          {
            DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "output Port found: ", (*it_rzPortParams).name);
            output_port_link = (*it_rzPortPtrs);
            output_port_found = true;
            break;
            //output found
          }
        }
        if (output_port_found == true)
        {
          break;
        }
      }//finish traversing all routing zone ports

      if(output_port_found == false)
      {
        //Traverse all pipelines output pins
        for (std::vector<IasConfigPipeline>::iterator iter_pipe = pipelines.begin(); iter_pipe != pipelines.end(); iter_pipe++)
        {

          std::vector<IasAudioPinParams> pipelineOutputPinParams = (*iter_pipe).getPipelineOutputPinParams();
          std::vector<IasAudioPinParams>::iterator it_pipelineOutputPinsParams = pipelineOutputPinParams.begin();

          std::vector<IasAudioPinPtr> pipelineOutputPinPtrs = (*iter_pipe).getPipelineOutputPins();
          std::vector<IasAudioPinPtr>::iterator it_pipelineOutputPinPtrs = pipelineOutputPinPtrs.begin();


          for (; it_pipelineOutputPinsParams != pipelineOutputPinParams.end(), it_pipelineOutputPinPtrs != pipelineOutputPinPtrs.end(); it_pipelineOutputPinsParams++, it_pipelineOutputPinPtrs++)
          {
            if ((*it_pipelineOutputPinsParams).name == output_port)
            {
              DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "output Pin found: ", (*it_pipelineOutputPinsParams).name);
              output_pin_link = (*it_pipelineOutputPinPtrs);
              output_pin_found = true;
              break;
            }
          }//end of inner for loop
          if (output_pin_found == true)
          {
            break;
          }
        }//end of outer for loop
      }

      if(output_port_found == false && output_pin_found == false)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "No output port/pin found in routing zone and pipeline to form the link");
        return IasConfigParserResult::eIasFailed;
      }

      //Possible input ports: pipeline input/sink input ports
      //Traverse all pipelines input pins or all sink input ports
      for (std::vector<IasConfigPipeline>::iterator iter_pipe = pipelines.begin(); iter_pipe != pipelines.end(); iter_pipe++)
      {
        std::vector<IasAudioPinParams> pipelineInputPinParams = (*iter_pipe).getPipelineInputPinParams();
        std::vector<IasAudioPinParams>::iterator it_pipelineInputPinsParams = pipelineInputPinParams.begin();

        std::vector<IasAudioPinPtr> pipelineInputPinPtrs = (*iter_pipe).getPipelineInputPins();
        std::vector<IasAudioPinPtr>::iterator it_pipelineInputPinPtrs = pipelineInputPinPtrs.begin();

        for (; it_pipelineInputPinsParams != pipelineInputPinParams.end(), it_pipelineInputPinPtrs != pipelineInputPinPtrs.end(); it_pipelineInputPinsParams++, it_pipelineInputPinPtrs++)
        {
          if ((*it_pipelineInputPinsParams).name == output_port)
          {
            input_pin_link = (*it_pipelineInputPinPtrs);
            DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "input_pin_link: ", (*it_pipelineInputPinsParams).name);
            //input_pin_found = true;
            if (output_port_found == true)
            {
              //Make connection here
              IasISetup::IasResult res = setup->link(output_port_link, input_pin_link);
              if(res != IasISetup::eIasOk)
              {
                DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error linking zone output port to pipeline input pin");
                return IasConfigParserResult::eIasNullPointer;
              }
              linked = true;
              break;
            }
            else
            {
              //output port is of type pin. This can never be true as routing zone has ports
              DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, " Error: Pipeline input can only be connected to routing zone port.");
              return IasConfigParserResult::eIasFailed;
            }
          }//end of inner for loop
        }//Traverse all the sink devices

        if (linked == true)
        {
          //Linked
          break;
        }
      }
      if (linked == true)
      {
        //Linked, move to next link connection
        continue;
      }
      //Traverse all sinks ports
      for (int i = 0; i < numSinkDevices; i++) //traverse all sink devices ports
      {
          std::vector<IasAudioPortParams> sinkPortParams = sinkDeviceList.at(i).getSinkPortParams();
          std::vector<IasAudioPortParams>::iterator it_sinkPortParams = sinkPortParams.begin();

          std::vector<IasAudioPortPtr> sinkPortPtrs = sinkDeviceList.at(i).getSinkPorts();
          std::vector<IasAudioPortPtr>::iterator it_sinkPortPtrs = sinkPortPtrs.begin();


          for (; it_sinkPortParams != sinkPortParams.end(), it_sinkPortPtrs != sinkPortPtrs.end(); it_sinkPortParams++, it_sinkPortPtrs++)
          {
            if((*it_sinkPortParams).name == input_port)
            {
              input_port_link = (*it_sinkPortPtrs);
              if (output_port_found == true)
              {
                //Make connection here
                IasISetup::IasResult res = setup->link(output_port_link, input_port_link);
                if(res != IasISetup::eIasOk)
                {
                   DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error linking zone output port to sink input pin");
                   return IasConfigParserResult::eIasNullPointer;
                }
                linked = true;
                break;
              }
              else
              {
                //Make connection here
                IasISetup::IasResult res = setup->link( input_port_link, output_pin_link);
                if(res != IasISetup::eIasOk)
                {
                  DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error linking pipeline output pin sink input pin");
                  return IasConfigParserResult::eIasNullPointer;
                }
                linked = true;
                break;
              }
            }
          }//end traversing all sink pins
          if(linked == true)
          {
            break;
          }
        }//end traversing all sink devices
      //link already established
      }//end traversing all link connections
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "No Processing Pipeline link node");
  }

  return IasConfigParserResult::eIasOk;
}

IasConfigParserResult parseSourceDevices(IasISetup *setup, const xmlNodePtr rootNode)
{
  DltContext *mLog(IasAudioLogging::registerDltContext("PAR", "XML SOURCEPARSER"));

  if (setup == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "setup == nullptr");
    return IasConfigParserResult::eIasNullPointer;
  }

  xmlNodePtr cur_node = nullptr;

  //Get sources node
  cur_node = getXmlChildNodePtr(rootNode, "Sources");
  if (cur_node == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Sources node not found");
    return IasConfigParserResult::eIasNodeNotFound;
  }


  for (cur_node = cur_node->children; cur_node != nullptr ; cur_node = cur_node->next)
  {
    if (!xmlStrcmp(cur_node->name, (const xmlChar *)"Source"))
    {
      std::int32_t const numPorts = getXmlChildNodeCount(cur_node);

      IasConfigSourceDevice sourceDevice(numPorts);

      IasConfigSourceDevice::IasResult result = sourceDevice.setSourceParams(cur_node);
      if (result != IasConfigSourceDevice::eIasOk)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "setSourceParams Failed");
        return IasConfigParserResult::eIasFailed;
      }

      IasAudioSourceDevicePtr sourceDevicePtr = nullptr;

      IasISetup::IasResult setupRes = setup->createAudioSourceDevice(sourceDevice.getSourceDeviceParams(), &sourceDevicePtr);
      if (setupRes != IasISetup::eIasOk)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error creating: ", sourceDevice.getSourceDeviceParams().name, ": ", toString(setupRes));
        return IasConfigParserResult::eIasFailed;
      }
      IAS_ASSERT(sourceDevicePtr != nullptr);
      sourceDevice.setSourceDevicePtr(sourceDevicePtr);

      //Parse source device ports
      xmlNodePtr cur_node_port = nullptr;
      for (cur_node_port = cur_node->children; cur_node_port != nullptr ; cur_node_port = cur_node_port->next)
      {
        if (!xmlStrcmp(cur_node_port->name, (const xmlChar *)"Port"))
        {
          IasConfigSourceDevice::IasResult result = sourceDevice.setSourcePortParams(setup, cur_node_port);
          if(result != IasConfigSourceDevice::eIasOk)
          {
            DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "setSourcePortParams Failed");
            return IasConfigParserResult::eIasFailed;
          }
          sourceDevice.incrementSourcePortCounter();
        }
        else
        {
          DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Sources node contains no Port tag");
          return IasConfigParserResult::eIasOk;
        }
      }
    }
    else
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Sources node contains no Source tag");
      return IasConfigParserResult::eIasOk;
    }
  }
  return IasConfigParserResult::eIasOk;

}

IasConfigParserResult parseRoutingLinks(IasIRouting *routing, const xmlNodePtr rootNode)
{
  DltContext *mLog(IasAudioLogging::registerDltContext("PAR", "XML PARSEROUTINGLINKS"));
  xmlNodePtr cur_node = nullptr;

  //Get links node
  cur_node = getXmlChildNodePtr(rootNode, "Links");
  if (cur_node == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "No Links Node Found");
    return IasConfigParserResult::eIasNodeNotFound;
  }

  //Get RoutingLinks node
  cur_node = getXmlChildNodePtr(cur_node, "RoutingLinks");
  if (cur_node == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "No Routing Links Found");
    return IasConfigParserResult::eIasOk;
  }

  std::int32_t sourceId;
  std::int32_t sinkId;

  //Parse all routing links
  xmlNodePtr cur_rl_node = nullptr;
  for (cur_rl_node = cur_node->children; cur_rl_node != nullptr ; cur_rl_node = cur_rl_node->next)
  {
    try
    {
      sourceId = stoui(getXmlAttributePtr(cur_rl_node,"output_port_id"));
      sinkId = stoui(getXmlAttributePtr(cur_rl_node,"rz_input_port_id"));

      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Connecting source: ", sourceId, "to sink: ", sinkId );
    }
    catch(...)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error in sourceId/sinkId");
      return IasConfigParserResult::eIasFailed;
    }

    if (routing->connect(sourceId, sinkId) != IasAudio::IasIRouting::eIasOk)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Failed to connect source: ", sourceId, "to sink: ", sinkId);
      return IasConfigParserResult::eIasFailed;
    }
  }

  return IasConfigParserResult::eIasOk;
}

/*
 * Function to get a IasConfigParserResult as string.
 */
#define STRING_RETURN_CASE(name) case name: return std::string(#name); break
#define DEFAULT_STRING(name) default: return std::string(name)
std::string toString(const IasConfigParserResult& type)
{
  switch(type)
  {
    STRING_RETURN_CASE(IasConfigParserResult::eIasOk);
    STRING_RETURN_CASE(IasConfigParserResult::eIasFailed);
    STRING_RETURN_CASE(IasConfigParserResult::eIasNodeNotFound);
    STRING_RETURN_CASE(IasConfigParserResult::eIasOutOfMemory);
    STRING_RETURN_CASE(IasConfigParserResult::eIasNullPointer);
    STRING_RETURN_CASE(IasConfigParserResult::eIasTimeout);
    STRING_RETURN_CASE(IasConfigParserResult::eIasNoEventAvailable);
    DEFAULT_STRING("Invalid IasConfigParserResult => " + std::to_string(type));
  }
}

}


