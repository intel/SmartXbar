/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file IasVolumeCmdInterface.hpp
 * @date 2012
 * @brief the header file of the command interface for the volume module
 */

#ifndef IASVOLUMECMDINTERFACE_HPP_
#define IASVOLUMECMDINTERFACE_HPP_

#include "audio/smartx/rtprocessingfwx/IasIModuleId.hpp"

namespace IasAudio {

class IasVolumeLoudnessCore;

/**
 * @brief This class defines the command interface for the volume module
 */
class IAS_AUDIO_PUBLIC IasVolumeCmdInterface : public IasIModuleId
{
  public:
    /**
     *  @brief Constructor.
     */
    IasVolumeCmdInterface(const IasIGenericAudioCompConfig *config, IasVolumeLoudnessCore *core);

    /**
     *  @brief Destructor, virtual by default.
     */
    virtual ~IasVolumeCmdInterface();

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
    IasVolumeCmdInterface(IasVolumeCmdInterface const &other); //lint !e1704

    /*!
     *  @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasVolumeCmdInterface& operator=(IasVolumeCmdInterface const &other); //lint !e1704

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
    IasIModuleId::IasResult getStreamId(const IasProperties &cmdProperties, int32_t &streamId);

    IasIModuleId::IasResult setModuleState(const IasProperties &cmdProperties, IasProperties &returnProperties);
    IasIModuleId::IasResult setVolume(const IasProperties &cmdProperties, IasProperties &returnProperties);
    IasIModuleId::IasResult setMuteState(const IasProperties &cmdProperties, IasProperties &returnProperties);
    IasIModuleId::IasResult setLoudness(const IasProperties &cmdProperties, IasProperties &returnProperties);
    IasIModuleId::IasResult setSpeedControlledVolume(const IasProperties &cmdProperties, IasProperties &returnProperties);
    IasIModuleId::IasResult setLoudnessTable(const IasProperties &cmdProperties, IasProperties &returnProperties);
    IasIModuleId::IasResult getLoudnessTable(const IasProperties &cmdProperties, IasProperties &returnProperties);
    IasIModuleId::IasResult setSpeed(const IasProperties &cmdProperties, IasProperties &returnProperties);
    IasIModuleId::IasResult setLoudnessFilter(const IasProperties &cmdProperties, IasProperties &returnProperties);
    IasIModuleId::IasResult getLoudnessFilter(const IasProperties &cmdProperties, IasProperties &returnProperties);
    IasIModuleId::IasResult setSdvTable(const IasProperties &cmdProperties, IasProperties &returnProperties);
    IasIModuleId::IasResult getSdvTable(const IasProperties &cmdProperties, IasProperties &returnProperties);
    IasIModuleId::IasResult getParameters(const IasProperties &cmdProperties, IasProperties &returnProperties);
    IasIModuleId::IasResult setParameters(const IasProperties &cmdProperties, IasProperties &returnProperties);


    using IasCmdFunction = std::function<IasIModuleId::IasResult(IasVolumeCmdInterface*, const IasProperties &cmdProperties, IasProperties &returnProperties)>;

    IasVolumeLoudnessCore                  *mCore;            //!< pointer to the module core
    std::vector<IasCmdFunction>             mCmdFuncTable;    //!< The cmd function table
    uint32_t                             mNumFilterBands;  //!< The number of filter bands as configured
    DltContext                             *mLogContext;      //!< The log context for the volume component.

};

} //namespace IasAudio

#endif /* IASVOLUMECMDINTERFACE_HPP_ */
