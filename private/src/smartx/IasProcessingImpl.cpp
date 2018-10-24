/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasProcessingImpl.cpp
 * @date   2015
 * @brief
 */

#include "IasProcessingImpl.hpp"

#include "rtprocessingfwx/IasCmdDispatcher.hpp"
#include "audio/smartx/IasProperties.hpp"

namespace IasAudio {

IasProcessingImpl::IasProcessingImpl(IasCmdDispatcherPtr cmdDispatcher)
  :mCmdDispatcher(cmdDispatcher)
{
  IAS_ASSERT(mCmdDispatcher != nullptr);
}

IasProcessingImpl::~IasProcessingImpl()
{
  mCmdDispatcher = nullptr;
}

IasIProcessing::IasResult translate(IasICmdRegistration::IasResult cmdres)
{
  if (cmdres == IasICmdRegistration::eIasOk)
  {
    return IasIProcessing::eIasOk;
  }
  else
  {
    return IasIProcessing::eIasFailed;
  }
}

IasIProcessing::IasResult IasProcessingImpl::sendCmd(const std::string &instanceName, const IasProperties &cmdProperties, IasProperties &returnProperties)
{
  return translate(mCmdDispatcher->dispatchCmd(instanceName, cmdProperties, returnProperties));
}


} //namespace IasAudio
