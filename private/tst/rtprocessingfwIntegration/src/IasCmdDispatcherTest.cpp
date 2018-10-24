/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasCmdDispatcherTest.cpp
 * @brief  Contains some test cases for the IasCmdDispatcher class
 */

#include "audio/smartx/rtprocessingfwx/IasIModuleId.hpp"
#include "rtprocessingfwx/IasCmdDispatcher.hpp"
#include "IasRtProcessingFwTest.hpp"

namespace IasAudio {

class IasTestModuleId : public IasIModuleId
{
  public:
    IasTestModuleId(const IasIGenericAudioCompConfig *config)
      :IasIModuleId(config)
    {}

    virtual IasResult init()
    {
      return eIasOk;
    }

    virtual IasResult processCmd(const IasProperties &cmdProperties, IasProperties &returnProperties)
    {
      (void)cmdProperties;
      (void)returnProperties;
      return eIasOk;
    }
};

TEST_F(IasRtProcessingFwTest, CmdDispatcherTest)
{
  IasCmdDispatcher *cmdDispatcher = new IasCmdDispatcher();
  ASSERT_TRUE(cmdDispatcher != nullptr);
  IasICmdRegistration::IasResult res = cmdDispatcher->registerModuleIdInterface("", nullptr);
  ASSERT_EQ(IasICmdRegistration::eIasFailed, res);
  res = cmdDispatcher->registerModuleIdInterface("MyModule", nullptr);
  ASSERT_EQ(IasICmdRegistration::eIasFailed, res);

  IasTestModuleId *testModule = new IasTestModuleId(nullptr);
  ASSERT_TRUE(testModule != nullptr);
  res = cmdDispatcher->registerModuleIdInterface("MyModule", testModule);
  ASSERT_EQ(IasICmdRegistration::eIasOk, res);
  res = cmdDispatcher->registerModuleIdInterface("MyModule", testModule);
  ASSERT_EQ(IasICmdRegistration::eIasFailed, res);

  std::string name = "";
  cmdDispatcher->unregisterModuleIdInterface(name);
  cmdDispatcher->unregisterModuleIdInterface("unregistered");

  delete cmdDispatcher;
}


}
