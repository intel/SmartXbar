/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file
 */

#include "model/IasAudioPort.hpp"
#include "avbaudiomodules/internal/audio/common/audiobuffer/IasAudioRingBuffer.hpp"
#include "model/IasAudioDevice.hpp"
#include "model/IasAudioPortOwner.hpp"


namespace IasAudio {

static const std::string cClassName = "IasAudioPort::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"
#define LOG_PORT "port=" + mParams->name + ":"


IasAudioPort::IasAudioPort(IasAudioPortParamsPtr params)
  :mLog(IasAudioLogging::registerDltContext("MDL", "SmartX Model"))
  ,mParams(params)
  ,mRingbuffer(nullptr)
  ,mOwner(nullptr)
  ,mNumConnections(0)
  ,mSwitchMatrix(nullptr)
{
  IAS_ASSERT(params != nullptr)
}

IasAudioPort::~IasAudioPort()
{
  DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX,LOG_PORT);
  mSwitchMatrix = nullptr;
}

IasAudioPort::IasResult IasAudioPort::setRingBuffer(IasAudioRingBuffer* buffer)
{
  if(buffer == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PORT, "buffer == nullptr");
    return eIasFailed;
  }
  else
  {
    mRingbuffer = buffer;
    return eIasOk;
  }
}

void IasAudioPort::clearRingBuffer()
{
  mRingbuffer = nullptr;
}

IasAudioPort::IasResult IasAudioPort::setOwner(IasAudioPortOwnerPtr owner)
{
  if(owner == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PORT, "owner == nullptr");
    return eIasFailed;
  }
  mOwner = owner;
  return eIasOk;
}

IasAudioPort::IasResult IasAudioPort::getOwner(IasAudioPortOwnerPtr* owner)
{
  if(owner == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PORT, "owner == nullptr");
    return eIasFailed;
  }
  if(mOwner == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PORT, "Owner wasn't set before");
    *owner = nullptr;
    return eIasFailed;
  }
  *owner = mOwner;
  return eIasOk;
}

void IasAudioPort::clearOwner()
{
  mOwner = nullptr;
}

IasAudioPort::IasResult IasAudioPort::getRingBuffer(IasAudioRingBuffer** ringBuffer) const
{
  if(ringBuffer == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PORT, "ringBuffer == nullptr");
    return eIasFailed;
  }
  if(mRingbuffer != nullptr)
  {
    *ringBuffer = mRingbuffer;
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PORT, "Ringbuffer was not set before");
    *ringBuffer = nullptr;
    return eIasFailed;
  }
  return eIasOk;
}

IasAudioPort::IasResult IasAudioPort::getCopyInformation(IasAudioPortCopyInformation* copyInfos) const
{
  if(copyInfos == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PORT, "copyInfos == nullptr");
    return eIasFailed;
  }

  if((mRingbuffer == nullptr) || (mOwner == nullptr))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PORT, "Object has not been initialized appropriately");
    return eIasFailed;
  }

  DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX, LOG_PORT, "Filling copy information");
  IAS_ASSERT(mRingbuffer != nullptr);
  IasAudioRingBufferResult rbres = mRingbuffer->getAreas(&(copyInfos->areas));
  IAS_ASSERT(rbres != IasAudioRingBufferResult::eIasRingBuffInvalidParam);
  if(rbres == IasAudioRingBufferResult::eIasRingBuffOk)
  {
    copyInfos->start = copyInfos->areas->start;
  }
  copyInfos->numChannels = mParams->numChannels;
  copyInfos->index = mParams->index;
  copyInfos->periodSize = mOwner->getPeriodSize();
  copyInfos->sampleRate = mOwner->getSampleRate();
  // Ask the ring buffer about its dataFormat.
  IasAudioRingBufferResult rbResult = mRingbuffer->getDataFormat(&copyInfos->dataFormat);
  if (rbResult != IasAudioRingBufferResult::eIasRingBuffOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PORT, "Error during IasAudioRingBuffer::getDataFormat:", toString(rbResult));
    return eIasFailed;
  }

  return eIasOk;
}

IasAudioPort::IasResult IasAudioPort::storeConnection(IasSwitchMatrixPtr switchMatrix)
{
  // Check whether this will be the first connections.
  if (mNumConnections == 0)
  {
    mSwitchMatrix = switchMatrix;
    mNumConnections++;
    return eIasOk;
  }

  // Verify that the existing connections use the same switchMatrix.
  if (switchMatrix != mSwitchMatrix)
  {
    /**
     * @log Audio port <NAME> is already connected to an independent zone (different clock domain).
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PORT, "Audio port is already connected to an independent zone");
    return eIasFailed;
  }

  mNumConnections++;
  return eIasOk;
}


IasAudioPort::IasResult IasAudioPort::forgetConnection(IasSwitchMatrixPtr switchMatrix)
{
  // Verify that the audio port is connected to the specified switchMatrix.
  if(mNumConnections == 0)
  {
    /**
     * @log Audio port <NAME> is not connected to the specified zone.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PORT, "Audio port is not connected to the specified zone");
    return eIasFailed;
  }
  if (switchMatrix != mSwitchMatrix)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PORT, "Port has no connection to the switch matrix passed in as parameter");
    return eIasFailed;
  }

  // Reduce the number of connections by one.
  mNumConnections--;
  if (mNumConnections == 0)
  {
    mSwitchMatrix = nullptr;
  }

  return eIasOk;
}

} // namespace IasAudio
