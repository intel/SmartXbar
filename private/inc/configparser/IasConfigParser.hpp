/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasConfigParser.hpp
 * @date   2017
 * @brief  The smartXConfigParser parsing interface
 */


#ifndef IASCONFIGPARSERFILE_HPP
#define IASCONFIGPARSERFILE_HPP


#include <libxml2/libxml/parser.h>
#include <libxml2/libxml/tree.h>
#include <libxml2/libxml/xmlmemory.h>
#include "audio/configparser/IasSmartXconfigParser.hpp"


namespace IasAudio {

/**
 * @brief The result type for the smartXconfigparser
 */
enum IasConfigParserResult
{
  eIasOk,                        //!< Operation successful
  eIasFailed,                    //!< Operation failed
  eIasNodeNotFound,              //!< Node not found
  eIasOutOfMemory,               //!< Out of memory during memory allocation
  eIasNullPointer,               //!< One of the parameters was a nullptr
  eIasTimeout,                   //!< Timeout occured
  eIasNoEventAvailable           //!< No event available in the event queue
};

/**
 * @brief Parse sink devices
 *
 * @param[in] setup Pointer to the smartx setup
 * @param[in] rootNode Pointer to XML  root node
 * @return Status of parsing sink devices
 */
 IasConfigParserResult parseSinkDevices(IasISetup *setup, const xmlNodePtr rootNode);

/**
 * @brief Parse source devices
 *
 * @param[in] setup Pointer to the smartx setup
 * @param[in] rootNode Pointer to XML  root node
 * @return Status of parsing source devices
 */
 IasConfigParserResult parseSourceDevices(IasISetup *setup, const xmlNodePtr rootNode);

/**
 * @brief Parse Routing Links
 *
 * @param[in] routing  Pointer to the smartx routing
 * @param[in] rootNode Pointer to XML root node
 * @return Status of parsing RoutingLinks
 */
 IasConfigParserResult parseRoutingLinks(IasIRouting *routing, const xmlNodePtr rootNode);

 /**
  * @brief Function to get a IasConfigParserResult as string.
  *
  * @param[in] IasConfigParserResult type
  * @return String carrying the result message.
  */
 std::string toString(const IasConfigParserResult& type);

}


#endif // IASCONFIGPARSERFILE_HPP
