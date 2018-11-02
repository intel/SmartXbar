/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/*
 * IasSmartX_API_Test.cpp
 *
 *  Created 2015
 */

#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <alsa/asoundlib.h>
#include <errno.h>
#include <boost/filesystem.hpp>
#include "IasSmartX_API_Test.hpp"
#include "audio/smartx/IasSmartX.hpp"
#include "audio/smartx/IasISetup.hpp"
#include "audio/smartx/IasIDebug.hpp"
#include "audio/smartx/IasIProcessing.hpp"
#include "audio/smartx/IasIRouting.hpp"
#include "audio/smartx/IasSetupHelper.hpp"
#include "audio/smartx/IasConnectionEvent.hpp"
#include "audio/smartx/IasSetupEvent.hpp"
#include "smartx/IasDebugMutexDecorator.hpp"
#include "IasCustomerEventHandler.hpp"
#include "IasAudioTypedefs.hpp"
#include "IasConfiguration.hpp"
#include "model/IasAudioPort.hpp"
#include "model/IasAudioSourceDevice.hpp"
#include "model/IasAudioSinkDevice.hpp"
#include "model/IasRoutingZone.hpp"
#include "model/IasRoutingZoneWorkerThread.hpp"
#include "model/IasAudioPin.hpp"
#include "model/IasProcessingModule.hpp"
#include "switchmatrix/IasSwitchMatrix.hpp"
#include "smartx/IasConfigFile.hpp"
#include "IasSmartXClient.hpp"
#include "IasSmartXPriv.hpp"
#include "avbaudiomodules/internal/audio/common/alsa_smartx_plugin/IasAlsaPluginIpc.hpp"
#include "audio/smartx/IasProperties.hpp"
#include "rtprocessingfwx/IasCmdDispatcher.hpp"
#include "smartx/IasProcessingImpl.hpp"
#include "smartx/IasDebugImpl.hpp"

#ifndef SMARTX_CONFIG_DIR
#define SMARTX_CONFIG_DIR "./"
#endif

using namespace std;
namespace fs = boost::filesystem;

#ifndef ALSA_DEVICE_NAME
#define ALSA_DEVICE_NAME  "hw:31,0"
#endif
#ifndef PLUGHW_DEVICE_NAME
#define PLUGHW_DEVICE_NAME  "plughw:31,0"
#endif

#ifndef AUDIO_PLUGIN_DIR
#define AUDIO_PLUGIN_DIR    "../../.."
#endif

extern "C" {

static bool setShallFail = false;
static bool getShallFail = false;
static bool setAffinityShallFail = false;
int pthread_setschedparam (
    pthread_t ,
    int ,
    const struct sched_param *)
{
  if (setShallFail == false)
  {
    cout << "pthread_setschedparam will succeed" << endl;
    return 0;
  }
  else
  {
    cout << "pthread_setschedparam will fail" << endl;
    return -1;
  }
}

int pthread_getschedparam (
    pthread_t ,
    int *__restrict ,
    struct sched_param *__restrict )
{
  if (getShallFail == false)
  {
    cout << "pthread_getschedparam will succeed" << endl;
    return 0;
  }
  else
  {
    cout << "pthread_getschedparam will fail" << endl;
    return -1;
  }
}

int pthread_setaffinity_np (
    pthread_t ,
    size_t ,
    const cpu_set_t *)
{
  if (setAffinityShallFail == false)
  {
    cout << "pthread_setaffinity_np will succeed" << endl;
    return 0;
  }
  else
  {
    cout << "pthread_setaffinity_np will fail" << endl;
    return -1;
  }
}


} // extern "C"

namespace IasAudio
{


void IasSmartX_API_Test::SetUpTestCase()
{
  system("exec rm -r /run/smartx/*");
  system("exec rm -r /dev/shm/smartx*");
}


void IasSmartX_API_Test::SetUp()
{
  mNrEvents = 0;
  mThreadIsRunning = true;
  IasConfigFile::getInstance()->setDefaultPath("./");
}

void IasSmartX_API_Test::TearDown()
{
}

TEST_F(IasSmartX_API_Test, checkVersion)
{
  int32_t major = IasSmartX::getMajor();
  ASSERT_EQ(SMARTX_API_MAJOR, major);
  int32_t minor = IasSmartX::getMinor();
  ASSERT_EQ(SMARTX_API_MINOR, minor);
  int32_t patch = IasSmartX::getPatch();
  ASSERT_EQ(SMARTX_API_PATCH, patch);
  std::string version = IasSmartX::getVersion();
  std::string refversion = std::to_string(SMARTX_API_MAJOR) + "." + std::to_string(SMARTX_API_MINOR) + "." + std::to_string(SMARTX_API_PATCH);
  ASSERT_STREQ(refversion.c_str(), version.c_str());
  ASSERT_FALSE(IasSmartX::hasFeature("Test"));
  ASSERT_TRUE(IasSmartX::isAtLeast(-1, SMARTX_API_MINOR, SMARTX_API_PATCH));
  ASSERT_FALSE(IasSmartX::isAtLeast(1000, SMARTX_API_MINOR, SMARTX_API_PATCH));
  ASSERT_TRUE(IasSmartX::isAtLeast(SMARTX_API_MAJOR, -1, SMARTX_API_PATCH));
  ASSERT_FALSE(IasSmartX::isAtLeast(SMARTX_API_MAJOR, 1000, SMARTX_API_PATCH));
  ASSERT_TRUE(IasSmartX::isAtLeast(SMARTX_API_MAJOR, SMARTX_API_MINOR, -1));
  ASSERT_FALSE(IasSmartX::isAtLeast(SMARTX_API_MAJOR, SMARTX_API_MINOR, 1000));
  ASSERT_TRUE(IasSmartX::isAtLeast(SMARTX_API_MAJOR, SMARTX_API_MINOR, SMARTX_API_PATCH));
}

TEST_F(IasSmartX_API_Test, create_destroy)
{
  IasSmartX::destroy(nullptr);
  IasSmartX *smartx = IasSmartX::create();
  ASSERT_TRUE(smartx != nullptr);
  IasSmartX *notAllowed = IasSmartX::create();
  ASSERT_TRUE(notAllowed == nullptr);
  IasSmartX::destroy(smartx);
}

TEST_F(IasSmartX_API_Test, setup_api)
{
  IasSmartX *smartx = IasSmartX::create();
  ASSERT_TRUE(smartx != nullptr);
  IasISetup *setup = smartx->setup();
  ASSERT_TRUE(setup != nullptr);
  IasISetup *setup_same = smartx->setup();
  ASSERT_EQ(setup, setup_same);
  IasIProcessing *processing = smartx->processing();
  ASSERT_TRUE(processing != nullptr);
  IasIProcessing *processing_same = smartx->processing();
  ASSERT_TRUE(processing_same != nullptr);
  ASSERT_EQ(processing, processing_same);
  IasIRouting *routing = smartx->routing();
  ASSERT_TRUE(routing != nullptr);
  IasIRouting *routing_same = smartx->routing();
  ASSERT_TRUE(routing_same != nullptr);
  ASSERT_EQ(routing, routing_same);
  IasIDebug *debug = smartx->debug();
  ASSERT_TRUE(debug != nullptr);
  IasIDebug *debug_same = smartx->debug();
  ASSERT_TRUE(debug_same != nullptr);
  ASSERT_EQ(debug, debug_same);
  IasSmartX::destroy(smartx);
}

/**
 * Negative test for the following methods of IasISetup:
 *
 * SinkDevice type = SmartXClient
 *
 * addAudioInputPort(sink,inputport)
 * deleteAudioInputPort
 * createAudioSinkDevice
 * destroyAudioSinkDevice
 * createAudioPort
 * destroyAudioPort
 */
TEST_F(IasSmartX_API_Test, setup_api_fail_smx_client)
{
  IasSmartX *smartx = IasSmartX::create();
  ASSERT_TRUE(smartx != nullptr);
  IasISetup *setup = smartx->setup();
  ASSERT_TRUE(setup != nullptr);
  IasAudioSinkDevicePtr sink = nullptr;
  IasAudioPortPtr port = nullptr;
  IasISetup::IasResult setres = setup->addAudioInputPort(sink, port);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  IasAudioDeviceParams devParams;
  setres = setup->createAudioSinkDevice(devParams, nullptr);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  devParams.clockType = eIasClockProvided;
  setres = setup->createAudioSinkDevice(devParams, nullptr);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  setres = setup->createAudioSinkDevice(devParams, &sink);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  devParams.samplerate = 48000;
  setres = setup->createAudioSinkDevice(devParams, &sink);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  devParams.name = "MySink";
  setres = setup->createAudioSinkDevice(devParams, &sink);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  devParams.dataFormat = eIasFormatInt16;
  setres = setup->createAudioSinkDevice(devParams, &sink);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  devParams.periodSize = 256;
  setres = setup->createAudioSinkDevice(devParams, &sink);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  devParams.numPeriods = 4;
  setres = setup->createAudioSinkDevice(devParams, &sink);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  devParams.numChannels = 2;
  setres = setup->createAudioSinkDevice(devParams, &sink);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setres = setup->createAudioSinkDevice(devParams, &sink);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  setres = setup->addAudioInputPort(sink, port);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  IasAudioPortParams portParams;
  setres = setup->createAudioPort(portParams, nullptr);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  setres = setup->createAudioPort(portParams, &port);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  portParams.numChannels = 2;
  setres = setup->createAudioPort(portParams, &port);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  portParams.direction = eIasPortDirectionOutput;
  setres = setup->createAudioPort(portParams, &port);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  portParams.index = 0;
  setres = setup->createAudioPort(portParams, &port);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setres = setup->addAudioInputPort(sink, port);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  setup->destroyAudioPort(nullptr);
  setup->destroyAudioPort(port);
  portParams.direction = eIasPortDirectionInput;
  setres = setup->createAudioPort(portParams, &port);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setres = setup->addAudioInputPort(sink, port);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  IasAudioSinkDevicePtr sink2 = nullptr;
  devParams.name = "MySink2";
  setres = setup->createAudioSinkDevice(devParams, &sink2);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  IasAudioPortPtr port2 = nullptr;
  setres = setup->createAudioPort(portParams, &port2);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setres = setup->addAudioInputPort(sink2, port2);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setup->deleteAudioInputPort(sink2, port);
  setup->deleteAudioInputPort(sink2, port2);
  IasAudioSinkDevicePtr tmpSink = nullptr;
  IasAudioPortPtr tmpPort = nullptr;
  setup->deleteAudioInputPort(tmpSink, tmpPort);
  setup->deleteAudioInputPort(sink, tmpPort);
  setup->deleteAudioInputPort(sink, port);
  setup->destroyAudioPort(port);
  portParams.id = 123;
  setres = setup->createAudioPort(portParams, &port);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setres = setup->addAudioInputPort(sink, port);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setres = setup->addAudioInputPort(sink, port);
  ASSERT_EQ(IasISetup::eIasFailed, setres);

  setup->destroyAudioSinkDevice(nullptr);
  setup->destroyAudioSinkDevice(sink);

  IasSmartX::destroy(smartx);
}

/**
 * Negative test for the following methods of IasISetup:
 *
 * SinkDevice type = ALSA handler
 *
 * addAudioInputPort(sink,inputport)
 * deleteAudioInputPort(sink,inputport)
 * createAudioSinkDevice
 * destroyAudioSinkDevice
 * createAudioPort
 * destroyAudioPort
 */
TEST_F(IasSmartX_API_Test, setup_api_fail_alsahandler)
{
  IasSmartX *smartx = IasSmartX::create();
  ASSERT_TRUE(smartx != nullptr);
  IasISetup *setup = smartx->setup();
  ASSERT_TRUE(setup != nullptr);
  IasAudioSinkDevicePtr sink = nullptr;
  IasAudioPortPtr port = nullptr;
  IasISetup::IasResult setres = setup->addAudioInputPort(sink, port);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  IasAudioDeviceParams devParams;
  setres = setup->createAudioSinkDevice(devParams, nullptr);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  devParams.clockType = eIasClockReceived;
  setres = setup->createAudioSinkDevice(devParams, nullptr);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  setres = setup->createAudioSinkDevice(devParams, &sink);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  devParams.samplerate = 48000;
  setres = setup->createAudioSinkDevice(devParams, &sink);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  devParams.name = "MySink";
  setres = setup->createAudioSinkDevice(devParams, &sink);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  devParams.dataFormat = eIasFormatInt16;
  setres = setup->createAudioSinkDevice(devParams, &sink);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  devParams.periodSize = 256;
  setres = setup->createAudioSinkDevice(devParams, &sink);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  devParams.numPeriods = 4;
  setres = setup->createAudioSinkDevice(devParams, &sink);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  devParams.numChannels = 2;
  setres = setup->createAudioSinkDevice(devParams, &sink);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setres = setup->createAudioSinkDevice(devParams, &sink);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  setres = setup->addAudioInputPort(sink, port);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  IasAudioPortParams portParams;
  setres = setup->createAudioPort(portParams, nullptr);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  setres = setup->createAudioPort(portParams, &port);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  portParams.numChannels = 2;
  setres = setup->createAudioPort(portParams, &port);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  portParams.direction = eIasPortDirectionOutput;
  setres = setup->createAudioPort(portParams, &port);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  portParams.index = 0;
  setres = setup->createAudioPort(portParams, &port);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setres = setup->addAudioInputPort(sink, port);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  setup->destroyAudioPort(nullptr);
  setup->destroyAudioPort(port);
  portParams.direction = eIasPortDirectionInput;
  setres = setup->createAudioPort(portParams, &port);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setres = setup->addAudioInputPort(sink, port);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  IasAudioSinkDevicePtr tmpSink = nullptr;
  IasAudioPortPtr tmpPort = nullptr;
  setup->deleteAudioInputPort(tmpSink, tmpPort);
  setup->deleteAudioInputPort(sink, tmpPort);
  setup->deleteAudioInputPort(sink, port);
  setup->destroyAudioSinkDevice(nullptr);
  setup->destroyAudioSinkDevice(sink);
  setup->destroyAudioPort(nullptr);
  setup->destroyAudioPort(tmpPort);
  setup->destroyAudioPort(port);

  IasSmartX::destroy(smartx);
}

/**
 * Negative test for the following methods of IasISetup:
 *
 * addAudioInputPort(routingzone,inputport)
 * deleteAudioInputPort(routingzone, inputport)
 * createRoutingZone
 * destroyRoutingZone
 * createAudioSinkDevice
 * destroyAudioSinkDevice
 * createAudioPort
 * destroyAudioPort
 * link(routingzone, sink)
 * unlink(routingzone, sink)
 * unlink(port, port)
 */
TEST_F(IasSmartX_API_Test, setup_api_fail_add_in_port_zone)
{
  IasSmartX *smartx = IasSmartX::create();
  ASSERT_TRUE(smartx != nullptr);
  IasISetup *setup = smartx->setup();
  ASSERT_TRUE(setup != nullptr);
  IasRoutingZonePtr rzn = nullptr;
  IasAudioPortPtr port = nullptr;
  IasISetup::IasResult setres = setup->addAudioInputPort(rzn, port);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  IasRoutingZoneParams rznParams;
  setres = setup->createRoutingZone(rznParams, nullptr);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  setres = setup->createRoutingZone(rznParams, &rzn);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  rznParams.name = "rzn";
  setres = setup->createRoutingZone(rznParams, &rzn);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setres = setup->addAudioInputPort(rzn, port);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  IasAudioPortParams portParams;
  portParams.numChannels = 2;
  portParams.direction = eIasPortDirectionOutput;
  portParams.index = 0;
  setres = setup->createAudioPort(portParams, &port);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setres = setup->addAudioInputPort(rzn, port);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  setup->destroyAudioPort(port);
  portParams.direction = eIasPortDirectionInput;
  setres = setup->createAudioPort(portParams, &port);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setres = setup->addAudioInputPort(rzn, port);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  IasAudioSinkDevicePtr sink = nullptr;
  IasAudioDeviceParams devParams;
  devParams.clockType = eIasClockProvided;
  devParams.name = "MySink";
  devParams.dataFormat = eIasFormatInt16;
  devParams.periodSize = 256;
  devParams.numPeriods = 4;
  devParams.numChannels = 2;
  devParams.samplerate = 48000;
  setres = setup->createAudioSinkDevice(devParams, &sink);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  IasRoutingZonePtr tmpRzn = nullptr;
  IasAudioSinkDevicePtr tmpSink = nullptr;
  setres = setup->link(tmpRzn, tmpSink);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  setres = setup->link(rzn, tmpSink);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  setres = setup->link(rzn, sink);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setres = setup->link(rzn, sink);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  setres = setup->addDerivedZone(tmpRzn, tmpRzn);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  setres = setup->addDerivedZone(rzn, tmpRzn);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  IasRoutingZonePtr rzn2 = nullptr;
  rznParams.name = "rzn2";
  setres = setup->createRoutingZone(rznParams, &rzn2);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setres = setup->addDerivedZone(rzn, rzn2);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  IasAudioSinkDevicePtr sink2 = nullptr;
  devParams.name = "MySink2";
  setres = setup->createAudioSinkDevice(devParams, &sink2);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setres = setup->link(rzn2, sink2);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setup->deleteDerivedZone(tmpRzn, tmpRzn);
  setup->deleteDerivedZone(rzn, tmpRzn);
  setup->deleteDerivedZone(rzn, rzn2);

  setres = setup->addAudioInputPort(rzn, port);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setres = setup->startRoutingZone(nullptr);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  setres = setup->startRoutingZone(rzn);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  setup->stopRoutingZone(nullptr);
  setup->stopRoutingZone(rzn);

  IasAudioPortPtr port2 = nullptr;
  setres = setup->createAudioPort(portParams, &port2);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setres = setup->addAudioInputPort(sink, port2);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setup->deleteAudioInputPort(rzn, port2);
  IasAudioPortPtr tmpPort = nullptr;
  setup->deleteAudioInputPort(tmpRzn, tmpPort);
  setup->deleteAudioInputPort(rzn, tmpPort);
  setup->deleteAudioInputPort(rzn, port);

  portParams.id = 123;
  setres = setup->createAudioPort(portParams, &port);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  IasAudioPortVector inports = setup->getAudioInputPorts();
  ASSERT_EQ(0u, inports.size());
  setres = setup->addAudioInputPort(rzn, port);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  inports = setup->getAudioInputPorts();
  ASSERT_EQ(1u, inports.size());
  ASSERT_EQ(123, inports[0]->getParameters()->id);
  ASSERT_EQ(2u, inports[0]->getParameters()->numChannels);
  ASSERT_EQ(0u, inports[0]->getParameters()->index);
  ASSERT_EQ(eIasPortDirectionInput, inports[0]->getParameters()->direction);
  setres = setup->addAudioInputPort(rzn, port);
  ASSERT_EQ(IasISetup::eIasFailed, setres);

  setup->unlink(tmpRzn, tmpSink);
  setup->unlink(rzn, tmpSink);
  setup->unlink(rzn, sink);
  IasAudioPortPtr dummy1 = nullptr;
  IasAudioPortPtr dummy2 = nullptr;
  // Create source port and add to source device
  IasAudioPortParams sourcePortParams =
  {
   "mySourcePort",
   2,
   123,
   eIasPortDirectionOutput,
   0
  };

  IasAudioPortPtr tmp = nullptr;
  setup->createAudioPort(sourcePortParams, &tmp);
  setup->unlink(tmp, dummy1);
  setup->unlink(dummy2, dummy1);

  setup->destroyRoutingZone(nullptr);
  setup->destroyRoutingZone(rzn);
  setup->destroyAudioPort(port);
  setup->destroyAudioSinkDevice(sink);

  IasSmartX::destroy(smartx);
}

/**
 * Negative test for the following methods of IasISetup:
 *
 * SourceDevice type = SmartXClient
 *
 * addAudioOutputPort(source,outputport)
 * createAudioSourceDevice
 * createAudioPort
 * destroyAudioPort
 * destroyAudioSourceDevice
 */
TEST_F(IasSmartX_API_Test, setup_api_fail_add_out_port_smx)
{
  IasSmartX *smartx = IasSmartX::create();
  ASSERT_TRUE(smartx != nullptr);
  IasISetup *setup = smartx->setup();
  ASSERT_TRUE(setup != nullptr);
  IasAudioSourceDevicePtr source = nullptr;
  IasAudioPortPtr port = nullptr;
  IasISetup::IasResult setres = setup->addAudioOutputPort(source, port);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  IasAudioDeviceParams devParams;
  setres = setup->createAudioSourceDevice(devParams, nullptr);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  devParams.clockType = eIasClockProvided;
  setres = setup->createAudioSourceDevice(devParams, nullptr);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  setres = setup->createAudioSourceDevice(devParams, &source);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  devParams.samplerate = 48000;
  setres = setup->createAudioSourceDevice(devParams, &source);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  devParams.name = "MySource";
  setres = setup->createAudioSourceDevice(devParams, &source);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  devParams.dataFormat = eIasFormatInt16;
  setres = setup->createAudioSourceDevice(devParams, &source);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  devParams.periodSize = 256;
  setres = setup->createAudioSourceDevice(devParams, &source);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  devParams.numPeriods = 4;
  setres = setup->createAudioSourceDevice(devParams, &source);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  devParams.numChannels = 2;
  setres = setup->createAudioSourceDevice(devParams, &source);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setres = setup->startAudioSourceDevice(nullptr);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  setres = setup->startAudioSourceDevice(source);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setup->stopAudioSourceDevice(nullptr);
  setup->stopAudioSourceDevice(source);
  setres = setup->addAudioOutputPort(source, port);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  IasAudioPortParams portParams;
  portParams.numChannels = 2;
  portParams.direction = eIasPortDirectionInput;
  portParams.index = 0;
  portParams.id = 321;
  setres = setup->createAudioPort(portParams, &port);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setres = setup->addAudioOutputPort(source, port);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  setup->destroyAudioPort(port);
  portParams.direction = eIasPortDirectionOutput;
  setres = setup->createAudioPort(portParams, &port);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  IasAudioPortVector outports = setup->getAudioOutputPorts();
  ASSERT_EQ(0u, outports.size());
  setres = setup->addAudioOutputPort(source, port);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  outports = setup->getAudioOutputPorts();
  ASSERT_EQ(1u, outports.size());
  ASSERT_EQ(321, outports[0]->getParameters()->id);
  ASSERT_EQ(2u, outports[0]->getParameters()->numChannels);
  ASSERT_EQ(0u, outports[0]->getParameters()->index);
  ASSERT_EQ(eIasPortDirectionOutput, outports[0]->getParameters()->direction);
  IasAudioSourceDevicePtr tmpSource = nullptr;
  IasAudioPortPtr tmpPort = nullptr;
  setup->deleteAudioOutputPort(tmpSource, tmpPort);
  setup->deleteAudioOutputPort(source, tmpPort);
  IasAudioPortPtr port2 = nullptr;
  setres = setup->createAudioPort(portParams, &port2);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setup->deleteAudioOutputPort(source, port2);
  IasAudioSourceDevicePtr source2 = nullptr;
  devParams.name = "MySource2";
  setres = setup->createAudioSourceDevice(devParams, &source2);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setres = setup->addAudioOutputPort(source2, port2);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setup->deleteAudioOutputPort(source, port2);

  setup->deleteAudioOutputPort(source, port);
  setup->destroyAudioPort(port);
  setup->destroyAudioSourceDevice(nullptr);
  setup->destroyAudioSourceDevice(source);

  IasSmartX::destroy(smartx);
}

/**
 * Negative test for the following methods of IasISetup:
 *
 * SourceDevice type = ALSA handler
 *
 * addAudioOutputPort(source,outputport)
 * createAudioSourceDevice
 * createAudioPort
 * destroyAudioPort
 * destroyAudioSourceDevice
 */
TEST_F(IasSmartX_API_Test, setup_api_fail_add_out_port_ah)
{
  IasSmartX *smartx = IasSmartX::create();
  ASSERT_TRUE(smartx != nullptr);
  IasISetup *setup = smartx->setup();
  ASSERT_TRUE(setup != nullptr);
  IasAudioSourceDevicePtr source = nullptr;
  IasAudioPortPtr port = nullptr;
  IasISetup::IasResult setres = setup->addAudioOutputPort(source, port);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  IasAudioDeviceParams devParams;
  setres = setup->createAudioSourceDevice(devParams, nullptr);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  devParams.clockType = eIasClockReceived;
  setres = setup->createAudioSourceDevice(devParams, nullptr);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  setres = setup->createAudioSourceDevice(devParams, &source);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  devParams.samplerate = 48000;
  setres = setup->createAudioSourceDevice(devParams, &source);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  devParams.name = "MySource";
  setres = setup->createAudioSourceDevice(devParams, &source);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  devParams.dataFormat = eIasFormatInt16;
  setres = setup->createAudioSourceDevice(devParams, &source);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  devParams.periodSize = 256;
  setres = setup->createAudioSourceDevice(devParams, &source);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  devParams.numPeriods = 4;
  setres = setup->createAudioSourceDevice(devParams, &source);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  devParams.numChannels = 2;
  setres = setup->createAudioSourceDevice(devParams, &source);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setres = setup->startAudioSourceDevice(nullptr);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  setres = setup->startAudioSourceDevice(source);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  setup->destroyAudioSourceDevice(source);
  devParams.name = PLUGHW_DEVICE_NAME;
  setres = setup->createAudioSourceDevice(devParams, &source);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setres = setup->startAudioSourceDevice(source);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  setup->stopAudioSourceDevice(nullptr);
  setup->stopAudioSourceDevice(source);
  setres = setup->addAudioOutputPort(source, port);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  IasAudioPortParams portParams;
  portParams.numChannels = 2;
  portParams.direction = eIasPortDirectionInput;
  portParams.index = 0;
  setres = setup->createAudioPort(portParams, &port);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setres = setup->addAudioOutputPort(source, port);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  setup->destroyAudioPort(port);
  portParams.direction = eIasPortDirectionOutput;
  setres = setup->createAudioPort(portParams, &port);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setres = setup->addAudioOutputPort(source, port);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  IasAudioSourceDevicePtr tmpSource = nullptr;
  IasAudioPortPtr tmpPort = nullptr;
  setup->deleteAudioOutputPort(tmpSource, tmpPort);
  setup->deleteAudioOutputPort(source, tmpPort);
  IasAudioPortPtr port2 = nullptr;
  setres = setup->createAudioPort(portParams, &port2);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setup->deleteAudioOutputPort(source, port2);
  IasAudioSourceDevicePtr source2 = nullptr;
  devParams.name = "MySource2";
  setres = setup->createAudioSourceDevice(devParams, &source2);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setres = setup->addAudioOutputPort(source2, port2);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setup->deleteAudioOutputPort(source, port2);

  setup->deleteAudioOutputPort(source, port);
  setup->destroyAudioPort(port);
  setup->destroyAudioSourceDevice(nullptr);
  setup->destroyAudioSourceDevice(source);

  IasSmartX::destroy(smartx);
}

TEST_F(IasSmartX_API_Test, createDevice_exceed_memory)
{
  IasISetup::IasResult result = IasISetup::eIasOk;

  // Create the smartx instance
  IasSmartX *smartx = IasSmartX::create();
  ASSERT_TRUE(smartx != nullptr);

  // Get the smartx setup
  IasISetup *setup = smartx->setup();
  ASSERT_TRUE(setup != nullptr);

  // Create an audio source
  IasAudioDeviceParams sourceDeviceParams =
  {
    "mySource",
    2,
    48000,
    eIasFormatInt16,
    eIasClockProvided,
    4294967295,
    4294967295
  };
  IasAudioSourceDevicePtr source = nullptr;
  result = setup->createAudioSourceDevice(sourceDeviceParams, &source);
  EXPECT_EQ(IasISetup::eIasFailed, result);
  IasSmartX::destroy(smartx);
}

TEST_F(IasSmartX_API_Test, setup_one_source_two_sinks)
{
  IasISetup::IasResult result = IasISetup::eIasOk;

  // Create the smartx instance
  IasSmartX *smartx = IasSmartX::create();
  ASSERT_TRUE(smartx != nullptr);

  // Get the smartx setup
  IasISetup *setup = smartx->setup();
  ASSERT_TRUE(setup != nullptr);

  // Create an audio source
  IasAudioDeviceParams sourceDeviceParams =
  {
    "mySource",
    2,
    48000,
    eIasFormatInt16,
    eIasClockProvided,
    192,
    4
  };
  IasAudioSourceDevicePtr source = nullptr;
  result = setup->createAudioSourceDevice(sourceDeviceParams, &source);
  EXPECT_EQ(IasISetup::eIasOk, result);
  EXPECT_TRUE(source != nullptr);
  EXPECT_EQ(nullptr, source->getSwitchMatrix());

  // Create source port and add to source device
  IasAudioPortParams sourcePortParams =
  {
    "mySourcePort",
    2,
    123,
    eIasPortDirectionOutput,
    0
  };
  IasAudioPortPtr sourcePort = nullptr;
  result = setup->createAudioPort(sourcePortParams, &sourcePort);
  EXPECT_EQ(IasISetup::eIasOk, result);
  EXPECT_TRUE(sourcePort != nullptr);
  result = setup->addAudioOutputPort(source, sourcePort);
  EXPECT_EQ(IasISetup::eIasOk, result);

  // Create audio sink1
  IasAudioSinkDevicePtr sink1 = nullptr;
  IasAudioDeviceParams sink1DeviceParams =
  {
    "mySink1",
    2,
    48000,
    eIasFormatInt16,
    eIasClockProvided,
    192,
    4
  };
  result = setup->createAudioSinkDevice(sink1DeviceParams, &sink1);
  EXPECT_EQ(IasISetup::eIasOk, result);
  EXPECT_TRUE(sink1 != nullptr);

  // Create sink port and add to sink device
  IasAudioPortParams sink1PortParams =
  {
    "mySink1Port",
    2,
    123,
    eIasPortDirectionInput,
    0
  };
  IasAudioPortPtr sink1Port = nullptr;
  result = setup->createAudioPort(sink1PortParams, &sink1Port);
  EXPECT_EQ(IasISetup::eIasOk, result);
  EXPECT_TRUE(sink1Port != nullptr);
  result = setup->addAudioInputPort(sink1, sink1Port);
  EXPECT_EQ(IasISetup::eIasOk, result);

  // Create a routing zone
  IasRoutingZoneParams rzparams1 =
  {
    "routingZone1"
  };
  IasRoutingZonePtr routingZone1 = nullptr;
  result = setup->createRoutingZone(rzparams1, &routingZone1);
  EXPECT_EQ(IasISetup::eIasOk, result);
  EXPECT_TRUE(routingZone1 != nullptr);

  // Link the routing zone with the sink
  result = setup->link(routingZone1, sink1);
  EXPECT_EQ(IasISetup::eIasOk, result);

  // Create audio sink2
  IasAudioSinkDevicePtr sink2 = nullptr;
  IasAudioDeviceParams sink2DeviceParams =
  {
    PLUGHW_DEVICE_NAME,
    2,
    48000,
    eIasFormatInt16,
    eIasClockReceived,
    192,
    4
  };
  result = setup->createAudioSinkDevice(sink2DeviceParams, &sink2);
  EXPECT_EQ(IasISetup::eIasOk, result);
  EXPECT_TRUE(sink2 != nullptr);

  // Create sink port and add to sink device
  IasAudioPortParams sink2PortParams =
  {
    "mySink2Port",
    2,
    24,
    eIasPortDirectionInput,
    0
  };
  IasAudioPortPtr sink2Port = nullptr;
  result = setup->createAudioPort(sink2PortParams, &sink2Port);
  EXPECT_EQ(IasISetup::eIasOk, result);
  EXPECT_TRUE(sink2Port != nullptr);
  result = setup->addAudioInputPort(sink2, sink2Port);
  EXPECT_EQ(IasISetup::eIasOk, result);

  // Create a routing zone
  IasRoutingZoneParams rzparams2 =
  {
    "routingZone2"
  };
  IasRoutingZonePtr routingZone2 = nullptr;
  result = setup->createRoutingZone(rzparams2, &routingZone2);
  EXPECT_EQ(IasISetup::eIasOk, result);
  EXPECT_TRUE(routingZone2 != nullptr);

  // Link the routing zone with the sink
  result = setup->link(routingZone2, sink2);
  EXPECT_EQ(IasISetup::eIasOk, result);

  // Create sink port and add to sink device
  IasAudioPortParams rznPortParams =
  {
    "myRznPort",
    2,
    127,
    eIasPortDirectionInput,
    0
  };
  IasAudioPortPtr rznPort = nullptr;
  result = setup->createAudioPort(rznPortParams, &rznPort);
  EXPECT_EQ(IasISetup::eIasOk, result);
  EXPECT_TRUE(rznPort != nullptr);
  result = setup->addAudioInputPort(routingZone2, rznPort);
  EXPECT_EQ(IasISetup::eIasOk, result);

  // Link the routing zone input port with the audio sink device
  result = setup->link(rznPort, sink2Port);
  EXPECT_EQ(IasISetup::eIasOk, result);

  setup->unlink(rznPort, sink2Port);

  // Try to establish invalid links. This must fail.
  result = setup->link(static_cast<IasAudioPortPtr>(nullptr), sink2Port);
  EXPECT_EQ(IasISetup::eIasFailed, result);
  result = setup->link(rznPort, static_cast<IasAudioPortPtr>(nullptr));
  EXPECT_EQ(IasISetup::eIasFailed, result);

  result = setup->addDerivedZone(routingZone1, routingZone2);
  EXPECT_EQ(IasISetup::eIasOk, result);

  IasRoutingZoneVector rzns = setup->getRoutingZones();
  cout << endl << "All configured routing zones:" << endl;
  for (auto &entry : rzns)
  {
    cout << IasSetupHelper::getRoutingZoneName(entry) << endl;
  }

  // Verify the IasSetupImpl::getAudioSourceDevice method.
  const std::string mySourceDeviceName = "mySource";
  IasAudioSourceDevicePtr mySourceDevice = setup->getAudioSourceDevice(mySourceDeviceName);
  IasAudioDeviceParamsPtr mySourceDeviceParams = mySourceDevice->getDeviceParams();
  EXPECT_TRUE(mySourceDeviceParams->name == mySourceDeviceName);
  mySourceDevice = setup->getAudioSourceDevice("invalidDeviceName");
  EXPECT_EQ(nullptr, mySourceDevice);

  IasAudioSourceDeviceVector sources = setup->getAudioSourceDevices();
  cout << endl << "All configured source devices:" << endl;
  for (auto &entry : sources)
  {
    cout << IasSetupHelper::getAudioSourceDeviceName(entry) << endl;
  }

  // Verify the IasSetupImpl::getAudioSinkDevice method.
  const std::string mySinkDeviceName = "mySink1";
  IasAudioSinkDevicePtr mySinkDevice = setup->getAudioSinkDevice(mySinkDeviceName);
  IasAudioDeviceParamsPtr mySinkDeviceParams = mySinkDevice->getDeviceParams();
  EXPECT_TRUE(mySinkDeviceParams->name == mySinkDeviceName);
  mySinkDevice = setup->getAudioSinkDevice("invalidDeviceName");
  EXPECT_EQ(nullptr, mySinkDevice);

  // Verify the IasSetupImpl::getRoutingZone method.
  const std::string myRoutingZoneName = "routingZone1";
  IasRoutingZonePtr myRoutingZone = setup->getRoutingZone(myRoutingZoneName);
  IasRoutingZoneParamsPtr myRoutingZoneParams = myRoutingZone->getRoutingZoneParams();
  EXPECT_TRUE(myRoutingZoneParams->name == myRoutingZoneName);
  myRoutingZone = setup->getRoutingZone("invalidRoutingZoneName");
  EXPECT_EQ(nullptr, myRoutingZone);


  IasAudioSinkDeviceVector sinks = setup->getAudioSinkDevices();
  cout << endl << "All configured sink devices:" << endl;
  for (auto &entry : sinks)
  {
    cout << IasSetupHelper::getAudioSinkDeviceName(entry) << endl;
  }
  cout << endl;

  setup->deleteAudioOutputPort(source, sourcePort);
  setup->destroyAudioSourceDevice(source);

  result = IasSetupHelper::deleteSinkDevice(setup, "invalidDeviceName");
  EXPECT_EQ(IasISetup::IasResult::eIasFailed, result);
  result = IasSetupHelper::deleteSinkDevice(setup, mySinkDeviceName);
  EXPECT_EQ(IasISetup::eIasOk, result);

  result = IasSetupHelper::deleteSourceDevice(setup, "invalidDeviceName");
  EXPECT_EQ(IasISetup::IasResult::eIasFailed, result);

  IasSmartX::destroy(smartx);
}

TEST_F(IasSmartX_API_Test, smartxclient)
{
  IasAudioDeviceParamsPtr devParams = std::make_shared<IasAudioDeviceParams>();
  devParams->clockType = eIasClockProvided;
  devParams->dataFormat = eIasFormatInt16;
  devParams->name = "MyClient";
  devParams->numChannels = 2;
  devParams->numPeriods = 4;
  devParams->periodSize = 256;
  devParams->samplerate = 48000;
  IasSmartXClient *smartxclient = new IasSmartXClient(devParams);
  ASSERT_TRUE(smartxclient != nullptr);
  IasSmartXClient::IasResult res = smartxclient->getRingBuffer(nullptr);
  ASSERT_EQ(IasSmartXClient::eIasFailed, res);
  IasAudioRingBuffer *rb = nullptr;
  res = smartxclient->getRingBuffer(&rb);
  ASSERT_EQ(IasSmartXClient::eIasFailed, res);
  res = smartxclient->init(eIasDeviceTypeSource);
  ASSERT_EQ(IasSmartXClient::eIasOk, res);
  IasAlsaPluginShmConnection *shmCon = new IasAlsaPluginShmConnection();
  ASSERT_TRUE(shmCon != nullptr);
  IasAudioCommonResult comres;
  comres = shmCon->findConnection("smartx_MyClient_p");
  ASSERT_EQ(eIasResultOk, comres);
  comres = shmCon->getOutIpc()->push<IasAudioIpcPluginControl>(eIasAudioIpcGetLatency);
  ASSERT_EQ(eIasResultOk, comres);
  IasAudioIpcPluginInt32Data latencyResponse;
  comres = shmCon->getInIpc()->pop_timed_wait<IasAudioIpcPluginInt32Data>(&latencyResponse, 100);
  ASSERT_EQ(eIasResultOk, comres);
  ASSERT_EQ(eIasAudioIpcGetLatency, latencyResponse.control);
  ASSERT_EQ(0, latencyResponse.response);
  comres = shmCon->getOutIpc()->push<IasAudioIpcPluginControl>(eIasAudioIpcStart);
  ASSERT_EQ(eIasResultOk, comres);
  IasAudioIpcPluginControlResponse genericResponse;
  comres = shmCon->getInIpc()->pop_timed_wait<IasAudioIpcPluginControlResponse>(&genericResponse, 100);
  ASSERT_EQ(eIasResultOk, comres);
  ASSERT_EQ(eIasAudioIpcStart, genericResponse.control);
  ASSERT_EQ(eIasAudioIpcACK, genericResponse.response);
  comres = shmCon->getOutIpc()->push<IasAudioIpcPluginControl>(eIasAudioIpcStop);
  ASSERT_EQ(eIasResultOk, comres);
  comres = shmCon->getInIpc()->pop_timed_wait<IasAudioIpcPluginControlResponse>(&genericResponse, 100);
  ASSERT_EQ(eIasResultOk, comres);
  ASSERT_EQ(eIasAudioIpcStop, genericResponse.control);
  ASSERT_EQ(eIasAudioIpcACK, genericResponse.response);
  comres = shmCon->getOutIpc()->push<IasAudioIpcPluginControl>(eIasAudioIpcDrain);
  ASSERT_EQ(eIasResultOk, comres);
  comres = shmCon->getInIpc()->pop_timed_wait<IasAudioIpcPluginControlResponse>(&genericResponse, 100);
  ASSERT_EQ(eIasResultOk, comres);
  ASSERT_EQ(eIasAudioIpcDrain, genericResponse.control);
  ASSERT_EQ(eIasAudioIpcACK, genericResponse.response);
  comres = shmCon->getOutIpc()->push<IasAudioIpcPluginControl>(eIasAudioIpcPause);
  ASSERT_EQ(eIasResultOk, comres);
  comres = shmCon->getInIpc()->pop_timed_wait<IasAudioIpcPluginControlResponse>(&genericResponse, 100);
  ASSERT_EQ(eIasResultOk, comres);
  ASSERT_EQ(eIasAudioIpcPause, genericResponse.control);
  ASSERT_EQ(eIasAudioIpcACK, genericResponse.response);
  comres = shmCon->getOutIpc()->push<IasAudioIpcPluginControl>(eIasAudioIpcResume);
  ASSERT_EQ(eIasResultOk, comres);
  comres = shmCon->getInIpc()->pop_timed_wait<IasAudioIpcPluginControlResponse>(&genericResponse, 100);
  ASSERT_EQ(eIasResultOk, comres);
  ASSERT_EQ(eIasAudioIpcResume, genericResponse.control);
  ASSERT_EQ(eIasAudioIpcACK, genericResponse.response);
  uint32_t illegalValueInt = 100;
  IasAudioIpcPluginControl *illegalValue = reinterpret_cast<IasAudioIpcPluginControl*>(&illegalValueInt);
  comres = shmCon->getOutIpc()->push<IasAudioIpcPluginControl>(*illegalValue);
  ASSERT_EQ(eIasResultOk, comres);
  comres = shmCon->getInIpc()->pop_timed_wait<IasAudioIpcPluginControlResponse>(&genericResponse, 100);
  ASSERT_EQ(eIasResultOk, comres);
  ASSERT_EQ(*illegalValue, genericResponse.control);
  ASSERT_EQ(eIasAudioIpcNAK, genericResponse.response);
  ASSERT_FALSE(shmCon->getOutIpc()->packagesAvailable());
  for (int32_t count=0; count<IAS_AUDIO_IPC_QUEUE_SIZE; ++count)
  {
    comres = shmCon->getOutIpc()->push<IasAudioIpcPluginControl>(eIasAudioIpcStart);
    ASSERT_EQ(eIasResultOk, comres);
  }
  usleep(100000);
  comres = shmCon->getInIpc()->discardAll();
  ASSERT_EQ(eIasResultOk, comres);
  IasAudioIpcPluginParamData paramData;
  paramData.control = eIasAudioIpcInvalid;
  comres = shmCon->getOutIpc()->push<IasAudioIpcPluginParamData>(paramData);
  ASSERT_EQ(eIasResultOk, comres);
  comres = shmCon->getInIpc()->pop_timed_wait<IasAudioIpcPluginControlResponse>(&genericResponse, 100);
  ASSERT_EQ(eIasResultOk, comres);
  ASSERT_EQ(eIasAudioIpcInvalid, genericResponse.control);
  ASSERT_EQ(eIasAudioIpcNAK, genericResponse.response);
  for (int32_t count=0; count<IAS_AUDIO_IPC_QUEUE_SIZE; ++count)
  {
    comres = shmCon->getOutIpc()->push<IasAudioIpcPluginParamData>(paramData);
    ASSERT_EQ(eIasResultOk, comres);
  }
  usleep(500000);
  comres = shmCon->getInIpc()->discardAll();
  ASSERT_EQ(eIasResultOk, comres);
  IasAudioIpcPluginFloatData floatData;
  for (int32_t count=0; count<IAS_AUDIO_IPC_QUEUE_SIZE; ++count)
  {
    comres = shmCon->getOutIpc()->push<IasAudioIpcPluginFloatData>(floatData);
    ASSERT_EQ(eIasResultOk, comres);
  }
  usleep(100000);
  comres = shmCon->getInIpc()->discardAll();
  ASSERT_EQ(eIasResultOk, comres);

  delete shmCon;
  delete smartxclient;
}

TEST_F(IasSmartX_API_Test, setup_two_source_two_sinks_helper)
{
  IasISetup::IasResult result = IasISetup::eIasOk;

  // Create the smartx instance
  IasSmartX *smartx = IasSmartX::create();
  ASSERT_TRUE(smartx != nullptr);

  // Get the smartx setup
  IasISetup *setup = smartx->setup();
  ASSERT_TRUE(setup != nullptr);

  // Create an audio source
  IasAudioDeviceParams sourceDeviceParams =
  {
    "mySource",
    2,
    48000,
    eIasFormatInt16,
    eIasClockProvided,
    192,
    4
  };
  IasAudioSourceDevicePtr source = nullptr;
  result = IasSetupHelper::createAudioSourceDevice(setup, sourceDeviceParams, 123, &source);
  EXPECT_EQ(IasISetup::eIasOk, result);
  EXPECT_TRUE(source != nullptr);
  IasAudioDeviceParams source4DeviceParams =
  {
    "mySource4",
    2,
    48000,
    eIasFormatInt16,
    eIasClockReceived,
    192,
    4
  };
  IasAudioSourceDevicePtr source4 = nullptr;
  result = IasSetupHelper::createAudioSourceDevice(setup, source4DeviceParams, 126, &source4);
  EXPECT_EQ(IasISetup::eIasOk, result);
  EXPECT_TRUE(source4 != nullptr);
  IasSmartXClientPtr smartXClient = nullptr;
  IasAudioCommonResult comres = source->getConcreteDevice(&smartXClient);
  EXPECT_EQ(eIasResultOk, comres);
  EXPECT_TRUE(smartXClient != nullptr);
  IasSmartXClient::IasResult smxclres = smartXClient->getRingBuffer(nullptr);
  EXPECT_EQ(IasSmartXClient::eIasFailed, smxclres);

  // Create audio sink1
  IasAudioSinkDevicePtr sink1 = nullptr;
  IasRoutingZonePtr routingZone1 = nullptr;
  IasAudioDeviceParams sink1DeviceParams =
  {
    "mySink1",
    2,
    48000,
    eIasFormatInt16,
    eIasClockProvided,
    192,
    4
  };
  result = IasSetupHelper::createAudioSinkDevice(setup, sink1DeviceParams, 123, &sink1, &routingZone1);
  EXPECT_EQ(IasISetup::eIasOk, result);
  EXPECT_TRUE(sink1 != nullptr);
  EXPECT_TRUE(routingZone1 != nullptr);

  // Create audio sink2
  IasAudioSinkDevicePtr sink2 = nullptr;
  IasRoutingZonePtr routingZone2 = nullptr;
  IasAudioDeviceParams sink2DeviceParams =
  {
    "mySink2",
    2,
    48000,
    eIasFormatInt16,
    eIasClockReceived,
    192,
    4
  };
  result = IasSetupHelper::createAudioSinkDevice(setup, sink2DeviceParams, 124, &sink2, &routingZone2);
  EXPECT_EQ(IasISetup::eIasOk, result);
  result = setup->addDerivedZone(routingZone1, routingZone2);
  EXPECT_EQ(IasISetup::eIasOk, result);
  IasSmartX::IasResult sxres = smartx->start();
  EXPECT_EQ(IasSmartX::eIasFailed, sxres);
  sxres = smartx->stop();
  EXPECT_EQ(IasSmartX::eIasOk, sxres);
  IasAudioPortParams portParams;
  portParams.direction = eIasPortDirectionInput;
  portParams.name = "testport";
  portParams.id = 125;
  portParams.numChannels = 2;
  portParams.index = 0;
  IasAudioPortPtr port = nullptr;
  result = setup->createAudioPort(portParams, &port);
  EXPECT_EQ(IasISetup::eIasOk, result);
  IasAudioDeviceParams sink3DeviceParams =
  {
    "mySink3",
    2,
    48000,
    eIasFormatInt16,
    eIasClockReceived,
    192,
    4
  };
  IasAudioSinkDevicePtr sink3 = nullptr;
  result = setup->createAudioSinkDevice(sink3DeviceParams, &sink3);
  EXPECT_EQ(IasISetup::eIasOk, result);
  result = setup->addAudioInputPort(sink3, port);
  EXPECT_EQ(IasISetup::eIasOk, result);
  IasAudioDeviceParams source2DeviceParams =
  {
    "mySource2",
    2,
    48000,
    eIasFormatInt16,
    eIasClockProvided,
    192,
    4
  };
  IasAudioSourceDevicePtr source2 = nullptr;
  result = IasSetupHelper::createAudioSourceDevice(setup, source2DeviceParams, 121, &source2);
  EXPECT_EQ(IasISetup::eIasOk, result);
  IasAudioDeviceParams source3DeviceParams =
  {
    "mySource3",
    2,
    48000,
    eIasFormatInt16,
    eIasClockProvided,
    192,
    4
  };
  IasAudioSourceDevicePtr source3 = nullptr;
  result = IasSetupHelper::createAudioSourceDevice(setup, source3DeviceParams, 122, &source3);
  EXPECT_EQ(IasISetup::eIasOk, result);
  setup->addSourceGroup("myGroup", 121);
  setup->addSourceGroup("myGroup", 122);
  // Create audio sink4
  IasAudioSinkDevicePtr sink4 = nullptr;
  IasRoutingZonePtr routingZone4 = nullptr;
  IasAudioDeviceParams sink4DeviceParams =
  {
    "mySink4",
    2,
    44100,
    eIasFormatInt16,
    eIasClockReceived,
    192,
    4
  };
  result = IasSetupHelper::createAudioSinkDevice(setup, sink4DeviceParams, 126, &sink4, &routingZone4);
  EXPECT_EQ(IasISetup::eIasOk, result);

  IasAudioSourceDevicePtr tmpSourceDev = nullptr;
  std::string sourceName = IasSetupHelper::getDeviceName(tmpSourceDev);
  ASSERT_EQ("", sourceName);
  sourceName = IasSetupHelper::getDeviceName(source);
  ASSERT_EQ("mySource", sourceName);
  std::string sinkName = IasSetupHelper::getDeviceName(sink1);
  ASSERT_EQ("mySink1", sinkName);
  int32_t portId = IasSetupHelper::getPortId(nullptr);
  ASSERT_EQ(-1, portId);
  portId = IasSetupHelper::getPortId(port);
  ASSERT_EQ(125, portId);
  std::string portName = IasSetupHelper::getPortName(nullptr);
  ASSERT_EQ("", portName);
  portName = IasSetupHelper::getPortName(port);
  ASSERT_EQ("testport", portName);
  uint32_t portNumChannels = IasSetupHelper::getPortNumChannels(nullptr);
  ASSERT_EQ(0u, portNumChannels);
  portNumChannels = IasSetupHelper::getPortNumChannels(port);
  ASSERT_EQ(2u, portNumChannels);
  std::string dir = IasSetupHelper::getPortDirection(nullptr);
  ASSERT_EQ("",dir);
  dir = IasSetupHelper::getPortDirection(port);
  ASSERT_EQ("Input",dir);

  IasAudioPortVector portVec = setup->getAudioPorts();
  ASSERT_TRUE( portVec.size() > 0);

  IasConnectionVector connections;
  IasIRouting *routing = smartx->routing();
  ASSERT_TRUE(routing != nullptr);
  IasIRouting::IasResult roures = routing->connect(0, 0);
  EXPECT_EQ(IasIRouting::eIasFailed, roures);

  connections = routing->getActiveConnections();
  ASSERT_EQ(0u, connections.size());

  roures = routing->connect(123, 0);
  EXPECT_EQ(IasIRouting::eIasFailed, roures);
  roures = routing->connect(123, 123);
  EXPECT_EQ(IasIRouting::eIasOk, roures);

  connections = routing->getActiveConnections();
  ASSERT_EQ(1u, connections.size());
  ASSERT_EQ(123, connections[0].first->getParameters()->id);
  ASSERT_EQ(123, connections[0].second->getParameters()->id);

  roures = routing->connect(123, 123);
  EXPECT_EQ(IasIRouting::eIasFailed, roures);
  roures = routing->connect(123, 124);
  EXPECT_EQ(IasIRouting::eIasOk, roures);

  connections = routing->getActiveConnections();
  ASSERT_EQ(2u, connections.size());
  ASSERT_EQ(123, connections[0].first->getParameters()->id);
  ASSERT_EQ(123, connections[0].second->getParameters()->id);
  ASSERT_EQ(123, connections[1].first->getParameters()->id);
  ASSERT_EQ(124, connections[1].second->getParameters()->id);

  roures = routing->connect(123, 123);
  EXPECT_EQ(IasIRouting::eIasFailed, roures);
  roures = routing->connect(123, 125);
  EXPECT_EQ(IasIRouting::eIasFailed, roures);
  roures = routing->disconnect(123, 123);
  EXPECT_EQ(IasIRouting::eIasOk, roures);
  roures = routing->disconnect(123, 124);
  EXPECT_EQ(IasIRouting::eIasOk, roures);
  setup->addSourceGroup("myGroup", 123);
  setup->deleteDerivedZone(routingZone1, routingZone2);
  roures = routing->connect(123, 123);
  EXPECT_EQ(IasIRouting::eIasOk, roures);
  roures = routing->connect(122, 124);
  EXPECT_EQ(IasIRouting::eIasOk, roures);
  roures = routing->disconnect(122, 124);
  EXPECT_EQ(IasIRouting::eIasOk, roures);
  roures = routing->connect(123, 124);
  EXPECT_EQ(IasIRouting::eIasFailed, roures);
  roures = routing->connect(121, 124);
  EXPECT_EQ(IasIRouting::eIasOk, roures);
  roures = routing->disconnect(121, 124);
  EXPECT_EQ(IasIRouting::eIasOk, roures);
  roures = routing->connect(121, 126);
  EXPECT_EQ(IasIRouting::eIasOk, roures);

  roures = routing->disconnect(0, 0);
  EXPECT_EQ(IasIRouting::eIasFailed, roures);
  roures = routing->disconnect(123, 0);
  EXPECT_EQ(IasIRouting::eIasFailed, roures);
  roures = routing->disconnect(123, 125);
  EXPECT_EQ(IasIRouting::eIasFailed, roures);
  roures = routing->disconnect(123, 123);
  EXPECT_EQ(IasIRouting::eIasOk, roures);
  roures = routing->disconnect(123, 123);
  EXPECT_EQ(IasIRouting::eIasFailed, roures);

  IasSmartX::destroy(smartx);
}

TEST_F(IasSmartX_API_Test, setup_two_sinks_helper_check_for_crash)
{
  IasISetup::IasResult result = IasISetup::eIasOk;

  // Create the smartx instance
  IasSmartX *smartx = IasSmartX::create();
  ASSERT_TRUE(smartx != nullptr);

  // Get the smartx setup
  IasISetup *setup = smartx->setup();
  ASSERT_TRUE(setup != nullptr);

  IasAudioDeviceParams sourceDeviceParams =
  {
    "mySink2",
    2,
    48000,
    eIasFormatInt16,
    eIasClockProvided,
    192,
    4
  };
  IasAudioSourceDevicePtr source = nullptr;
  result = IasSetupHelper::createAudioSourceDevice(setup, sourceDeviceParams, 123, &source);
  EXPECT_EQ(IasISetup::eIasOk, result);

  // Create audio sink1
  IasAudioSinkDevicePtr sink1 = nullptr;
  IasRoutingZonePtr routingZone1 = nullptr;
  IasAudioDeviceParams sink1DeviceParams =
  {
    "plughw:Dummy,0",
    2,
    48000,
    eIasFormatInt32,
    eIasClockReceived,
    192,
    4
  };
  result = IasSetupHelper::createAudioSinkDevice(setup, sink1DeviceParams, 123, &sink1, &routingZone1);
  EXPECT_EQ(IasISetup::eIasOk, result);
  EXPECT_TRUE(sink1 != nullptr);
  EXPECT_TRUE(routingZone1 != nullptr);

  // Create audio sink2
  IasAudioSinkDevicePtr sink2 = nullptr;
  IasRoutingZonePtr routingZone2 = nullptr;
  IasAudioDeviceParams sink2DeviceParams =
  {
    "smartx:mySink2",
    2,
    48000,
    eIasFormatInt16,
    eIasClockReceived,
    192,
    4
  };
  result = IasSetupHelper::createAudioSinkDevice(setup, sink2DeviceParams, 124, &sink2, &routingZone2);
  EXPECT_EQ(IasISetup::eIasOk, result);
  result = setup->addDerivedZone(routingZone1, routingZone2);
  EXPECT_EQ(IasISetup::eIasOk, result);
  IasSmartX::IasResult sxres = smartx->start();
  EXPECT_EQ(IasSmartX::eIasOk, sxres);

  IasIRouting::IasResult routres = smartx->routing()->connect(123, 123);
  EXPECT_EQ(IasIRouting::IasResult::eIasOk, routres);
  for (std::uint32_t count=0; count<200; count++)
  {
    std::cout << "************************************************** Iteration " << count << std::endl;
    usleep(500000);
    std::cout << "************************************************** Iteration, stop routing zone now " << count << std::endl;
    setup->stopRoutingZone(routingZone2);
    result = setup->startRoutingZone(routingZone2);
    EXPECT_EQ(IasISetup::eIasOk, result);
  }

  sxres = smartx->stop();
  EXPECT_EQ(IasSmartX::eIasOk, sxres);

  IasSmartX::destroy(smartx);
}

TEST_F(IasSmartX_API_Test, delete_connected_source_device)
{
  IasISetup::IasResult result = IasISetup::eIasOk;

  // Create the smartx instance
  IasSmartX *smartx = IasSmartX::create();
  ASSERT_TRUE(smartx != nullptr);

  // Get the smartx setup
  IasISetup *setup = smartx->setup();
  ASSERT_TRUE(setup != nullptr);

  IasAudioDeviceParams sourceDeviceParams =
  {
    "mySource",
    2,
    48000,
    eIasFormatInt16,
    eIasClockProvided,
    192,
    4
  };
  IasAudioSourceDevicePtr source = nullptr;
  result = IasSetupHelper::createAudioSourceDevice(setup, sourceDeviceParams, 123, &source);
  EXPECT_EQ(IasISetup::eIasOk, result);

  // Create audio sink1
  IasAudioSinkDevicePtr sink1 = nullptr;
  IasRoutingZonePtr routingZone1 = nullptr;
  IasAudioDeviceParams sink1DeviceParams =
  {
    "map_2_4",
    2,
    48000,
    eIasFormatInt32,
    eIasClockReceived,
    192,
    4
  };
  result = IasSetupHelper::createAudioSinkDevice(setup, sink1DeviceParams, 123, &sink1, &routingZone1);
  EXPECT_EQ(IasISetup::eIasOk, result);
  EXPECT_TRUE(sink1 != nullptr);
  EXPECT_TRUE(routingZone1 != nullptr);

  IasSmartX::IasResult sxres = smartx->start();
  EXPECT_EQ(IasSmartX::eIasOk, sxres);

  IasIRouting::IasResult routres = smartx->routing()->connect(123, 123);
  EXPECT_EQ(IasIRouting::IasResult::eIasOk, routres);

  sxres = smartx->waitForEvent(100);
  EXPECT_EQ(IasSmartX::eIasOk, sxres);
  IasEventPtr event = nullptr;
  sxres = smartx->getNextEvent(&event);
  EXPECT_EQ(IasSmartX::eIasOk, sxres);
  class : public IasEventHandler
  {
    virtual void receivedConnectionEvent(IasConnectionEvent* event)
    {
      cout << "Received connection event" << endl;
      EXPECT_EQ(IasConnectionEvent::eIasConnectionEstablished, event->getEventType());
      EXPECT_EQ(123, event->getSourceId());
      EXPECT_EQ(123, event->getSinkId());
    }
  } connectionEstablishedEH;
  event->accept(connectionEstablishedEH);


  // Now stop the connected source.
  // Expected behavior is that the connection is automatically disconnected and afterwards the device is successfully stopped.
  // This will also generate an event to notify the application that a connection is lost due to a source is being stopped/removed.
  setup->stopAudioSourceDevice(source);
  sxres = smartx->waitForEvent(100);
  EXPECT_EQ(IasSmartX::eIasOk, sxres);
  event = nullptr;
  sxres = smartx->getNextEvent(&event);
  EXPECT_EQ(IasSmartX::eIasOk, sxres);
  class : public IasEventHandler
  {
    virtual void receivedConnectionEvent(IasConnectionEvent* event)
    {
      cout << "Received connection event" << endl;
      EXPECT_EQ(IasConnectionEvent::eIasSourceDeleted, event->getEventType());
      EXPECT_EQ(123, event->getSourceId());
      EXPECT_EQ(123, event->getSinkId());
    }
  } connectionRemovedEH;
  event->accept(connectionRemovedEH);

  // Now try to establish the same connection again. This has to work, as the connection was automatically removed.
  routres = smartx->routing()->connect(123, 123);
  ASSERT_EQ(IasIRouting::IasResult::eIasOk, routres);

  sxres = smartx->waitForEvent(100);
  EXPECT_EQ(IasSmartX::eIasOk, sxres);
  event = nullptr;
  sxres = smartx->getNextEvent(&event);
  EXPECT_EQ(IasSmartX::eIasOk, sxres);
  class : public IasEventHandler
  {
    virtual void receivedConnectionEvent(IasConnectionEvent* event)
    {
      cout << "Received connection event" << endl;
      EXPECT_EQ(IasConnectionEvent::eIasConnectionEstablished, event->getEventType());
      EXPECT_EQ(123, event->getSourceId());
      EXPECT_EQ(123, event->getSinkId());
    }
  } connectionEstablishedEH1;
  event->accept(connectionEstablishedEH1);

  sxres = smartx->stop();
  EXPECT_EQ(IasSmartX::eIasOk, sxres);

  IasSmartX::destroy(smartx);
}

TEST_F(IasSmartX_API_Test, setup_helper_fail)
{
  // Create the smartx instance
  IasSmartX *smartx = IasSmartX::create();
  ASSERT_TRUE(smartx != nullptr);

  // Get the smartx setup
  IasISetup *setup = smartx->setup();
  ASSERT_TRUE(setup != nullptr);

  IasISetup::IasResult setres;
  IasAudioDeviceParams sourceParams;
  setres = IasSetupHelper::createAudioSourceDevice(nullptr, sourceParams, 0, nullptr);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  setres = IasSetupHelper::createAudioSourceDevice(setup, sourceParams, 0, nullptr);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  IasAudioSourceDevicePtr source = nullptr;
  setres = IasSetupHelper::createAudioSourceDevice(setup, sourceParams, 0, &source);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  sourceParams.clockType = eIasClockProvided;
  sourceParams.samplerate = 48000;
  sourceParams.name = "MySource";
  sourceParams.dataFormat = eIasFormatInt16;
  sourceParams.periodSize = 256;
  sourceParams.numPeriods = 4;
  sourceParams.numChannels = 2;
  setres = IasSetupHelper::createAudioSourceDevice(setup, sourceParams, 0, &source);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  IasAudioDeviceParams sinkParams;
  setres = IasSetupHelper::createAudioSinkDevice(nullptr, sinkParams, 0, nullptr, nullptr);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  setres = IasSetupHelper::createAudioSinkDevice(setup, sinkParams, 0, nullptr, nullptr);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  IasAudioSinkDevicePtr sink = nullptr;
  setres = IasSetupHelper::createAudioSinkDevice(setup, sinkParams, 0, &sink, nullptr);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  IasRoutingZonePtr rzn = nullptr;
  setres = IasSetupHelper::createAudioSinkDevice(setup, sinkParams, 0, &sink, &rzn);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  sinkParams.clockType = eIasClockProvided;
  sinkParams.samplerate = 48000;
  sinkParams.name = "MySink";
  sinkParams.dataFormat = eIasFormatInt16;
  sinkParams.periodSize = 256;
  sinkParams.numPeriods = 4;
  sinkParams.numChannels = 2;
  setres = IasSetupHelper::createAudioSinkDevice(setup, sinkParams, 0, &sink, &rzn);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  IasSmartX::destroy(smartx);
}

TEST_F(IasSmartX_API_Test, setup_helper_fail2)
{
  // Create the smartx instance
  IasSmartX *smartx = IasSmartX::create();
  ASSERT_TRUE(smartx != nullptr);

  // Get the smartx setup
  IasISetup *setup = smartx->setup();
  ASSERT_TRUE(setup != nullptr);

  IasISetup::IasResult setres;
  IasAudioDeviceParams sourceParams;
  sourceParams.clockType = eIasClockProvided;
  sourceParams.samplerate = 48000;
  sourceParams.name = "MySource";
  sourceParams.dataFormat = eIasFormatInt16;
  sourceParams.periodSize = 256;
  sourceParams.numPeriods = 4;
  sourceParams.numChannels = 2;
  IasAudioSourceDevicePtr source = nullptr;
  setres = IasSetupHelper::createAudioSourceDevice(setup, sourceParams, 0, &source);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  IasAudioDeviceParams sinkParams;
  std::vector<int32_t> idVector;
  std::vector<uint32_t> numChannelsVector;
  std::vector<uint32_t> channelIndexVector;
  std::vector<IasAudioPortPtr> sinkPortVector;
  std::vector<IasAudioPortPtr> routingZonePortVector;
  IasAudioSinkDevicePtr sink = nullptr;
  IasRoutingZonePtr rzn = nullptr;


  setres = IasSetupHelper::createAudioSinkDevice(nullptr, sinkParams, idVector, numChannelsVector, channelIndexVector,
                                                 &sink, &rzn,
                                                 &sinkPortVector, &routingZonePortVector);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  setres = IasSetupHelper::createAudioSinkDevice(setup, sinkParams, idVector, numChannelsVector, channelIndexVector,
                                                 nullptr, &rzn,
                                                 &sinkPortVector, &routingZonePortVector);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  setres = IasSetupHelper::createAudioSinkDevice(setup, sinkParams, idVector, numChannelsVector, channelIndexVector,
                                                 &sink, nullptr,
                                                 &sinkPortVector, &routingZonePortVector);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  setres = IasSetupHelper::createAudioSinkDevice(setup, sinkParams, idVector, numChannelsVector, channelIndexVector,
                                                 &sink, &rzn,
                                                 nullptr, &routingZonePortVector);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  setres = IasSetupHelper::createAudioSinkDevice(setup, sinkParams, idVector, numChannelsVector, channelIndexVector,
                                                 &sink, &rzn,
                                                 &sinkPortVector, nullptr);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  setres = IasSetupHelper::createAudioSinkDevice(setup, sinkParams, idVector, numChannelsVector, channelIndexVector,
                                                 &sink, &rzn,
                                                 &sinkPortVector, &routingZonePortVector);
  ASSERT_EQ(IasISetup::eIasFailed, setres);

  // Test with idVector.size() != numChannelsVector.size()
  idVector.push_back(1);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  setres = IasSetupHelper::createAudioSinkDevice(setup, sinkParams, idVector, numChannelsVector, channelIndexVector,
                                                 &sink, &rzn,
                                                 &sinkPortVector, &routingZonePortVector);
  ASSERT_EQ(IasISetup::eIasFailed, setres);

  // Test with numChannelsVector.size() != channelIndexVector.size()
  numChannelsVector.push_back(1);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  setres = IasSetupHelper::createAudioSinkDevice(setup, sinkParams, idVector, numChannelsVector, channelIndexVector,
                                                 &sink, &rzn,
                                                 &sinkPortVector, &routingZonePortVector);
  ASSERT_EQ(IasISetup::eIasFailed, setres);

  channelIndexVector.push_back(0); // now all three vectors have the same size again

  sinkParams.clockType = eIasClockProvided;
  sinkParams.samplerate = 48000;
  sinkParams.name = "MySink";
  sinkParams.dataFormat = eIasFormatInt16;
  sinkParams.periodSize = 256;
  sinkParams.numPeriods = 4;
  sinkParams.numChannels = 2;
  setres = IasSetupHelper::createAudioSinkDevice(setup, sinkParams, idVector, numChannelsVector, channelIndexVector,
                                                 &sink, &rzn,
                                                 &sinkPortVector, &routingZonePortVector);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  IasSmartX::destroy(smartx);
}


void IasSmartX_API_Test::worker_thread()
{
  IasCustomerEventHandler eventHandler;
  while(mThreadIsRunning || mNrEvents > 0)
  {
    std::unique_lock<std::mutex> lk(mMutex);
    cout << "Worker waiting for event" << endl;
    mCondition.wait(lk, [this] { return mNrEvents != 0; });
    // Got an event
    cout << "Event received, NrEvents=" << mNrEvents << endl;
    IasEventPtr eventPtr;
    if (mEventQueue.try_pop(eventPtr))
    {
      eventPtr->accept(eventHandler);
      mNrEvents--;
    }
  }
}

TEST_F(IasSmartX_API_Test, EventHandler)
{
  mNrEvents = 0;
  std::thread worker(&IasSmartX_API_Test::worker_thread, this);
  IasConnectionEvent *event = new IasConnectionEvent();
  ASSERT_TRUE(event != nullptr);

  event->setEventType(IasConnectionEvent::eIasSinkDeleted);
  IasEventPtr eventPtr(event);
  mEventQueue.push(eventPtr);
  eventPtr = nullptr;
  {
    std::lock_guard<std::mutex> lk(mMutex);
    mNrEvents++;
    cout << "One event put into event queue" << endl;
  }
  mCondition.notify_one();

  event = new IasConnectionEvent();
  event->setEventType(IasConnectionEvent::eIasSourceDeleted);
  IasEventPtr eventPtr1(event);
  mEventQueue.push(eventPtr1);
  eventPtr1 = nullptr;
  {
    std::lock_guard<std::mutex> lk(mMutex);
    mNrEvents++;
    cout << "One event put into event queue" << endl;
  }
  mThreadIsRunning = false;
  mCondition.notify_one();

  worker.join();
  cout << "Number events after return=" << mNrEvents << endl;
  EXPECT_EQ(0u, mNrEvents);
}

TEST_F(IasSmartX_API_Test, Configuration)
{
  IasConfiguration config;
  IasConfiguration::IasResult result = config.addPort(nullptr);
  EXPECT_EQ(IasConfiguration::eIasFailed, result);
  IasAudioPortPtr port;
  result = config.getOutputPort(0, &port);
  EXPECT_EQ(IasConfiguration::eIasFailed, result);
  result = config.getOutputPort(0, nullptr);
  EXPECT_EQ(IasConfiguration::eIasFailed, result);
  result = config.getInputPort(0, nullptr);
  EXPECT_EQ(IasConfiguration::eIasFailed, result);
  config.deleteOutputPort(0);

  result = config.addPort(nullptr);
  EXPECT_EQ(IasConfiguration::eIasFailed, result);
  result = config.getInputPort(0, &port);
  EXPECT_EQ(IasConfiguration::eIasFailed, result);
  config.deleteInputPort(0);

  IasSmartX *smartx = IasSmartX::create();
  ASSERT_TRUE(smartx != nullptr);
  IasISetup *setup = smartx->setup();
  ASSERT_TRUE(setup != nullptr);
  IasAudioPortParams params;
  params.direction = eIasPortDirectionInput;
  params.id = 5;
  params.index = 0;
  params.name = "MyPort";
  params.numChannels = 2;
  IasISetup::IasResult setres = setup->createAudioPort(params, &port);
  EXPECT_EQ(IasISetup::eIasOk, setres);
  ASSERT_TRUE(port != nullptr);
  result = config.addPort(port);
  EXPECT_EQ(IasConfiguration::eIasOk, result);
  IasAudioPortPtr otherPortPtr;
  result = config.getOutputPort(5, &otherPortPtr);
  EXPECT_EQ(IasConfiguration::eIasFailed, result);
  result = config.getInputPort(5, &otherPortPtr);
  EXPECT_EQ(IasConfiguration::eIasOk, result);
  EXPECT_EQ(eIasPortDirectionInput, otherPortPtr->getParameters()->direction);
  EXPECT_EQ(5, otherPortPtr->getParameters()->id);
  EXPECT_EQ(0u, otherPortPtr->getParameters()->index);
  EXPECT_EQ("MyPort", otherPortPtr->getParameters()->name);
  EXPECT_EQ(2u, otherPortPtr->getParameters()->numChannels);
  config.deleteInputPort(5);

  IasAudioSourceDevicePtr source = nullptr;
  result = config.getAudioDevice("doesnt_exist", &source);
  EXPECT_EQ(IasConfiguration::eIasObjectNotFound, result);
  IasAudioSinkDevicePtr sink = nullptr;
  result = config.getAudioDevice("doesnt_exist", &sink);
  EXPECT_EQ(IasConfiguration::eIasObjectNotFound, result);

  IasAudioDeviceParams sourceParams =
  {
    "MySource",
    2,
    48000,
    eIasFormatInt16,
    eIasClockProvided,
    256,
    4
  };
  setres = setup->createAudioSourceDevice(sourceParams, &source);
  EXPECT_EQ(IasISetup::eIasOk, setres);
  result = config.addAudioDevice("MySource", source);
  EXPECT_EQ(IasConfiguration::eIasOk, result);
  result = config.addAudioDevice("MySource", source);
  EXPECT_EQ(IasConfiguration::eIasObjectAlreadyExists, result);
  IasAudioSourceDevicePtr tmpSource = nullptr;
  result = config.getAudioDevice("MySource", &tmpSource);
  EXPECT_EQ(IasConfiguration::eIasOk, result);
  config.deleteAudioDevice("doesnt_exist");
  config.deleteAudioDevice("MySource");
  IasAudioDeviceParams sinkParams =
  {
    "MySink",
    2,
    48000,
    eIasFormatInt16,
    eIasClockProvided,
    256,
    4
  };
  setres = setup->createAudioSinkDevice(sinkParams, &sink);
  EXPECT_EQ(IasISetup::eIasOk, setres);
  result = config.addAudioDevice("MySink", sink);
  EXPECT_EQ(IasConfiguration::eIasOk, result);
  result = config.addAudioDevice("MySink", sink);
  EXPECT_EQ(IasConfiguration::eIasObjectAlreadyExists, result);
  IasAudioSinkDevicePtr tmpSink = nullptr;
  result = config.getAudioDevice("MySink", &tmpSink);
  EXPECT_EQ(IasConfiguration::eIasOk, result);
  config.deleteAudioDevice("doesnt_exist");
  config.deleteAudioDevice("MySink");

  params.direction = eIasPortDirectionOutput;
  IasAudioPortPtr port2;
  setres = setup->createAudioPort(params, &port2);
  EXPECT_EQ(IasISetup::eIasOk, setres);
  ASSERT_TRUE(port2 != nullptr);
  result = config.addPort(port2);
  EXPECT_EQ(IasConfiguration::eIasOk, result);
  result = config.getInputPort(5, &otherPortPtr);
  EXPECT_EQ(IasConfiguration::eIasFailed, result);
  result = config.getOutputPort(5, &otherPortPtr);
  EXPECT_EQ(IasConfiguration::eIasOk, result);
  EXPECT_EQ(eIasPortDirectionOutput, otherPortPtr->getParameters()->direction);
  EXPECT_EQ(5, otherPortPtr->getParameters()->id);
  EXPECT_EQ(0u, otherPortPtr->getParameters()->index);
  EXPECT_EQ("MyPort", otherPortPtr->getParameters()->name);
  EXPECT_EQ(2u, otherPortPtr->getParameters()->numChannels);
  config.deleteOutputPort(5);
  uint32_t nrSources = config.getNumberSourceDevices();
  EXPECT_EQ(0u, nrSources);
  uint32_t nrSinks = config.getNumberSinkDevices();
  EXPECT_EQ(0u, nrSinks);
  uint32_t nrRoutingZones = config.getNumberRoutingZones();
  EXPECT_EQ(0u, nrRoutingZones);
  config.deleteRoutingZone("myRoutingZone");
  IasRoutingZonePtr routingZone = nullptr;
  IasRoutingZoneParams rzParams;
  rzParams.name = "myRoutingZone";
  setres = setup->createRoutingZone(rzParams, &routingZone);
  EXPECT_EQ(IasISetup::eIasOk, setres);
  EXPECT_TRUE(routingZone != nullptr);
  IasRoutingZonePtr tmpRoutingZone = nullptr;
  result = config.getRoutingZone(rzParams.name, nullptr);
  EXPECT_EQ(IasConfiguration::eIasNullPointer, result);
  result = config.getRoutingZone(rzParams.name, &tmpRoutingZone);
  EXPECT_EQ(IasConfiguration::eIasObjectNotFound, result);
  result = config.addRoutingZone(rzParams.name, routingZone);
  EXPECT_EQ(IasConfiguration::eIasOk, result);
  result = config.addRoutingZone(rzParams.name, routingZone);
  EXPECT_EQ(IasConfiguration::eIasObjectAlreadyExists, result);
  nrRoutingZones = config.getNumberRoutingZones();
  EXPECT_EQ(1u, nrRoutingZones);
  tmpRoutingZone = nullptr;
  result = config.getRoutingZone(rzParams.name, &tmpRoutingZone);
  EXPECT_EQ(IasConfiguration::eIasOk, result);
  EXPECT_TRUE(tmpRoutingZone != nullptr);

  IasSmartX::destroy(smartx);
}

TEST_F(IasSmartX_API_Test, add_delete_ports)
{
  // Create the smartx instance
  IasSmartX *smartx = IasSmartX::create();
  ASSERT_TRUE(smartx != nullptr);

  // Get the smartx setup
  IasISetup *setup = smartx->setup();
  ASSERT_TRUE(setup != nullptr);

  IasISetup::IasResult result;
  IasAudioSinkDevicePtr sink = nullptr;
  IasAudioPortPtr inPort = nullptr;
  result = setup->addAudioInputPort(sink, inPort);
  EXPECT_EQ(IasISetup::eIasFailed, result);
  IasAudioDeviceParams sinkParams;
  sinkParams.clockType = eIasClockProvided;
  sinkParams.dataFormat = eIasFormatInt16;
  sinkParams.name = "MySink";
  sinkParams.numChannels = 2;
  sinkParams.numPeriods = 4;
  sinkParams.periodSize = 128;
  sinkParams.samplerate = 48000;
  result = setup->createAudioSinkDevice(sinkParams, &sink);
  EXPECT_EQ(IasISetup::eIasOk, result);
  result = setup->addAudioInputPort(sink, inPort);
  EXPECT_EQ(IasISetup::eIasFailed, result);
  IasAudioPortParams portParams;
  portParams.direction = eIasPortDirectionUndef;
  portParams.id = 1234;
  portParams.index = 0;
  portParams.name = "sink_in_port";
  portParams.numChannels = 0;
  result = setup->createAudioPort(portParams, &inPort);
  EXPECT_EQ(IasISetup::eIasFailed, result);
  portParams.numChannels = 2;
  result = setup->createAudioPort(portParams, &inPort);
  EXPECT_EQ(IasISetup::eIasFailed, result);
  portParams.direction = eIasPortDirectionOutput;
  result = setup->createAudioPort(portParams, &inPort);
  EXPECT_EQ(IasISetup::eIasOk, result);
  result = setup->addAudioInputPort(sink, inPort);
  EXPECT_EQ(IasISetup::eIasFailed, result);
  setup->destroyAudioPort(inPort);
  portParams.direction = eIasPortDirectionInput;
  result = setup->createAudioPort(portParams, &inPort);
  EXPECT_EQ(IasISetup::eIasOk, result);
  result = setup->addAudioInputPort(sink, inPort);
  EXPECT_EQ(IasISetup::eIasOk, result);
  IasAudioSinkDevicePtr dummySink = nullptr;
  IasAudioPortPtr dummyPort = nullptr;
  portParams.direction = eIasPortDirectionOutput;
  portParams.name = "out_port";
  IasAudioPortPtr outPort = nullptr;
  result = setup->createAudioPort(portParams, &outPort);
  EXPECT_EQ(IasISetup::eIasOk, result);
  setup->deleteAudioInputPort(dummySink, dummyPort);
  setup->deleteAudioInputPort(sink, dummyPort);
  setup->deleteAudioInputPort(sink, outPort);
  setup->deleteAudioInputPort(sink, inPort);
  IasRoutingZoneParams zoneParams;
  zoneParams.name = "MyZone";
  IasRoutingZonePtr zone = nullptr;
  result = setup->addAudioInputPort(zone, dummyPort);
  EXPECT_EQ(IasISetup::eIasFailed, result);
  result = setup->createRoutingZone(zoneParams, &zone);
  EXPECT_EQ(IasISetup::eIasOk, result);
  result = setup->link(zone, sink);
  EXPECT_EQ(IasISetup::eIasOk, result);
  result = setup->addAudioInputPort(zone, dummyPort);
  EXPECT_EQ(IasISetup::eIasFailed, result);
  result = setup->addAudioInputPort(zone, outPort);
  EXPECT_EQ(IasISetup::eIasFailed, result);
  result = setup->addAudioInputPort(zone, inPort);
  EXPECT_EQ(IasISetup::eIasOk, result);
  IasRoutingZonePtr dummyZone = nullptr;
  setup->deleteAudioInputPort(dummyZone, dummyPort);
  setup->deleteAudioInputPort(zone, dummyPort);
  setup->deleteAudioInputPort(zone, outPort);
  setup->deleteAudioInputPort(zone, inPort);

  setup->destroyAudioPort(inPort);
  setup->destroyAudioPort(outPort);
  setup->destroyAudioSinkDevice(sink);

  IasSmartX::destroy(smartx);
}

void setHwParams(snd_pcm_t *pcm)
{
  ASSERT_TRUE(pcm != nullptr);
  snd_output_t *output = nullptr;
  int alsares = snd_output_stdio_attach(&output, stdout, 0);
  EXPECT_TRUE(alsares >= 0);
  snd_pcm_hw_params_t *hwParams = nullptr;
  alsares = snd_pcm_hw_params_malloc(&hwParams);
  EXPECT_EQ(0, alsares);
  ASSERT_TRUE(hwParams != nullptr);
  alsares = snd_pcm_hw_params_any(pcm, hwParams);
  EXPECT_EQ(1, alsares);
  alsares = snd_pcm_hw_params_set_period_size(pcm, hwParams, 256, 0);
  EXPECT_EQ(-EINVAL, alsares);
  alsares = snd_pcm_hw_params_set_period_size(pcm, hwParams, 192, 0);
  EXPECT_EQ(0, alsares);
  alsares = snd_pcm_hw_params(pcm, hwParams);
  EXPECT_EQ(0, alsares);
  alsares = snd_pcm_hw_params_current(pcm, hwParams);
  EXPECT_EQ(0, alsares);
  snd_pcm_dump(pcm, output);
  snd_pcm_hw_params_free(hwParams);
}

TEST_F(IasSmartX_API_Test, test_hw_constraints)
{
  // Create the smartx instance
  IasSmartX *smartx = IasSmartX::create();
  ASSERT_TRUE(smartx != nullptr);

  // Get the smartx setup
  IasISetup *setup = smartx->setup();
  ASSERT_TRUE(setup != nullptr);

  snd_pcm_t *pcm = nullptr;
  int alsares = snd_pcm_open(&pcm, "smartx:testConnection", SND_PCM_STREAM_PLAYBACK, 0);
  EXPECT_EQ(-ENODEV, alsares);

  IasISetup::IasResult res;
  IasAudioDeviceParams sourceParams;
  sourceParams.clockType = eIasClockProvided;
  sourceParams.dataFormat = eIasFormatInt16;
  sourceParams.name = "testConnection";
  sourceParams.numChannels = 2;
  sourceParams.numPeriods = 4;
  sourceParams.periodSize = 192;
  sourceParams.samplerate = 48000;
  IasAudioSourceDevicePtr source = nullptr;
  res = setup->createAudioSourceDevice(sourceParams, &source);
  EXPECT_EQ(IasISetup::eIasOk, res);
  alsares = snd_pcm_open(&pcm, "smartx:testConnection", SND_PCM_STREAM_PLAYBACK, 0);
  EXPECT_EQ(0, alsares);
  setHwParams(pcm);

  ASSERT_TRUE(pcm != nullptr);
  alsares = snd_pcm_close(pcm);
  EXPECT_EQ(0, alsares);

  IasSmartX::destroy(smartx);
}

TEST_F(IasSmartX_API_Test, test_event_api)
{
  IasSmartX *smartx = IasSmartX::create();
  ASSERT_TRUE(smartx != nullptr);

  IasSmartX::IasResult result = smartx->waitForEvent(1000);
  EXPECT_EQ(IasSmartX::eIasTimeout, result);

  result = smartx->getNextEvent(nullptr);
  EXPECT_EQ(IasSmartX::eIasFailed, result);
  IasEventPtr receivedEvent;
  result = smartx->getNextEvent(&receivedEvent);
  EXPECT_EQ(IasSmartX::eIasNoEventAvailable, result);

  IasEventProvider *eventProvider = IasEventProvider::getInstance();
  ASSERT_TRUE(eventProvider != nullptr);

  IasConnectionEventPtr conEvent = eventProvider->createConnectionEvent();
  ASSERT_TRUE(conEvent != nullptr);
  conEvent->setEventType(IasConnectionEvent::eIasConnectionEstablished);
  conEvent->setSourceId(5);
  conEvent->setSinkId(10);
  eventProvider->send(conEvent);

  result = smartx->waitForEvent(100);
  EXPECT_EQ(IasSmartX::eIasOk, result);

  result = smartx->getNextEvent(&receivedEvent);
  EXPECT_EQ(IasSmartX::eIasOk, result);
  EXPECT_TRUE(receivedEvent != nullptr);

  result = smartx->waitForEvent(100);
  EXPECT_EQ(IasSmartX::eIasTimeout, result);

  eventProvider->send(conEvent);
  eventProvider->send(conEvent);

  result = smartx->waitForEvent(100);
  EXPECT_EQ(IasSmartX::eIasOk, result);
  result = smartx->getNextEvent(&receivedEvent);
  EXPECT_EQ(IasSmartX::eIasOk, result);
  result = smartx->waitForEvent(100);
  EXPECT_EQ(IasSmartX::eIasOk, result);
  result = smartx->getNextEvent(&receivedEvent);
  EXPECT_EQ(IasSmartX::eIasOk, result);

  IasSetupEventPtr setupEvent = eventProvider->createSetupEvent();
  ASSERT_TRUE(setupEvent != nullptr);
  setupEvent->setEventType(IasSetupEvent::eIasUnrecoverableSourceDeviceError);
  setupEvent->setSourceDevice(nullptr);
  setupEvent->setSinkDevice(nullptr);
  eventProvider->send(setupEvent);
  class : public IasEventHandler
  {
    virtual void receivedConnectionEvent(IasConnectionEvent* event)
    {
      cout << "Received connection event" << endl;
      EXPECT_EQ(IasConnectionEvent::eIasConnectionEstablished, event->getEventType());
      EXPECT_EQ(5, event->getSourceId());
      EXPECT_EQ(10, event->getSinkId());
    }
    virtual void receivedSetupEvent(IasSetupEvent* event)
    {
      cout << "Received setup event" << endl;
      EXPECT_EQ(IasSetupEvent::eIasUnrecoverableSourceDeviceError, event->getEventType());
      EXPECT_EQ(nullptr, event->getSourceDevice());
      EXPECT_EQ(nullptr, event->getSinkDevice());
    }
  } myEventHandler;
  receivedEvent->accept(myEventHandler);
  IasEventHandler baseEventHandler;
  receivedEvent->accept(baseEventHandler);

  result = smartx->waitForEvent(100);
  EXPECT_EQ(IasSmartX::eIasOk, result);
  result = smartx->getNextEvent(&receivedEvent);
  EXPECT_EQ(IasSmartX::eIasOk, result);
  receivedEvent->accept(myEventHandler);
  receivedEvent->accept(baseEventHandler);

  eventProvider->destroyConnectionEvent(conEvent);
  eventProvider->destroySetupEvent(setupEvent);
  IasSmartX::destroy(smartx);
}

TEST_F(IasSmartX_API_Test, config_file_default_paths_fail)
{
  fs::path fullConfigPath(std::string(SMARTX_CONFIG_DIR) + "smartx_config.txt");
  if (fs::exists(fullConfigPath) == true)
  {
    fs::remove(fullConfigPath);
  }
  IasConfigFile *configFile = IasConfigFile::getInstance();
  ASSERT_TRUE(configFile != nullptr);
  configFile->load();
}

TEST_F(IasSmartX_API_Test, config_file_default_paths)
{
  std::string filePath = {std::string(SMARTX_CONFIG_DIR) + "./smartx_config.txt"};
  fs::path fullConfigPath;
  fullConfigPath = filePath.c_str();
  ASSERT_FALSE(fs::exists(fullConfigPath));
  ofstream outfile(filePath);
  outfile << "# empty file" << endl;
  outfile.close();
  IasConfigFile *configFile = IasConfigFile::getInstance();
  ASSERT_TRUE(configFile != nullptr);
  configFile->load();
  ASSERT_TRUE(fs::exists(fullConfigPath));
  fs::remove(fullConfigPath);
}

TEST_F(IasSmartX_API_Test, config_file_env_var_fail)
{
  setenv("SMARTX_CFG_DIR", "./test", true);
  IasConfigFile *configFile = IasConfigFile::getInstance();
  ASSERT_TRUE(configFile != nullptr);
  configFile->load();
}

TEST_F(IasSmartX_API_Test, config_file_env_var_fail_too_long)
{
  setenv("SMARTX_CFG_DIR", "0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789", true);
  IasConfigFile *configFile = IasConfigFile::getInstance();
  ASSERT_TRUE(configFile != nullptr);
  configFile->load();
}

TEST_F(IasSmartX_API_Test, config_file_env_var_cfs)
{
  setenv("SMARTX_CFG_DIR", std::string(std::string(SMARTX_CONFIG_DIR) + "cfs").c_str(), true);
  IasConfigFile *configFile = IasConfigFile::getInstance();
  ASSERT_TRUE(configFile != nullptr);
  EXPECT_EQ(SCHED_FIFO, configFile->getSchedPolicy());
  EXPECT_EQ(20u, configFile->getSchedPriority());
  configFile->load();
  EXPECT_EQ(SCHED_OTHER, configFile->getSchedPolicy());
  EXPECT_EQ(0u, configFile->getSchedPriority());
  std::vector<uint32_t> cpuAffinities = configFile->getCpuAffinities();
  EXPECT_EQ(0u, cpuAffinities.size());

  // Load again
  configFile->load();
  EXPECT_EQ(SCHED_OTHER, configFile->getSchedPolicy());
  EXPECT_EQ(0u, configFile->getSchedPriority());
  cpuAffinities = configFile->getCpuAffinities();
  EXPECT_EQ(0u, cpuAffinities.size());
}

TEST_F(IasSmartX_API_Test, config_file_unregistered_key)
{
  // This config file contains a key/value pair whose key is unregistered
  setenv("SMARTX_CFG_DIR", std::string(std::string(SMARTX_CONFIG_DIR) + "unregistered_key").c_str(), true);
  IasConfigFile *configFile = IasConfigFile::getInstance();
  ASSERT_TRUE(configFile != nullptr);
  configFile->load();
  EXPECT_EQ(SCHED_RR, configFile->getSchedPolicy());
  EXPECT_EQ(30u, configFile->getSchedPriority());
  std::vector<uint32_t> cpuAffinities = configFile->getCpuAffinities();
  EXPECT_EQ(0u, cpuAffinities.size());

  // Try to access the unregistered key/value pair
  std::string value = configFile->getKey("scheduling.rt.unregisteredkey");
  EXPECT_EQ("10", value);

  value = configFile->getKey("invalidkey");
  EXPECT_EQ("", value);
}

TEST_F(IasSmartX_API_Test, config_file_env_var_cfs_1)
{
  setenv("SMARTX_CFG_DIR", std::string(std::string(SMARTX_CONFIG_DIR) + "cfs_1").c_str(), true);
  IasConfigFile *configFile = IasConfigFile::getInstance();
  ASSERT_TRUE(configFile != nullptr);
  configFile->load();
  EXPECT_EQ(SCHED_OTHER, configFile->getSchedPolicy());
  EXPECT_EQ(25u, configFile->getSchedPriority());
}

TEST_F(IasSmartX_API_Test, config_file_env_var_rr)
{
  setenv("SMARTX_CFG_DIR", std::string(std::string(SMARTX_CONFIG_DIR) + "rr").c_str(), true);
  IasConfigFile *configFile = IasConfigFile::getInstance();
  ASSERT_TRUE(configFile != nullptr);
  configFile->load();
  EXPECT_EQ(SCHED_RR, configFile->getSchedPolicy());
  EXPECT_EQ(30u, configFile->getSchedPriority());
}

TEST_F(IasSmartX_API_Test, config_file_env_var_fifo)
{
  setenv("SMARTX_CFG_DIR", std::string(std::string(SMARTX_CONFIG_DIR) + "fifo").c_str(), true);
  IasConfigFile *configFile = IasConfigFile::getInstance();
  ASSERT_TRUE(configFile != nullptr);
  configFile->load();
  EXPECT_EQ(SCHED_FIFO, configFile->getSchedPolicy());
  EXPECT_EQ(30u, configFile->getSchedPriority());
  std::vector<uint32_t> cpuAffinities = configFile->getCpuAffinities();
  EXPECT_EQ(4u, cpuAffinities.size());
  for (auto &entry : cpuAffinities)
  {
    cout << entry << " ";
  }
  cout << endl;
}

TEST_F(IasSmartX_API_Test, config_file_env_var_fifo_100)
{
  setenv("SMARTX_CFG_DIR", std::string(std::string(SMARTX_CONFIG_DIR) + "fifo_100").c_str(), true);
  IasConfigFile *configFile = IasConfigFile::getInstance();
  ASSERT_TRUE(configFile != nullptr);
  configFile->load();
  EXPECT_EQ(SCHED_FIFO, configFile->getSchedPolicy());
  EXPECT_EQ(0u, configFile->getSchedPriority());
}

TEST_F(IasSmartX_API_Test, config_file_env_var_inv)
{
  setenv("SMARTX_CFG_DIR", std::string(std::string(SMARTX_CONFIG_DIR) + "inv").c_str(), true);
  IasConfigFile *configFile = IasConfigFile::getInstance();
  ASSERT_TRUE(configFile != nullptr);
  configFile->load();
  EXPECT_EQ(SCHED_FIFO, configFile->getSchedPolicy());
  EXPECT_EQ(30u, configFile->getSchedPriority());
}

TEST_F(IasSmartX_API_Test, config_file_env_var_empty_file)
{
  setenv("SMARTX_CFG_DIR", std::string(std::string(SMARTX_CONFIG_DIR) + "empty_file").c_str(), true);
  IasConfigFile *configFile = IasConfigFile::getInstance();
  ASSERT_TRUE(configFile != nullptr);
  configFile->load();
  EXPECT_EQ(SCHED_FIFO, configFile->getSchedPolicy());
  EXPECT_EQ(20u, configFile->getSchedPriority());
}

TEST_F(IasSmartX_API_Test, config_file_env_var_corrupt_file)
{
  setenv("SMARTX_CFG_DIR", std::string(std::string(SMARTX_CONFIG_DIR) + "corrupt").c_str(), true);
  IasConfigFile *configFile = IasConfigFile::getInstance();
  ASSERT_TRUE(configFile != nullptr);
  configFile->load();
  EXPECT_EQ(SCHED_FIFO, configFile->getSchedPolicy());
  EXPECT_EQ(30u, configFile->getSchedPriority());
}

TEST_F(IasSmartX_API_Test, config_file_env_var_cpu_affinities_1)
{
  setenv("SMARTX_CFG_DIR", std::string(std::string(SMARTX_CONFIG_DIR) + "cpu_affinities_1").c_str(), true);
  IasConfigFile *configFile = IasConfigFile::getInstance();
  ASSERT_TRUE(configFile != nullptr);
  configFile->load();
  setShallFail = false;
  getShallFail = false;
  setAffinityShallFail = false;
  DltContext *logCtx = IasAudioLogging::registerDltContext("TST", "Test Context");
  IasConfigFile::configureThreadSchedulingParameters(logCtx);
  IasConfigFile::configureThreadSchedulingParameters(logCtx, IasRunnerThreadSchedulePriority::eIasPriorityOneLess);
  const std::vector<uint32_t>& cpus = configFile->getCpuAffinities();
  EXPECT_EQ(5u, cpus.size());
}

TEST_F(IasSmartX_API_Test, config_file_env_var_cpu_affinities_2)
{
  setenv("SMARTX_CFG_DIR", std::string(std::string(SMARTX_CONFIG_DIR) + "cpu_affinities_1").c_str(), true);
  IasConfigFile *configFile = IasConfigFile::getInstance();
  ASSERT_TRUE(configFile != nullptr);
  configFile->load();
  const std::vector<uint32_t>& cpus = configFile->getCpuAffinities();
  EXPECT_EQ(5u, cpus.size());
  DltContext *logCtx = IasAudioLogging::registerDltContext("TST", "Test Context");
  setShallFail = false;
  getShallFail = false;
  setAffinityShallFail = true;
  IasConfigFile::configureThreadSchedulingParameters(logCtx);
}

TEST_F(IasSmartX_API_Test, config_file_runner_all_disabled)
{
  // This config file contains a key/value pair whose key is unregistered
  setenv("SMARTX_CFG_DIR", std::string(std::string(SMARTX_CONFIG_DIR) + "runner_all_disabled").c_str(), true);
  IasConfigFile *configFile = IasConfigFile::getInstance();
  ASSERT_TRUE(configFile != nullptr);
  configFile->load();
  EXPECT_EQ(IasConfigFile::eIasDisabled, configFile->getRunnerThreadState("MyRoutingZone1"));
  EXPECT_EQ(IasConfigFile::eIasDisabled, configFile->getRunnerThreadState("MyRoutingZone25"));
  EXPECT_EQ(IasConfigFile::eIasDisabled, configFile->getRunnerThreadState("19234_sink_I"));
}

TEST_F(IasSmartX_API_Test, config_file_runner_all_enabled)
{
  // This config file contains a key/value pair whose key is unregistered
  setenv("SMARTX_CFG_DIR", std::string(std::string(SMARTX_CONFIG_DIR) + "runner_all_enabled").c_str(), true);
  IasConfigFile *configFile = IasConfigFile::getInstance();
  ASSERT_TRUE(configFile != nullptr);
  configFile->load();
  EXPECT_EQ(IasConfigFile::eIasEnabled, configFile->getRunnerThreadState("MyRoutingZone1"));
  EXPECT_EQ(IasConfigFile::eIasEnabled, configFile->getRunnerThreadState("MyRoutingZone25"));
  EXPECT_EQ(IasConfigFile::eIasEnabled, configFile->getRunnerThreadState("19234_sink_I"));
  EXPECT_EQ(IasConfigFile::eIasEnabled, configFile->getRunnerThreadState("AdditionalOne"));
}

TEST_F(IasSmartX_API_Test, config_file_runner_not_included)
{
  // This config file contains a key/value pair whose key is unregistered
  setenv("SMARTX_CFG_DIR", std::string(std::string(SMARTX_CONFIG_DIR) + "runner_not_included").c_str(), true);
  IasConfigFile *configFile = IasConfigFile::getInstance();
  ASSERT_TRUE(configFile != nullptr);
  configFile->load();
  EXPECT_EQ(IasConfigFile::eIasDisabled, configFile->getRunnerThreadState("MyRoutingZone1"));
  EXPECT_EQ(IasConfigFile::eIasDisabled, configFile->getRunnerThreadState("MyRoutingZone25"));
  EXPECT_EQ(IasConfigFile::eIasDisabled, configFile->getRunnerThreadState("19234_sink_I"));
  EXPECT_EQ(IasConfigFile::eIasDisabled, configFile->getRunnerThreadState("AdditionalOne"));
}

TEST_F(IasSmartX_API_Test, config_file_runner_specific_enabled)
{
  // This config file contains a key/value pair whose key is unregistered
  setenv("SMARTX_CFG_DIR", std::string(std::string(SMARTX_CONFIG_DIR) + "runner_specific_enabled").c_str(), true);
  IasConfigFile *configFile = IasConfigFile::getInstance();
  ASSERT_TRUE(configFile != nullptr);
  configFile->load();
  EXPECT_EQ(IasConfigFile::eIasEnabled, configFile->getRunnerThreadState("MyRoutingZone1"));
  EXPECT_EQ(IasConfigFile::eIasEnabled, configFile->getRunnerThreadState("MyRoutingZone25"));
  EXPECT_EQ(IasConfigFile::eIasEnabled, configFile->getRunnerThreadState("19234_sink_I"));
  EXPECT_EQ(IasConfigFile::eIasDisabled, configFile->getRunnerThreadState("AdditionalOne"));
}

TEST_F(IasSmartX_API_Test, config_file_set_sched_params_too_long)
{
  DltContext *logCtx = IasAudioLogging::registerDltContext("TST", "Test Context");
  IasConfigFile::configureThreadSchedulingParameters(logCtx);
}

TEST_F(IasSmartX_API_Test, config_file_set_sched_params)
{
  DltContext *logCtx = IasAudioLogging::registerDltContext("TST", "Test Context");
  setShallFail = false;
  getShallFail = false;
  setAffinityShallFail = false;
  IasConfigFile::configureThreadSchedulingParameters(logCtx);
}

TEST_F(IasSmartX_API_Test, config_file_set_sched_params_fail)
{
  DltContext *logCtx = IasAudioLogging::registerDltContext("TST", "Test Context");
  setShallFail = true;
  getShallFail = false;
  setAffinityShallFail = false;
  IasConfigFile::configureThreadSchedulingParameters(logCtx);
}

TEST_F(IasSmartX_API_Test, config_file_get_sched_params_fail)
{
  DltContext *logCtx = IasAudioLogging::registerDltContext("TST", "Test Context");
  setShallFail = false;
  getShallFail = true;
  setAffinityShallFail = false;
  IasConfigFile::configureThreadSchedulingParameters(logCtx);
}

TEST_F(IasSmartX_API_Test, toStringTester)
{
  EXPECT_EQ("IasConfiguration::eIasOk", toString(IasConfiguration::eIasOk));
  EXPECT_EQ("IasIProcessing::eIasOk", toString(IasIProcessing::eIasOk));
  EXPECT_EQ("IasIRouting::eIasOk", toString(IasIRouting::eIasOk));
  EXPECT_EQ("IasISetup::eIasOk", toString(IasISetup::eIasOk));
  EXPECT_EQ("IasSmartX::eIasOk", toString(IasSmartX::eIasOk));
  EXPECT_EQ("IasSmartXClient::eIasOk", toString(IasSmartXClient::eIasOk));
  EXPECT_EQ("IasSmartXPriv::eIasOk", toString(IasSmartXPriv::eIasOk));
  EXPECT_EQ("IasSetupEvent::eIasUnrecoverableSinkDeviceError", toString(IasSetupEvent::eIasUnrecoverableSinkDeviceError));
}

/**
 * Negative test for the following methods of IasIDebug:
 *
 * startProbing
 *
 */
TEST_F(IasSmartX_API_Test, debug_api_fail_probing)
{
  setShallFail = false;
  getShallFail = false;
  setAffinityShallFail = false;

  const std::string file ="dummyFile";
  const uint32_t numSeconds = 10;
  const std::string portName = "dummyPort";

  IasISetup::IasResult stpRes;
  IasIDebug::IasResult dbgRes;
  IasIRouting::IasResult rtRes;
  IasRoutingZone::IasResult rznRes;

  IasSmartX *smartx = IasSmartX::create();
  ASSERT_TRUE(smartx != nullptr);
  IasIDebug *debug = smartx->debug();
  ASSERT_TRUE(debug != nullptr);
  IasISetup *setup = smartx->setup();
  ASSERT_TRUE(setup != nullptr);
  IasIRouting *routing = smartx->routing();
  ASSERT_TRUE(routing != nullptr);

  IasAudioSinkDevicePtr sink = nullptr;
  IasAudioSourceDevicePtr source = nullptr;
  IasAudioPortPtr inPort = nullptr;
  IasAudioPortPtr outPort = nullptr;
  IasRoutingZonePtr routingZone = nullptr;

  IasAudioDeviceParams sourceParams("MySource",2,48000,eIasFormatInt16,eIasClockProvided,128,4);
  IasAudioDeviceParams sinkParams(ALSA_DEVICE_NAME,2,48000,eIasFormatInt16,eIasClockReceived,128,4);

  stpRes = IasSetupHelper::createAudioSinkDevice(setup, sinkParams, 43, &sink, &routingZone);
  EXPECT_EQ(IasISetup::eIasOk, stpRes);

  stpRes = IasSetupHelper::createAudioSourceDevice(setup, sourceParams, 42, &source);
  EXPECT_EQ(IasISetup::eIasOk, stpRes);

  rznRes = routingZone->start();
  EXPECT_EQ(IasRoutingZone::eIasOk, rznRes);

  rtRes = routing->connect(42,43);
  EXPECT_EQ(rtRes, IasIRouting::eIasOk);
  sleep(1); //let the routing zone thread run to trigger switch matrix

  dbgRes = debug->startRecord(file,"dummy", numSeconds);
  EXPECT_EQ(dbgRes, IasIDebug::eIasFailed);

  dbgRes = debug->startRecord("/dummyFile","MySource_port", numSeconds);
  EXPECT_EQ(dbgRes, IasIDebug::eIasFailed);

  std::string tmp = "test";
  tmp.resize(256,'a');

  dbgRes = debug->startRecord(tmp,"MySource_port", numSeconds);
  EXPECT_EQ(dbgRes, IasIDebug::eIasOk);
  dbgRes = debug->stopProbing("MySource_port");
  EXPECT_EQ(dbgRes, IasIDebug::eIasOk);

  tmp.resize(5);

  dbgRes = debug->startRecord(file,"MySource_port", numSeconds);
  EXPECT_EQ(dbgRes, IasIDebug::eIasOk);
  dbgRes = debug->stopProbing("MySource_port");
  EXPECT_EQ(dbgRes, IasIDebug::eIasOk);

  const std::string sinkPortName = std::string(ALSA_DEVICE_NAME) + "_port";
  const std::string sinkRzPortName = std::string(ALSA_DEVICE_NAME) + "_rznport";
  dbgRes = debug->startRecord(file,sinkPortName, numSeconds);
  EXPECT_EQ(dbgRes, IasIDebug::eIasOk);
  dbgRes = debug->stopProbing(sinkPortName);
  EXPECT_EQ(dbgRes, IasIDebug::eIasOk);

  dbgRes = debug->startRecord(file,sinkRzPortName, numSeconds);
  EXPECT_EQ(dbgRes, IasIDebug::eIasOk);
  dbgRes = debug->stopProbing(sinkRzPortName);
  EXPECT_EQ(dbgRes, IasIDebug::eIasOk);


  dbgRes = debug->startInject(file,sinkPortName, numSeconds);
  EXPECT_EQ(dbgRes, IasIDebug::eIasOk);
  dbgRes = debug->stopProbing(sinkPortName);
  EXPECT_EQ(dbgRes, IasIDebug::eIasOk);

  dbgRes = debug->startInject(file,sinkRzPortName, numSeconds);
  EXPECT_EQ(dbgRes, IasIDebug::eIasOk);
  dbgRes = debug->stopProbing(sinkRzPortName);
  EXPECT_EQ(dbgRes, IasIDebug::eIasOk);

  dbgRes = debug->stopProbing("non_existing");
  EXPECT_EQ(dbgRes, IasIDebug::eIasFailed);

  rznRes = routingZone->stop();
  EXPECT_EQ(IasRoutingZone::eIasOk, rznRes);



  IasSmartX::destroy(smartx);

  sink = nullptr;
  source = nullptr;
  inPort = nullptr;
  outPort = nullptr;
  routingZone = nullptr;

}

/**
 * Negative test for the following methods of IasIDebug:
 *
 * startProbing
 *
 */
TEST_F(IasSmartX_API_Test, debug_api_mutex_decorator_fail_probing)
{
  setShallFail = false;
  getShallFail = false;
  setAffinityShallFail = false;

  const std::string file ="dummyFile";
  const uint32_t numSeconds = 10;
  const std::string portName = "dummyPort";

  IasISetup::IasResult stpRes;
  IasIDebug::IasResult dbgRes;
  IasIRouting::IasResult rtRes;
  IasRoutingZone::IasResult rznRes;

  IasSmartX *smartx = IasSmartX::create();
  ASSERT_TRUE(smartx != nullptr);
  IasIDebug *debug = new IasDebugMutexDecorator(smartx->debug());
  ASSERT_TRUE(debug != nullptr);
  IasISetup *setup = smartx->setup();
  ASSERT_TRUE(setup != nullptr);
  IasIRouting *routing = smartx->routing();
  ASSERT_TRUE(routing != nullptr);

  IasAudioSinkDevicePtr sink = nullptr;
  IasAudioSourceDevicePtr source = nullptr;
  IasAudioPortPtr inPort = nullptr;
  IasAudioPortPtr outPort = nullptr;
  IasRoutingZonePtr routingZone = nullptr;

  IasAudioDeviceParams sourceParams("MySource",2,48000,eIasFormatInt16,eIasClockProvided,128,4);
  IasAudioDeviceParams sinkParams(ALSA_DEVICE_NAME,2,48000,eIasFormatInt16,eIasClockReceived,128,4);

  stpRes = IasSetupHelper::createAudioSinkDevice(setup, sinkParams, 43, &sink, &routingZone);
  EXPECT_EQ(IasISetup::eIasOk, stpRes);

  stpRes = IasSetupHelper::createAudioSourceDevice(setup, sourceParams, 42, &source);
  EXPECT_EQ(IasISetup::eIasOk, stpRes);

  rznRes = routingZone->start();
  EXPECT_EQ(IasRoutingZone::eIasOk, rznRes);

  rtRes = routing->connect(42,43);
  EXPECT_EQ(rtRes, IasIRouting::eIasOk);
  usleep(3000); //let the routing zone thread run to trigger switch matrx
  IasSwitchMatrixPtr switchMatrix = routingZone->getSwitchMatrix();

  dbgRes = debug->startRecord(file,"dummy", numSeconds);
  EXPECT_EQ(dbgRes, IasIDebug::eIasFailed);

  dbgRes = debug->startRecord("/dummyFile","MySource_port", numSeconds);
  EXPECT_EQ(dbgRes, IasIDebug::eIasFailed);

  std::string tmp = "test";
  tmp.resize(256,'a');

  dbgRes = debug->startRecord(tmp,"MySource_port", numSeconds);
  EXPECT_EQ(dbgRes, IasIDebug::eIasOk);
  dbgRes = debug->stopProbing("MySource_port");
  EXPECT_EQ(dbgRes, IasIDebug::eIasOk);

  tmp.resize(5);

  dbgRes = debug->startRecord(file,"MySource_port", numSeconds);
  EXPECT_EQ(dbgRes, IasIDebug::eIasOk);
  dbgRes = debug->stopProbing("MySource_port");
  EXPECT_EQ(dbgRes, IasIDebug::eIasOk);

  const std::string sinkPortName = std::string(ALSA_DEVICE_NAME) + "_port";
  const std::string sinkRzPortName = std::string(ALSA_DEVICE_NAME) + "_port";
  dbgRes = debug->startRecord(file,sinkPortName, numSeconds);
  EXPECT_EQ(dbgRes, IasIDebug::eIasOk);
  dbgRes = debug->stopProbing(sinkPortName);
  EXPECT_EQ(dbgRes, IasIDebug::eIasOk);

  dbgRes = debug->startRecord(file,sinkRzPortName, numSeconds);
  EXPECT_EQ(dbgRes, IasIDebug::eIasOk);
  dbgRes = debug->stopProbing(sinkRzPortName);
  EXPECT_EQ(dbgRes, IasIDebug::eIasOk);


  dbgRes = debug->startInject(file,sinkPortName, numSeconds);
  EXPECT_EQ(dbgRes, IasIDebug::eIasOk);
  dbgRes = debug->stopProbing(sinkPortName);
  EXPECT_EQ(dbgRes, IasIDebug::eIasOk);

  dbgRes = debug->startInject(file,sinkRzPortName, numSeconds);
  EXPECT_EQ(dbgRes, IasIDebug::eIasOk);
  dbgRes = debug->stopProbing(sinkRzPortName);
  EXPECT_EQ(dbgRes, IasIDebug::eIasOk);

  dbgRes = debug->stopProbing("non_existing");
  EXPECT_EQ(dbgRes, IasIDebug::eIasFailed);

  rznRes = routingZone->stop();
  EXPECT_EQ(IasRoutingZone::eIasOk, rznRes);

  IasSmartX::destroy(smartx);

  // We create and destroy another IasDebugMutexDecorator for being able to call the destructor.
  // We cannot delete the previously generated IasDebugMutexDecorator, because this would also delete the given
  // IasIDebug instance, but this is already deleted by calling IasSmartX::destroy.
  // Normally this IasDebugMutexDecorator is used with the LD_PRELOAD feature by overwriting the debugHook and that's why
  // the delete in the destructor works in that use-case.
  IasIDebug *dummyDebugInstance = new IasDebugMutexDecorator(nullptr);
  delete dummyDebugInstance;

  sink = nullptr;
  source = nullptr;
  inPort = nullptr;
  outPort = nullptr;
  routingZone = nullptr;

}

TEST_F(IasSmartX_API_Test, VolumeModuleCreation)
{
  setenv("AUDIO_PLUGIN_DIR", AUDIO_PLUGIN_DIR, true);
  IasSmartX *smartx = IasSmartX::create();
  ASSERT_TRUE(smartx != nullptr);
  IasISetup *setup = smartx->setup();
  ASSERT_TRUE(setup != nullptr);

  // Create volume module
  IasISetup::IasResult setres;
  IasProcessingModuleParams moduleParams;
  moduleParams.typeName = "ias.volume";
  moduleParams.instanceName = "MyVolume";
  IasProcessingModulePtr volume = nullptr;
  setres = setup->createProcessingModule(moduleParams, &volume);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  ASSERT_TRUE(volume != nullptr);

  // Set volume properties
  IasProperties volumeProperties;
  volumeProperties.set<int32_t>("numFilterBands", 3);
  setup->setProperties(volume, volumeProperties);

  // Create pipeline
  IasPipelineParams pipelineParams;
  pipelineParams.name = "MyPipeline";
  pipelineParams.periodSize = 192;
  pipelineParams.samplerate = 48000;
  IasPipelinePtr pipeline = nullptr;
  setres = setup->createPipeline(pipelineParams, &pipeline);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  // Get pipeline again
  IasPipelinePtr testPipeline = setup->getPipeline("MyPipelineDoesNotExist");
  ASSERT_EQ(nullptr, testPipeline);
  testPipeline = setup->getPipeline("MyPipeline");
  ASSERT_EQ(pipeline, testPipeline);

  // Create pipeline input pin
  IasAudioPinPtr pipelineInputPin = nullptr;
  IasAudioPinParams pipelineInputParams;
  pipelineInputParams.name = "pipelineInput";
  pipelineInputParams.numChannels = 2;
  setres = setup->createAudioPin(pipelineInputParams, &pipelineInputPin);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  ASSERT_TRUE(pipelineInputPin != nullptr);

  IasAudioPinVector pins = setup->getAudioPins();
  ASSERT_EQ(pins.size(), 1u);

  // Create pipeline output pins
  IasAudioPinPtr pipelineOutputPin = nullptr;
  IasAudioPinParams pipelineOutputParams;
  pipelineOutputParams.name = "pipelineOutput";
  pipelineOutputParams.numChannels = 2;
  setres = setup->createAudioPin(pipelineOutputParams, &pipelineOutputPin);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  ASSERT_TRUE(pipelineOutputPin != nullptr);

  IasAudioPinPtr pipelineOutputPin2 = nullptr;
  IasAudioPinParams pipelineOutputParams2;
  pipelineOutputParams2.name = "pipelineOutput2";
  pipelineOutputParams2.numChannels = 1;
  setres = setup->createAudioPin(pipelineOutputParams2, &pipelineOutputPin2);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  ASSERT_TRUE(pipelineOutputPin2 != nullptr);

  IasAudioPinPtr pipelineOutputPin3 = nullptr;
  IasAudioPinParams pipelineOutputParams3;
  pipelineOutputParams3.name = "pipelineOutput3";
  pipelineOutputParams3.numChannels = 2;
  setres = setup->createAudioPin(pipelineOutputParams3, &pipelineOutputPin3);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  ASSERT_TRUE(pipelineOutputPin3 != nullptr);

  // Add input pin to pipeline
  setres = setup->addAudioInputPin(pipeline, pipelineInputPin);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  std::string dir = IasSetupHelper::getPinDirection(pipelineInputPin);
  ASSERT_EQ(dir,"IasAudioPin::eIasPinDirectionPipelineInput");
  dir.clear();
  dir = IasSetupHelper::getPinDirection(nullptr);
  ASSERT_EQ(dir,"");
  dir.clear();

  std::string name = IasSetupHelper::getPinName(pipelineInputPin);
  ASSERT_EQ(name,"pipelineInput");
  name.clear();
  name = IasSetupHelper::getPinName(nullptr);
  ASSERT_EQ(name,"");

  uint32_t numChan = IasSetupHelper::getPinNumChannels(pipelineInputPin);
  ASSERT_EQ(numChan,2u);
  numChan = IasSetupHelper::getPinNumChannels(nullptr);
  ASSERT_EQ(numChan,0u);

  // Add output pins to pipeline
  setres = setup->addAudioOutputPin(pipeline, pipelineOutputPin);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setres = setup->addAudioOutputPin(pipeline, pipelineOutputPin2);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  ASSERT_TRUE(pipeline != nullptr);

  // Add processing module to pipeline
  setres = setup->addProcessingModule(pipeline, volume);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  // Create input/output pin for volume module
  IasAudioPinPtr volumePin = nullptr;
  IasAudioPinParams pinParams;
  pinParams.name = "stereo0";
  pinParams.numChannels = 2;
  setres = setup->createAudioPin(pinParams, &volumePin);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  ASSERT_TRUE(volumePin != nullptr);

  // Add input/output pin to module
  setres = setup->addAudioInOutPin(volume, volumePin);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  // Define the signal flow through the pipeline
  setres = setup->link(pipelineInputPin, volumePin, eIasAudioPinLinkTypeImmediate);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setres = setup->link(volumePin, pipelineOutputPin, eIasAudioPinLinkTypeImmediate);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  // Initialize the pipeline
  setres = setup->initPipelineAudioChain(pipeline);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  // Create audio sink1
  IasAudioSinkDevicePtr sink1 = nullptr;
  IasAudioDeviceParams sink1DeviceParams =
  {
    "mySink1",
    2,
    48000,
    eIasFormatInt16,
    eIasClockProvided,
    192,
    4
  };
  setres = setup->createAudioSinkDevice(sink1DeviceParams, &sink1);
  EXPECT_EQ(IasISetup::eIasOk, setres);
  EXPECT_TRUE(sink1 != nullptr);

  // Create sink port and add to sink device
  IasAudioPortParams sink1PortParams =
  {
    "mySink1Port",
    2,
    123,
    eIasPortDirectionInput,
    0
  };
  IasAudioPortPtr sink1Port = nullptr;
  setres = setup->createAudioPort(sink1PortParams, &sink1Port);
  EXPECT_EQ(IasISetup::eIasOk, setres);
  EXPECT_TRUE(sink1Port != nullptr);
  setres = setup->addAudioInputPort(sink1, sink1Port);
  EXPECT_EQ(IasISetup::eIasOk, setres);

  // Create a routing zone
  IasRoutingZoneParams rzparams1 =
  {
    "routingZone1"
  };
  IasRoutingZonePtr routingZone1 = nullptr;
  setres = setup->createRoutingZone(rzparams1, &routingZone1);
  EXPECT_EQ(IasISetup::eIasOk, setres);
  EXPECT_TRUE(routingZone1 != nullptr);

  // Link the routing zone with the sink
  setres = setup->link(routingZone1, sink1);
  EXPECT_EQ(IasISetup::eIasOk, setres);

  setres = setup->addPipeline(routingZone1, pipeline);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  IasAudioPortPtr dummyPort = nullptr;
  IasAudioPinPtr dummyPin = nullptr;
  setres = setup->link(dummyPort, dummyPin);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  setres = setup->link(sink1Port, dummyPin);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  IasAudioPortParams dummyPortParams =
  {
    "myDummyOutputPort",
    2,
    124,
    eIasPortDirectionOutput,
    0
  };
  setres = setup->createAudioPort(dummyPortParams, &dummyPort);
  EXPECT_EQ(IasISetup::eIasOk, setres);
  setres = setup->link(dummyPort, pipelineOutputPin);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  setup->destroyAudioPort(dummyPort);
  dummyPortParams.direction = eIasPortDirectionInput;
  setres = setup->createAudioPort(dummyPortParams, &dummyPort);
  EXPECT_EQ(IasISetup::eIasOk, setres);
  setres = setup->link(dummyPort, pipelineOutputPin);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  setres = setup->addAudioInputPort(routingZone1, dummyPort);
  EXPECT_EQ(IasISetup::eIasOk, setres);
  setres = setup->link(dummyPort, pipelineOutputPin);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  setres = setup->link(sink1Port, pipelineInputPin);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  setres = setup->link(sink1Port, pipelineOutputPin2);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  setres = setup->link(sink1Port, pipelineOutputPin3);
  ASSERT_EQ(IasISetup::eIasFailed, setres);

  setres = setup->link(sink1Port, pipelineOutputPin);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setup->unlink(sink1Port, pipelineOutputPin);

  // Define the signal flow through the pipeline
  setup->unlink(pipelineInputPin, volumePin);

  setup->unlink(volumePin, pipelineOutputPin);


  IasSmartX::destroy(smartx);
}

TEST_F(IasSmartX_API_Test, configuration)
{
  IasConfiguration* myConfig = new IasConfiguration();
  int32_t numPipes = myConfig->getNumberPipelines();
  ASSERT_TRUE(numPipes == 0);

  IasPipelinePtr pipe = nullptr;

  IasConfiguration::IasResult res = myConfig->getPipeline("Kasperl", static_cast<IasPipelinePtr*>(nullptr));
  ASSERT_TRUE(res == IasConfiguration::eIasNullPointer);


  delete myConfig;
}

TEST_F(IasSmartX_API_Test, add_port_index_num_channels_out_of_range)
{
  // Create the smartx instance
  IasSmartX *smartx = IasSmartX::create();
  ASSERT_TRUE(smartx != nullptr);

  // Get the smartx setup
  IasISetup *setup = smartx->setup();
  ASSERT_TRUE(setup != nullptr);

  IasISetup::IasResult result;
  IasAudioSinkDevicePtr sink = nullptr;
  IasAudioPortPtr inPort = nullptr;
  IasAudioDeviceParams sinkParams;
  sinkParams.clockType = eIasClockProvided;
  sinkParams.dataFormat = eIasFormatInt16;
  sinkParams.name = "MySink";
  sinkParams.numChannels = 2;
  sinkParams.numPeriods = 4;
  sinkParams.periodSize = 128;
  sinkParams.samplerate = 48000;
  result = setup->createAudioSinkDevice(sinkParams, &sink);
  EXPECT_EQ(IasISetup::eIasOk, result);
  IasAudioPortParams portParams;
  portParams.direction = eIasPortDirectionInput;
  portParams.id = 1234;
  portParams.index = 1;
  portParams.name = "sink_in_port";
  portParams.numChannels = 2;
  result = setup->createAudioPort(portParams, &inPort);
  EXPECT_EQ(IasISetup::eIasOk, result);
  result = setup->addAudioInputPort(sink, inPort);
  EXPECT_EQ(IasISetup::eIasFailed, result);

  portParams.name = "zone_in_port";
  result = setup->createAudioPort(portParams, &inPort);
  EXPECT_EQ(IasISetup::eIasOk, result);

  IasRoutingZonePtr rzn = nullptr;
  IasRoutingZoneParams rznParams;
  rznParams.name = "MyRzn";
  result = setup->createRoutingZone(rznParams, &rzn);
  EXPECT_EQ(IasISetup::eIasOk, result);
  result = setup->addAudioInputPort(rzn, inPort);
  EXPECT_EQ(IasISetup::eIasFailed, result);

  IasAudioSourceDevicePtr source = nullptr;
  IasAudioDeviceParams sourceParams;
  sourceParams.clockType = eIasClockProvided;
  sourceParams.dataFormat = eIasFormatInt16;
  sourceParams.name = "MySource";
  sourceParams.numChannels = 2;
  sourceParams.numPeriods = 4;
  sourceParams.periodSize = 128;
  sourceParams.samplerate = 48000;
  result = setup->createAudioSourceDevice(sourceParams, &source);
  EXPECT_EQ(IasISetup::eIasOk, result);
  portParams.direction = eIasPortDirectionOutput;
  portParams.name = "source_out_port";
  IasAudioPortPtr outPort = nullptr;
  result = setup->createAudioPort(portParams, &outPort);
  EXPECT_EQ(IasISetup::eIasOk, result);
  result = setup->addAudioOutputPort(source, outPort);
  EXPECT_EQ(IasISetup::eIasFailed, result);

  IasSmartX::destroy(smartx);
}

TEST_F(IasSmartX_API_Test, IasProcessingImpl)
{
  IasCmdDispatcherPtr cmdDispatcher = std::make_shared<IasCmdDispatcher>();
  ASSERT_TRUE(cmdDispatcher != nullptr);
  IasProcessingImpl *procImpl = new IasProcessingImpl(cmdDispatcher);
  ASSERT_TRUE(procImpl != nullptr);

  IasProperties cmdProp;
  IasProperties retProp;
  IasIProcessing::IasResult res = procImpl->sendCmd("invalid_instance", cmdProp, retProp);
  ASSERT_EQ(IasIProcessing::IasResult::eIasFailed, res);

  delete procImpl;
}

TEST_F(IasSmartX_API_Test, extending_coverage_part_one)
{
  IasSmartX *smartx = IasSmartX::create();
  ASSERT_TRUE(smartx != nullptr);
  IasISetup *setup = smartx->setup();
  ASSERT_TRUE(setup != nullptr);

  // volume module parameters
  IasProcessingModuleParams volumeModuleParams;
  volumeModuleParams.typeName     = "ias.volume";
  volumeModuleParams.instanceName = "VolumeLoudness";

  IasPipelineParams pipelineParams;
  pipelineParams.name ="ExamplePipeline";
  pipelineParams.samplerate = 48000;
  pipelineParams.periodSize = 192;
  IasPipelinePtr pipeline = nullptr;

  //volume module pointer
  IasProcessingModulePtr volume = nullptr;

  //setres variable with operation result
  IasISetup::IasResult setres;

  // Create input/output pin for volume module
  IasAudioPinPtr volumePin = nullptr;
  IasAudioPinParams pinParams;
  pinParams.name = "stereo0";
  pinParams.numChannels = 2;
  setres = setup->createAudioPin(pinParams, &volumePin);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  ASSERT_TRUE(volumePin != nullptr);

  // addAudioInOutPin when module = nullpt
  setres = setup->addAudioInOutPin(volume, volumePin);
  ASSERT_EQ(IasISetup::eIasFailed, setres);

  // deleteAudioInOutPin when module = nullptr
  setup->deleteAudioInOutPin(volume, volumePin);

  // addAudioInOutPin when inOutPin = nullptr
  setres = setup->createProcessingModule(volumeModuleParams, &volume);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  volumePin = nullptr;
  setres = setup->addAudioInOutPin(volume, volumePin);
  ASSERT_EQ(IasISetup::eIasFailed, setres);

  // deleteAudioInOutPin when inOutPin = nullptr
  setup->deleteAudioInOutPin(volume, volumePin);

  // addAudioPinMapping when module = nullptr
  volume = nullptr;

  IasAudioPinPtr testOutputPin = nullptr;

  IasAudioPinParams mixerOutputPinParams;
  mixerOutputPinParams.name        = "Mixer_OutputPin_stereo";
  mixerOutputPinParams.numChannels = 2;

  setres = setup->createAudioPin(mixerOutputPinParams, &testOutputPin);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  ASSERT_TRUE(testOutputPin != nullptr);

  setres = setup->addAudioPinMapping(volume, volumePin, testOutputPin);
  ASSERT_EQ(IasISetup::eIasFailed, setres);

  //deleteAudioPinMapping when module = nullptr
  setup->deleteAudioPinMapping(volume, volumePin, testOutputPin);

  // addAudioPinMapping when inputPin = nullptr
  setres = setup->createProcessingModule(volumeModuleParams, &volume);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  volumePin = nullptr;

  setres = setup->addAudioPinMapping(volume, volumePin, testOutputPin);
  ASSERT_EQ(IasISetup::eIasFailed, setres);

  //deleteAudioPinMapping when inputPin = nullptr
  setup->deleteAudioPinMapping(volume, volumePin, testOutputPin);

  // addAudioPinMapping when outputPin = nullptr
  setres = setup->createAudioPin(pinParams, &volumePin);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  ASSERT_TRUE(volumePin != nullptr);

  testOutputPin = nullptr;

  setres = setup->addAudioPinMapping(volume, volumePin, testOutputPin);
  ASSERT_EQ(IasISetup::eIasFailed, setres);

  // deleteAudioPinMapping when outputPin = nullptr
  setup->deleteAudioPinMapping(volume, volumePin, testOutputPin);

  //createProcessingModule when typeName empty
  volumeModuleParams.typeName     = "";
  volumeModuleParams.instanceName = "VolumeLoudness";

  setres = setup->createProcessingModule(volumeModuleParams, &volume);
  ASSERT_EQ(IasISetup::eIasFailed, setres);

  //createProcessingModule when instanceName empty
  volumeModuleParams.typeName     = "ias.volume";
  volumeModuleParams.instanceName = "";

  setres = setup->createProcessingModule(volumeModuleParams, &volume);
  ASSERT_EQ(IasISetup::eIasFailed, setres);

  //createProcessingModule when instanceName module = nullptr
  volume = nullptr;

  setres = setup->createProcessingModule(volumeModuleParams, &volume);
  ASSERT_EQ(IasISetup::eIasFailed, setres);

  // addProcessing module when module = nullptr
  setres = setup->createPipeline(pipelineParams, &pipeline);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  setres = setup->addProcessingModule(pipeline, volume);
  ASSERT_EQ(IasISetup::eIasFailed, setres);

  IasSmartX::destroy(smartx);
}

TEST_F(IasSmartX_API_Test, setup_interface_fails_when_routing_zone_is_active)
{
  IasSmartX *smartx = IasSmartX::create();
  ASSERT_TRUE(smartx != nullptr);
  IasISetup *setup = smartx->setup();
  ASSERT_TRUE(setup != nullptr);

  //create sink device
  IasAudioDeviceParams sinkParams =
  {
    "map_2_4", 4, 48000, eIasFormatInt16, eIasClockReceived, 256, 4
  };
  IasAudioSinkDevicePtr sink = nullptr;
  IasISetup::IasResult setres = setup->createAudioSinkDevice(sinkParams, &sink);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  //create two stereo ports and add them to the sink device as input ports
  IasAudioPortParams sinkInputPortParams1 =
  {
    "sinkInputPort1", 2, -1, eIasPortDirectionInput, 0
  };
  IasAudioPortPtr sinkInputPort1 = nullptr;
  setres = setup->createAudioPort(sinkInputPortParams1, &sinkInputPort1);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setres = setup->addAudioInputPort(sink, sinkInputPort1);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  IasAudioPortParams sinkInputPortParams2 =
  {
    "sinkInputPort2", 2, -1, eIasPortDirectionInput, 0
  };
  IasAudioPortPtr sinkInputPort2 = nullptr;
  setres = setup->createAudioPort(sinkInputPortParams2, &sinkInputPort2);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setres = setup->addAudioInputPort(sink, sinkInputPort2);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  //create routing zone and link it with sink device
  IasRoutingZoneParams routingZoneParams =
  {
    "testRoutingZone"
  };
  IasRoutingZonePtr routingZone = nullptr;
  setres = setup->createRoutingZone(routingZoneParams, &routingZone);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setres = setup->link(routingZone, sink);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  //create two stereo ports and add them to the routing zone as input ports
  IasAudioPortParams zoneInputPortParams1 =
  {
    "zoneInputPort1", 2, -1, eIasPortDirectionInput, 0
  };
  IasAudioPortPtr zoneInputPort1 = nullptr;
  setres = setup->createAudioPort(zoneInputPortParams1, &zoneInputPort1);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setres = setup->addAudioInputPort(routingZone, zoneInputPort1);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  IasAudioPortParams zoneInputPortParams2 =
  {
    "zoneInputPort2", 2, -1, eIasPortDirectionInput, 0
  };
  IasAudioPortPtr zoneInputPort2 = nullptr;
  setres = setup->createAudioPort(zoneInputPortParams2, &zoneInputPort2);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setres = setup->addAudioInputPort(routingZone, zoneInputPort2);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  //link first zone input port with first sink input port (we will try to link remaining ports later)
  setres = setup->link(zoneInputPort1, sinkInputPort1);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  //start routing zone
  setres = setup->startRoutingZone(routingZone);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  ASSERT_TRUE(routingZone->isActive() != false);

  //try to destroy sink device, this should fail because routing zone is active
  setup->destroyAudioSinkDevice(sink);
  ASSERT_TRUE(sink != nullptr);

  //try to delete input ports from sink device, this should fail because routing zone is active
  setup->deleteAudioInputPort(sink, sinkInputPort1);
  IasAudioPortOwnerPtr sinkInputPort1_owner;
  sinkInputPort1->getOwner(&sinkInputPort1_owner);
  ASSERT_TRUE(sink == sinkInputPort1_owner);

  setup->deleteAudioInputPort(sink, sinkInputPort2);
  IasAudioPortOwnerPtr sinkInputPort2_owner;
  sinkInputPort2->getOwner(&sinkInputPort2_owner);
  ASSERT_TRUE(sink == sinkInputPort2_owner);

  //try to destroy ports which belong to sink device, this should fail because routing zone is active
  setup->destroyAudioPort(sinkInputPort1);
  ASSERT_TRUE(sinkInputPort1 != nullptr);

  setup->destroyAudioPort(sinkInputPort2);
  ASSERT_TRUE(sinkInputPort2 != nullptr);

  //create another port for sink device
  IasAudioPortParams sinkInputPortParams3 =
  {
    "sinkInputPort3", 2, -1, eIasPortDirectionInput, 0
  };
  IasAudioPortPtr sinkInputPort3 = nullptr;
  setres = setup->createAudioPort(sinkInputPortParams3, &sinkInputPort3);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  //try to add port to sink device, this should fail because routing zone is active
  setres = setup->addAudioInputPort(sink, sinkInputPort3);
  ASSERT_EQ(IasISetup::eIasFailed, setres);

  //try to delete input ports from routing zone, this should fail because routing zone is active
  setup->deleteAudioInputPort(routingZone, zoneInputPort1);
  IasAudioPortOwnerPtr zoneInputPort1_owner;
  zoneInputPort1->getOwner(&zoneInputPort1_owner);
  ASSERT_TRUE(routingZone == zoneInputPort1_owner);

  setup->deleteAudioInputPort(routingZone, zoneInputPort2);
  IasAudioPortOwnerPtr zoneInputPort2_owner;
  zoneInputPort2->getOwner(&zoneInputPort2_owner);
  ASSERT_TRUE(routingZone == zoneInputPort2_owner);

  //try to destroy ports which belong to routing zone, this should fail because routing zone is active
  setup->destroyAudioPort(zoneInputPort1);
  ASSERT_TRUE(zoneInputPort1 != nullptr);
  setup->destroyAudioPort(zoneInputPort2);
  ASSERT_TRUE(zoneInputPort2 != nullptr);

  //create another port for routing zone
  IasAudioPortParams zoneInputPortParams3 =
  {
    "zoneInputPort3", 2, -1, eIasPortDirectionInput, 0
  };
  IasAudioPortPtr zoneInputPort3 = nullptr;
  setres = setup->createAudioPort(zoneInputPortParams3, &zoneInputPort3);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  //try to add port to routing zone, this should fail because routing zone is active
  setres = setup->addAudioInputPort(routingZone, zoneInputPort3);
  ASSERT_EQ(IasISetup::eIasFailed, setres);

  //create another routing zone
  IasRoutingZoneParams routingZoneParams2 =
  {
    "routingZone2"
  };
  IasRoutingZonePtr routingZone2 = nullptr;
  setres = setup->createRoutingZone(routingZoneParams2, &routingZone2);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  //try to add derived zone, this should fail because routing zone is active
  setres = setup->addDerivedZone(routingZone, routingZone2);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  setres = setup->addDerivedZone(routingZone2, routingZone);
  ASSERT_EQ(IasISetup::eIasFailed, setres);

  //try to link routing zone with sink device, this should fail because routing zone is active
  setres = setup->link(routingZone, sink);
  ASSERT_EQ(IasISetup::eIasFailed, setres);

  //try to unlink routing zone from sink device, this should fail because routing zone is active
  setup->unlink(routingZone, sink);
  ASSERT_TRUE(routingZone->hasLinkedAudioSinkDevice() == true);
  ASSERT_TRUE(sink->getRoutingZone() == routingZone);

  //try to link previously added but not linked zone and sink audio ports, this should fail because routing zone is active
  setres = setup->link(zoneInputPort2, sinkInputPort2);
  ASSERT_EQ(IasISetup::eIasFailed, setres);

  //try to unlink zone and sink audio ports, this should fail because routing zone is active
  setup->unlink(zoneInputPort1, sinkInputPort1);
  IasRoutingZoneWorkerThread::IasConversionBufferParamsMap convBufParamsMap;
  convBufParamsMap = routingZone->getWorkerThread()->getConversionBufferParamsMap();
  ASSERT_TRUE(convBufParamsMap[zoneInputPort1].sinkDeviceInputPort == sinkInputPort1);

  IasSmartX::destroy(smartx);
}

TEST_F(IasSmartX_API_Test, setup_pipeline_related_interface_fails_when_routing_zone_is_active)
{
  IasSmartX *smartx = IasSmartX::create();
  ASSERT_TRUE(smartx != nullptr);
  IasISetup *setup = smartx->setup();
  ASSERT_TRUE(setup != nullptr);

  //create sink device
  IasAudioDeviceParams sinkParams =
  {
    "map_2_4", 4, 48000, eIasFormatInt16, eIasClockReceived, 256, 4
  };
  IasAudioSinkDevicePtr sink = nullptr;
  IasISetup::IasResult setres = setup->createAudioSinkDevice(sinkParams, &sink);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  //create two stereo ports and add them to the sink device as input ports
  IasAudioPortParams sinkInputPortParams1 =
  {
    "sinkInputPort1", 2, -1, eIasPortDirectionInput, 0
  };
  IasAudioPortPtr sinkInputPort1 = nullptr;
  setres = setup->createAudioPort(sinkInputPortParams1, &sinkInputPort1);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setres = setup->addAudioInputPort(sink, sinkInputPort1);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  IasAudioPortParams sinkInputPortParams2 =
  {
    "sinkInputPort2", 2, -1, eIasPortDirectionInput, 0
  };
  IasAudioPortPtr sinkInputPort2 = nullptr;
  setres = setup->createAudioPort(sinkInputPortParams2, &sinkInputPort2);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setres = setup->addAudioInputPort(sink, sinkInputPort2);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  //create routing zone and link it with sink device
  IasRoutingZoneParams routingZoneParams =
  {
    "testRoutingZone"
  };
  IasRoutingZonePtr routingZone = nullptr;
  setres = setup->createRoutingZone(routingZoneParams, &routingZone);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setres = setup->link(routingZone, sink);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  //create two input stereo ports and add them to the routing zone
  IasAudioPortParams zoneInputPortParams1 =
  {
    "zoneInputPort1", 2, -1, eIasPortDirectionInput, 0
  };
  IasAudioPortPtr zoneInputPort1 = nullptr;
  setres = setup->createAudioPort(zoneInputPortParams1, &zoneInputPort1);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setres = setup->addAudioInputPort(routingZone, zoneInputPort1);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  IasAudioPortParams zoneInputPortParams2 =
  {
    "zoneInputPort2", 2, -1, eIasPortDirectionInput, 0
  };
  IasAudioPortPtr zoneInputPort2 = nullptr;
  setres = setup->createAudioPort(zoneInputPortParams2, &zoneInputPort2);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setres = setup->addAudioInputPort(routingZone, zoneInputPort2);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  //link first zone input port with first sink input port (we will try to link remaining ports later)
  setres = setup->link(zoneInputPort1, sinkInputPort1);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  //create pipeline
  IasPipelineParams pipelineParams =
  {
    "testPipeline", 48000, 256
  };
  IasPipelinePtr pipeline = nullptr;
  setres = setup->createPipeline(pipelineParams, &pipeline);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  //add pipeline to routing zone
  setres = setup->addPipeline(routingZone, pipeline);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  //create two pipeline input pins and add them to the pipeline
  IasAudioPinParams pipelineInputPinParams1 =
  {
    "pipelineInputPin1", 2
  };
  IasAudioPinPtr pipelineInputPin1 = nullptr;
  setres = setup->createAudioPin(pipelineInputPinParams1, &pipelineInputPin1);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  ASSERT_TRUE(pipelineInputPin1 != nullptr);
  setres = setup->addAudioInputPin(pipeline, pipelineInputPin1);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  IasAudioPinParams pipelineInputPinParams2 =
  {
    "pipelineInputPin2", 2
  };
  IasAudioPinPtr pipelineInputPin2 = nullptr;
  setres = setup->createAudioPin(pipelineInputPinParams2, &pipelineInputPin2);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  ASSERT_TRUE(pipelineInputPin2 != nullptr);
  setres = setup->addAudioInputPin(pipeline, pipelineInputPin2);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  //create two pipeline output pins and add them to the pipeline
  IasAudioPinParams pipelineOutputPinParams1 =
  {
    "pipelineOutputPin1", 2
  };
  IasAudioPinPtr pipelineOutputPin1 = nullptr;
  setres = setup->createAudioPin(pipelineOutputPinParams1, &pipelineOutputPin1);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  ASSERT_TRUE(pipelineOutputPin1 != nullptr);
  setres = setup->addAudioOutputPin(pipeline, pipelineOutputPin1);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  IasAudioPinParams pipelineOutputPinParams2 =
  {
    "pipelineOutputPin2", 2
  };
  IasAudioPinPtr pipelineOutputPin2 = nullptr;
  setres = setup->createAudioPin(pipelineOutputPinParams2, &pipelineOutputPin2);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  ASSERT_TRUE(pipelineOutputPin2 != nullptr);
  setres = setup->addAudioOutputPin(pipeline, pipelineOutputPin2);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  //link first zone input port and first pipeline input pin (we will try to link remaining ports and pins later)
  setres = setup->link(zoneInputPort1, pipelineInputPin1);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  //link first pipeline output pin and first sink input port (we will try to link remaining ports and pins later)
  setres = setup->link(sinkInputPort1, pipelineOutputPin1);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  //init pipeline audio chain
  setres = setup->initPipelineAudioChain(pipeline);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  //start routing zone
  setres = setup->startRoutingZone(routingZone);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  ASSERT_TRUE(routingZone->isActive() != false);

  //try to destroy pipeline, this should fail because routing zone is active
  setup->destroyPipeline(&pipeline);
  ASSERT_TRUE(pipeline != nullptr);

  //try to delete pipeline from routing zone, this should fail because routing zone is active
  setup->deletePipeline(routingZone);
  ASSERT_TRUE(routingZone->hasPipeline(pipeline) != false);

  //try to destroy audio pins that belong to the pipeline, this should fail because routing zone is active
  setup->destroyAudioPin(&pipelineInputPin1);
  ASSERT_TRUE(pipelineInputPin1 != nullptr);
  setup->destroyAudioPin(&pipelineInputPin2);
  ASSERT_TRUE(pipelineInputPin2 != nullptr);
  setup->destroyAudioPin(&pipelineOutputPin1);
  ASSERT_TRUE(pipelineOutputPin1 != nullptr);
  setup->destroyAudioPin(&pipelineOutputPin2);
  ASSERT_TRUE(pipelineOutputPin2 != nullptr);

  //try to delete audio pins from the pipeline, this should fail because routing zone is active
  setup->deleteAudioInputPin(pipeline, pipelineInputPin1);
  ASSERT_TRUE(pipelineInputPin1->getPipeline() == pipeline);
  setup->deleteAudioInputPin(pipeline, pipelineInputPin2);
  ASSERT_TRUE(pipelineInputPin2->getPipeline() == pipeline);
  setup->deleteAudioOutputPin(pipeline, pipelineOutputPin1);
  ASSERT_TRUE(pipelineOutputPin1->getPipeline() == pipeline);
  setup->deleteAudioOutputPin(pipeline, pipelineOutputPin2);
  ASSERT_TRUE(pipelineOutputPin2->getPipeline() == pipeline);

  //create another pipeline input pin
  IasAudioPinParams pipelineInputPinParams =
  {
    "pipelineInputPin", 2
  };
  IasAudioPinPtr pipelineInputPin = nullptr;
  setres = setup->createAudioPin(pipelineInputPinParams, &pipelineInputPin);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  ASSERT_TRUE(pipelineInputPin != nullptr);

  //create another pipeline output pin
  IasAudioPinParams pipelineOutputPinParams =
  {
    "pipelineOutputPin", 2
  };
  IasAudioPinPtr pipelineOutputPin = nullptr;
  setres = setup->createAudioPin(pipelineOutputPinParams, &pipelineOutputPin);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  ASSERT_TRUE(pipelineOutputPin != nullptr);

  //try to add input pin to pipeline, this should fail because routing zone is active
  setres = setup->addAudioInputPin(pipeline, pipelineInputPin);
  ASSERT_EQ(IasISetup::eIasFailed, setres);

  //try to add output pin to pipeline, this should fail because routing zone is active
  setres = setup->addAudioOutputPin(pipeline, pipelineOutputPin);
  ASSERT_EQ(IasISetup::eIasFailed, setres);

  //try to unlink first zone input port and first pipeline input pin, this should fail because routing zone is active
  setup->unlink(zoneInputPort1, pipelineInputPin1);

  //try to unlink first pipeline output pin and first sink input port, this should fail because routing zone is active
  setup->unlink(sinkInputPort1, pipelineOutputPin1);

  //try to link second zone input port and second pipeline input pin, this should fail because routing zone is active
  setres = setup->link(zoneInputPort2, pipelineInputPin2);
  ASSERT_EQ(IasISetup::eIasFailed, setres);

  //try to link second pipeline output pin and second sink input port, this should fail because routing zone is active
  setres = setup->link(sinkInputPort2, pipelineOutputPin2);
  ASSERT_EQ(IasISetup::eIasFailed, setres);

  IasSmartX::destroy(smartx);
}

TEST_F(IasSmartX_API_Test, setup_processing_module_related_interface_fails_when_routing_zone_is_active)
{
  setenv("AUDIO_PLUGIN_DIR", AUDIO_PLUGIN_DIR, true);
  IasSmartX *smartx = IasSmartX::create();
  ASSERT_TRUE(smartx != nullptr);
  IasISetup *setup = smartx->setup();
  ASSERT_TRUE(setup != nullptr);

  //create sink device
  IasAudioDeviceParams sinkParams =
  {
    "map_2_4", 4, 48000, eIasFormatInt16, eIasClockReceived, 256, 4
  };
  IasAudioSinkDevicePtr sink = nullptr;
  IasISetup::IasResult setres = setup->createAudioSinkDevice(sinkParams, &sink);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  //create input stereo port and add it to the sink device
  IasAudioPortParams sinkInputPortParams =
  {
    "sinkInputPort", 2, -1, eIasPortDirectionInput, 0
  };
  IasAudioPortPtr sinkInputPort = nullptr;
  setres = setup->createAudioPort(sinkInputPortParams, &sinkInputPort);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setres = setup->addAudioInputPort(sink, sinkInputPort);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  //create routing zone and link it with sink device
  IasRoutingZoneParams routingZoneParams =
  {
    "testRoutingZone"
  };
  IasRoutingZonePtr routingZone = nullptr;
  setres = setup->createRoutingZone(routingZoneParams, &routingZone);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setres = setup->link(routingZone, sink);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  //create input stereo port and add it to the routing zone
  IasAudioPortParams zoneInputPortParams =
  {
    "zoneInputPort", 2, -1, eIasPortDirectionInput, 0
  };
  IasAudioPortPtr zoneInputPort = nullptr;
  setres = setup->createAudioPort(zoneInputPortParams, &zoneInputPort);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setres = setup->addAudioInputPort(routingZone, zoneInputPort);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  //link zone input port with the sink input port
  setres = setup->link(zoneInputPort, sinkInputPort);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  //create pipeline and add it to the routing zone
  IasPipelineParams pipelineParams =
  {
    "MyPipeline", 48000, 256
  };
  IasPipelinePtr pipeline = nullptr;
  setres = setup->createPipeline(pipelineParams, &pipeline);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setres = setup->addPipeline(routingZone, pipeline);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  //create pipeline input pin and add it to the pipeline
  IasAudioPinParams pipelineInputPinParams =
  {
    "pipelineInputPin", 2
  };
  IasAudioPinPtr pipelineInputPin = nullptr;
  setres = setup->createAudioPin(pipelineInputPinParams, &pipelineInputPin);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  ASSERT_TRUE(pipelineInputPin != nullptr);
  setres = setup->addAudioInputPin(pipeline, pipelineInputPin);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  //create audio output pin and add it to the pipeline
  IasAudioPinParams pipelineOutputPinParams =
  {
    "pipelineOutputPin", 2
  };
  IasAudioPinPtr pipelineOutputPin = nullptr;
  setres = setup->createAudioPin(pipelineOutputPinParams, &pipelineOutputPin);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  ASSERT_TRUE(pipelineOutputPin != nullptr);
  setres = setup->addAudioOutputPin(pipeline, pipelineOutputPin);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  //link zone input port and pipeline input pin
  setres = setup->link(zoneInputPort, pipelineInputPin);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  //link pipeline output pin and sink input port
  setres = setup->link(sinkInputPort, pipelineOutputPin);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  //create volume module
  IasProcessingModuleParams volumeModuleParams =
  {
    "ias.volume", "MyVolume"
  };
  IasProcessingModulePtr volume = nullptr;
  setres = setup->createProcessingModule(volumeModuleParams, &volume);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  ASSERT_TRUE(volume != nullptr);

  //set volume properties
  IasProperties volumeProperties;
  volumeProperties.set<int32_t>("numFilterBands", 3);
  setup->setProperties(volume, volumeProperties);

  //add volume module to the pipeline
  setres = setup->addProcessingModule(pipeline, volume);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  //create in/out pin for volume module and add it to the module
  IasAudioPinParams volumeInOutPinParams =
  {
    "volumeInOutPin", 2
  };
  IasAudioPinPtr volumeInOutPin = nullptr;
  setres = setup->createAudioPin(volumeInOutPinParams, &volumeInOutPin);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  ASSERT_TRUE(volumeInOutPin != nullptr);
  setres = setup->addAudioInOutPin(volume, volumeInOutPin);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  //create two audio pins and add them to the volume module as pin mapping
  IasAudioPinParams volumeInputPinParams =
  {
    "volumeInputPin", 2
  };
  IasAudioPinPtr volumeInputPin = nullptr;
  setres = setup->createAudioPin(volumeInputPinParams, &volumeInputPin);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  ASSERT_TRUE(volumeInputPin != nullptr);

  IasAudioPinParams volumeOutputPinParams =
  {
    "volumeOutputPin", 2
  };
  IasAudioPinPtr volumeOutputPin = nullptr;
  setres = setup->createAudioPin(volumeOutputPinParams, &volumeOutputPin);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  ASSERT_TRUE(volumeOutputPin != nullptr);

  setres = setup->addAudioPinMapping(volume, volumeInputPin, volumeOutputPin);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  //init pipeline audio chain
  setres = setup->initPipelineAudioChain(pipeline);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  //start routing zone
  setres = setup->startRoutingZone(routingZone);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  ASSERT_TRUE(routingZone->isActive() != false);

  //try to destroy volume module, this should fail because routing zone is active
  setup->destroyProcessingModule(&volume);
  ASSERT_TRUE(volume != nullptr);

  //try to delete volume module from pipeline, this should fail because routing zone is active
  setup->deleteProcessingModule(pipeline, volume);
  ASSERT_TRUE(volume->getPipeline() == pipeline);

  //try to destroy volume module in/out pin, this should fail because routing zone is active
  setup->destroyAudioPin(&volumeInOutPin);
  ASSERT_TRUE(volumeInOutPin != nullptr);

  //try to delete in/out pin from volume module, this should fail because routing zone is active
  setup->deleteAudioInOutPin(volume, volumeInOutPin);
  ASSERT_TRUE(volumeInOutPin->getPipeline() == pipeline);

  //try to delete pin mapping from volume module, this should fail because routing zone is active
  setup->deleteAudioPinMapping(volume, volumeInputPin, volumeOutputPin);
  ASSERT_TRUE(volumeInputPin->getPipeline() == pipeline);
  ASSERT_TRUE(volumeOutputPin->getPipeline() == pipeline);

  //try to link module in/out pin and pipeline input and output pins, this should fail because routing zone is active
  setres = setup->link(pipelineInputPin, volumeInOutPin, eIasAudioPinLinkTypeImmediate);
  ASSERT_EQ(IasISetup::eIasFailed, setres);
  setres = setup->link(volumeInOutPin, pipelineOutputPin, eIasAudioPinLinkTypeImmediate);
  ASSERT_EQ(IasISetup::eIasFailed, setres);

  //create two audio pins, we will try to use them as module in/out pin and for pin mapping
  IasAudioPinParams audioPinParams1 =
  {
    "audioPin1", 2
  };
  IasAudioPinPtr audioPin1 = nullptr;
  setres = setup->createAudioPin(audioPinParams1, &audioPin1);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  ASSERT_TRUE(audioPin1 != nullptr);

  IasAudioPinParams audioPinParams2 =
  {
    "audioPin2", 2
  };
  IasAudioPinPtr audioPin2 = nullptr;
  setres = setup->createAudioPin(audioPinParams2, &audioPin2);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  ASSERT_TRUE(audioPin2 != nullptr);

  //try to add in/out pin to the volume module, this should fail because routing zone is active
  setres = setup->addAudioInOutPin(volume, audioPin1);
  ASSERT_EQ(IasISetup::eIasFailed, setres);

  //try to add audio pin mapping to the volume module, this should fail because routing zone is active
  setres = setup->addAudioPinMapping(volume, audioPin1, audioPin2);
  ASSERT_EQ(IasISetup::eIasFailed, setres);

  //create mixer module
  IasProcessingModuleParams mixerModuleParams =
  {
    "ias.mixer", "MyMixer"
  };
  IasProcessingModulePtr mixer = nullptr;
  setres = setup->createProcessingModule(mixerModuleParams, &mixer);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  ASSERT_TRUE(mixer != nullptr);

  //try to add mixer module to the pipeline, this should fail because routing zone is active
  setres = setup->addProcessingModule(pipeline, mixer);
  ASSERT_EQ(IasISetup::eIasFailed, setres);

  IasSmartX::destroy(smartx);
}

TEST_F(IasSmartX_API_Test, connect_independent_zones)
{
  IasISetup::IasResult setupRes = IasISetup::eIasOk;
  IasIRouting::IasResult connRes = IasIRouting::eIasOk;

  IasSmartX *smartx = IasSmartX::create();
  ASSERT_TRUE(smartx != nullptr);
  IasISetup *setup = smartx->setup();
  ASSERT_TRUE(setup != nullptr);
  IasIRouting *routing = smartx->routing();
  ASSERT_TRUE(routing != nullptr);


  IasAudioDeviceParams sourceParams1 =
  {
    "source1", 2, 48000, eIasFormatInt16, eIasClockProvided, 256, 4
  };

  IasAudioSourceDevicePtr source1 = nullptr;
  setupRes = IasSetupHelper::createAudioSourceDevice(setup, sourceParams1, 1, &source1);
  EXPECT_EQ(IasISetup::eIasOk, setupRes);

  IasAudioDeviceParams sourceParams2 =
  {
    "source2", 2, 48000, eIasFormatInt16, eIasClockProvided, 256, 4
  };
  IasAudioSourceDevicePtr source2 = nullptr;
  setupRes = IasSetupHelper::createAudioSourceDevice(setup, sourceParams2, 2, &source2);
  EXPECT_EQ(IasISetup::eIasOk, setupRes);

  IasAudioDeviceParams sinkParams1 =
  {
    "sink1", 2, 48000, eIasFormatInt16, eIasClockReceived, 256, 4
  };
  IasAudioSinkDevicePtr sink1 = nullptr;
  IasRoutingZonePtr routingZone1 = nullptr;
  setupRes = IasSetupHelper::createAudioSinkDevice(setup, sinkParams1, 1, &sink1, &routingZone1);
  EXPECT_EQ(IasISetup::eIasOk, setupRes);

  IasAudioDeviceParams sinkParams2 =
  {
    "sink2", 2, 48000, eIasFormatInt16, eIasClockReceived, 256, 4
  };
  IasAudioSinkDevicePtr sink2 = nullptr;
  IasRoutingZonePtr routingZone2 = nullptr;
  setupRes = IasSetupHelper::createAudioSinkDevice(setup, sinkParams2, 2, &sink2, &routingZone2);
  EXPECT_EQ(IasISetup::eIasOk, setupRes);

  connRes =routing->connect(1,1);
  EXPECT_EQ(IasIRouting::eIasOk, connRes);
  connRes = routing->connect(1,2);
  EXPECT_EQ(IasIRouting::eIasFailed, connRes);
  connRes = routing->connect(2,2);
  EXPECT_EQ(IasIRouting::eIasOk, connRes);

  connRes =routing->disconnect(1,1);
  EXPECT_EQ(IasIRouting::eIasOk, connRes);
  connRes =routing->disconnect(1,2);
  EXPECT_EQ(IasIRouting::eIasFailed, connRes);
  connRes =routing->disconnect(2,2);
  EXPECT_EQ(IasIRouting::eIasOk, connRes);

  IasSmartX::destroy(smartx);
}

TEST_F(IasSmartX_API_Test, destroy_fails_after_connect_with_weird_period_size)
{
  // This is a test for RTC defect 210988
  setenv("AUDIO_PLUGIN_DIR", AUDIO_PLUGIN_DIR, true);
  IasSmartX *smartx = IasSmartX::create();
  ASSERT_TRUE(smartx != nullptr);
  IasISetup *setup = smartx->setup();
  ASSERT_TRUE(setup != nullptr);
  IasISetup::IasResult setres;

  IasAudioDeviceParams dev_params;
  dev_params.clockType = IasClockType::eIasClockProvided;
  dev_params.numChannels = 1;
  dev_params.name = "mono_source";
  dev_params.dataFormat = IasAudioCommonDataFormat::eIasFormatInt16;
  dev_params.periodSize = 384;
  dev_params.numPeriods = 4;
  dev_params.samplerate = 48000;
  IasAudioSourceDevicePtr mono_source = nullptr;
  setres = setup->createAudioSourceDevice(dev_params, &mono_source);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  IasAudioPortParams port_params;
  port_params.direction = IasPortDirection::eIasPortDirectionOutput;
  port_params.id = 123;
  port_params.index = 0;
  port_params.name = "source_out";
  port_params.numChannels = 1;
  IasAudioPortPtr source_out = nullptr;
  setres = setup->createAudioPort(port_params, &source_out);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setres = setup->addAudioOutputPort(mono_source, source_out);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  dev_params.clockType = IasClockType::eIasClockReceived;
  dev_params.name = "plughw:Dummy,0";
  dev_params.samplerate = 8080;
  dev_params.periodSize = 202;
  dev_params.numPeriods = 3;
  IasAudioSinkDevicePtr mono_sink = nullptr;
  setres = setup->createAudioSinkDevice(dev_params, &mono_sink);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  port_params.direction = IasPortDirection::eIasPortDirectionInput;
  port_params.id = -1;
  port_params.index = 0;
  port_params.name = "sink_in";
  port_params.numChannels = 1;
  IasAudioPortPtr sink_in = nullptr;
  setres = setup->createAudioPort(port_params, &sink_in);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setres = setup->addAudioInputPort(mono_sink, sink_in);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  IasRoutingZoneParams rz_params;
  rz_params.name = "mono_sink_rz";
  IasRoutingZonePtr mono_sink_rz;
  setres = setup->createRoutingZone(rz_params, &mono_sink_rz);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setres = setup->link(mono_sink_rz, mono_sink);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  port_params.name = "rz_in";
  port_params.id = 123;
  IasAudioPortPtr rz_in = nullptr;
  setres = setup->createAudioPort(port_params, &rz_in);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setres = setup->addAudioInputPort(mono_sink_rz, rz_in);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  IasPipelineParams pipe_params;
  pipe_params.name = "pipeline";
  pipe_params.periodSize = 202;
  pipe_params.samplerate = 8080;
  IasPipelinePtr pipeline = nullptr;
  setres = setup->createPipeline(pipe_params, &pipeline);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  IasAudioPinParams pin_params;
  pin_params.name = "pipe_in";
  pin_params.numChannels = 1;
  IasAudioPinPtr pipe_in = nullptr;
  setres = setup->createAudioPin(pin_params, &pipe_in);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  pin_params.name = "pipe_out";
  IasAudioPinPtr pipe_out = nullptr;
  setres = setup->createAudioPin(pin_params, &pipe_out);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  setres = setup->addAudioInputPin(pipeline, pipe_in);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setres = setup->addAudioOutputPin(pipeline, pipe_out);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  IasProcessingModuleParams vol_params;
  vol_params.typeName = "ias.volume";
  vol_params.instanceName = "volume";
  IasProcessingModulePtr volume = nullptr;
  setres = setup->createProcessingModule(vol_params, &volume);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  setres = setup->addProcessingModule(pipeline, volume);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  pin_params.name = "vol_inout";
  IasAudioPinPtr vol_inout = nullptr;
  setres = setup->createAudioPin(pin_params, &vol_inout);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setres = setup->addAudioInOutPin(volume, vol_inout);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  IasProperties vol_properties;
  vol_properties.set<Ias::Int32>("numFilterBands", 2);
  setup->setProperties(volume, vol_properties);

  setres = setup->link(pipe_in, vol_inout, IasAudioPinLinkType::eIasAudioPinLinkTypeImmediate);
  ASSERT_EQ(IasISetup::eIasOk, setres);
  setres = setup->link(vol_inout, pipe_out, IasAudioPinLinkType::eIasAudioPinLinkTypeImmediate);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  setres = setup->initPipelineAudioChain(pipeline);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  setres = setup->addPipeline(mono_sink_rz, pipeline);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  setres = setup->link(rz_in, pipe_in);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  setres = setup->link(sink_in, pipe_out);
  ASSERT_EQ(IasISetup::eIasOk, setres);

  IasSmartX::IasResult smxres;
  smxres = smartx->start();
  ASSERT_EQ(IasSmartX::IasResult::eIasOk, smxres);

  IasIRouting::IasResult roures;
  roures = smartx->routing()->connect(123, 123);
  ASSERT_EQ(IasIRouting::eIasFailed, roures);

  // Without the fix, the destroy of the SmartXbar will now lead to a crash
  IasSmartX::destroy(smartx);
}


TEST_F(IasSmartX_API_Test, test_TC1_5_4_1_121)
{
  IasSmartX *smartx = IasSmartX::create();
  ASSERT_TRUE(smartx != nullptr);
  IasISetup *setup = smartx->setup();
  ASSERT_TRUE(setup != nullptr);
  IasISetup::IasResult setres;
  IasIRouting *routing = smartx->routing();
  ASSERT_TRUE(routing != nullptr);
  IasIRouting::IasResult connres;

  IasAudioDeviceParams sourceParams1;
  sourceParams1.clockType = IasClockType::eIasClockProvided;
  sourceParams1.numChannels = 2;
  sourceParams1.name = "stereo_source";
  sourceParams1.dataFormat = IasAudioCommonDataFormat::eIasFormatInt16;
  sourceParams1.periodSize = 256;
  sourceParams1.numPeriods = 4;
  sourceParams1.samplerate = 48000;

  IasAudioSourceDevicePtr stereo_source = nullptr;
  setres = IasSetupHelper::createAudioSourceDevice(setup, sourceParams1, 1, &stereo_source);
  ASSERT_TRUE(setres == IasISetup::eIasOk);

  IasAudioDeviceParams sinkParams =
  {
    "map_2_4", 2, 48000, eIasFormatInt16, eIasClockReceived, 256, 4
  };
  IasAudioSinkDevicePtr sink = nullptr;

  setres = setup->createAudioSinkDevice(sinkParams, &sink);
  ASSERT_TRUE(setres == IasISetup::eIasOk);
  ASSERT_TRUE(sink != nullptr);

  IasAudioPortParams sinkPortParams =
  {
    sinkParams.name + "_port",
    sinkParams.numChannels,
    -1,
    eIasPortDirectionInput,
    0
  };
  IasAudioPortPtr sinkPort = nullptr;
  setres = setup->createAudioPort(sinkPortParams, &sinkPort);
  ASSERT_TRUE(setres == IasISetup::eIasOk);
  ASSERT_TRUE(sinkPort != nullptr);

  setres = setup->addAudioInputPort(sink, sinkPort);
  ASSERT_TRUE(setres == IasISetup::eIasOk);

  IasRoutingZoneParams routingZoneParams =
  {
    sinkParams.name + "_rz"
  };
  IasRoutingZonePtr myRoutingZone = nullptr;
  setres = setup->createRoutingZone(routingZoneParams, &myRoutingZone);
  ASSERT_TRUE(setres == IasISetup::eIasOk);
  ASSERT_TRUE(myRoutingZone != nullptr);

  setres = setup->link(myRoutingZone, sink);
  ASSERT_TRUE(setres == IasISetup::eIasOk);

  // Create a zone port
  IasAudioPortParams zonePortParams =
  {
    sinkParams.name + "_rznport",
    sinkParams.numChannels,
    1,
    eIasPortDirectionInput,
    0
  };
  IasAudioPortPtr zonePort = nullptr;
  setres = setup->createAudioPort(zonePortParams, &zonePort);
  ASSERT_TRUE(setres == IasISetup::eIasOk);
  setres = setup->addAudioInputPort(myRoutingZone, zonePort);
  ASSERT_TRUE(setres == IasISetup::eIasOk);
  setres = setup->link(zonePort, sinkPort);
  ASSERT_TRUE(setres == IasISetup::eIasOk);

  IasSmartX::IasResult smxres = smartx->start();
  ASSERT_TRUE(smxres == IasSmartX::eIasOk);

  connres = routing->connect(1,1);
  ASSERT_TRUE(connres == IasIRouting::eIasOk);

  setup->stopRoutingZone(myRoutingZone);
  //ASSERT_TRUE(setres == IasISetup::eIasOk);

  setup->destroyAudioPort(zonePort);
  //ASSERT_TRUE(setres == IasISetup::eIasOk);

  smxres = smartx->stop();
  ASSERT_TRUE(smxres == IasSmartX::eIasOk);

  IasAudioPortPtr zonePort2 = nullptr;
  setres = setup->createAudioPort(zonePortParams, &zonePort2);
  ASSERT_TRUE(setres == IasISetup::eIasOk);
  ASSERT_TRUE(zonePort2 != nullptr);

  setres = setup->addAudioInputPort(myRoutingZone, zonePort2);
  ASSERT_TRUE(setres == IasISetup::eIasFailed);
  setres = setup->link(zonePort2, sinkPort);
  ASSERT_TRUE(setres == IasISetup::eIasFailed);

  smxres = smartx->start();
  ASSERT_TRUE(smxres == IasSmartX::eIasOk);

  connres = routing->connect(1,1);
  ASSERT_TRUE(connres == IasIRouting::eIasFailed);

  connres = routing->disconnect(1,1);
  ASSERT_TRUE(connres == IasIRouting::eIasFailed);

  IasSmartX::destroy(smartx);
}

TEST_F(IasSmartX_API_Test, test_TC1_5_4_1_100)
{
  IasSmartX *smartx = IasSmartX::create();
  ASSERT_TRUE(smartx != nullptr);
  IasISetup *setup = smartx->setup();
  ASSERT_TRUE(setup != nullptr);
  IasISetup::IasResult setres;
  IasIRouting *routing = smartx->routing();
  ASSERT_TRUE(routing != nullptr);
  IasIRouting::IasResult connres;

  IasAudioSourceDevicePtr stereo;
  IasAudioSinkDevicePtr   speaker;
  IasAudioSinkDevicePtr   derived;
  IasRoutingZonePtr       speakerZone;
  IasRoutingZonePtr       derivedZone;
  IasAudioPortPtr         sourceOutputPort;
  IasAudioPortPtr         sinkInputPort1;
  IasAudioPortPtr         sinkInputPort2;
  IasAudioPortPtr         zoneInputPort1;
  IasAudioPortPtr         zoneInputPort2;

  IasAudioDeviceParams sourceParams;
  sourceParams.clockType = IasClockType::eIasClockProvided;
  sourceParams.numChannels = 2;
  sourceParams.name = "stereo";
  sourceParams.dataFormat = IasAudioCommonDataFormat::eIasFormatInt16;
  sourceParams.periodSize = 256;
  sourceParams.numPeriods = 4;
  sourceParams.samplerate = 48000;

  setres = setup->createAudioSourceDevice(sourceParams, &stereo);
  ASSERT_TRUE(setres == IasISetup::eIasOk);

  IasAudioPortParams sourcePortParams =
  {
    "sourcePort",
    2,
    1,
    eIasPortDirectionOutput,
    0
  };

  setres =setup->createAudioPort(sourcePortParams, &sourceOutputPort);
  ASSERT_TRUE(setres == IasISetup::eIasOk);
  std::cout <<"usecount of source port after create:" <<sourceOutputPort.use_count() <<std::endl;;

  setres = setup->addAudioOutputPort(stereo, sourceOutputPort);
  ASSERT_TRUE(setres == IasISetup::eIasOk);
  std::cout <<"usecount of source port after adding to device:" <<sourceOutputPort.use_count() <<std::endl;;

  IasAudioDeviceParams sinkParams;
  sinkParams.clockType = IasClockType::eIasClockReceived;
  sinkParams.numChannels = 2;
  sinkParams.name = "map_2_4";
  sinkParams.dataFormat = IasAudioCommonDataFormat::eIasFormatInt16;
  sinkParams.periodSize = 256;
  sinkParams.numPeriods = 4;
  sinkParams.samplerate = 48000;

  setres = setup->createAudioSinkDevice(sinkParams, &speaker);
  ASSERT_TRUE(setres == IasISetup::eIasOk);

  IasAudioPortParams sinkPortParams =
  {
    "map_2_4_port",
    2,
    -1,
    eIasPortDirectionInput,
    0
  };

  setres =setup->createAudioPort(sinkPortParams, &sinkInputPort1);
  ASSERT_TRUE(setres == IasISetup::eIasOk);

  setres = setup->addAudioInputPort(speaker, sinkInputPort1);
  ASSERT_TRUE(setres == IasISetup::eIasOk);

  IasRoutingZoneParams routingZoneParams =
  {
    "map_2_4_rz"
  };
  setres = setup->createRoutingZone(routingZoneParams, &speakerZone);
  ASSERT_TRUE(setres == IasISetup::eIasOk);
  ASSERT_TRUE(speakerZone != nullptr);

  setres = setup->link(speakerZone, speaker);
  ASSERT_TRUE(setres == IasISetup::eIasOk);

  // Create a zone port
  IasAudioPortParams zonePortParams =
  {
    "map_2_4_rznport",
    2,
    1,
    eIasPortDirectionInput,
    0
  };

  setres = setup->createAudioPort(zonePortParams, &zoneInputPort1);
  ASSERT_TRUE(setres == IasISetup::eIasOk);
  setres = setup->addAudioInputPort(speakerZone, zoneInputPort1);
  ASSERT_TRUE(setres == IasISetup::eIasOk);
  setres = setup->link(zoneInputPort1, sinkInputPort1);
  ASSERT_TRUE(setres == IasISetup::eIasOk);

  IasSmartX::IasResult smxres = smartx->start();
  ASSERT_TRUE(smxres == IasSmartX::eIasOk);

  connres = routing->connect(1,1);
  ASSERT_TRUE(connres == IasIRouting::eIasOk);
  smxres = smartx->waitForEvent(1000);
  EXPECT_EQ(IasSmartX::eIasOk, smxres);
  IasEventPtr event = nullptr;
  smxres = smartx->getNextEvent(&event);
  EXPECT_EQ(IasSmartX::eIasOk, smxres);
  sleep(1);

  setup->stopRoutingZone(speakerZone);
  sleep(1);
  setup->destroyAudioSourceDevice(stereo);
  stereo = nullptr;

  setup->destroyAudioPort(sinkInputPort1);
  sinkInputPort1 = nullptr;
  setup->destroyAudioPort(zoneInputPort1);
  zoneInputPort1 = nullptr;

  smartx->stop();

  setres = setup->createAudioSourceDevice(sourceParams, &stereo);
  ASSERT_TRUE(setres == IasISetup::eIasOk);

  setres =setup->createAudioPort(sourcePortParams, &sourceOutputPort);
  ASSERT_TRUE(setres == IasISetup::eIasOk);

  setres = setup->addAudioOutputPort(stereo, sourceOutputPort);
  ASSERT_TRUE(setres == IasISetup::eIasOk);

  setres = setup->createAudioSinkDevice(sinkParams, &speaker);
  ASSERT_TRUE(setres == IasISetup::eIasFailed);

  setres =setup->createAudioPort(sinkPortParams, &sinkInputPort1);
  ASSERT_TRUE(setres == IasISetup::eIasOk);

  setres = setup->addAudioInputPort(speaker, sinkInputPort1);
  ASSERT_TRUE(setres == IasISetup::eIasOk);

  setres = setup->createRoutingZone(routingZoneParams, &speakerZone);
  ASSERT_TRUE(setres == IasISetup::eIasFailed);

  setres = setup->link(speakerZone, speaker);
  ASSERT_TRUE(setres == IasISetup::eIasFailed);

  setres = setup->createAudioPort(zonePortParams, &zoneInputPort1);
  ASSERT_TRUE(setres == IasISetup::eIasOk);

  setres = setup->addAudioInputPort(speakerZone, zoneInputPort1);
  ASSERT_TRUE(setres == IasISetup::eIasFailed);

  setres = setup->link(zoneInputPort1, sinkInputPort1);
  ASSERT_TRUE(setres == IasISetup::eIasFailed);

  smxres = smartx->start();
  ASSERT_TRUE(smxres == IasSmartX::eIasFailed);

  connres = routing->connect(1,1);
  ASSERT_TRUE(connres == IasIRouting::eIasFailed);

  connres = routing->disconnect(1,1);
  ASSERT_TRUE(connres == IasIRouting::eIasFailed);

  IasSmartX::destroy(smartx);
}
}

