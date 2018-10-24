/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasTestComp.hpp
 * @date   Sept 24, 2012
 * @brief
 */

#ifndef IASTESTCOMP_HPP_
#define IASTESTCOMP_HPP_

#include "audio/smartx/rtprocessingfwx/IasGenericAudioComp.hpp"
#include "audio/smartx/rtprocessingfwx/IasGenericAudioCompCore.hpp"
#include "audio/smartx/rtprocessingfwx/IasIModuleId.hpp"
#include "audio/smartx/rtprocessingfwx/IasProcessingTypes.hpp"

namespace IasAudio {

class IasTestCompCore : public IasGenericAudioCompCore
{
  public:
    /*!
     *  @brief Constructor.
     */
    IasTestCompCore(const IasIGenericAudioCompConfig *config, const std::string &typeName);
    /*!
     *  @brief Destructor, virtual by default.
     */
    virtual ~IasTestCompCore();

    // Inherited by IasGenericAudioComp
    virtual IasAudioProcessingResult reset();
    virtual IasAudioProcessingResult init();

  private:
    /*!
     *  @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasTestCompCore(IasTestCompCore const &other);
    /*!
     *  @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasTestCompCore& operator=(IasTestCompCore const &other);

    // Inherited by IasGenericAudioComp
    virtual IasAudioProcessingResult processChild();
};

class IasTestCompCmd : public IasIModuleId
{
  public:
    enum IasCmdIds
    {
      eIasSetVolume,
      eIasGetVolume,
    };
    IasTestCompCmd(const IasIGenericAudioCompConfig *config, IasTestCompCore*)
      :IasIModuleId(config)
      ,mCmdFuncTable()
      ,mCurrentVolume(1.0f)
    {}
    ~IasTestCompCmd() {}
    virtual IasResult init();
    virtual IasIModuleId::IasResult setProperty( IasProperties &) { return IasIModuleId::eIasOk; }
    virtual IasIModuleId::IasResult getProperty( IasProperties &) { return IasIModuleId::eIasOk; }
    virtual IasIModuleId::IasResult processCmd(const IasProperties &cmdProperties, IasProperties &returnProperties);

  private:
    IasIModuleId::IasResult setVolume(const IasProperties &cmdProperties, IasProperties &returnProperties);
    IasIModuleId::IasResult getVolume(const IasProperties &cmdProperties, IasProperties &returnProperties);

    using IasCmdFunction = std::function<IasIModuleId::IasResult(IasTestCompCmd*, const IasProperties &cmdProperties, IasProperties &returnProperties)>;

    std::vector<IasCmdFunction> mCmdFuncTable;
    float                mCurrentVolume;
};

} //namespace IasAudio


#endif /* IASTESTCOMP_HPP_ */
