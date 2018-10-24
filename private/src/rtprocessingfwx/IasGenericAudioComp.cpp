/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasGenericAudioComp.cpp
 * @date   2012
 * @brief  This is the implementation of the IasGenericAudioComp class.
 */

#include "audio/smartx/rtprocessingfwx/IasGenericAudioComp.hpp"

namespace IasAudio {

IasGenericAudioComp::IasGenericAudioComp(const std::string &typeName, const std::string &instanceName)
  :mCmdInterface(nullptr)
  ,mCore(nullptr)
  ,mTypeName(typeName)
  ,mInstanceName(instanceName)
{
}

IasGenericAudioComp::~IasGenericAudioComp()
{
}

} // namespace IasAudio
