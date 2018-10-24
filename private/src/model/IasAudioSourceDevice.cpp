/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file
 */

#include "model/IasAudioSourceDevice.hpp"

namespace IasAudio {

IasAudioSourceDevice::IasAudioSourceDevice(IasAudioDeviceParamsPtr params)
  :IasAudioDevice(params)
  ,mDirection(eIasPortDirectionInput)
{

}

IasAudioSourceDevice::~IasAudioSourceDevice()
{
}

} // namespace IasAudio
