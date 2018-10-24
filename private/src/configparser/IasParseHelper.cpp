/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file   IasParseHelper.cpp
 * @date   2017
 * @brief  The XML Parser Helper functions.
 */

#include "configparser/IasParseHelper.hpp"

static const std::string fFunction = "XMLPARSER::";
#define LOG_PREFIX fFunction +  __func__ + "(" + std::to_string(__LINE__) + "):"

std::string getXmlAttributePtr(const xmlNodePtr nodePtr, const char *attribute)
{
  xmlChar *xmlValue = xmlGetProp(nodePtr, const_cast<unsigned char *>(reinterpret_cast<const unsigned char *>(attribute)));
  if (xmlValue == nullptr)
  {
    return "";
  }
  std::string value(reinterpret_cast<const char *>(xmlValue));
  xmlFree(xmlValue);
  return value;

}


xmlNodePtr getXmlChildNodePtr(xmlNodePtr nodePtr, const char *nodeName)
{
  xmlNodePtr cur_node = nullptr;
  for (cur_node = nodePtr->children; cur_node != nullptr ; cur_node = cur_node->next)
  {
    if (!xmlStrcmp(cur_node->name, const_cast<unsigned char *>(reinterpret_cast<const unsigned char *>(nodeName))))
    {
      return cur_node;
    }
  }
  //Error condition node not found
  return nullptr;
}


std::int32_t getXmlChildNodeCount(const xmlNodePtr nodePtr)
{
  int countChildNodes = 0;
  xmlNodePtr cur_node = nullptr;

  for (cur_node = nodePtr->children; cur_node != nullptr ; cur_node = cur_node->next)
  {
    ++countChildNodes;
  }

  return countChildNodes;
}

std::int32_t stoui(const std::string& integer, size_t* pos /*=0*/, int base /*=10*/)
{
  DltContext *mLog(IasAudio::IasAudioLogging::registerDltContext("PAR", "XML PARSEHELPER"));

  int value= 0;

  try
  {
    value = stoi(integer, pos, base);
    if( value < 0 )
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid attribute value. Should only be positive for assignment to unsigned");
      throw std::out_of_range ("invalid value.");
    }
  }
  catch(...)
  {
    throw std::out_of_range ("invalid value.");
    //exit (EXIT_FAILURE);
  }

  return value;
}
