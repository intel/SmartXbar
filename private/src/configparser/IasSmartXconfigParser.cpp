/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file   IasSmartXconfigParser.cpp
 * @date   2017
 * @brief  The SmartXbar XML Parser.
 *
 *
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "audio/smartx/IasSmartX.hpp"
#include "audio/smartx/IasIRouting.hpp"
#include "audio/smartx/IasISetup.hpp"
#include "audio/smartx/IasIDebug.hpp"
#include "audio/smartx/IasIProcessing.hpp"
#include "audio/smartx/IasSetupHelper.hpp"
#include "audio/smartx/IasEventHandler.hpp"
#include "audio/smartx/IasEvent.hpp"
#include "audio/smartx/IasConnectionEvent.hpp"
#include "audio/smartx/IasSetupEvent.hpp"
#include "audio/configparser/IasSmartXconfigParser.hpp"
#include "configparser/IasConfigParser.hpp"
#include "configparser/IasParseHelper.hpp"
#include "avbaudiomodules/internal/audio/common/IasAudioLogging.hpp"

using namespace std;

static const std::string fFunction = "XMLCONFIGPARSER::";
#define LOG_PREFIX fFunction +  __func__ + "(" + std::to_string(__LINE__) + "):"

namespace IasAudio{

bool parseConfig(IasAudio::IasSmartX *smartx, const char * xmlFileName)
{
  DltContext *mLog(IasAudioLogging::registerDltContext("PAR", "XML CONF"));
  DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, "starting XML parser");

  DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, "Inside the SmartXConfigParser WP2 Library");

  if (xmlFileName == nullptr )
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid Condition: xmlFileName is nullptr");
    return false;
  }

  //check xml file extension
  std::string fileName(xmlFileName, strnlen(xmlFileName, strlen(xmlFileName)));
  if(strcasecmp((fileName.substr(fileName.find_last_of(".") + 1)).c_str(), "xml") >= 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "file extension valid");
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid file extension");
    return false;
  }

  if (smartx == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: smartx == nullptr");
    return false;
  }
  if (smartx->isAtLeast(SMARTX_API_MAJOR, SMARTX_API_MINOR, SMARTX_API_PATCH) == false)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "SmartX API version does not match");
    return false;
  }

  IasAudio::IasISetup *setup = smartx->setup();
  IAS_ASSERT(setup != nullptr);
  IasAudio::IasIRouting *routing = smartx->routing();
  IAS_ASSERT(routing != nullptr);

  xmlDocPtr doc = xmlReadFile(xmlFileName, NULL, XML_PARSE_NOBLANKS);
  if (doc == nullptr )
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid Condition: Document not parsed successfully");
    return false;
  }

  xmlNodePtr rootNode = xmlDocGetRootElement(doc);
  if (rootNode == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid Condition: empty document");
    xmlFreeDoc(doc);
    return false;
  }

  DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, "Parsing sink devices");
  //parse sink devices
  IasConfigParserResult result = parseSinkDevices(setup, rootNode);
  if (result != IasConfigParserResult::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Parsing sink devices Failed: ", toString(result));
    xmlFreeDoc(doc);
    return false;
  }

  DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, "Parsing source devices");
  //parse source devices
  result = parseSourceDevices(setup, rootNode);
  if (result != IasConfigParserResult::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Parsing source devices Failed: ", toString(result));
    xmlFreeDoc(doc);
    return false;
  }

  DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, "starting smartx");
  //start smartXbar to initialize the devices
  IasAudio::IasSmartX::IasResult smxres = smartx->start();
  // We are not interested in the result of the start. Details about what can be started and what not can be found
  // in the DLT. A printout here is just distracting.
  (void)smxres;

  DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, "Parsing routing links");
  //parse routing links
  result = parseRoutingLinks(routing, rootNode);
  if (result != IasConfigParserResult::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Parsing routing links Failed: ", toString(result));
    xmlFreeDoc(doc);
    return false;
  }

  return true;

}
}

