/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasAudioBufferPoolHandler.cpp
 * @date   2013
 * @brief  The definition of the IasAudioBufferPoolHandler class.
 */

#include "rtprocessingfwx/IasAudioBufferPoolHandler.hpp"
#include "rtprocessingfwx/IasAudioBufferPool.hpp"

namespace IasAudio {


IasAudioBufferPoolHandler::IasAudioBufferPoolHandler()
{
}

IasAudioBufferPoolHandler::~IasAudioBufferPoolHandler()
{
  for (IasSizePoolMap::iterator mapIt = mSizePoolMap.begin(); mapIt != mSizePoolMap.end(); ++mapIt)
  {
    delete mapIt->second;
  }

  mSizePoolMap.clear();
}

IasAudioBufferPoolHandler* IasAudioBufferPoolHandler::getInstance()
{
  static IasAudioBufferPoolHandler theInstance;
  return &theInstance;
}

IasAudioBufferPool* IasAudioBufferPoolHandler::getBufferPool(uint32_t ringBufferSize)
{
  IasAudioBufferPool* ringBufferPool = NULL;
  IasSizePoolMap::iterator poolIt = mSizePoolMap.find(ringBufferSize);
  if (poolIt != mSizePoolMap.end())
  {
    // Pool exists, so just return it
    ringBufferPool = (*poolIt).second;
  }
  else
  {
    // Currently no pool for that buffer size exists, so create one
    ringBufferPool = new IasAudioBufferPool(ringBufferSize);
    IAS_ASSERT(ringBufferPool != nullptr);
    mSizePoolMap.insert(std::make_pair(ringBufferSize, ringBufferPool));
  }

  return ringBufferPool;
}


} // namespace IasAudio
