/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file    IasProcessingTypes.hpp
 * @date    2012
 * @brief   The definition of generic audio processing types.
 */

#ifndef IASPROCESSINGTYPES_HPP_
#define IASPROCESSINGTYPES_HPP_

#include "avbaudiomodules/audio/common/IasAudioCommonTypes.hpp"
#include <memory>

/**
 * @namespace IasAudio
 * @brief The common namespace for all audio related classes.
 */
namespace IasAudio {

class IasAudioDevice;
class IasAudioStream;

/**
 * @brief Struct used to create an mapping between an enumValue and the corresponding string.
 */
template<typename ENUM_TYPE>
struct EnumStringStruct
{
    /**
     * @brief The enumValue.
     */
    ENUM_TYPE enumValue;

    /**
     * @brief The corresponding String value.
     */
    const char *stringValue;
};


static const char * emptyString = "";


/**
 * @brief Implementation of the conversion from a enum to a string.
 *
 * @param enumStringStruct mapping table of enum to string.
 * @param numElements numbers of elements of enumStringStruct.
 * @param enumValue the enumValue that should be converted.
 * @return the converted String. Empty if an error occurs.
 */
template<typename T>
char const * toString(const EnumStringStruct<T>  * const enumStringStruct, size_t numElements, T const enumValue)
{
  if(enumStringStruct != NULL)
  {
    for(uint32_t i = 0u; i < numElements; ++i)
    {
      if(enumStringStruct[i].enumValue == enumValue)
      {
        return enumStringStruct[i].stringValue;
      }
    }
  }
  return emptyString;
}

/**
 * @brief Implementation of the conversion from a String to an enum
 *
 * @param enumString the string value that should be converted
 * @param enumStringStruct mapping table between enum and string
 * @param numElements numElements numbers of elements of enumStringStruct
 * @param enumValue the resulting enum value
 * @return true or false
 */
template<typename T>
bool toEnum(std::string const & enumString, const EnumStringStruct<T> * const enumStringStruct, size_t numElements, T &enumValue)
{
  bool result = false;

  if (enumStringStruct != 0)
  {
    for (uint32_t i = 0u; (i < numElements) && !result; ++i)
    {
      if (enumString.compare(enumStringStruct[i].stringValue) == 0)
      {
        enumValue = enumStringStruct[i].enumValue;
        result = true;
      }
    }
  }
  return result;
}
/**
 * @brief Result values used by the audio components.
 */
enum IasAudioProcessingResult
{
  eIasAudioProcOK = 0,
  eIasAudioProcInvalidParam,
  eIasAudioProcOff,
  eIasAudioProcInitializationFailed,
  eIasAudioProcNotInitialization,
  eIasAudioProcNoSpaceLeft,
  eIasAudioProcNotEnoughMemory,
  eIasAudioProcAlreadyInUse,
  eIasAudioProcCallbackError,
  eIasAudioProcUnsupportedFormat,
  eIasAudioProcNotImplemented,
  eIasAudioProcCouldNotBeStarted,
  eIasAudioProcCouldNotBeStopped,
  eIasAudioProcCouldNotBeOpened,
  eIasAudioProcCouldNotBeClosed,
  eIasAudioProcWrongState,
  eIasAudioProcIdAlreadyExists,
  eIasAudioProcTooManyChannels,
  eIasAudioProcAlreadyAdded,
  eIasAudioProcInvalidDevice,
  eIasAudioProcInvalidStream,
  eIasAudioProcInvalidDeviceChanIdx,
  eIasAudioProcInvalidStreamChanIdx,
  eIasAudioProcNothingRemoved,
  eIasAudioProcRegistrationFailed,
};

static EnumStringStruct<IasAudioProcessingResult> const enumIasAudioProcessingResultToStringMapping[] =
{
  { eIasAudioProcOK,                     "eIasAudioProcOK" },
  { eIasAudioProcInvalidParam,           "eIasAudioProcInvalidParam" },
  { eIasAudioProcOff,                    "eIasAudioProcOff" },
  { eIasAudioProcInitializationFailed,   "eIasAudioProcInitializationFailed" },
  { eIasAudioProcNotInitialization,      "eIasAudioProcNotInitialization" },
  { eIasAudioProcNoSpaceLeft,            "eIasAudioProcNoSpaceLeft" },
  { eIasAudioProcNotEnoughMemory,        "eIasAudioProcNotEnoughMemory" },
  { eIasAudioProcAlreadyInUse,           "eIasAudioProcAlreadyInUse" },
  { eIasAudioProcCallbackError,          "eIasAudioProcCallbackError" },
  { eIasAudioProcUnsupportedFormat,      "eIasAudioProcUnsupportedFormat" },
  { eIasAudioProcNotImplemented,         "eIasAudioProcNotImplemented" },
  { eIasAudioProcCouldNotBeStarted,      "eIasAudioProcCouldNotBeStarted" },
  { eIasAudioProcCouldNotBeStopped,      "eIasAudioProcCouldNotBeStopped" },
  { eIasAudioProcCouldNotBeOpened,       "eIasAudioProcCouldNotBeOpened" },
  { eIasAudioProcCouldNotBeClosed,       "eIasAudioProcCouldNotBeClosed" },
  { eIasAudioProcWrongState,             "eIasAudioProcWrongState" },
  { eIasAudioProcIdAlreadyExists,        "eIasAudioProcIdAlreadyExists" },
  { eIasAudioProcTooManyChannels,        "eIasAudioProcTooManyChannels" },
  { eIasAudioProcAlreadyAdded,           "eIasAudioProcAlreadyAdded" },
  { eIasAudioProcInvalidDevice,          "eIasAudioProcInvalidDevice" },
  { eIasAudioProcInvalidStream,          "eIasAudioProcInvalidStream" },
  { eIasAudioProcInvalidDeviceChanIdx,   "eIasAudioProcInvalidDeviceChanIdx" },
  { eIasAudioProcInvalidStreamChanIdx,   "eIasAudioProcInvalidStreamChanIdx" },
  { eIasAudioProcNothingRemoved,         "eIasAudioProcNothingRemoved" },
  { eIasAudioProcRegistrationFailed,     "eIasAudioProcRegistrationFailed" },
};

static inline char const* toString(IasAudioProcessingResult enumValue)
{
  return toString(enumIasAudioProcessingResultToStringMapping, ARRAYLEN(enumIasAudioProcessingResultToStringMapping), enumValue);
}


/**
 * @brief Data type for defining the filter type. Required for Volume/Loudness, Equalizer, ...
 */
enum IasAudioFilterTypes
{
  eIasFilterTypeFlat = 0,
  eIasFilterTypePeak,
  eIasFilterTypeLowpass,
  eIasFilterTypeHighpass,
  eIasFilterTypeLowShelving,
  eIasFilterTypeHighShelving,
  eIasFilterTypeBandpass
};

/**
 * @brief Function to get a IasAudioFilterTypes as string.
 *
 * @return Enum Member as string
 * @return eIasFilterTypeInvalid on unknown value.
 */
std::string toString(const IasAudioFilterTypes& type);

/**
 * @brief The possible ramp shapes
 */
enum IasRampShapes
{
  eIasRampShapeLinear,        //!< Linear ramp
  eIasRampShapeExponential,   //!< Exponential ramp
  eIasRampShapeLogarithm,     //!< Logarithmic ramp
  eIasMaxNumRampShapes        //!< LAST ENTRY
};

/**
 * @brief Function to get a IasRampShapes as string.
 *
 * @return Enum Member as string
 * @return eIasRampShapeInvalid on unknown value.
 */
std::string toString(const IasRampShapes& type);


/**
 *  @brief Struct defining the parameters that are required to configure one filter stage.
 */
struct IasAudioFilterConfigParams
{
  uint32_t         freq;     ///< cut-off frequency or mid frequency of the filter
  float        gain;     ///< gain (linear, not in dB), required only for peak and shelving filters
  float        quality;  ///< quality, required only for band-pass and peak filters
  IasAudioFilterTypes type;     ///< filter type
  uint32_t         order;    ///< filter order
};

/**
 * @brief A vector of float pointers defines an audio frame.
 */
using IasAudioFrame = std::vector<float*>;


class IasAudioChainEnvironment;
/**
 * @brief Shared ptr for IasAudioChainEnvironment
 */
using IasAudioChainEnvironmentPtr = std::shared_ptr<IasAudioChainEnvironment>;


class IasBundleAssignment;
/**
 * @brief Vector of IasBundleAssignments
 */
using IasBundleAssignmentVector = std::vector<IasBundleAssignment>;

} //namespace IasAudio

#endif /* IASPROCESSINGTYPES_HPP_ */
