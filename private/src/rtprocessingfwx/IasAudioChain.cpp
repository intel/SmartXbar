/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasAudioChain.cpp
 * @date   2012
 * @brief  This is the implementation of the IasAudioChain class.
 */

#include "rtprocessingfwx/IasAudioChain.hpp"


#include "audio/smartx/rtprocessingfwx/IasAudioStream.hpp"
#include "audio/smartx/rtprocessingfwx/IasGenericAudioComp.hpp"
#include "audio/smartx/rtprocessingfwx/IasGenericAudioCompCore.hpp"
#include "avbaudiomodules/internal/audio/common/IasAudioLogging.hpp"
#include "audio/smartx/rtprocessingfwx/IasIModuleId.hpp"


namespace IasAudio {

static const std::string cClassName = "IasAudioChain::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"

class IasAudioChainItem;

IasAudioChain::IasAudioChain()
  :mInputBundleSequencer()
  ,mOutputBundleSequencer()
  ,mIntermediateInputBundleSequencer()
  ,mIntermediateOutputBundleSequencer()
  ,mInputAudioStreams()
  ,mOutputAudioStreams()
  ,mIntermediateInputAudioStreams()
  ,mIntermediateOutputAudioStreams()
  ,mComps()
  ,mCompCores()
  ,mZoneIdOutputStreamMap()
  ,mLog(IasAudioLogging::registerDltContext("PFW", "Log of rtprocessing framework"))
  ,mEnv(nullptr)
{
}

IasAudioChain::~IasAudioChain()
{
  // mCompCores only contains pointers to members of the mComps, so just clearing the vector without
  // deleting the pointers is fine.
  mCompCores.clear();

  // We also do not delete the comps as they are deleted by the IasConfigurationCreatorImpl
  mComps.clear();

  while (!mInputAudioStreams.empty())
  {
    delete mInputAudioStreams.back();
    mInputAudioStreams.pop_back();
  }

  while (!mOutputAudioStreams.empty())
  {
    delete mOutputAudioStreams.back();
    mOutputAudioStreams.pop_back();
  }

  while (!mIntermediateInputAudioStreams.empty())
  {
    delete mIntermediateInputAudioStreams.back();
    mIntermediateInputAudioStreams.pop_back();
  }

  while (!mIntermediateOutputAudioStreams.empty())
  {
    delete mIntermediateOutputAudioStreams.back();
    mIntermediateOutputAudioStreams.pop_back();
  }

  mZoneIdOutputStreamMap.clear();
  mEnv = nullptr;

  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Deleted");
}

void IasAudioChain::addAudioComponent(IasGenericAudioComp *newAudioProcessingModule)
{
  IAS_ASSERT(newAudioProcessingModule != nullptr);
  IasGenericAudioCompCore *core = newAudioProcessingModule->getCore();
  IAS_ASSERT(core != nullptr);

  IasAudioProcessingResult result = core->init(mEnv);

  if (result == eIasAudioProcOK)
  {
    mComps.push_back(newAudioProcessingModule);
    mCompCores.push_back(core);

    newAudioProcessingModule->getCore()->setComponentIndex((int32_t)(mCompCores.size()-1));
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "index=", mCompCores.size()-1);

    IasIModuleId* cmdInterface = newAudioProcessingModule->getCmdInterface();
    IAS_ASSERT(cmdInterface != nullptr);

    IasIModuleId::IasResult const modres = cmdInterface->init();

    if (modres == IasIModuleId::eIasOk)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Init cmd interface successfully");
    }
    else
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Init cmd interface error");
    }

  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error initializing audio module of type", newAudioProcessingModule->getTypeName());
  }

}

IasAudioStream*  IasAudioChain::createInputAudioStream(const std::string& name, int32_t id, int32_t numberChannels, bool sidAvailable)
{
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX,
                  "Creating new input audio stream: name=", name.c_str(),
                  "id=", id,
                  "nrChannels=", numberChannels,
                  "sidAvailable=", sidAvailable);
  IasAudioStream *newStream = new IasAudioStream(name,
                                                 id,
                                                 numberChannels,
                                                 IasBaseAudioStream::eIasAudioStreamInput,
                                                 sidAvailable);
  IAS_ASSERT(newStream != nullptr);
  newStream->setBundleSequencer(&mInputBundleSequencer);
  IasAudioProcessingResult result = newStream->init(mEnv);
  if (result == eIasAudioProcOK)
  {
    mInputAudioStreams.push_back(newStream);
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX
                "Successfully created new input audio stream");
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error initializing new input audio stream: name=", name.c_str(),
                "error=", toString(result));
    delete newStream;
    newStream = NULL;
  }
  return newStream;
}

IasAudioStream*  IasAudioChain::createOutputAudioStream(const std::string& name, int32_t id, int32_t numberChannels, bool sidAvailable)
{
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX
                  "Creating new output audio stream: name=", name.c_str(),
                  "id=", id,
                  "nrChannels=", numberChannels,
                  "sidAvailable=", sidAvailable);
  IasAudioStream *newStream = new IasAudioStream(name,
                                                 id,
                                                 numberChannels,
                                                 IasBaseAudioStream::eIasAudioStreamOutput,
                                                 sidAvailable);
  IAS_ASSERT(newStream != nullptr);
  newStream->setBundleSequencer(&mOutputBundleSequencer);
  IasAudioProcessingResult result = newStream->init(mEnv);
  if (result == eIasAudioProcOK)
  {
    mOutputAudioStreams.push_back(newStream);
    mZoneIdOutputStreamMap[id] = newStream;
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX,
                "Successfully created new output audio stream");
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Error initializing new output audio stream: name=", name.c_str(),
                "error=", toString(result));
    delete newStream;
    newStream = NULL;
  }
  return newStream;
}

IasAudioStream* IasAudioChain::createIntermediateInputAudioStream(const std::string& name, int32_t id, int32_t numberChannels, bool sidAvailable)
{
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX,
                  "Creating new intermediate input audio stream: name=", name.c_str(),
                  "id=", id,
                  "nrChannels=", numberChannels,
                  "sidAvailable=", sidAvailable);
  IasAudioStream *newStream = new IasAudioStream(name,
                                                 id,
                                                 numberChannels,
                                                 IasBaseAudioStream::eIasAudioStreamIntermediate,
                                                 sidAvailable);
  IAS_ASSERT(newStream != nullptr);
  newStream->setBundleSequencer(&mIntermediateInputBundleSequencer);
  IasAudioProcessingResult result = newStream->init(mEnv);
  if (result == eIasAudioProcOK)
  {
    mIntermediateInputAudioStreams.push_back(newStream);
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX,
                "Successfully created new intermediate input audio stream");
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Error initializing new input intermediate audio stream: name=",name.c_str(),
                "error=", toString(result));
    delete newStream;
    newStream = NULL;
  }
  return newStream;

}

IasAudioStream* IasAudioChain::createIntermediateOutputAudioStream(const std::string& name, int32_t id, int32_t numberChannels, bool sidAvailable)
{
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX,
                  "Creating new intermediate output audio stream: name=", name.c_str(),
                  "id=", id,
                  "nrChannels=", numberChannels,
                  "sidAvailable=", sidAvailable);
  IasAudioStream *newStream = new IasAudioStream(name,
                                                 id,
                                                 numberChannels,
                                                 IasBaseAudioStream::eIasAudioStreamIntermediate,
                                                 sidAvailable);
  IAS_ASSERT(newStream != nullptr);
  newStream->setBundleSequencer(&mIntermediateOutputBundleSequencer);
  IasAudioProcessingResult result = newStream->init(mEnv);
  if (result == eIasAudioProcOK)
  {
    mIntermediateOutputAudioStreams.push_back(newStream);
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX,
                "Successfully created new intermediate output audio stream");
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Error initializing new output intermediate audio stream: name=", name.c_str(),
                "error=", toString(result));
    delete newStream;
    newStream = NULL;
  }
  return newStream;

}

IasAudioChain::IasResult IasAudioChain::init(const IasInitParams& params)
{
  mEnv = std::make_shared<IasAudioChainEnvironment>();
  IAS_ASSERT(mEnv != nullptr);
  mEnv->setFrameLength(params.periodSize);
  mEnv->setSampleRate(params.sampleRate);
  mInputBundleSequencer.init(mEnv);
  mIntermediateInputBundleSequencer.init(mEnv);
  mOutputBundleSequencer.init(mEnv);
  mIntermediateOutputBundleSequencer.init(mEnv);
  float periodTime = 1000.0f * static_cast<float>(params.periodSize) / static_cast<float>(params.sampleRate);
  if (periodTime < 1.0f) // periodTime is expressed in ms
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_WARN, LOG_PREFIX, "The period time is", periodTime, "ms. Is this really intended?");
  }

  return eIasOk;
}

const IasAudioStream* IasAudioChain::getOutputStream(int32_t zoneId) const
{
  const IasAudioStream *outputStream = NULL;
  IasZoneIdOutputStreamMap::const_iterator mapIt = mZoneIdOutputStreamMap.find(zoneId);
  if (mapIt != mZoneIdOutputStreamMap.end())
  {
    outputStream = (*mapIt).second;
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                    "Can't find output audio stream with id=", zoneId);
  }

  return outputStream;
}

void IasAudioChain::process() const
{
  IasGenericAudioCompCoreVector::const_iterator coreIt;
  for (coreIt=mCompCores.begin(); coreIt<mCompCores.end(); ++coreIt)
  {
    (void)(*coreIt)->process();
  }
}

void IasAudioChain::clearOutputBundleBuffers() const
{
  mOutputBundleSequencer.clearAllBundleBuffers();
  mInputBundleSequencer.clearAllBundleBuffers();
  mIntermediateOutputBundleSequencer.clearAllBundleBuffers();
}

#define STRING_RETURN_CASE(name) case name: return std::string(#name); break
#define DEFAULT_STRING(name) default: return std::string(name)

IAS_AUDIO_PUBLIC std::string toString(const IasAudioChain::IasResult& type)
{
  switch(type)
  {
    STRING_RETURN_CASE(IasAudioChain::eIasOk);
    STRING_RETURN_CASE(IasAudioChain::eIasFailed);
    DEFAULT_STRING("Invalid IasAudioChain::IasResult => " + std::to_string(type));
  }
}


} // namespace IasAudio
