/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasAudioStream.cpp
 * @date   2013
 * @brief  The definition of the IasAudioStream class.
 */

#include "audio/smartx/rtprocessingfwx/IasBaseAudioStream.hpp"

namespace IasAudio {

IasBaseAudioStream::IasBaseAudioStream(const std::string &name,
                               int32_t id,
                               uint32_t numberChannels,
                               IasAudioStreamTypes type,
                               bool sidAvailable)
  :mName(name)
  ,mId(id)
  ,mNumberChannels(numberChannels)
  ,mType(type)
  ,mSidAvailable(sidAvailable)
  ,mSid(0)
  ,mSampleLayout(eIasUndefined)
{
}

IasBaseAudioStream::~IasBaseAudioStream()
{
}


} // namespace IasAudio
