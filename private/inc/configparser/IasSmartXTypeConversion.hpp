/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasSmartXTypeConversion.hpp
 * @date   2017
 * @brief  Helper functions to convert strings to smartXbar internal types
 */


#ifndef IASSMARTXTYPECONVERSION_HPP
#define IASSMARTXTYPECONVERSION_HPP

#include <libxml2/libxml/parser.h>
#include <libxml2/libxml/tree.h>
#include "audio/smartx/IasSmartX.hpp"


namespace IasAudio {

/*!
 * @brief Returns smartXbar clockType
 *
 * @param[in] clockType string clockType.
 * @return IasClockType
 */
IasClockType toClockType(const std::string& clockType);

/*!
 * @brief Returns xml schema clockType
 *
 * @param[in] clockType string clockType.
 * @return clockType string
 */
std::string toXmlClockType(const std::string& clockType);

/*!
 * @brief Returns smartXbar dataFormat
 *
 * @param[in] data_format string data format.
 * @return IasAudioCommonDataFormat
 */
IasAudioCommonDataFormat toDataFormat(const std::string& data_format);

/*!
 * @brief Returns xml schema dataFormat
 *
 * @param[in] data_format string data format.
 * @return data format string
 */
std::string toXmlDataFormat(const std::string& data_format);

/*!
 * @brief Returns smartXbar audio pin link type
 *
 * @param[in] pinLinkType string pin link type.
 * @return IasAudioPinLinkType
 */
IasAudioPinLinkType toAudioPinLinkType(const std::string& pinLinkType);

}//end of IasAudio namespace

#endif // IASSMARTXTYPECONVERSION_HPP
