/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file   IasTestFramework.cpp
 * @date   2017
 * @brief  The test framework of the SmartXbar for verifying custom processing modules.
 */

#include "testfwx/IasTestFrameworkPriv.hpp"
#include "audio/testfwx/IasTestFramework.hpp"



namespace IasAudio {


std::atomic_int IasTestFramework::mNumberInstances(0);


IasTestFramework::IasTestFramework()
  :mPriv(nullptr)
{
}


IasTestFramework::~IasTestFramework()
{
  delete mPriv;
}


IasTestFramework* IasTestFramework::create(const IasPipelineParams &pipelineParams)
{
  int32_t expected = 0;
  if (mNumberInstances.compare_exchange_weak(expected, 1) == false)
  {
    return nullptr;
  }
  IasTestFramework *testFramework = new IasTestFramework();
  IAS_ASSERT(testFramework != nullptr);
  IasResult result = testFramework->init(pipelineParams);
  if(result != eIasOk)
  {
    delete testFramework;
    return nullptr;
  }
  (void)result;
  return testFramework;
}


void IasTestFramework::destroy(IasTestFramework *instance)
{
  int32_t expected = 1;
  if (mNumberInstances.compare_exchange_weak(expected, 0) == true)
  {
    delete instance;
  }
}


IasTestFramework::IasResult IasTestFramework::init(const IasPipelineParams &pipelineParams)
{
  mPriv = new IasTestFrameworkPriv();
  IAS_ASSERT(mPriv != nullptr);
  return static_cast<IasTestFramework::IasResult>(mPriv->init(pipelineParams));
}


IasTestFramework::IasResult IasTestFramework::start()
{
  IAS_ASSERT(mPriv != nullptr);
  return static_cast<IasTestFramework::IasResult>(mPriv->start());
}


IasTestFramework::IasResult IasTestFramework::stop()
{
  IAS_ASSERT(mPriv != nullptr);
  return static_cast<IasTestFramework::IasResult>(mPriv->stop());
}


IasTestFramework::IasResult IasTestFramework::process(uint32_t numPeriods)
{
  IAS_ASSERT(mPriv != nullptr);
  return static_cast<IasTestFramework::IasResult>(mPriv->process(numPeriods));
}


IasTestFrameworkSetup* IasTestFramework::setup()
{
  IAS_ASSERT(mPriv != nullptr);
  return mPriv->setup();
}


IasIProcessing* IasTestFramework::processing()
{
  IAS_ASSERT(mPriv != nullptr);
  return mPriv->processing();
}

IasIDebug* IasTestFramework::debug()
{
  IAS_ASSERT(mPriv != nullptr);
  return mPriv->debug();
}


IasTestFramework::IasResult IasTestFramework::waitForEvent(uint32_t timeout) const
{
  IAS_ASSERT(mPriv != nullptr);
  return static_cast<IasTestFramework::IasResult>(mPriv->waitForEvent(timeout));
}


IasTestFramework::IasResult IasTestFramework::getNextEvent(IasEventPtr* event)
{
  IAS_ASSERT(mPriv != nullptr);
  return static_cast<IasTestFramework::IasResult>(mPriv->getNextEvent(event));
}


/*
 * Function to get a IasTestFramework::IasResult as string.
 */
#define STRING_RETURN_CASE(name) case name: return std::string(#name); break
#define DEFAULT_STRING(name) default: return std::string(name)
std::string toString(const IasTestFramework::IasResult& type)
{
  switch(type)
  {
    STRING_RETURN_CASE(IasTestFramework::eIasOk);
    STRING_RETURN_CASE(IasTestFramework::eIasFinished);
    STRING_RETURN_CASE(IasTestFramework::eIasFailed);
    STRING_RETURN_CASE(IasTestFramework::eIasOutOfMemory);
    STRING_RETURN_CASE(IasTestFramework::eIasNullPointer);
    STRING_RETURN_CASE(IasTestFramework::eIasTimeout);
    STRING_RETURN_CASE(IasTestFramework::eIasNoEventAvailable);
    DEFAULT_STRING("Invalid IasTestFramework::IasResult => " + std::to_string(type));
  }
}


} // namespace IasAudio
