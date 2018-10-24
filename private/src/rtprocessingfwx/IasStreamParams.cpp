/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasStreamParams.cpp
 * @date   2012
 * @brief  This is the implementation of the IasStreamParams class.
 */

#include "rtprocessingfwx/IasStreamParams.hpp"

namespace IasAudio {

IasStreamParams::IasStreamParams(uint32_t bundleIndex,
                                 uint32_t channelIndex,
                                 uint32_t numberChannels)
  :mBundleIndex(bundleIndex)
  ,mChannelIndex(channelIndex)
  ,mNumberChannels(numberChannels)
{
}

IasStreamParams::~IasStreamParams()
{
}

} // namespace IasAudio
