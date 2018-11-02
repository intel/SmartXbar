/*
 * IasSmartX_API_Mutex_Test.cpp
 *
 *  Created on: Nov 7, 2017
 *      Author: kduchnox
 */

#include <thread>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <atomic>

#include "gtest/gtest.h"
#include "avbaudiomodules/internal/audio/common/IasAudioLogging.hpp"
#include "avbaudiomodules/internal/audio/common/alsa_smartx_plugin/IasAlsaPluginIpc.hpp"

#include "smartx/IasAudioTypedefs.hpp"
#include "smartx/IasSmartXClient.hpp"
#include "smartx/IasSmartXPriv.hpp"

#include "model/IasPipeline.hpp"
#include "model/IasAudioPin.hpp"
#include "model/IasAudioPort.hpp"
#include "model/IasRoutingZone.hpp"
#include "model/IasAudioSourceDevice.hpp"
#include "model/IasAudioSinkDevice.hpp"

#include "volume/IasVolumeLoudnessCore.hpp"
#include "switchmatrix/IasSwitchMatrix.hpp"
#include "audio/volumex/IasVolumeCmd.hpp"

#include "audio/smartx/IasProperties.hpp"
#include "audio/smartx/IasSmartX.hpp"
#include "audio/smartx/IasISetup.hpp"
#include "audio/smartx/IasIDebug.hpp"
#include "audio/smartx/IasIProcessing.hpp"
#include "audio/smartx/IasIRouting.hpp"
#include "audio/smartx/IasSetupHelper.hpp"
#include "audio/smartx/IasConnectionEvent.hpp"
#include "audio/smartx/IasSetupEvent.hpp"
#include "audio/smartx/IasEventProvider.hpp"

#include "smartx/IasConfigFile.hpp"
#include "smartx/IasDebugMutexDecorator.hpp"
#include "smartx/IasSetupMutexDecorator.hpp"
#include "smartx/IasProcessingMutexDecorator.hpp"
#include "smartx/IasRoutingMutexDecorator.hpp"

#include <boost/filesystem.hpp>

#ifndef SMARTX_CONFIG_DIR
#define SMARTX_CONFIG_DIR "./"
#endif

#ifndef AUDIO_PLUGIN_DIR
#define AUDIO_PLUGIN_DIR "./"
#endif

#ifndef ALSA_DEVICE_NAME
#define ALSA_DEVICE_NAME "hw:31,0"
#endif

#ifndef PLUGHW_DEVICE_NAME
#define PLUGHW_DEVICE_NAME "plughw:31,0"
#endif

using namespace std;
using namespace IasAudio;
namespace fs = boost::filesystem;

int main(int argc, char* argv[])
{
  setenv("DLT_INITIAL_LOG_LEVEL", "::6", true);

  DLT_REGISTER_APP("TST", "Test Application");
  DLT_VERBOSE_MODE();
  DLT_ENABLE_LOCAL_PRINT();

  ::testing::InitGoogleTest(&argc, argv);
  int result = RUN_ALL_TESTS();
  DLT_UNREGISTER_APP();
  return result;
}

class IasSmartX_API_Mutex_Test : public ::testing::Test
{
  public:
    void worker_thread();

  protected:
    virtual void SetUp();
    virtual void TearDown();

    mutex               mMutex;
    condition_variable  mCondition;
    std::uint32_t       mNrEvents;
    IasEventQueue       mEventQueue;
    bool                mThreadIsRunning;

    std::uint32_t       commonPeriodSize;
    std::uint32_t       commonPeriodNum;
    std::uint32_t       commonSamplerate;
    std::uint32_t       commonChannelNum;

    static void listings(IasISetup *setup);
};

void IasSmartX_API_Mutex_Test::SetUp()
{
  mNrEvents = 0;
  mThreadIsRunning = true;
  setenv("AUDIO_PLUGIN_DIR", AUDIO_PLUGIN_DIR, true);

  commonPeriodSize = 196;
  commonPeriodNum = 4;
  commonSamplerate = 48000;
  commonChannelNum = 1;
}

void IasSmartX_API_Mutex_Test::TearDown()
{
}

void IasSmartX_API_Mutex_Test::listings(IasISetup *setup)
{
  IasSetupMutexDecorator::IasResult setres;

  IasProcessingModulePtr testProcessingModule = nullptr;
  IasProcessingModuleParams testProcessingModuleParams =
  {
   "ias.volume",
   "simplevolume"
  };
  setres = setup->createProcessingModule(testProcessingModuleParams, &testProcessingModule);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  setup->getAudioPorts();
  setup->getAudioInputPorts();
  setup->getAudioOutputPorts();
  setup->getAudioPins();
  setup->getAudioSinkDevices();
  setup->getAudioSourceDevices();
  setup->getRoutingZones();
  setup->getSourceGroups();
  setup->getAudioSinkDevice("test_sink");
  setup->getAudioSourceDevice("test_source");
  setup->getRoutingZone("test_rz");
  setup->getProperties(testProcessingModule);
  setup->getPipeline("testPipeline");
}

TEST_F(IasSmartX_API_Mutex_Test, IasSetupMutexDecoratorTest)
{
  IasISetup *setup = new IasSetupMutexDecorator(nullptr);
  delete setup;
}

TEST_F(IasSmartX_API_Mutex_Test, IasRoutingMutexDecoratorTest)
{
  IasIRouting *routing = new IasRoutingMutexDecorator(nullptr);
  delete routing;
}

TEST_F(IasSmartX_API_Mutex_Test, IasProcessingMutexDecoratorTest)
{
  IasIProcessing *processing = new IasProcessingMutexDecorator(nullptr);
  delete processing;
}

TEST_F(IasSmartX_API_Mutex_Test, IasDebugMutexDecoratorTest)
{
  IasIDebug *debug = new IasDebugMutexDecorator(nullptr);
  delete debug;
}

static void debugTest(IasIRouting *routing,
               IasIDebug *debug,
               IasAudioSinkDevicePtr sink,
               IasAudioSourceDevicePtr source,
               int index)
{
  std::string sinkFile = "sinkTest" + to_string(index);
  std::string sourceFile = "sourceTest" + to_string(index);
  std::uint32_t numSeconds = 1;

  IasDebugMutexDecorator::IasResult dbgRes;
  IasIRouting::IasResult rtRes;

  std::string sourcePortName = source->getName() + "_port";

  std::string sinkPortName = sink->getName() + "_port";
  std::string sinkRzPortName = sink->getName() + "_rznport";

  dbgRes = debug->startRecord(sourceFile, "dummy", numSeconds);
  EXPECT_EQ(dbgRes, IasIDebug::eIasFailed);

  dbgRes = debug->startRecord("/dummyFile", sourcePortName, numSeconds);
  EXPECT_EQ(dbgRes, IasIDebug::eIasFailed);

  dbgRes = debug->stopProbing("non_existing");
  EXPECT_EQ(dbgRes, IasIDebug::eIasFailed);

  dbgRes = debug->startRecord(sourceFile, sourcePortName, numSeconds);
  EXPECT_EQ(dbgRes, IasIDebug::eIasOk);
  dbgRes = debug->stopProbing(sourcePortName);
  EXPECT_EQ(dbgRes, IasIDebug::eIasOk);

  dbgRes = debug->startRecord(sinkFile, sinkPortName, numSeconds);
  EXPECT_EQ(dbgRes, IasIDebug::eIasOk);
  dbgRes = debug->stopProbing(sinkPortName);
  EXPECT_EQ(dbgRes, IasIDebug::eIasOk);

  dbgRes = debug->startRecord(sinkFile, sinkRzPortName, numSeconds);
  EXPECT_EQ(dbgRes, IasIDebug::eIasOk);
  dbgRes = debug->stopProbing(sinkRzPortName);
  EXPECT_EQ(dbgRes, IasIDebug::eIasOk);

  dbgRes = debug->startInject(sinkFile, sinkPortName, numSeconds);
  EXPECT_EQ(dbgRes, IasIDebug::eIasOk);
  dbgRes = debug->stopProbing(sinkPortName);
  EXPECT_EQ(dbgRes, IasIDebug::eIasOk);

  dbgRes = debug->startInject(sinkFile, sinkRzPortName, numSeconds);
  EXPECT_EQ(dbgRes, IasIDebug::eIasOk);
  dbgRes = debug->stopProbing(sinkRzPortName);
  EXPECT_EQ(dbgRes, IasIDebug::eIasOk);

  rtRes = routing->disconnect(42 + index, 42 + index);
  EXPECT_EQ(rtRes, IasIRouting::eIasOk);
}

TEST_F(IasSmartX_API_Mutex_Test, IasDebugMutexDecoratorMethodsTest)
{
  IasSmartX *smartx = IasSmartX::create();
  IasIDebug *debug = new IasDebugMutexDecorator(smartx->debug());
  IasISetup *setup = new IasSetupMutexDecorator(smartx->setup());
  IasIRouting *routing = new IasRoutingMutexDecorator(smartx->routing());

  const int numOfThreads = 10;
  std::vector<std::thread> threads;

  IasRoutingZonePtr baseRoutingZone = nullptr;
  IasAudioSinkDevicePtr baseSink = nullptr;

  IasISetup::IasResult stpRes;
  IasIRouting::IasResult rtRes;
  IasRoutingZone::IasResult rznRes;

  IasAudioDeviceParams baseSinkParams(ALSA_DEVICE_NAME,2,48000,eIasFormatInt16,eIasClockReceived,128,4);

  stpRes = IasSetupHelper::createAudioSinkDevice(setup, baseSinkParams, 20, &baseSink, &baseRoutingZone);
  EXPECT_EQ(IasISetup::eIasOk, stpRes);

  vector<IasAudioSinkDevicePtr> sinks(numOfThreads);
  vector<IasAudioSourceDevicePtr> sources(numOfThreads);
  vector<IasRoutingZonePtr> routingZones(numOfThreads);

  IasAudioDeviceParams sinkParams("tmp", 2, 48000, eIasFormatInt16, eIasClockReceived, 128, 4);
  IasAudioDeviceParams sourceParams("tmp",2,48000,eIasFormatInt16,eIasClockProvided,128,4);

  for(int i = 0; i < numOfThreads; i++)
  {
    sinkParams.name = "MySink" + to_string(i);
    sourceParams.name = "MySource" + to_string(i);

    stpRes = IasSetupHelper::createAudioSinkDevice(setup, sinkParams, 42 + i, &sinks[i], &routingZones[i]);
    EXPECT_EQ(IasISetup::eIasOk, stpRes);

    stpRes = IasSetupHelper::createAudioSourceDevice(setup, sourceParams, 42 + i, &sources[i]);
    EXPECT_EQ(IasISetup::eIasOk, stpRes);

    stpRes = setup->addDerivedZone(baseRoutingZone, routingZones[i]);
    EXPECT_EQ(IasISetup::eIasOk, stpRes);

    rtRes = routing->connect(42 + i, 42 + i);
    EXPECT_EQ(rtRes, IasIRouting::eIasOk);
  }

  rznRes = baseRoutingZone->start();
  EXPECT_EQ(IasRoutingZone::eIasOk, rznRes);

  usleep(3000); //let the routing zone thread run to trigger switch matrix
  IasSwitchMatrixPtr switchMatrix = baseRoutingZone->getSwitchMatrix();

  // run threads
  for(int i = 0; i < numOfThreads; i++)
  {
    threads.emplace_back(debugTest, routing, debug, sinks[i], sources[i], i);
  }

  for(auto& t : threads)
  {
    t.join();
  }

  threads.clear();

  IasSmartX::destroy(smartx);
}

TEST_F(IasSmartX_API_Mutex_Test, SetupDecoratorListingsTest)
{
  IasSmartX *smartx = IasSmartX::create();
  IasISetup *setup = new IasSetupMutexDecorator(smartx->setup());

  const int numOfThreads = 20;
  vector<thread> threads;

  IasSetupMutexDecorator::IasResult setres;

  // Create sink device for getSinkDevice(..) method
  IasAudioSinkDevicePtr testSink = nullptr;
  IasAudioDeviceParams testSinkParams = {
                                         "test_sink",
                                         commonChannelNum,
                                         commonSamplerate,
                                         eIasFormatInt16,
                                         eIasClockReceived,
                                         commonPeriodSize,
                                         commonPeriodNum
  };
  setres = setup->createAudioSinkDevice(testSinkParams, &testSink);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  // Create source device for getSourceDevice(..) method
  IasAudioSourceDevicePtr testSource = nullptr;
  IasAudioDeviceParams testSourceParams = {
                                           "test_source",
                                           commonChannelNum,
                                           commonSamplerate,
                                           eIasFormatInt16,
                                           eIasClockProvided,
                                           commonPeriodSize,
                                           commonPeriodNum
  };
  setres = setup->createAudioSourceDevice(testSourceParams, &testSource);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  // Create routing zone for getRoutingZone(..) method
  IasRoutingZonePtr testRZone = nullptr;
  IasRoutingZoneParams testRZoneParams = {
                                          "test_rz"
  };
  setres = setup->createRoutingZone(testRZoneParams, &testRZone);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  // Create pipeline for getPipeline(..) method
  IasPipelinePtr testPipeline = nullptr;
  IasPipelineParams testPipelineParams =
  {
   "testPipeline",
   commonSamplerate,
   commonPeriodSize
  };
  setres = setup->createPipeline(testPipelineParams, &testPipeline);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  // run threads
  for(int i=0; i<numOfThreads; i++)
  {
    threads.push_back(thread(listings, setup));
  }

  for(auto& t : threads)
  {
    t.join();
  }

  threads.clear();

  IasSmartX::destroy(smartx);
}

static void test(IasISetup *setup, IasIRouting *routing, int index)
{
  IasSetupMutexDecorator::IasResult setres;
  IasRoutingMutexDecorator::IasResult route_res;

  IasPipelinePtr testPipeline = nullptr;
  IasPipelineParams testPipelineParams =
  {
   "testPipeline" + to_string(index),
   48000,
   192
  };
  setres = setup->createPipeline(testPipelineParams, &testPipeline);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  IasProcessingModulePtr testProcessingModule = nullptr;
  IasProcessingModuleParams testProcessingModuleParams;
  testProcessingModuleParams.typeName     = "ias.volume";
  testProcessingModuleParams.instanceName = "VolumeLoudness" + to_string(index);
  setres = setup->createProcessingModule(testProcessingModuleParams, &testProcessingModule);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  IasProperties props;
  setup->setProperties(testProcessingModule, props);

  IasProperties volumeProperties;
  volumeProperties.set("numFilterBands", 1);
  IasStringVector activePins;
  activePins.push_back("testProcessingModuleInPin" + to_string(index));
  volumeProperties.set("activePinsForBand.0", activePins);
  setup->setProperties(testProcessingModule,volumeProperties);

  setres = setup->addProcessingModule(testPipeline, testProcessingModule);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  IasAudioPinPtr testPipelineInputPin = nullptr;
  IasAudioPinParams testPipelineInputPinParams =
  {
   "testPipelineInputPin" + to_string(index),
   1
  };
  setres = setup->createAudioPin(testPipelineInputPinParams, &testPipelineInputPin);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  IasAudioPinPtr testPipelineOutputPin = nullptr;
  IasAudioPinParams testPipelineOutputPinParams =
  {
   "testPipelineOutputPin" + to_string(index),
   1
  };
  setres = setup->createAudioPin(testPipelineOutputPinParams, &testPipelineOutputPin);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  setres = setup->addAudioInputPin(testPipeline, testPipelineInputPin);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setres = setup->addAudioOutputPin(testPipeline, testPipelineOutputPin);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  IasAudioPinPtr testProcessingModuleInPin = nullptr;
  IasAudioPinParams testProcessingModuleInPinParams =
  {
   "testProcessingModuleInPin" + to_string(index),
   1
  };
  setres = setup->createAudioPin(testProcessingModuleInPinParams, &testProcessingModuleInPin);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  IasAudioPinPtr testProcessingModuleOutPin = nullptr;
  IasAudioPinParams testProcessingModuleOutPinParams =
  {
   "testProcessingModuleOutPin" + to_string(index),
   1
  };
  setres = setup->createAudioPin(testProcessingModuleOutPinParams, &testProcessingModuleOutPin);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  setres = setup->addAudioPinMapping(testProcessingModule, testProcessingModuleInPin, testProcessingModuleOutPin);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  setup->deleteAudioPinMapping(testProcessingModule, testProcessingModuleInPin, testProcessingModuleOutPin);

  setres = setup->addAudioInOutPin(testProcessingModule, testProcessingModuleInPin);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  setres = setup->addAudioInOutPin(testProcessingModule, testProcessingModuleOutPin);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  setres = setup->link(testProcessingModuleInPin, testProcessingModuleOutPin, eIasAudioPinLinkTypeImmediate);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  setres = setup->link(testPipelineInputPin, testProcessingModuleInPin, eIasAudioPinLinkTypeImmediate);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  setres = setup->link(testProcessingModuleOutPin, testPipelineOutputPin, eIasAudioPinLinkTypeImmediate);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  IasAudioSinkDevicePtr testSink = nullptr;
  IasAudioDeviceParams testSinkParams = {
                                         "test_sink" + to_string(index),
                                         1,
                                         48000,
                                         eIasFormatInt16,
                                         eIasClockReceived,
                                         192,
                                         4
  };
  setres = setup->createAudioSinkDevice(testSinkParams, &testSink);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  IasAudioPortPtr testSinkPort = nullptr;
  IasAudioPortParams testSinkPortParams = {
                                           "test_sink_port" + to_string(index),
                                           1,
                                           -1,
                                           eIasPortDirectionInput,
                                           0
  };
  setres = setup->createAudioPort(testSinkPortParams, &testSinkPort);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  setres = setup->addAudioInputPort(testSink, testSinkPort);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  IasAudioSourceDevicePtr testSource = nullptr;
  IasAudioDeviceParams testSourceParams = {
                                           "test_source" + to_string(index),
                                           1,
                                           48000,
                                           eIasFormatInt16,
                                           eIasClockProvided,
                                           192,
                                           4
  };
  setres = setup->createAudioSourceDevice(testSourceParams, &testSource);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  IasAudioPortPtr testSourcePort = nullptr;
  IasAudioPortParams testSourcePortParams = {
                                             "test_sink_source" + to_string(index),
                                             1,
                                             index,
                                             eIasPortDirectionOutput,
                                             0
  };
  setres = setup->createAudioPort(testSourcePortParams, &testSourcePort);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  setres = setup->addAudioOutputPort(testSource, testSourcePort);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  setres = setup->startAudioSourceDevice(testSource);
  ASSERT_TRUE(IasISetup::eIasOk == setres);

  IasRoutingZonePtr testRZone = nullptr;
  IasRoutingZoneParams testRZoneParams = {
                                          "test_rz" + to_string(index)
  };
  setres = setup->createRoutingZone(testRZoneParams, &testRZone);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  setres = setup->link(testRZone, testSink);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  IasAudioPortPtr testRZoneInput = nullptr;
  IasAudioPortParams testRZoneInputParams = {
                                             "testRZoneInput" + to_string(index),
                                             1,
                                             index,
                                             eIasPortDirectionInput,
                                             0
  };
  setres = setup->createAudioPort(testRZoneInputParams, &testRZoneInput);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  setres = setup->addAudioInputPort(testRZone, testRZoneInput);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  setres = setup->startRoutingZone(testRZone);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  setup->stopRoutingZone(testRZone);

  IasRoutingZonePtr testDerRZone = nullptr;
  IasRoutingZoneParams testDerRZoneParams = {
                                             "test_der_rz" + to_string(index)
  };
  setres = setup->createRoutingZone(testDerRZoneParams, &testDerRZone);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  IasAudioSinkDevicePtr testDerSink = nullptr;
  IasAudioDeviceParams testDerSinkParams = {
                                            "test_der_sink" + to_string(index),
                                            1,
                                            48000,
                                            eIasFormatInt16,
                                            eIasClockReceived,
                                            192,
                                            4
  };
  setres = setup->createAudioSinkDevice(testDerSinkParams, &testDerSink);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  IasAudioPortPtr testDerSinkPort = nullptr;
  IasAudioPortParams testDerSinkPortParams = {
                                              "test_der_sink_port" + to_string(index),
                                              1,
                                              -1,
                                              eIasPortDirectionInput,
                                              0
  };
  setres = setup->createAudioPort(testDerSinkPortParams, &testDerSinkPort);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  setres = setup->addAudioInputPort(testDerSink, testDerSinkPort);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  setres = setup->link(testDerRZone, testDerSink);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  setres = setup->addDerivedZone(testRZone, testDerRZone);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  setres = setup->addPipeline(testRZone, testPipeline);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  setres = setup->link(testRZoneInput, testPipelineInputPin);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setres = setup->link(testRZoneInput, testSinkPort);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  setup->unlink(testRZoneInput, testPipelineInputPin);
  setup->unlink(testRZoneInput, testSinkPort);

  setres = setup->initPipelineAudioChain(testPipeline);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  route_res = routing->connect(index, index);
  ASSERT_EQ(IasIRouting::eIasOk, route_res);
  routing->getActiveConnections();
  routing->disconnect(index, index);
  IasDummySourcesSet sourcesSet = routing->getDummySources();

  setup->stopAudioSourceDevice(testSource);

  setup->unlink(testProcessingModuleOutPin, testPipelineOutputPin);
  setup->unlink(testRZone, testSink);
  setup->unlink(testDerRZone, testDerSink);

  setup->addSourceGroup("testSourceGroup", index);

  setup->deleteAudioInputPort(testRZone, testRZoneInput);
  setup->deleteDerivedZone(testRZone, testDerRZone);
  setup->deleteAudioInputPort(testSink, testSinkPort);
  setup->deleteAudioOutputPort(testSource, testSourcePort);
  setup->deleteAudioInOutPin(testProcessingModule, testProcessingModuleInPin);
  setup->deleteAudioInOutPin(testProcessingModule, testProcessingModuleOutPin);
  setup->deleteAudioInputPin(testPipeline, testPipelineInputPin);
  setup->deleteAudioOutputPin(testPipeline, testPipelineOutputPin);
  setup->deletePipeline(testRZone);
  setup->deleteProcessingModule(testPipeline, testProcessingModule);

  setup->destroyAudioPort(testRZoneInput);
  setup->destroyRoutingZone(testRZone);
  setup->destroyRoutingZone(testDerRZone);
  setup->destroyAudioPort(testSinkPort);
  setup->destroyAudioPort(testSourcePort);
  setup->destroyAudioSinkDevice(testSink);
  setup->destroyAudioSourceDevice(testSource);
  setup->destroyAudioPin(&testProcessingModuleInPin);
  setup->destroyAudioPin(&testProcessingModuleOutPin);
  setup->destroyAudioPin(&testPipelineInputPin);
  setup->destroyAudioPin(&testPipelineOutputPin);
  setup->destroyProcessingModule(&testProcessingModule);
  setup->destroyPipeline(&testPipeline);
}

TEST_F(IasSmartX_API_Mutex_Test, API_Test_MultiThread)
{
  const int numOfThreads = 10;
  vector<thread> threads;

  for(int thNumber = 0; thNumber < numOfThreads; thNumber++)
  {
    int temp = 0;

    IasSmartX *smartx = IasSmartX::create();

    IasISetup *setup = new IasSetupMutexDecorator(smartx->setup());
    IasIRouting *routing = new IasRoutingMutexDecorator(smartx->routing());

    for(int i = 0; i < thNumber; i++)
    {
      threads.emplace_back(test, setup, routing, i);
      temp++;
    }

    for(auto& t : threads)
    {
      t.join();
    }

    threads.clear();

    IasSmartX::destroy(smartx);
  }
}

static void processingSendCmd(IasIProcessing *processing, IasProperties volumeProperties)
{
  volumeProperties.set<std::string>("pin","Volume_InOutMonoPin");
  volumeProperties.set("volume",0);
  IasAudio::IasInt32Vector ramp;
  ramp.push_back(100);
  ramp.push_back(0);
  volumeProperties.set("ramp",ramp);

  IasProperties returnProperties;

  const auto res = processing->sendCmd("VolumeLoudness", volumeProperties, returnProperties);

  ASSERT_EQ(IasISetup::eIasOk, res);
}

TEST_F(IasSmartX_API_Mutex_Test, IasProcessingMutexDecoratorSendCmd)
{
  auto smartx = IasAudio::IasSmartX::create();
  if (smartx == nullptr)
  {
    EXPECT_TRUE(false) << "Create smartx error\n";
  }
  if (smartx->isAtLeast(SMARTX_API_MAJOR, SMARTX_API_MINOR, SMARTX_API_PATCH) == false)
  {
    std::cerr << "SmartX API version does not match" << std::endl;
    IasAudio::IasSmartX::destroy(smartx);
    EXPECT_TRUE(false);
  }

  auto setup = smartx->setup();

  IasRoutingZonePtr rzn = nullptr;

  // create sink device
  IasAudioDeviceParams sinkParam;
  sinkParam.clockType = IasAudio::eIasClockProvided;
  sinkParam.dataFormat = IasAudio::eIasFormatInt16;
  sinkParam.name = "mono";
  sinkParam.numChannels = 1;
  sinkParam.numPeriods = 4;
  sinkParam.periodSize = 192;
  sinkParam.samplerate = 48000;
  IasAudioSinkDevicePtr monoSink;
  auto setupRes = IasSetupHelper::createAudioSinkDevice(setup, sinkParam, 0, &monoSink,&rzn);
  EXPECT_EQ(setupRes, IasISetup::eIasOk) << "Error creating " << sinkParam.name << ": " << toString(setupRes) << "\n";

  // Create the pipeline
  IasPipelineParams pipelineParams;
  pipelineParams.name ="ExamplePipeline";
  pipelineParams.samplerate = sinkParam.samplerate;
  pipelineParams.periodSize = sinkParam.periodSize;

  IasPipelinePtr pipeline = nullptr;
  IasISetup::IasResult res = setup->createPipeline(pipelineParams, &pipeline);
  EXPECT_EQ(res, IasISetup::eIasOk) << "Error creating example pipeline (please review DLT output for details)\n";

  // create pin
  IasAudioPinPtr pipelineInputMonoPin = nullptr;
  IasAudioPinPtr monoPin = nullptr;
  IasAudioPinParams pipelineInputMonoParams;
  pipelineInputMonoParams.name = "pipeInputMono";
  pipelineInputMonoParams.numChannels = 1;

  res = setup->createAudioPin(pipelineInputMonoParams, &pipelineInputMonoPin);
  EXPECT_EQ(res, IasISetup::eIasOk) << "Error creating pipeline input mono pin\n";

  /// create volume module
  IasProcessingModuleParams volumeModuleParams;
  volumeModuleParams.typeName     = "ias.volume";
  volumeModuleParams.instanceName = "VolumeLoudness";
  IasProcessingModulePtr volume = nullptr;

  IasAudioPinPtr volumeMonoPin = nullptr;
  IasAudioPinParams volumeMonoPinParams;
  volumeMonoPinParams.name = "Volume_InOutMonoPin";
  volumeMonoPinParams.numChannels = 1;

  res = setup->createProcessingModule(volumeModuleParams, &volume);
  EXPECT_EQ(res, IasISetup::eIasOk) << "Error creating volume module\n";

  //// create volume loundness
  IasProperties volumeProperties;
  volumeProperties.set<std::int32_t>("numFilterBands",1);
  IasStringVector activePins;
  activePins.push_back("Volume_InOutMonoPin");
  volumeProperties.set("activePinsForBand.0", activePins);
  std::int32_t tempVol = 0;
  std::int32_t tempGain = 0;
  IasInt32Vector ldGains;
  IasInt32Vector ldVolumes;
  for(std::uint32_t i=0; i< 8; i++)
  {
    ldVolumes.push_back(tempVol);
    ldGains.push_back(tempGain);
    tempVol-= 60;
    tempGain+= 30;
  }
  volumeProperties.set("ld.volumes.0", ldVolumes);
  volumeProperties.set("ld.gains.0", ldGains);
  volumeProperties.set("ld.volumes.1", ldVolumes);
  volumeProperties.set("ld.gains.1", ldGains);
  volumeProperties.set("ld.volumes.2", ldVolumes);
  volumeProperties.set("ld.gains.2", ldGains);

  IasInt32Vector freqOrderType;
  IasFloat32Vector gainQual;
  freqOrderType.resize(3);
  gainQual.resize(2);
  freqOrderType[0] = 100;
  freqOrderType[1] = 2;
  freqOrderType[2] = eIasFilterTypeLowShelving;
  gainQual[0] = 1.0f; // gain
  gainQual[1] = 2.0f; // quality
  volumeProperties.set("ld.freq_order_type.0", freqOrderType);
  volumeProperties.set("ld.gain_quality.0", gainQual);
  freqOrderType[0] = 8000;
  freqOrderType[1] = 2;
  freqOrderType[2] = eIasFilterTypeHighShelving;
  gainQual[0] = 1.0f; // gain
  gainQual[1] = 2.0f; // quality
  volumeProperties.set("ld.freq_order_type.1", freqOrderType);
  volumeProperties.set("ld.gain_quality.1", gainQual);

  setup->setProperties(volume,volumeProperties);

  res = setup->addProcessingModule(pipeline,volume);
  EXPECT_EQ(res, IasISetup::eIasOk) << "Error adding volume module to pipeline\n";

  res = setup->createAudioPin(volumeMonoPinParams,&volumeMonoPin);
  EXPECT_EQ(res, IasISetup::eIasOk) << "Error creating volume mono pin\n";

  res = setup->addAudioInOutPin(volume, volumeMonoPin);
  EXPECT_EQ(res, IasISetup::eIasOk) << "Error adding volume mono pin\n";

  res = setup->addAudioInputPin(pipeline,pipelineInputMonoPin);
  EXPECT_EQ(res, IasISetup::eIasOk) << "Error adding pipeline input mono pin\n";

  res = setup->link(pipelineInputMonoPin, volumeMonoPin, eIasAudioPinLinkTypeImmediate);
  EXPECT_EQ(res, IasISetup::eIasOk) << "Error linking pipe input to mono volume mono input\n";

  res = setup->initPipelineAudioChain(pipeline);
  EXPECT_EQ(res, IasISetup::eIasOk) << "Error init pipeline chain - Please check DLT output for additional info\n";

  res = setup->addPipeline(rzn, pipeline);
  EXPECT_EQ(res, IasISetup::eIasOk) << "Error adding pipeline to zone\n";

  IasProperties volCmdProperties;
  volCmdProperties.set("cmd",static_cast<std::int32_t>(IasAudio::IasVolume::eIasSetVolume));

  volCmdProperties.set<std::string>("pin","Volume_InOutMonoPin");
  volCmdProperties.set("volume",0);
  IasAudio::IasInt32Vector ramp;
  ramp.push_back(100);
  ramp.push_back(0);
  volCmdProperties.set("ramp",ramp);

  std::vector<std::thread> sendCmdThreads;
  IasIProcessing *processing = new IasProcessingMutexDecorator(smartx->processing());

  for(int i=0; i<15; i++)
  {
    sendCmdThreads.emplace_back(processingSendCmd, processing, volCmdProperties);
  }

  for(auto& sendThread : sendCmdThreads)
  {
    sendThread.join();
  }

  const IasSmartX::IasResult smres = smartx->stop();
  EXPECT_EQ(IasSmartX::eIasOk, smres);
  IasSmartX::destroy(smartx);
  smartx = nullptr;
}

