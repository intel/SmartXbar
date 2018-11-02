/*
 * * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasIDebug.hpp
 * @date   2016
 * @brief
 */

#ifndef IASIDEBUG_HPP_
#define IASIDEBUG_HPP_


#include "avbaudiomodules/audio/common/IasAudioCommonTypes.hpp"

namespace IasAudio {

/**
  * @brief The debug interface class
  *
  * This class is used for debug purposes. See chapter @ref probing for more details.
  */
class IAS_AUDIO_PUBLIC IasIDebug
{
  public:
    /**
     * @brief The result type for the IasIDebug methods
     */
    enum IasResult
    {
      eIasOk,                     //!< Operation successful
      eIasFailed,                 //!< Operation failed
    };

    /**
    * @brief Destructor.
    */
    virtual ~IasIDebug() {}

    /**
     * @brief Inject data from wave files at a certain audio port/pin.
     *        Every channel of the audio port/pin must be fed with a separate
     *        wav file.
     *
     * @param[in] fileNamePrefix The prefix for wav file names, without directory. For injecting, the wav files need to be present in /tmp.
     *                           The naming convention is #fileNamePrefix_chx.wav ,e.g. for a stereo port/pin inject, the following files must be present
     *                           (assuming fileNamePrefix="test"):
     *                           /tmp/test_ch0.wav
     *                           /tmp/test_ch1.wav
     * @param[in] name the name of the port/pin to inject data
     * @param[in] numSeconds the number of seconds to be injected
     *
     * @return The result of the inject operation
     * @retval IasIDebug::eIasOk All went well, inject started
     * @retval IasIDebug::eIasFailed Failed to start injection
     */
    virtual IasResult startInject(const std::string &fileNamePrefix, const std::string &name, uint32_t numSeconds)=0;

    /**
     * @brief Record data of an audio port/pin to wave files.
     *        Every channel of the audio port/pin will be recorded to a separate wav file
     *
     *
     * @param[in] fileNamePrefix the prefix for wav file names, without directory. For recording, the wav files will be stored /tmp.
     *                           The naming convention is #fileNamePrefix_chx.wav, e.g. for a stereo port/pin record, the following files will be stored
     *                           (assuming fileNamePrefix="test"):
     *                           /tmp/test_ch0.wav
     *                           /tmp/test_ch1.wav
     * @param[in] name the name of the port/pin
     * @param[in] numSeconds the number of seconds to be recorded
     *
     * @return The result of the record operation
     * @retval IasIDebug::eIasOk All went well, record started
     * @retval IasIDebug::eIasFailed Failed to start recording
     */
    virtual IasResult startRecord(const std::string &fileNamePrefix, const std::string &name, uint32_t numSeconds)=0;

    /**
     * @brief Stops any inject or recording operation for a given audio port/pin
     *
     * @param[in] name The name of the audio port/pin
     *
     * @return The result of the stop
     * @retval IasIDebug::eIasOk All went well
     * @retval IasIDebug::eIasFailed Failed to stop probing
     */
    virtual IasResult stopProbing(const std::string &name)=0;

};

/**
 * @brief Function to get a IasIDebug::IasResult as string.
 *
 * @param[in] type The IasIDebug::IasResult value
 *
 * @return Enum Member as string
 */
std::string toString(const IasIDebug::IasResult& type);


} //namespace IasAudio

#endif /* IASIDEBUG_HPP_ */
