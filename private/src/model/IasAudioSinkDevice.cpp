/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file
 */

#include "model/IasAudioSinkDevice.hpp"

namespace IasAudio {

IasAudioSinkDevice::IasAudioSinkDevice(IasAudioDeviceParamsPtr params)
  :IasAudioDevice(params)
  ,mDirection(eIasPortDirectionOutput)
{
}

IasAudioSinkDevice::~IasAudioSinkDevice()
{
  unlinkZone();
}

void IasAudioSinkDevice::linkZone(IasRoutingZonePtr routingZone)
{
  mRoutingZone = routingZone;
}

void IasAudioSinkDevice::unlinkZone()
{
  mRoutingZone = nullptr;
}

} // namespace IasAudio
