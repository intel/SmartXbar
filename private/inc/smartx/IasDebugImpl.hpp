/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasDebugImpl.hpp
 * @date   2016
 * @brief
 */

#ifndef IASDEBUGIMPL_HPP
#define IASDEBUGIMPL_HPP


#include "audio/common/IasAudioCommonTypes.hpp"
#include "internal/audio/common/IasAudioLogging.hpp"
#include "audio/smartx/IasIDebug.hpp"
#include "IasConfiguration.hpp"
#include "model/IasAudioPort.hpp"
#include "model/IasAudioPortOwner.hpp"
#include "model/IasAudioPin.hpp"
#include "model/IasPipeline.hpp"

namespace IasAudio {

/**
 * @brief
 *
 */
class IAS_AUDIO_PUBLIC IasDebugImpl : public IasIDebug
{
  public:

    enum IasProbeLocation
    {
      eIasProbeSwitchMatrix=0,
      eIasProbeRoutingZone
    };

    /**
     * @brief Constructor.
     */
    IasDebugImpl(IasConfigurationPtr config);
    /**
     * @brief Destructor.
     */
    virtual ~IasDebugImpl();

    /**
     * @brief Inject data from wave files at a certain audio port.
     *        Every channel of the audio port must be fed with a separate
     *        wav file. The naming convention is #PREFIX_chx.wav
     *        E.g. for a stereo port, the following files must be present
     *        (assuming PREFIX="/data/test")
     *        /data/test_ch0.wav
     *        /data/test_ch1.wav
     *
     * @param[in] fileNamePrefix the prefix for wav file names
     * @param[in] portName the name of the port
     * @param[in] numSeconds number of seconds to inject
     *
     * @return The result of the inject operation
     * @retval IasIDebug::eIasOk All went well, inject started
     * @retval IasIDebug::eIasFailed Failed to start injection
     */
    virtual IasResult startInject(const std::string &fileNamePrefix, const std::string &portName, uint32_t numSeconds);

    /**
     * @brief Record data of an audio port to wave files.
     *        Every channel of the audio port will be recorded to a separate wav file
     *        The naming convention is #PREFIX_chx.wav
     *        E.g. for a stereo port, the following files will be created
     *        (assuming PREFIX="/data/test")
     *        /data/test_ch0.wav
     *        /data/test_ch1.wav
     *
     * @param[in] fileNamePrefix the prefix for wav file names
     * @param[in] portName the name of the port
     * @param[in] numSeconds number of seconds to record
     *
     * @return The result of the record operation
     * @retval IasIDebug::eIasOk All went well, record started
     * @retval IasIDebug::eIasFailed Failed to start recording
     *
     */
    virtual IasResult startRecord(const std::string &fileNamePrefix, const std::string &portName, uint32_t numSeconds);

    /**
     * @brief Stops any inject or recording operation for a given audio port
     *
     * @param[in] portName The name of the audio port
     *
     * @return The result of the stop
     * @retval IasIDebug::eIasOk All went well, record started
     * @retval IasIDebug::eIasFailed Failed to start injection
     */
    virtual IasResult stopProbing(const std::string &portName);

  private:
    /**
     * @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasDebugImpl(IasDebugImpl const &other);
    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasDebugImpl& operator=(IasDebugImpl const &other);

    IasResult startProbing(const std::string &fileNamePrefix,
                           const std::string &portName,
                           uint32_t numSeconds,
                           bool inject);

    IasProbeLocation getProbeLocation(IasAudioPortParamsPtr portParams);

    IasProbeLocation getProbeLocation(IasAudioPinPtr pin);

    IasResult startSmartxPluginProbe(IasAudioPortParamsPtr portParams,
                                     IasAudioPortOwnerPtr portOwner,
                                     const std::string &prefix,
                                     bool inject,
                                     uint32_t numSeconds);

    IasResult startSwitchMatrixProbing(IasAudioPortPtr port,
                                       IasAudioPortOwnerPtr portOwner,
                                       const std::string &prefix,
                                       bool inject,
                                       uint32_t numSeconds,
                                       uint32_t sampleRate);

    void stopSwitchMatrixProbing(IasAudioPortPtr port,
                                 IasAudioPortOwnerPtr portOwner);

    IasResult startRoutingZoneProbing(IasAudioPortParamsPtr portParams,
                                      IasAudioPortOwnerPtr portOwner,
                                      const std::string &prefix,
                                      bool inject,
                                      uint32_t numSeconds);

    void stopRoutingZoneProbing(IasAudioPortOwnerPtr portOwner);

    IasResult doPortProbing(const std::string &fileNamePrefix,
                            uint32_t numSeconds,
                            IasAudioPortPtr port);
    IasResult doPinProbing(const std::string &fileNamePrefix,
                           uint32_t numSeconds,
                           IasAudioPinPtr pin);

    IasIDebug::IasResult translate(IasPipeline::IasResult result);
    IasIDebug::IasResult translate(IasConfiguration::IasResult result);

    DltContext          *mLog;          //!< DLT context
    IasConfigurationPtr  mConfig;       //!< Pointer to the configuration
};

} //namespace IasAudio

#endif // IASDEBUGIMPL_HPP
