/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasAudioPlugin.hpp
 * @date   2013
 * @brief
 */

#ifndef IASAUDIOPLUGIN_HPP_
#define IASAUDIOPLUGIN_HPP_

#include "audio/smartx/rtprocessingfwx/IasGenericAudioComp.hpp"
#include "audio/smartx/rtprocessingfwx/IasPluginLibrary.hpp"




namespace IasAudio {

class IasIGenericAudioCompConfig;

template <typename TCore, typename TCmd>
class IAS_AUDIO_PUBLIC IasModule : public IasAudio::IasGenericAudioComp
{
public:
  /**
   * @brief Constructor.
   */
  IasModule(const IasAudio::IasIGenericAudioCompConfig *config, const std::string &typeName, const std::string &instanceName);

  /**
   * @brief Destructor, virtual by default.
   */
  virtual ~IasModule();


private:
  /**
   * @brief Copy constructor, private unimplemented to prevent misuse.
   */
  IasModule(IasModule const &other);

  /**
   * @brief Assignment operator, private unimplemented to prevent misuse.
   */
  IasModule& operator=(IasModule const &other);

  void cleanup();
};

template <typename TCore, typename TCmd>
IasModule<TCore, TCmd>::IasModule(const IasAudio::IasIGenericAudioCompConfig *config, const std::string &typeName, const std::string &instanceName)
  :IasAudio::IasGenericAudioComp(typeName, instanceName)
{
  TCore *core = new TCore(config, instanceName);
  IAS_ASSERT(core != NULL);
  mCore = core;
  TCmd *cmd = new TCmd(config, core);
  IAS_ASSERT(cmd != NULL);
  mCmdInterface = cmd;
}

template <typename TCore, typename TCmd>
IasModule<TCore, TCmd>::~IasModule()
{
  cleanup();
}

template <typename TCore, typename TCmd>
void IasModule<TCore, TCmd>::cleanup()
{
  delete mCore;
  delete mCmdInterface;
}

} /* namespace IasAudio */

#endif /* IASAUDIOPLUGIN_HPP_ */
