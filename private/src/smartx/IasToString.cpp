/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasToString.cpp
 * @date   2015
 * @brief
 */

#include <string>
#include "audio/smartx/IasConnectionEvent.hpp"
#include "audio/smartx/IasSetupEvent.hpp"
#include "audio/smartx/IasIRouting.hpp"
#include "audio/smartx/IasISetup.hpp"
#include "audio/smartx/IasIProcessing.hpp"
#include "audio/smartx/IasSmartX.hpp"
#include "audio/smartx/IasProperties.hpp"
#include "IasSmartXPriv.hpp"
#include "IasConfiguration.hpp"
#include "IasSmartXClient.hpp"

#define STRING_RETURN_CASE(name) case name: return std::string(#name); break
#define DEFAULT_STRING(name) default: return std::string(name)

namespace IasAudio {

IAS_AUDIO_PUBLIC std::string toString(const IasConnectionEvent::IasEventType& type)
{
  switch(type)
  {
    STRING_RETURN_CASE(IasConnectionEvent::eIasUninitialized);
    STRING_RETURN_CASE(IasConnectionEvent::eIasConnectionEstablished);
    STRING_RETURN_CASE(IasConnectionEvent::eIasConnectionRemoved);
    STRING_RETURN_CASE(IasConnectionEvent::eIasSourceDeleted);
    STRING_RETURN_CASE(IasConnectionEvent::eIasSinkDeleted);
    DEFAULT_STRING("Invalid IasConnectionEvent::IasEventType");
  }
}

IAS_AUDIO_PUBLIC std::string toString(const IasSetupEvent::IasEventType&  type)
{
  switch(type)
  {
    STRING_RETURN_CASE(IasSetupEvent::eIasUninitialized);
    STRING_RETURN_CASE(IasSetupEvent::eIasUnrecoverableSourceDeviceError);
    STRING_RETURN_CASE(IasSetupEvent::eIasUnrecoverableSinkDeviceError);
    DEFAULT_STRING("Invalid IasSetupEvent::IasEventType");
  }
}

IAS_AUDIO_PUBLIC std::string toString(const IasISetup::IasResult& type)
{
  switch(type)
  {
    STRING_RETURN_CASE(IasISetup::eIasOk);
    STRING_RETURN_CASE(IasISetup::eIasFailed);
    DEFAULT_STRING("Invalid IasISetup::IasResult => " + std::to_string(type));
  }
}

IAS_AUDIO_PUBLIC std::string toString(const IasSmartX::IasResult& type)
{
  switch(type)
  {
    STRING_RETURN_CASE(IasSmartX::eIasOk);
    STRING_RETURN_CASE(IasSmartX::eIasFailed);
    STRING_RETURN_CASE(IasSmartX::eIasOutOfMemory);
    STRING_RETURN_CASE(IasSmartX::eIasNullPointer);
    STRING_RETURN_CASE(IasSmartX::eIasTimeout);
    STRING_RETURN_CASE(IasSmartX::eIasNoEventAvailable);
    DEFAULT_STRING("Invalid IasSmartX::IasResult => " + std::to_string(type));
  }
}

std::string toString(const IasSmartXPriv::IasResult& type)
{
  switch(type)
  {
    STRING_RETURN_CASE(IasSmartXPriv::eIasOk);
    STRING_RETURN_CASE(IasSmartXPriv::eIasFailed);
    STRING_RETURN_CASE(IasSmartXPriv::eIasOutOfMemory);
    STRING_RETURN_CASE(IasSmartXPriv::eIasNullPointer);
    STRING_RETURN_CASE(IasSmartXPriv::eIasTimeout);
    STRING_RETURN_CASE(IasSmartXPriv::eIasNoEventAvailable);
    DEFAULT_STRING("Invalid IasSmartXPriv::IasResult => " + std::to_string(type));
  }
}

std::string toString(const IasConfiguration::IasResult& type)
{
  switch(type)
  {
    STRING_RETURN_CASE(IasConfiguration::eIasOk);
    STRING_RETURN_CASE(IasConfiguration::eIasNullPointer);
    STRING_RETURN_CASE(IasConfiguration::eIasObjectAlreadyExists);
    STRING_RETURN_CASE(IasConfiguration::eIasObjectNotFound);
    DEFAULT_STRING("Invalid IasConfiguration::IasResult => " + std::to_string(type));
  }
}

std::string toString(const IasSmartXClient::IasResult& type)
{
  switch(type)
  {
    STRING_RETURN_CASE(IasSmartXClient::eIasOk);
    STRING_RETURN_CASE(IasSmartXClient::eIasFailed);
    DEFAULT_STRING("Invalid IasSmartXClient::IasResult => " + std::to_string(type));
  }
}

IAS_AUDIO_PUBLIC std::string toString(const IasIRouting::IasResult& type)
{
  switch(type)
  {
    STRING_RETURN_CASE(IasIRouting::eIasOk);
    STRING_RETURN_CASE(IasIRouting::eIasFailed);
    DEFAULT_STRING("Invalid IasIRouting::IasResult => " + std::to_string(type));
  }
}

IAS_AUDIO_PUBLIC std::string toString(const IasIProcessing::IasResult& type)
{
  switch(type)
  {
    STRING_RETURN_CASE(IasIProcessing::eIasOk);
    STRING_RETURN_CASE(IasIProcessing::eIasFailed);
    DEFAULT_STRING("Invalid IasIProcessing::IasResult => " + std::to_string(type));
  }
}

IAS_AUDIO_PUBLIC std::string toString(const IasProperties::IasResult& type)
{
  switch(type)
  {
    STRING_RETURN_CASE(IasProperties::eIasOk);
    STRING_RETURN_CASE(IasProperties::eIasKeyNotFound);
    STRING_RETURN_CASE(IasProperties::eIasInvalidParam);
    DEFAULT_STRING("Invalid IasProperties::IasResult => " + std::to_string(type));
  }
}

} //namespace IasAudio
