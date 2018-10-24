/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/*
 * IasPluginsTest.cpp
 *
 *  Created 2016
 */

#include <rtprocessingfwx/IasCmdDispatcher.hpp>
#include <stdlib.h>
#include <gtest/gtest.h>
#include <dlt/dlt.h>
#include "rtprocessingfwx/IasPluginEngine.hpp"
#include "audio/smartx/rtprocessingfwx/IasGenericAudioComp.hpp"
#include "audio/smartx/IasProperties.hpp"
#include "IasTestComp.hpp"

#include "audio/smartx/rtprocessingfwx/IasPluginLibrary.hpp" // required only for test case test_pluginLibraryUnregisterFactoryMethods

DLT_DECLARE_CONTEXT(tstCtx);

#ifndef AUDIO_PLUGIN_DIR
#define AUDIO_PLUGIN_DIR    "."
#endif

int main(int argc, char* argv[])
{
  setenv("DLT_INITIAL_LOG_LEVEL", "::6", true);
  DLT_REGISTER_APP("TST", "IasPluginsTest applications");
  DLT_REGISTER_CONTEXT(tstCtx, "TST", "IasPluginsTest context");
  DLT_ENABLE_LOCAL_PRINT();

  ::testing::InitGoogleTest(&argc, argv);
  int result = RUN_ALL_TESTS();

  DLT_UNREGISTER_CONTEXT(tstCtx);
  DLT_UNREGISTER_APP();
  return result;
}

namespace IasAudio {

class  IasPluginsTest : public ::testing::Test
{
  protected:
    virtual void SetUp()
    {
      mCmdDispatcher = std::make_shared<IasCmdDispatcher>();
      ASSERT_TRUE(mCmdDispatcher != nullptr);
      mEngine = new IasPluginEngine(mCmdDispatcher);
      ASSERT_TRUE(mEngine != nullptr);
    }

    virtual void TearDown()
    {
      delete mEngine;
      mCmdDispatcher = nullptr;
    }

    IasPluginEngine         *mEngine;         //!< The plugin engine
    IasCmdDispatcherPtr      mCmdDispatcher;  //!< The cmd dispatcher
};

TEST_F(IasPluginsTest, no_plugins)
{
  setenv("AUDIO_PLUGIN_DIR", "/there_are_no_plugins", true);
  IasAudioProcessingResult res = mEngine->loadPluginLibraries();
  ASSERT_EQ(eIasAudioProcInvalidParam, res);
}

TEST_F(IasPluginsTest, test_plugins)
{
  setenv("AUDIO_PLUGIN_DIR", AUDIO_PLUGIN_DIR, true);
  IasAudioProcessingResult res = mEngine->loadPluginLibraries();
  ASSERT_EQ(eIasAudioProcOK, res);
  IasIGenericAudioCompConfig *config = nullptr;
  res = mEngine->createModuleConfig(nullptr);
  ASSERT_EQ(eIasAudioProcInvalidParam, res);
  res = mEngine->createModuleConfig(&config);
  ASSERT_EQ(eIasAudioProcOK, res);
  IasGenericAudioComp *careq = nullptr;
  IasGenericAudioComp *usereq = nullptr;
  IasGenericAudioComp *simplevolume = nullptr;
  IasGenericAudioComp *downmixer = nullptr;
  mEngine->destroyModule(careq);
  res = mEngine->createModule(nullptr, "undefined_type", "not_used", nullptr);
  ASSERT_EQ(eIasAudioProcInvalidParam, res);
  ASSERT_TRUE(careq == nullptr);
  res = mEngine->createModule(nullptr, "undefined_type", "not_used",  &careq);
  ASSERT_EQ(eIasAudioProcInvalidParam, res);
  ASSERT_TRUE(careq == nullptr);
  res = mEngine->createModule(nullptr, "careq", "MyCarEq", &careq);
  ASSERT_EQ(eIasAudioProcInvalidParam, res); // must fail due to invalid config
  ASSERT_TRUE(careq == nullptr);
  res = mEngine->createModule(config, "careq", "MyCarEq", &careq);
  ASSERT_EQ(eIasAudioProcOK, res);
  ASSERT_TRUE(careq != nullptr);
  ASSERT_TRUE(careq->getTypeName().compare("careq") == 0);
  ASSERT_TRUE(careq->getCmdInterface() != nullptr);
  ASSERT_TRUE(careq->getCore() != nullptr);
  res = mEngine->createModule(config, "usereq", "MyUserEq", &usereq);
  ASSERT_EQ(eIasAudioProcOK, res);
  ASSERT_TRUE(usereq != nullptr);
  ASSERT_TRUE(usereq->getTypeName().compare("usereq") == 0);
  ASSERT_TRUE(usereq->getCmdInterface() != nullptr);
  ASSERT_TRUE(usereq->getCore() != nullptr);
  res = mEngine->createModule(config, "simplevolume", "MySimpleVolume", &simplevolume);
  ASSERT_EQ(eIasAudioProcOK, res);
  ASSERT_TRUE(simplevolume != nullptr);
  ASSERT_TRUE(simplevolume->getTypeName().compare("simplevolume") == 0);
  ASSERT_TRUE(simplevolume->getCmdInterface() != nullptr);
  ASSERT_TRUE(simplevolume->getCore() != nullptr);
  res = mEngine->createModule(config, "downmixer", "MyDownMixer", &downmixer);
  ASSERT_EQ(eIasAudioProcOK, res);
  ASSERT_TRUE(downmixer != nullptr);
  ASSERT_TRUE(downmixer->getTypeName().compare("downmixer") == 0);
  ASSERT_TRUE(downmixer->getCmdInterface() != nullptr);
  ASSERT_TRUE(downmixer->getCore() != nullptr);

  // We have to fake the init of the cmd interface here, because this was moved to the IasAudioChain
  IasIModuleId::IasResult modres;
  modres = simplevolume->getCmdInterface()->init();
  ASSERT_EQ(IasIModuleId::eIasOk, modres);

  IasProperties cmdProperties;
  IasProperties returnProperties;
  IasICmdRegistration::IasResult cmdres;
  cmdres = mCmdDispatcher->dispatchCmd("does_not_exist", cmdProperties, returnProperties);
  ASSERT_EQ(IasICmdRegistration::eIasFailed, cmdres);
  cmdres = mCmdDispatcher->dispatchCmd("MySimpleVolume", cmdProperties, returnProperties);
  ASSERT_EQ(IasICmdRegistration::eIasFailed, cmdres);
  int32_t cmdId = -1;
  cmdProperties.set("cmd", cmdId);
  cmdres = mCmdDispatcher->dispatchCmd("MySimpleVolume", cmdProperties, returnProperties);
  ASSERT_EQ(IasICmdRegistration::eIasFailed, cmdres);
  cmdId = IasTestCompCmd::eIasGetVolume;
  cmdProperties.set("cmd", cmdId);
  cmdres = mCmdDispatcher->dispatchCmd("MySimpleVolume", cmdProperties, returnProperties);
  ASSERT_EQ(IasICmdRegistration::eIasOk, cmdres);
  ASSERT_TRUE(returnProperties.hasProperties());
  float currentVolume = 0.0f;
  IasProperties::IasResult propres;
  propres = returnProperties.get("volume", &currentVolume);
  ASSERT_EQ(IasProperties::eIasOk, propres);
  ASSERT_EQ(1.0f, currentVolume);
  float newVolume = 5.0f;
  cmdId = IasTestCompCmd::eIasSetVolume;
  cmdProperties.set("cmd", cmdId);
  cmdProperties.set("volume", newVolume);
  cmdres = mCmdDispatcher->dispatchCmd("MySimpleVolume", cmdProperties, returnProperties);
  ASSERT_EQ(IasICmdRegistration::eIasOk, cmdres);
  propres = returnProperties.get("volume", &currentVolume);
  ASSERT_EQ(IasProperties::eIasOk, propres);
  ASSERT_EQ(5.0f, currentVolume);

  // Just to cover the function registerModuleIdInterface. Might be removed in the future.
  cmdres = mCmdDispatcher->registerModuleIdInterface(0, 0);
  ASSERT_EQ(IasICmdRegistration::eIasFailed, cmdres);

  // Just to cover the function propertiesEventCallback. Might be removed in the future.
  IasProperties properties;
  cmdres = mCmdDispatcher->propertiesEventCallback(0, returnProperties);
  ASSERT_EQ(IasICmdRegistration::eIasFailed, cmdres);
  (void)returnProperties;

  mEngine->destroyModule(careq);
  mEngine->destroyModule(usereq);
  mEngine->destroyModule(simplevolume);
  mEngine->destroyModule(downmixer);
}


// Test of the IasPluginLibrary::registerFactoryMethods method.
TEST_F(IasPluginsTest, test_pluginLibraryUnregisterFactoryMethods)
{
  IasPluginLibrary* myPluginLibrary = new IasPluginLibrary("myPluginLibrary");
  myPluginLibrary->unregisterFactoryMethods("myModule");
  delete myPluginLibrary;
}


} // namespace IasAudio
