/*
 * Copyright (C) 2019 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file   IasSimpleVolumeCmd.cpp
 * @date   2013
 * @brief
 */

#include "audio/smartx/rtprocessingfwx/IasICmdRegistration.hpp"
#include "audio/smartx/rtprocessingfwx/IasAudioPlugin.hpp"
#include "audio/simplevolume/IasSimpleVolumeCmd.hpp"
#include "audio/simplevolume/IasSimpleVolumeCore.hpp"

DLT_DECLARE_CONTEXT(logCtxSimpleVolume);

namespace IasAudio {

static const std::string cClassName = "IasSimpleVolumeCmd::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"

IasSimpleVolumeCmd::IasSimpleVolumeCmd(const IasIGenericAudioCompConfig *config, IasSimpleVolumeCore* core)
  :IasIModuleId(config)
  ,mCore(core)
{
  DLT_REGISTER_CONTEXT(logCtxSimpleVolume, "SV", "Simple volume audio module");
}

IasSimpleVolumeCmd::~IasSimpleVolumeCmd()
{
}

IasIModuleId::IasResult IasSimpleVolumeCmd::init()
{
  return IasIModuleId::eIasOk;
}

IasIModuleId::IasResult IasSimpleVolumeCmd::processCmd(const IasProperties &cmdProperties, IasProperties &)
{
  DLT_LOG_CXX(logCtxSimpleVolume, DLT_LOG_INFO, LOG_PREFIX, "Received cmd");
  float newVolume = 0.0f;
  IasProperties::IasResult status = cmdProperties.get("volume", &newVolume);
  if (status == IasProperties::eIasOk)
  {
    DLT_LOG_CXX(logCtxSimpleVolume, DLT_LOG_INFO, LOG_PREFIX, "Set new volume to", newVolume);
    mCore->setVolume(newVolume);
    return eIasOk;
  }
  else
  {
    cmdProperties.dump("cmdProperties");
    return eIasFailed;
  }
}



} // namespace IasAudio
