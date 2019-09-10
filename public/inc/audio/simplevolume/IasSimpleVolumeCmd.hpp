/*
 * Copyright (C) 2019 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file   IasSimpleVolumeCmd.hpp
 * @date   2013
 * @brief
 */

#ifndef IASSIMPLEVOLUMECMD_HPP_
#define IASSIMPLEVOLUMECMD_HPP_

#include "audio/smartx/rtprocessingfwx/IasIModuleId.hpp"

namespace IasAudio {

class IasSimpleVolumeCore;

class IAS_AUDIO_PUBLIC IasSimpleVolumeCmd : public IasIModuleId
{
  public:
    /**
     * @brief Constructor.
     */
    IasSimpleVolumeCmd(const IasIGenericAudioCompConfig *config, IasSimpleVolumeCore* core);

    /**
     * @brief Destructor, virtual by default.
     */
    virtual ~IasSimpleVolumeCmd();

    /**
     * @brief Initialize the command instance implementation
     *
     * @returns The result of the method
     * @retval eIasOk Successfully processed cmd
     * @retval eIasFailed Faled to process cmd
     */
    virtual IasResult init();

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
     */
    virtual IasResult processCmd(const IasProperties &cmdProperties, IasProperties &returnProperties);

  private:
    /**
     * @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasSimpleVolumeCmd(IasSimpleVolumeCmd const &other);

    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasSimpleVolumeCmd& operator=(IasSimpleVolumeCmd const &other);

    // Member variables
    IasSimpleVolumeCore        *mCore;    //!< A pointer to the core implementation in order to execute the commands.
};

} //namespace IasAudio

#endif /* IASSIMPLEVOLUMECMD_HPP_ */
