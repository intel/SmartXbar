/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasBundleAssignment.cpp
 * @date   2012
 * @brief  This is the implementation of the IasBundleAssignment class.
 */

#include <stdio.h>
#include "rtprocessingfwx/IasBundleAssignment.hpp"
#include "audio/smartx/rtprocessingfwx/IasProcessingTypes.hpp"

namespace IasAudio {

IasBundleAssignment::IasBundleAssignment(IasAudioChannelBundle *bundle, uint32_t channelIndex, uint32_t numberChannels)
  :mBundle(bundle)
  ,mChannelIndex(channelIndex)
  ,mNumberChannels(numberChannels)
{
}

IasBundleAssignment::~IasBundleAssignment()
{
}

} // namespace IasAudio
