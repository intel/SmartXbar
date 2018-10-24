/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file IasMixerCmdInterface.hpp
 * @date August 2016
 * @brief the header file of the command interface for the mixer module
 */


#ifndef IASMIXERCMDINTERFACE_HPP_
#define IASMIXERCMDINTERFACE_HPP_

#include "audio/smartx/rtprocessingfwx/IasIModuleId.hpp"


namespace IasAudio {

class IasMixerCore;

class IAS_AUDIO_PUBLIC IasMixerCmdInterface : public IasIModuleId
{

  public:

    /**
     *  @brief Constructor
     */
    IasMixerCmdInterface(const IasIGenericAudioCompConfig* config, IasMixerCore* core);

    /**
     *  @brief Destructor, virtual by default.
     */
    virtual ~IasMixerCmdInterface();

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


    IasIModuleId::IasResult translate(IasAudioProcessingResult result);

    /*!
     *  @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasMixerCmdInterface(IasMixerCmdInterface const& other) = delete;

    /*!
     *  @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasMixerCmdInterface& operator=(IasMixerCmdInterface const& other) = delete;

    /**
     *  @brief Move constructor, private unimplemented to prevent misuse.
     */
    IasMixerCmdInterface(IasMixerCmdInterface&& other) = delete;

    /**
     *  @brief Move assignment operator, private unimplemented to prevent misuse.
     */
    IasMixerCmdInterface& operator=(IasMixerCmdInterface&& other) = delete;

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
     *  @brief Set balance
         *
     * @param[in] cmdProperties The cmd properties received from the user application
     * @param[out] returnProperties
     *
     * @return The result of the method
     * @retval eIasOk Set data Successfully
     * @retval eIasFailed Set data Failed
     */
    IasIModuleId::IasResult setBalance(const IasProperties& cmdProperties, IasProperties& returnProperties);

    /**
     *  @brief Set Fader
     *
     * @param[in] cmdProperties The cmd properties received from the user application
     * @param[out] returnProperties
     *
     * @return The result of the method
     * @retval eIasOk Set data Successfully
     * @retval eIasFailed Set data Failed
     */
    IasIModuleId::IasResult setFader(const IasProperties& cmdProperties, IasProperties& returnProperties);

    /**
     *  @brief Set input gain offset
     *
     * @param[in] cmdProperties The cmd properties received from the user application
     * @param[out] returnProperties
     *
     * @return The result of the method
     * @retval eIasOk Set data Successfully
     * @retval eIasFailed Set data Failed
     */
    IasIModuleId::IasResult setInputGainOffset(const IasProperties& cmdProperties, IasProperties& returnProperties);

    using IasCmdFunction = std::function<IasIModuleId::IasResult(IasMixerCmdInterface*, const IasProperties& cmdProperties, IasProperties& returnProperties)>;

    IasMixerCore                            *mCore;           //!< pointer to the module core
    std::vector<IasCmdFunction>             mCmdFuncTable;    //!< The cmd function table
    DltContext                              *mLogContext;      //!< The log context for the volume component.

};

} //namespace IasAudio


#endif /* IASMIXERCMDINTERFACE_HPP_ */
