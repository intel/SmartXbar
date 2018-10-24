/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file IasEqualizerCmd.hpp
 * @date 2016
 * @brief This file contains the declaration for all public command and control related types of the equalizer module.
 */


#ifndef IASEQUALIZERCMD_HPP_
#define IASEQUALIZERCMD_HPP_


/*!
 * @brief namespace IasAudio
 */
namespace IasAudio {


/*!
 * @brief namespace IasEqualizer
 */
namespace IasEqualizer {


/**
 * @brief Mode option
 */
enum class IasEqualizerMode
{
  eIasUser,                      //!< Equalizer cmd interface user mode
  eIasCar,                       //!< Equalizer cmd interface car mode
  eIasLastEntry = eIasCar        //!< ATTENTION: Always has to be last entry
};


/**
 * @brief this is the list of the available commands of the audio processing interface
 */
enum IasEqualizerCmdIds
{
  // Commands available in both modes
  eIasSetModuleState,                      //!< Turn the module on or off

  // User Equalizer commands
  eIasUserModeSetEqualizer,                //!< Set gain
  eIasUserModeSetEqualizerParams,          //!< Set params
  eIasSetConfigFilterParamsStream,         //!< Set config filter params
  eIasGetConfigFilterParamsStream,         //!< Get config filter params
  eIasUserModeSetRampGradientSingleStream, //!< Set gradient

  // Car Equalizer commands
  eIasCarModeSetEqualizerNumFilters,       //!< Set number of filters
  eIasCarModeSetEqualizerFilterParams,     //!< Set params
  eIasCarModeGetEqualizerNumFilters,       //!< Get number of filters
  eIasCarModeGetEqualizerFilterParams,     //!< Get params

  // Helpers data
  eIasLastEntry,                          //!< ATTENTION: Always has to be last entry

  eIasUserModeFirst = eIasUserModeSetEqualizer,          //!< First user mode command
  eIasUserModeLast = eIasUserModeSetRampGradientSingleStream,     //!< Last user mode command

  eIasCarModeFirst =  eIasCarModeSetEqualizerNumFilters, //!< First car mode command
  eIasCarModeLast = eIasCarModeGetEqualizerFilterParams  //!< Last car mode command
};

/**
 * @brief The event types for the equalizer module
 */
enum IasEqualizerEventTypes
{
  eIasGainRampingFinished,                 //!< The gain rampind has finished
};


} // namespace IasEqualizer

} // namespace IasAudio

#endif /* IASEQUALIZERCMD_HPP_ */
