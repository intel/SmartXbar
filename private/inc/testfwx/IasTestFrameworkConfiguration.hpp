/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file   IasTestFrameworkConfiguration.hpp
 * @date   2017
 * @brief  Configuration database of test framework.
 */

#ifndef AUDIO_DAEMON2_PRIVATE_INC_TESTFWX_IASTESTFRAMEWORKCONFIGURATION_HPP_
#define AUDIO_DAEMON2_PRIVATE_INC_TESTFWX_IASTESTFRAMEWORKCONFIGURATION_HPP_


#include "smartx/IasConfiguration.hpp"
#include "testfwx/IasTestFrameworkTypes.hpp"
#include "avbaudiomodules/audio/common/IasAudioCommonTypes.hpp"

/*!
 * @brief namespace IasAudio
 */
namespace IasAudio {

/**
 * @brief @brief Map wave file pointer to audio pin pointer
 */
using IasTestFrameworkWaveFileMap = std::map<IasAudioPinPtr, IasTestFrameworkWaveFilePtr>;

/*!
 * @brief Documentation for class IasTestFrameworkConfiguration
 */
class IasTestFrameworkConfiguration: public IasConfiguration
{
  public:

    /**
     * @brief Constructor.
     */
    IasTestFrameworkConfiguration();

    /**
     * @brief Destructor.
     */
    ~IasTestFrameworkConfiguration();

    /**
     * @brief Add wave file linked with audio pin
     *
     * It is possible to link several input pins with the same file, which will be opened for read access several times.
     * Several output pins must not be linked with the same file, which will be opened for write access only once.
     * The same file can't be linked with input and output pin at the same time.
     *
     * @param[in] audioPin audio pin pointer
     * @param[in] waveFile wave file pointer
     *
     * @return the result of the function call
     * @retval eIasOk wave file was added successfully
     * @retval eIasNullPointer invalid input parameter
     * @retval eIasObjectAlreadyExists pin and/or wave file is already linked
     */
    IasResult addWaveFile(IasAudioPinPtr audioPin, IasTestFrameworkWaveFilePtr waveFile);

    /**
     * @brief Get wave file linked with audio pin
     *
     * @param[in] audioPin audio pin pointer
     * @param[out] waveFile wave file pointer
     *
     * @return the result of the function call
     * @retval eIasOk wave file was returned successfully
     * @retval eIasNullPointer invalid input parameter
     * @retval eIasObjectNotFound wave file not found for audio pin
     */
    IasResult getWaveFile(IasAudioPinPtr audioPin, IasTestFrameworkWaveFilePtr *waveFile);

    /**
     * @brief Delete wave file linked with audio pin
     *
     * @param[in] audioPin audio pin pointer
     */
    void deleteWaveFile(IasAudioPinPtr audioPin);

    /**
     * @brief Add test framework routing zone
     *
     * It is possible to add only one test framework routing zone.
     *
     * @param[in] routingZone routing zone to be added
     *
     * @return the result of the function call
     * @retval eIasOk test framework routing zone was added successfully
     * @retval eIasNullPointer invalid input parameter
     * @retval eIasObjectAlreadyExists test framework routing zone was already added
     */
    IasResult addTestFrameworkRoutingZone(IasTestFrameworkRoutingZonePtr routingZone);

    /**
     * @brief Get test framework routing zone
     *
     * @return pointer to the test framework routing zone
     */
    IasTestFrameworkRoutingZonePtr getTestFrameworkRoutingZone() { return mTestFrameworkRoutingZone; };

    /**
     * @brief Delete test framework routing zone
     */
    void deleteTestFrameworkRoutingZone();

    /**
     * @brief Add test framework pipeline
     *
     * It is possible to add only one pipeline for the test framework.
     *
     * @param[in] pipeline  pipeline to be added
     *
     * @return the result of the function call
     * @retval eIasOk pipeline was added successfully
     * @retval eIasNullPointer invalid input parameter
     * @retval eIasObjectAlreadyExists pipeline was already added
     */
    IasResult addTestFrameworkPipeline(IasPipelinePtr pipeline);

    /**
     * @brief Get test framework pipeline
     *
     * @return pointer to the test framework pipeline
     */
    IasPipelinePtr getTestFrameworkPipeline() { return mTestFrameworkPipeline; }

    /**
     * @brief Delete test framework pipeline
     */
    void deleteTestFrameworkPipeline();

  private:

    /*!
     *  @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasTestFrameworkConfiguration(IasTestFrameworkConfiguration const &other);

    /*!
     *  @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasTestFrameworkConfiguration& operator=(IasTestFrameworkConfiguration const &other);

    IasTestFrameworkWaveFileMap     mInputWaveFileMap;
    IasTestFrameworkWaveFileMap     mOutputWaveFileMap;

    IasTestFrameworkRoutingZonePtr  mTestFrameworkRoutingZone;
    IasPipelinePtr                  mTestFrameworkPipeline;

    DltContext                      *mLog;
};

} // namespace IasAudio

#endif // AUDIO_DAEMON2_PRIVATE_INC_TESTFWX_IASTESTFRAMEWORKCONFIGURATION_HPP_
