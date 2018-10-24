/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasAudioSubdevice.cpp
 * @date   2012
 * @brief  This is the implementation of the IasAudioSubdevice class.
 */

#include <sstream>
#include <iomanip>
#include "IasAudioSubdevice.hpp"
#include "IasAudioDevice.hpp"
#include "audio/smartx/rtprocessingfwx/IasProcessingTypes.hpp"

namespace IasAudio {

IasAudioSubdevice::IasAudioSubdevice(const IasAudioDevice *parent,
                                     const std::string &name,
                                     int32_t id,
                                     uint32_t numberChannels,
                                     uint32_t offset)
  :mParent(parent)
  ,mName(name)
  ,mId(id)
  ,mNumberChannels(numberChannels)
  ,mOffset(offset)
{
}

IasAudioSubdevice::~IasAudioSubdevice()
{
}

IasAudioProcessingResult IasAudioSubdevice::init()
{
  createChannelNames();
  return eIasAudioProcOK;
}


void IasAudioSubdevice::createChannelNames()
{
  if (mChannelNames.empty())
  {
    mChannelNames.reserve(mNumberChannels);
    mChannelNames.resize(mNumberChannels);
  }
  for (uint32_t chanIdx = 0; chanIdx < mNumberChannels; ++chanIdx)
  {
    mChannelNames[chanIdx] = mParent->createChannelName(mName, chanIdx, mOffset);
  }
}

const IasStringVector& IasAudioSubdevice::getChannelNames() const
{
  return mChannelNames;
}

const std::string* IasAudioSubdevice::getChannelName(uint32_t channelIndex) const
{
  if (channelIndex < mChannelNames.size())
  {
    return &mChannelNames[channelIndex];
  }
  else
  {
    return NULL;
  }
}


} // namespace IasAudio
