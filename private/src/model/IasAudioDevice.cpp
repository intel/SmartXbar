/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file
 */


#include "IasSmartXClient.hpp"
#include "alsahandler/IasAlsaHandler.hpp"
#include "internal/audio/common/IasAudioLogging.hpp"
#include "model/IasAudioPort.hpp"
#include "model/IasAudioDevice.hpp"


namespace IasAudio {

class IasAudioRingBuffer;

static const std::string cClassName = "IasAudioDevice::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + ")[" + mDeviceParams->name + "]:"


#define STRING_RETURN_CASE(name) case name: return std::string(#name); break
#define DEFAULT_STRING(name) default: return std::string(name)


IAS_AUDIO_PUBLIC std::string toString(const IasAudioDevice::IasEventType&  eventType)
{
  switch(eventType)
  {
    STRING_RETURN_CASE(IasAudioDevice::eIasNoEvent);
    STRING_RETURN_CASE(IasAudioDevice::eIasStart);
    STRING_RETURN_CASE(IasAudioDevice::eIasStop);
    DEFAULT_STRING("Undefined IasEventType");
  }
}


IasAudioDevice::IasAudioDevice(IasAudioDeviceParamsPtr params)
  :mLog(IasAudioLogging::registerDltContext("MDL", "SmartX Model"))
  ,mDeviceParams(params)
  ,mSmartXClient(nullptr)
  ,mAlsaHandler(nullptr)
{
  IAS_ASSERT(params != nullptr);
  mAudioPortSet = std::make_shared<IasAudioPortSet>();
}

IasAudioDevice::~IasAudioDevice()
{
  DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX, "[", mDeviceParams->name, "]");
  mAudioPortSet.reset();
}

void IasAudioDevice::setConcreteDevice(IasSmartXClientPtr smartXClient)
{
  if (smartXClient != nullptr)
  {
    mSmartXClient = smartXClient;
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_WARN, LOG_PREFIX,"Null pointer passed to function! Function ignored the parameter");
  }
}

bool IasAudioDevice::isSmartXClient() const
{
  return(mSmartXClient != NULL);
}

IasAudioCommonResult IasAudioDevice::getConcreteDevice(IasSmartXClientPtr* smartXClient)
{
  if (smartXClient == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "SmartX client is not set");
    return eIasResultInvalidParam;
  }
  *smartXClient = mSmartXClient;
  return eIasResultOk;
}

void IasAudioDevice::setConcreteDevice(IasAlsaHandlerPtr alsaHandler)
{
  if (alsaHandler != nullptr)
  {
    mAlsaHandler = alsaHandler;
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_WARN, LOG_PREFIX,"Null pointer passed to function! Function ignored the parameter");
  }
}

bool IasAudioDevice::isAlsaHandler() const
{
  return(mAlsaHandler != nullptr);
}

IasAudioCommonResult IasAudioDevice::getConcreteDevice(IasAlsaHandlerPtr* alsaHandler)
{
  if (alsaHandler == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "ALSA handler is not set");
    return eIasResultInvalidParam;
  }
  *alsaHandler = mAlsaHandler;
  return eIasResultOk;
}

const IasAudioDeviceParamsPtr IasAudioDevice::getDeviceParams() const
{
  return mDeviceParams;
}

IasAudioRingBuffer* IasAudioDevice::getRingBuffer()
{
  IasAudioRingBuffer *ringBuffer = nullptr;
  if (isAlsaHandler())
  {
    mAlsaHandler->getRingBuffer(&ringBuffer);
    IAS_ASSERT(ringBuffer != nullptr);
  }
  else if (isSmartXClient())
  {
    mSmartXClient->getRingBuffer(&ringBuffer);
    IAS_ASSERT(ringBuffer != nullptr);
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Audio device is neither a SmartX client nor an ALSA handler");
  }
  return ringBuffer;
}

IasAudioCommonResult IasAudioDevice::addAudioPort(IasAudioPortPtr audioPort)
{
  auto status = mAudioPortSet->insert(audioPort);
  if (!status.second)
  {
    IasAudioPortParamsPtr audioPortParams = audioPort->getParameters();
    IAS_ASSERT(audioPortParams != nullptr); // already checked in constructor of IasAudioPort

    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error: Audio port", audioPortParams->name,
                "has already been added to this device");
    return eIasResultObjectAlreadyExists;
  }
  return eIasResultOk;
}


void IasAudioDevice::deleteAudioPort(IasAudioPortPtr audioPort)
{
  auto numElements = mAudioPortSet->erase(audioPort);
  if (numElements != 1)
  {
    IasAudioPortParamsPtr audioPortParams = audioPort->getParameters();
    IAS_ASSERT(audioPortParams != nullptr); // already checked in constructor of IasAudioPort

    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error: cannot find audio port", audioPortParams->name,
                "in set of audio ports of this device");
  }
}

void IasAudioDevice::enableEventQueue(bool enable)
{
  if (isSmartXClient())
  {
    mSmartXClient->enableEventQueue(enable);
  }
}

IasAudioDevice::IasEventType IasAudioDevice::getNextEventType()
{
  if (isSmartXClient())
  {
    return mSmartXClient->getNextEventType();
  }
  else
  {
    return eIasNoEvent;
  }
}

} // namespace Ias
