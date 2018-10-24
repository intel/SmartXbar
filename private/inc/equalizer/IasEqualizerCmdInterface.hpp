/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file IasEqualizerCmdInterface.hpp
 * @date August 2016
 * @brief the header file of the command interface for the equalizer module
 */


#ifndef IASEQUALIZERCMDINTERFACE_HPP_
#define IASEQUALIZERCMDINTERFACE_HPP_

#include "audio/smartx/rtprocessingfwx/IasIModuleId.hpp"
#include "audio/smartx/IasProperties.hpp"
#include "audio/equalizerx/IasEqualizerCmd.hpp"
#include "equalizer/IasEqualizerConfiguration.hpp"

namespace IasAudio {

class IasEqualizerCore;
struct IasAudioFilterParams;

class IAS_AUDIO_PUBLIC IasEqualizerCmdInterface : public IasIModuleId
{
  public:

    /**
     *  Type definition for a Vector carrying all filter parameters for one
     *  stream (common for all channels of this stream).
     */
    using IasFilterParamsSingleStreamVec = std::vector<IasAudioFilterParams>;

    /**
     *  Type definition for a Map carrying the filter parameter vectors for all streams.
     */
    using IasFilterParamsMap = std::map<int32_t,IasFilterParamsSingleStreamVec>;

    /**
     * Type definition for a Vector carrying all filter parameters for one channel.
     */
    using IasCarEqualizerFilterParamsVec = std::vector<IasAudioFilterParams>;

    struct IasCarEqualizerStreamParams
    {
        uint32_t numChannels;
        IasCarEqualizerFilterParamsVec** filterParamsVec;
    };

    using IasCarEqualizerStreamParamsMap = std::map<int32_t, IasCarEqualizerStreamParams>;

    /**
     *  @brief Constructor
     */
    IasEqualizerCmdInterface(const IasIGenericAudioCompConfig* config, IasEqualizerCore* core);

    /**
     *  @brief Destructor, virtual by default.
     */
    virtual ~IasEqualizerCmdInterface();

    /**
     *  @brief Command interface initialization.
     */
    IasIModuleId::IasResult init() override;

    /**
     * @brief This pure virtual function has to be implemented by the derived class.
     *
     * It is used whenever a command, i.e. properties are passed to the audio module. It is up to the module
     * on how to react on the cmd. The command properties that are provided to the module are all defined
     * in the cmdProperties instance. The audio module can directly respond to the cmdProperties by setting
     * certain values in the returnProperties. It is up to the module if and how the module responds using
     * the returnProperties. If there is an error during processing of the cmd this can also be signalled
     * to the caller by setting the returnProperties. It is up to the calling application to react on those
     * errors.
     *
     * @returns The result of the method
     * @retval eIasOk Successfully processed cmd
     * @retval eIasFailed Failed to process cmd
     */
    IasIModuleId::IasResult processCmd(const IasProperties& cmdProperties, IasProperties& returnProperties) override;

  private:

    /**
     *  @brief Initialization car mode.
     */
    IasIModuleId::IasResult initCar();

    /**
     *  @brief Initialization user mode.
     */
    IasIModuleId::IasResult initUser();


    IasIModuleId::IasResult translate(IasAudioProcessingResult result);

    /*!
     *  @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasEqualizerCmdInterface(IasEqualizerCmdInterface const& other) = delete;

    /*!
     *  @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasEqualizerCmdInterface& operator=(IasEqualizerCmdInterface const& other) = delete;

    /**
     *  @brief Move constructor, private unimplemented to prevent misuse.
     */
    IasEqualizerCmdInterface(IasEqualizerCmdInterface&& other) = delete;

    /**
     *  @brief Move assignment operator, private unimplemented to prevent misuse.
     */
    IasEqualizerCmdInterface& operator=(IasEqualizerCmdInterface&& other) = delete;

    /**
     * @brief Set Module State
     *
     * @param[in] cmdProperties The cmd properties received from the user application
     * @param[out] returnProperties
     *
     * @return The result of the method
     * @retval eIasOk Set data Successfully
     * @retval eIasFailed Set data Failed
     */
    IasIModuleId::IasResult setModuleState(const IasProperties& cmdProperties, IasProperties& returnProperties);

    /**
     * @brief internal used function to fill default filter params vector
     *
     * @param[in,out]    vec pointer to the param vector
     *
     *
     * @return                           The error code
     * @retval eIasAudioProcOK           Set data Successfully
     * @retval eIasAudioProcInvalidParam Set data Failed
     */
    IasAudioProcessingResult setDefaultFilterParams(IasFilterParamsSingleStreamVec *vec);

    /**
     * @brief Get the streamId from the cmdProperties
     *
     * @param[in] cmdProperties The cmd properties received from the user application
     * @param[out] streamId The stream id extracted from the cmd properties
     *
     * @return The result of the method
     * @retval eIasOk Successfully extraced streamId
     * @retval eIasFailed Failed to get streamId
     */
    IasIModuleId::IasResult getStreamId(const IasProperties& cmdProperties, int32_t& streamId);


    /**
     *  @brief Set gradient
     *
     * @param[in] cmdProperties The cmd properties received from the user application
     * @param[out] returnProperties
     *
     * @return The result of the method
     * @retval eIasOk Set data Successfully
     * @retval eIasFailed Set data Failed
     */
    IasIModuleId::IasResult setRampGradientSingleStream(const IasProperties& cmdProperties, IasProperties& returnProperties);

    /**
     *  @brief Set config filter params
     *
     * @param[in] cmdProperties The cmd properties received from the user application
     * @param[out] returnProperties
     *
     * @return The result of the method
     * @retval eIasOk Set data Successfully
     * @retval eIasFailed Set data Failed
     */
    IasIModuleId::IasResult setConfigFilterParamsStream(const IasProperties& cmdProperties, IasProperties& returnProperties);

    /**
     *  @brief
     *
     * @param[in] cmdProperties The cmd properties received from the user application
     * @param[out] returnProperties
     *
     * @return The result of the method
     * @retval eIasOk Set data Successfully
     * @retval eIasFailed Set data Failed
     */
    IasIModuleId::IasResult getConfigFilterParamsStream(const IasProperties& cmdProperties, IasProperties& returnProperties);

    /**
     *  @brief Set gain in user equalizer
     *
     * @param[in] cmdProperties The cmd properties received from the user application
     * @param[out] returnProperties
     *
     * @return The result of the method
     * @retval eIasOk Set data Successfully
     * @retval eIasFailed Set data Failed
     */
    IasIModuleId::IasResult setUserEqualizer(const IasProperties& cmdProperties, IasProperties& returnProperties);

    /**
     *  @brief Set user equalizer params
     *
     * @param[in] cmdProperties The cmd properties received from the user application
     * @param[out] returnProperties
     *
     * @return The result of the method
     * @retval eIasOk Set data Successfully
     * @retval eIasFailed Set data Failed
     */
    IasIModuleId::IasResult setUserEqualizerParams(const IasProperties& cmdProperties, IasProperties& returnProperties);

    /**
     *  @brief Set number of filters in car equalizer
     *
     * @param[in] cmdProperties The cmd properties received from the user application
     * @param[out] returnProperties
     *
     * @return The result of the method
     * @retval eIasOk Set data Successfully
     * @retval eIasFailed Set data Failed
     */
    IasIModuleId::IasResult setCarEqualizerNumFilters(const IasProperties& cmdProperties, IasProperties& returnProperties);

    /**
     *  @brief Get number of filters from car equalizer
     *
     * @param[in] cmdProperties The cmd properties received from the user application
     * @param[out] returnProperties
     *
     * @return The result of the method
     * @retval eIasOk Set data Successfully
     * @retval eIasFailed Set data Failed
     */
    IasIModuleId::IasResult getCarEqualizerNumFilters(const IasProperties& cmdProperties, IasProperties& returnProperties);

    /**
     *  @brief Set filter in car equalizer
     *
     * @param[in] cmdProperties The cmd properties received from the user application
     * @param[out] returnProperties
     *
     * @return The result of the method
     * @retval eIasOk Set data Successfully
     * @retval eIasFailed Set data Failed
     */
    IasIModuleId::IasResult setCarEqualizerFilterParams(const IasProperties& cmdProperties, IasProperties& returnProperties);

    /**
     *  @brief Get filter params from car equalizer
     *
     * @param[in] cmdProperties The cmd properties received from the user application
     * @param[out] returnProperties
     *
     * @return The result of the method
     * @retval eIasOk Set data Successfully
     * @retval eIasFailed Set data Failed
     */
    IasIModuleId::IasResult getCarEqualizerFilterParams(const IasProperties& cmdProperties, IasProperties& returnProperties);

    using IasCmdFunction = std::function<IasIModuleId::IasResult(IasEqualizerCmdInterface*, const IasProperties& cmdProperties, IasProperties& returnProperties)>;

    IasEqualizerCore                 *mCore;              //!< pointer to the module core
    IasEqualizerConfiguration         mCoreConfiguration; //!< configuration object
    uint32_t                          mNumFilterStages;   //!< Number of frequency bands, provided by the coreConfig.
    IasFilterParamsMap                mFilterParamsMap;   //!< Map carrying the filter parameter vectors for all streams.
    std::vector<IasCmdFunction>       mCmdFuncTable;      //!< The cmd function table
    IasCarEqualizerStreamParamsMap    mStreamParamsMap;   //!< map carrying the stream-specific parameters
    IasEqualizer::IasEqualizerMode    mMode;              //!< mode for equalizer module, user by default
    DltContext                       *mLogContext;        //!< The log context for the volume component.
};

} //namespace IasAudio


#endif /* IASEQUALIZERCMDINTERFACE_HPP_ */
