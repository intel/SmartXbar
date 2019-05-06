/*
 * Copyright (C) 2019 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file  IasSimpleMixerCmd.hpp
 * @date  2016
 * @brief The header file of the command interface for the simple mixer module
 */

#ifndef IASSIMPLEMIXERCMD_HPP_
#define IASSIMPLEMIXERCMD_HPP_

#include "audio/smartx/rtprocessingfwx/IasIModuleId.hpp"

namespace IasAudio {

/**
 * @brief The namespace for all public command and control related types of the mixple mixer module.
 */
namespace IasSimpleMixer {

/**
 * @brief The command ids for the simple mixer module
 */
enum IasSimpleMixerCmdIds
{
  eIasSetModuleState,                 //!< Turn the module on or off
  eIasSetInPlaceGain,                 //!< Set new gain for all in-place streams
  eIasSetXMixerGain,                  //!< Set mew gain for all stream mappings (x-bar mixer)
  eIasLastEntry,                      //!< ATTENTION: Always has to be last entry
};


/**
 * @brief The event types for the simple mixer module (currently, we support only one command).
 */
enum IasSimpleMixerEventTypes
{
  eIasGainUpdateFinished,             //!< The update of the gain has been finished
};

} // namespace IasSimpleMixer


// Forward declaration of the core component.
class IasSimpleMixerCore;

/**
 * @brief This class defines the command interface for the simple mixer module
 */
class IAS_AUDIO_PUBLIC IasSimpleMixerCmd : public IasIModuleId
{
  public:
    /**
     *  @brief Constructor.
     */
    IasSimpleMixerCmd(const IasIGenericAudioCompConfig *config, IasSimpleMixerCore *core);

    /**
     *  @brief Destructor, virtual by default.
     */
    virtual ~IasSimpleMixerCmd();

    /**
     *  @brief Command interface initialization.
     */
    IasIModuleId::IasResult init();

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
    virtual IasIModuleId::IasResult processCmd(const IasProperties &cmdProperties, IasProperties &returnProperties);

  private:
    /*!
     *  @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasSimpleMixerCmd(IasSimpleMixerCmd const &other); //lint !e1704

    /*!
     *  @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasSimpleMixerCmd& operator=(IasSimpleMixerCmd const &other); //lint !e1704

    /**
     * @brief Get the gain from the cmdProperties and convert it into a linear float value.
     *
     * This private function searches through the cmdProperties for the keyword "gain", and
     * returns the gain converted to a linear floating point value.
     *
     * @param[in] cmdProperties The cmd properties received from the user application
     * @param[out] streamId The stream id extracted from the cmd properties
     *
     * @return The result of the method
     * @retval eIasOk Successfully extracted gain
     * @retval eIasFailed Failed to get gain
     */
    IasIModuleId::IasResult getGain(const IasProperties &cmdProperties, float &gain);

    /**
     * @brief Get the streamId from the cmdProperties
     *
     * This private function searches through the cmdProperties for the keyword "pin", and then it
     * asks the configuration for the stream id that is associated with this pinName.
     *
     * @param[in] cmdProperties The cmd properties received from the user application
     * @param[out] streamId The stream id extracted from the cmd properties
     *
     * @return The result of the method
     * @retval eIasOk Successfully extracted streamId
     * @retval eIasFailed Failed to get streamId
     */
    IasIModuleId::IasResult getStreamId(const IasProperties &cmdProperties, int32_t &streamId);

    /**
     * @brief Get the streamIds from the cmdProperties
     *
     * This private function searches through the cmdProperties for the keywords "input_pin" and "output_pin",
     * and then it asks the configuration for the stream ids that are associated with these pinNames.
     *
     * @param[in] cmdProperties   The cmd properties received from the user application
     * @param[out] inputStreamId  The stream id of the "input_pin" extracted from the cmd properties
     * @param[out] inputStreamId  The stream id of the "output_pin" extracted from the cmd properties
     *
     * @return The result of the method
     * @retval eIasOk Successfully extracted streamIds
     * @retval eIasFailed Failed to get streamIds
     */
    IasIModuleId::IasResult getStreamIds(const IasProperties &cmdProperties,
                                         int32_t &inputStreamId,
                                         int32_t &outputStreamId);

    /**
     * @brief Set the module state (off or on).
     *
     * This private function is called if a command IasSimpleMixerCmdIds::eIasSetModuleState has been received.
     * The function retrieves the moduleState (via the keyword "moduleState") from the cmdProperties.
     * Then it calls one of the functions enableProcessing() or disableProcessing() of the IasGenericAudioCompCore.
     *
     * @param[in]  cmdProperties    The cmd properties received from the user application.
     * @param[out] returnProperties The properties to be returned to the user application (not used at the moment)
     *
     * @return The result of the method
     * @retval eIasOk Successfully extracted streamId and gain
     * @retval eIasFailed Failed to get streamId and/or gain
     */
    IasIModuleId::IasResult setModuleState(const IasProperties &cmdProperties, IasProperties &returnProperties);

    /**
     * @brief Set the gain for an in-place audio pin (combined input/output audio pin).
     *
     * This private function is called if a command IasSimpleMixerCmdIds::eIasSetInPlaceGain has been received.
     * The function retrieves the streamId (via the keyword "pin") and the gain (via the keyword "gain")
     * from the cmdProperties. Then it calls the function setInPlaceGain of the simple mixer core component.
     *
     * @param[in]  cmdProperties    The cmd properties received from the user application.
     * @param[out] returnProperties The properties to be returned to the user application (not used at the moment)
     *
     * @return The result of the method
     * @retval eIasOk Successfully extracted streamId and gain
     * @retval eIasFailed Failed to get streamId and/or gain
     */
    IasIModuleId::IasResult setInPlaceGain(const IasProperties &cmdProperties, IasProperties &returnProperties);

    /**
     * @brief Set the 'x-mixer' gain (i.e., the gain for mapping from an input audio pin to an output audio pin).
     *
     * This private function is called if a command IasSimpleMixerCmdIds::eIasSetXMixerGain has been received.
     * The function retrieves the streamIds (via the keywords "input_pin" and "output_pin") and the gain (via the keyword "gain")
     * from the cmdProperties. Then it calls the function setXMixerGain of the simple mixer core component.
     *
     * @param[in]  cmdProperties    The cmd properties received from the user application.
     * @param[out] returnProperties The properties to be returned to the user application (not used at the moment)
     *
     * @return The result of the method
     * @retval eIasOk Successfully extracted streamId and gain
     * @retval eIasFailed Failed to get streamId and/or gain
     */
    IasIModuleId::IasResult setXMixerGain(const IasProperties &cmdProperties, IasProperties &returnProperties);

    /**
     * @brief Function type for calling the member functions IasSimpleMixer::eIasSetModuleState,
     *        IasSimpleMixer::eIasSetInPlaceGain, or IasSimpleMixer::eIasSetXMixerGain
     */
    using IasCmdFunction = std::function<IasIModuleId::IasResult(IasSimpleMixerCmd*, const IasProperties &cmdProperties, IasProperties &returnProperties)>;


    // Member variables
    IasSimpleMixerCore                     *mCore;            //!< Pointer to the module core
    std::vector<IasCmdFunction>             mCmdFuncTable;    //!< The cmd function table
};

} //namespace IasAudio

#endif /* IASSIMPLEMIXERCMD_HPP_ */
