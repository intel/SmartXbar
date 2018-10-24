/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasParseHelper.hpp
 * @date   2017
 * @brief  Help functions to parse XML
 */


#ifndef IASPARSEHELPERFILE_HPP
#define IASPARSEHELPERFILE_HPP

#include <libxml2/libxml/parser.h>
#include <libxml2/libxml/tree.h>
#include <iostream>
#include "internal/audio/common/IasAudioLogging.hpp"

/*!
 * @brief To retrieve the node attribute value as string
 *
 * @param[in] nodePtr Pointer to the node.
 * @param[in] attribute Attribute name.
 * @return String attribute
 */
std::string getXmlAttributePtr(const xmlNodePtr nodePtr, const char *attribute);

/*!
 * @brief Returns pointer to the child node with specified name
 *
 * @param[in] nodePtr Pointer to the node
 * @param[in] nodeName Name of the child node
 * @return xmlNodePtr to the child node
 */
xmlNodePtr getXmlChildNodePtr(xmlNodePtr nodePtr, const char *nodeName);

/*!
 * @brief Counts number of child nodes
 *
 * @param[in] nodePtr Pointer to the node
 * @return Number of childs to a node
 */
std::int32_t getXmlChildNodeCount(const xmlNodePtr nodePtr);

/*!
 * @brief Convert string to unsignedInt
 *
 * @param[in] integer Integer string
 * @return converted unsigned int
 */
std::int32_t stoui(const std::string& integer, size_t* pos = 0, int base = 10);



#endif // IASPARSEHELPERFILE_HPP
