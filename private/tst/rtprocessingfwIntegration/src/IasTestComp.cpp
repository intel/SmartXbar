/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasTestComp.cpp
 * @date   Sept 24, 2012
 * @brief
 */



#include "IasTestComp.hpp"
#include "audio/smartx/rtprocessingfwx/IasGenericAudioCompCore.hpp"

namespace IasAudio {

IasTestCompCore::IasTestCompCore(const IasIGenericAudioCompConfig *config, const std::string &typeName)
  :IasGenericAudioCompCore(config, typeName)
{
}

IasTestCompCore::~IasTestCompCore()
{
}

IasAudioProcessingResult IasTestCompCore::reset()
{
  return eIasAudioProcOK;
}

IasAudioProcessingResult IasTestCompCore::init()
{
  return eIasAudioProcOK;
}

IasAudioProcessingResult IasTestCompCore::processChild()
{
  return eIasAudioProcOK;
}

IasTestComp::IasTestComp(const std::string &typeName, const std::string &instanceName)
  :IasGenericAudioComp(typeName, instanceName)
{
}

IasTestComp::~IasTestComp()
{
  delete mCore;
}

}//end namespace IasAudio
