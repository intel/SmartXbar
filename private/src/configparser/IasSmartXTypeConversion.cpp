/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file   IasSmartXTypeConversion.cpp
 * @date   2017
 * @brief  SmartXbar ConversionType.
 */

#include "configparser/IasSmartXTypeConversion.hpp"
#include "avbaudiomodules/internal/audio/common/IasAudioLogging.hpp"

namespace IasAudio {

IasClockType toClockType(const std::string& clockType)
{
  if(clockType == "ClockReceived")
    return eIasClockReceived;
  else if(clockType == "ClockProvided")
    return eIasClockProvided;
  else if(clockType == "ClockReceivedAsync")
    return eIasClockReceivedAsync;
  else
    return eIasClockUndef;
}

std::string toXmlClockType(const std::string& clockType)
{
  if(clockType == "eIasClockReceived")
    return "ClockReceived";
  else if(clockType == "eIasClockProvided")
    return "ClockProvided";
  else if(clockType == "eIasClockReceivedAsync")
    return "ClockReceivedAsync";
  else
    return "ClockUndef";
}

std::string toXmlDataFormat(const std::string& dataFormat)
{
  if(dataFormat == "eIasFormatFloat32")
    return "Float32";
  else if(dataFormat == "eIasFormatInt16")
    return "Int16";
  else if(dataFormat == "eIasFormatInt32")
    return "Int32";
  else
    return "FormatUndef";
}

IasAudioCommonDataFormat toDataFormat(const std::string& dataFormat)
{
  if(dataFormat == "Float32")
    return eIasFormatFloat32;
  else if(dataFormat == "Int16")
    return eIasFormatInt16;
  else if(dataFormat == "Int32")
    return eIasFormatInt32;
  else
    return eIasFormatUndef;
}

IasAudioPinLinkType toAudioPinLinkType(const std::string& pinLinkType)
{
  if(pinLinkType == "Immediate")
    return eIasAudioPinLinkTypeImmediate;
  else //By default returns delayed: pinLinkType == "Delayed". Add eIasAudioPinLinkTypeUndef in IasAudioCommonTypes to return Undef condition
    return eIasAudioPinLinkTypeDelayed;
}

}// end of IasAudio namespace
