/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file   IasAudioPin.cpp
 * @date   2016
 * @brief  Audio pins are used for connecting processing modules within a pipeline.
 */


#include "model/IasAudioPin.hpp"

namespace IasAudio {

static const std::string cClassName = "IasAudioPin::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"
#define LOG_PIN "pin=" + mParams->name + ":"


IasAudioPin::IasAudioPin(IasAudioPinParamsPtr params)
  :mLog(IasAudioLogging::registerDltContext("MDL", "SmartX Model"))
  ,mParams(params)
  ,mPipeline(nullptr)
  ,mDirection(eIasPinDirectionUndef)
{
  IAS_ASSERT(params != nullptr)
}

IasAudioPin::~IasAudioPin()
{
  DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX, LOG_PIN);
}


IasAudioPin::IasResult IasAudioPin::setDirection(IasPinDirection direction)
{
  if ((mDirection != eIasPinDirectionUndef) && (direction != eIasPinDirectionUndef) && (direction != mDirection))
  {
    return eIasFailed;
  }

  mDirection = direction;
  return eIasOk;
}


IasAudioPin::IasResult IasAudioPin::addToPipeline(IasPipelinePtr pipeline, const std::string &pipelineName)
{
  IAS_ASSERT(pipeline != nullptr); // already checked in IasSetupImpl::addAudioInputPin or IasSetupImpl::addAudioOutputPin

  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIN, "Adding audio pin to pipeline", pipelineName);

  // Check whether the pin already belongs to a different pipeline.
  if ((mPipeline != nullptr) && (mPipeline != pipeline))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_PIN,
                "Audio pin cannot be added to pipeline", pipelineName,
                "because it already belongs to pipeline", mPipelineName);
    return eIasFailed;
  }
  mPipeline     = pipeline;
  mPipelineName = pipelineName;
  return eIasOk;
}


void IasAudioPin::deleteFromPipeline()
{
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIN, "Deleting audio pin from pipeline");

  if (mPipeline == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_PIN,
                "Audio pin cannot be deleted from pipeline, since it has not been added before.");
  }
  mPipeline = nullptr;
}


/*
 * Function to get a IasAudioPin::IasResult as string.
 */
#define STRING_RETURN_CASE(name) case name: return std::string(#name); break
#define DEFAULT_STRING(name) default: return std::string(name)
std::string toString(const IasAudioPin::IasResult& type)
{
  switch(type)
  {
    STRING_RETURN_CASE(IasAudioPin::eIasOk);
    STRING_RETURN_CASE(IasAudioPin::eIasFailed);
    DEFAULT_STRING("Invalid IasAudioPin::IasResult => " + std::to_string(type));
  }
}


std::string toString(const IasAudioPin::IasPinDirection&  type)
{
  switch(type)
  {
    STRING_RETURN_CASE(IasAudioPin::eIasPinDirectionUndef);
    STRING_RETURN_CASE(IasAudioPin::eIasPinDirectionPipelineInput);
    STRING_RETURN_CASE(IasAudioPin::eIasPinDirectionPipelineOutput);
    STRING_RETURN_CASE(IasAudioPin::eIasPinDirectionModuleInput);
    STRING_RETURN_CASE(IasAudioPin::eIasPinDirectionModuleOutput);
    STRING_RETURN_CASE(IasAudioPin::eIasPinDirectionModuleInputOutput);
    DEFAULT_STRING("IasAudioPin::eIasPinDirectionInvalid");
  }
}


} // namespace IasAudio
