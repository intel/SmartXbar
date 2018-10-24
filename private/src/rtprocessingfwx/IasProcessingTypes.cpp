/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasProcessingTypes.cpp
 * @date   July 20, 2016
 * @brief  Provides toString functions for enum types.
 */

#include "audio/smartx/rtprocessingfwx/IasProcessingTypes.hpp"

namespace IasAudio {

#define STRING_RETURN_CASE(name) case name: return std::string(#name); break
#define DEFAULT_STRING(name) default: return std::string(name)


IAS_AUDIO_PUBLIC std::string toString ( const IasAudioFilterTypes& type )
{
  switch(type)
  {
    STRING_RETURN_CASE(eIasFilterTypeFlat);
    STRING_RETURN_CASE(eIasFilterTypePeak);
    STRING_RETURN_CASE(eIasFilterTypeLowpass);
    STRING_RETURN_CASE(eIasFilterTypeHighpass);
    STRING_RETURN_CASE(eIasFilterTypeLowShelving);
    STRING_RETURN_CASE(eIasFilterTypeHighShelving);
    STRING_RETURN_CASE(eIasFilterTypeBandpass);
    DEFAULT_STRING("eIasFilterTypeInvalid");
  }
}


IAS_AUDIO_PUBLIC std::string toString(const IasRampShapes& type)
{
  switch(type)
  {
    STRING_RETURN_CASE(eIasRampShapeLinear);
    STRING_RETURN_CASE(eIasRampShapeExponential);
    STRING_RETURN_CASE(eIasRampShapeLogarithm);
    DEFAULT_STRING("eIasRampShapeInvalid");
  }
}

#undef STRING_RETURN_CASE
#undef DEFAULT_STRING

} // namespace IasAudio
