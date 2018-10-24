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
#include "rtprocessingfwx/IasGenericAudioCompConfig.hpp"

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

class IasTestComp : public IasGenericAudioComp
{
  public:
    /*!
     *  @brief Constructor.
     */
    IasTestComp(const std::string &typeName, const std::string &instanceName);
    /*!
     *  @brief Destructor, virtual by default.
     */
    virtual ~IasTestComp();
};

class IasTestCompCmd : public IasIModuleId
{
  public:
    IasTestCompCmd(const IasIGenericAudioCompConfig *config, IasTestCompCore*)
      :IasIModuleId(config)
    {}
    ~IasTestCompCmd() {}
    virtual IasResult init() { return eIasOk; }
    virtual IasIModuleId::IasResult setProperty( IasProperties &) { return IasIModuleId::eIasOk; }
    virtual IasIModuleId::IasResult getProperty( IasProperties &) { return IasIModuleId::eIasOk; }
    virtual IasIModuleId::IasResult processCmd(const IasProperties &, IasProperties &) { return IasIModuleId::eIasOk; }
};

} //namespace IasAudio


#endif /* IASTESTCOMP_HPP_ */
