/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file IasVolumeCmd.hpp
 * @date 2016
 * @brief This file contains the declaration for all public command and control related types of the volume/loudness module.
 */

#ifndef IASVOLUMECMD_HPP_
#define IASVOLUMECMD_HPP_

namespace IasAudio {

/**
 * @brief The namespace for all public command and control related types of the volume/loudness module.
 */
namespace IasVolume {

/**
 * @brief The command ids for the volume module
 */
enum IasVolumeCmdIds
{
  eIasSetModuleState,                 //!< Turn the module on or off
  eIasSetVolume,                      //!< Set new volume
  eIasSetMuteState,                   //!< Set mute state
  eIasSetLoudness,                    //!< Set the loudness parameters
  eIasSetSpeedControlledVolume,       //!< Set the speed controlled volume
  eIasSetLoudnessTable,               //!< Set the loudness table
  eIasGetLoudnessTable,               //!< Get the loudness table
  eIasSetSpeed,                       //!< Set the current speed
  eIasSetLoudnessFilter,              //!< Set the loudness filter parameters
  eIasGetLoudnessFilter,              //!< Get the loudness filter parameters
  eIasSetSdvTable,                    //!< Set the SDV table
  eIasGetSdvTable,                    //!< Get the SDV table
  eIasLastEntry,                      //!< ATTENTION: Always has to be last entry
};

/**
 * @brief The event types for the volume module
 */
enum IasVolumeEventTypes
{
  eIasVolumeFadingFinished,           //!< The volume fade has finished
  eIasLoudnessSwitchFinished,         //!< The loudness switch has finished
  eIasSpeedControlledVolumeFinished,  //!< The speed controlled volume has finished
  eIasSetMuteStateFinished,           //!< The set mute state command has finished
};


} // namespace IasVolume
} // namespace IasAudio

#endif /* IASVOLUMECMD_HPP_ */
