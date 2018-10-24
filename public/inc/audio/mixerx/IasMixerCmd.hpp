/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file IasMixerCmd.hpp
 * @date 2016
 * @brief This file contains the declaration for all public command and control related types of the mixer module.
 */


#ifndef IASMIXERCMD_HPP_
#define IASMIXERCMD_HPP_


/*!
 * @brief namespace IasAudio
 */
namespace IasAudio {


/**
 * @brief The namespace for all public command and control related types of the mixer module.
 */
namespace IasMixer {


/**
 * @brief this is the list of the available commands of the audio processing interface
 */
enum IasMixerCmdIds
{
  eIasSetModuleState,                  //!< Turn the module on or off
  eIasSetInputGainOffset,              //!< Set the Input gain offset
  eIasSetBalance,                      //!< Set the new balance value
  eIasSetFader,                        //!< Set the new fader value
  eIasLastEntry                        //!< ATTENTION: Always has to be last entry
};

/**
 * @brief The event types for the mixer module
 */
enum IasMixerEventTypes
{
  eIasBalanceFinished,                 //!< The balance has finished
  eIasFaderFinished,                   //!< The fader has finished
  eIasInputGainOffsetFinished          //!< The input gain offset has finished
};

} // namespace IasMixer

} // namespace IasAudio

#endif /* IASMIXERCMD_HPP_ */
