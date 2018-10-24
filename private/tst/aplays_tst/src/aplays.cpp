/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/*
 * parallelPlay.cpp
 *
 *  Created 2017
 */


#include <iostream>
#include <iomanip>
#include <boost/algorithm/string/split.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include "audio/smartx/IasSmartX.hpp"
#include "audio/smartx/IasIRouting.hpp"
#include "audio/smartx/IasISetup.hpp"
#include "audio/smartx/IasIDebug.hpp"
#include "audio/smartx/IasIProcessing.hpp"
#include "audio/smartx/IasSetupHelper.hpp"
#include "audio/smartx/IasEventHandler.hpp"
#include "audio/smartx/IasEvent.hpp"
#include "audio/smartx/IasConnectionEvent.hpp"
#include "audio/smartx/IasSetupEvent.hpp"
#include "audio/smartx/rtprocessingfwx/IasModuleEvent.hpp"
#include "audio/smartx/IasProperties.hpp"
#include "audio/smartx/rtprocessingfwx/IasProcessingTypes.hpp"

#include "audio/volumex/IasVolumeCmd.hpp"           // the header file of the volume plug-in module
#include "audio/mixerx/IasMixerCmd.hpp"             // the header file of the mixer plug-in module
#include <type_traits>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <type_traits>
#include <thread>
#include <future>
#include <gtest/gtest.h>

using namespace IasAudio;
using namespace std;
using namespace IasAudio::IasSetupHelper;

#define ADD_ROOT(filename) "/nfs/ka/disks/ias_organisation_disk001/teams/audio/TestWavRefFiles/2014-06-02/" filename

enum IasResult
{
  eIasOk,
  eIasFailed
};

static int32_t sourceId = 0;
static int32_t sinkId = 0;

static bool noAvb = false;
static bool isHost = true;


void createSourceSmartXplugin_stereo0(IasISetup *setup)
{
  IasAudioDeviceParams sourceParams;
  sourceParams.clockType = IasAudio::eIasClockProvided;
  sourceParams.dataFormat = IasAudio::eIasFormatInt16;
  sourceParams.name = "stereo0";
  sourceParams.numChannels = 2;
  sourceParams.numPeriods = 4;
  sourceParams.periodSize = 1920;
  sourceParams.samplerate = 48000;
  IasAudioSourceDevicePtr stereo0;
  IasISetup::IasResult setupRes = IasSetupHelper::createAudioSourceDevice(setup, sourceParams, sourceId++, &stereo0);
  if (setupRes != IasISetup::eIasOk)
  {
    cerr << "Error creating " << sourceParams.name << ": " << toString(setupRes) << endl;
  }
}

void createSourceSmartXplugin_mono(IasISetup *setup)
{
  IasAudioDeviceParams sourceParams;
  sourceParams.clockType = IasAudio::eIasClockProvided;
  sourceParams.dataFormat = IasAudio::eIasFormatInt16;
  sourceParams.name = "mono";
  sourceParams.numChannels = 1;
  sourceParams.numPeriods = 4;
  sourceParams.periodSize = 192;
  sourceParams.samplerate = 48000;
  IasAudioSourceDevicePtr mono;
  IasISetup::IasResult setupRes = IasSetupHelper::createAudioSourceDevice(setup, sourceParams, sourceId++, &mono);
  if (setupRes != IasISetup::eIasOk)
  {
    cerr << "Error creating " << sourceParams.name << ": " << toString(setupRes) << endl;
  }
}

void createSourceSmartXplugin_stereo1(IasISetup *setup)
{
  IasAudioDeviceParams sourceParams;
  sourceParams.clockType = IasAudio::eIasClockProvided;
  sourceParams.dataFormat = IasAudio::eIasFormatInt16;
  sourceParams.name = "stereo1";
  sourceParams.numChannels = 2;
  sourceParams.numPeriods = 4;
  sourceParams.periodSize = 1920;
  sourceParams.samplerate = 48000;
  IasAudioSourceDevicePtr stereo1;
  IasISetup::IasResult setupRes = IasSetupHelper::createAudioSourceDevice(setup, sourceParams, sourceId++, &stereo1);
  if (setupRes != IasISetup::eIasOk)
  {
    cerr << "Error creating " << sourceParams.name << ": " << toString(setupRes) << endl;
  }
}

void createSourceSmartXplugin_44100(IasISetup *setup)
{
  IasAudioDeviceParams sourceParams;
  sourceParams.clockType = IasAudio::eIasClockProvided;
  sourceParams.dataFormat = IasAudio::eIasFormatInt16;
  sourceParams.name = "stereoIn44";
  sourceParams.numChannels = 2;
  sourceParams.numPeriods = 3;
  sourceParams.periodSize = 192;
  sourceParams.samplerate = 44100;
  IasAudioSourceDevicePtr stereo0;
  IasISetup::IasResult setupRes = IasSetupHelper::createAudioSourceDevice(setup, sourceParams, sourceId++, &stereo0);
  if (setupRes != IasISetup::eIasOk)
  {
    cerr << "Error creating " << sourceParams.name << ": " << toString(setupRes) << endl;
  }
}

/* aux */
void createSourceAlsaHandler_aux(IasISetup *setup)
{
  IasAudioDeviceParams sourceParams;
  sourceParams.clockType = IasAudio::eIasClockReceivedAsync;
  sourceParams.dataFormat = IasAudio::eIasFormatInt32;
  sourceParams.name = "hw:broxtongpmrb,2";
  sourceParams.numChannels = 2;
  sourceParams.numPeriods = 4;
  sourceParams.periodSize = 576;
  sourceParams.samplerate = 48000;
  IasAudioSourceDevicePtr aux;
  IasISetup::IasResult setupRes = setup->createAudioSourceDevice(sourceParams, &aux);
  if (setupRes != IasISetup::eIasOk)
  {
    cerr << "Error creating " << sourceParams.name << ": " << toString(setupRes) << endl;
  }

  // Create aux port
  IasAudioPortParams sourcePortParams;
  sourcePortParams.name = "aux";
  sourcePortParams.id = sourceId++;
  sourcePortParams.index = 0;
  sourcePortParams.numChannels = 2;
  sourcePortParams.direction = eIasPortDirectionOutput;
  IasAudioPortPtr port = nullptr;
  setupRes = setup->createAudioPort(sourcePortParams, &port);
  if (setupRes != IasISetup::eIasOk)
  {
    cerr << "Error creating " << sourcePortParams.name << ": " << toString(setupRes) << endl;
  }
  setupRes = setup->addAudioOutputPort(aux, port);
  if (setupRes != IasISetup::eIasOk)
  {
    cerr << "Error adding " << sourcePortParams.name << ": " << toString(setupRes) << endl;
  }

}

/* mic */
void createSourceAlsaHandler_mic(IasISetup *setup)
{
  IasAudioDeviceParams sourceParams;
  sourceParams.clockType = IasAudio::eIasClockReceivedAsync;
  sourceParams.dataFormat = IasAudio::eIasFormatInt32;
  sourceParams.name = "hw:broxtongpmrb,3";
  sourceParams.numChannels = 1;
  sourceParams.numPeriods = 4;
  sourceParams.periodSize = 576;
  sourceParams.samplerate = 48000;
  IasAudioSourceDevicePtr mic;
  IasISetup::IasResult setupRes = setup->createAudioSourceDevice(sourceParams, &mic);
  if (setupRes != IasISetup::eIasOk)
  {
    cerr << "Error creating " << sourceParams.name << ": " << toString(setupRes) << endl;
  }

  // Create mic port
  IasAudioPortParams sourcePortParams;
  sourcePortParams.name = "mic";
  sourcePortParams.id = sourceId++;
  sourcePortParams.numChannels = 1;
  sourcePortParams.index = 0;
  sourcePortParams.direction = eIasPortDirectionOutput;
  IasAudioPortPtr port = nullptr;
  setupRes = setup->createAudioPort(sourcePortParams, &port);
  if (setupRes != IasISetup::eIasOk)
  {
    cerr << "Error creating " << sourcePortParams.name << ": " << toString(setupRes) << endl;
  }
  setupRes = setup->addAudioOutputPort(mic, port);
  if (setupRes != IasISetup::eIasOk)
  {
    cerr << "Error adding " << sourcePortParams.name << ": " << toString(setupRes) << endl;
  }
}

/* Tuner */
void createSourceAlsaHandler_tuner(IasISetup *setup)
{
  IasAudioDeviceParams sourceParams;
  sourceParams.clockType = IasAudio::eIasClockReceivedAsync;
  sourceParams.dataFormat = IasAudio::eIasFormatInt32;
  sourceParams.name = "hw:broxtongpmrb,1";
  sourceParams.numChannels = 2;
  sourceParams.numPeriods = 4;
  sourceParams.periodSize = 576;
  sourceParams.samplerate = 48000;
  IasAudioSourceDevicePtr tun;
  IasISetup::IasResult setupRes = setup->createAudioSourceDevice(sourceParams, &tun);
  if (setupRes != IasISetup::eIasOk)
  {
    cerr << "Error creating " << sourceParams.name << ": " << toString(setupRes) << endl;
  }

  // Create tuner port
  IasAudioPortParams sourcePortParams;
  sourcePortParams.name = "tuner";
  sourcePortParams.numChannels = 2;
  sourcePortParams.id = sourceId++;
  sourcePortParams.direction = eIasPortDirectionOutput;
  sourcePortParams.index = 0;
  IasAudioPortPtr port = nullptr;
  setupRes = setup->createAudioPort(sourcePortParams, &port);
  if (setupRes != IasISetup::eIasOk)
  {
    cerr << "Error creating " << sourcePortParams.name << ": " << toString(setupRes) << endl;
  }
  setupRes = setup->addAudioOutputPort(tun, port);
  if (setupRes != IasISetup::eIasOk)
  {
    cerr << "Error adding " << sourcePortParams.name << ": " << toString(setupRes) << endl;
  }
}

void createSourceAlsaHandler_avb_stereo_0(IasISetup *setup)
{
  IasAudioDeviceParams sourceParams;
  sourceParams.clockType = IasAudio::eIasClockReceived;
  sourceParams.dataFormat = IasAudio::eIasFormatInt16;
  sourceParams.name = "plug:avb:stereo_0";
  sourceParams.numChannels = 2;
  sourceParams.numPeriods = 3;
  sourceParams.periodSize = 192;
  sourceParams.samplerate = 48000;
  IasAudioSourceDevicePtr avbStereo0_input;
  IasISetup::IasResult setupRes = IasSetupHelper::createAudioSourceDevice(setup, sourceParams, sourceId++, &avbStereo0_input);
  if (setupRes != IasISetup::eIasOk)
  {
    cerr << "Error creating " << sourceParams.name << ": " << toString(setupRes) << endl;
  }
}

void createSourceAlsaHandler_avb_stereo_1(IasISetup *setup)
{
  IasAudioDeviceParams sourceParams;
  sourceParams.clockType = IasAudio::eIasClockReceived;
  sourceParams.dataFormat = IasAudio::eIasFormatInt16;
  sourceParams.name = "plug:avb:stereo_1";
  sourceParams.numChannels = 2;
  sourceParams.numPeriods = 3;
  sourceParams.periodSize = 192;
  sourceParams.samplerate = 48000;
  IasAudioSourceDevicePtr avbStereo1_input;
  IasISetup::IasResult setupRes = IasSetupHelper::createAudioSourceDevice(setup, sourceParams, sourceId++, &avbStereo1_input);
  if (setupRes != IasISetup::eIasOk)
  {
    cerr << "Error creating " << sourceParams.name << ": " << toString(setupRes) << endl;
  }
}

void createSourceAlsaHandler_avb_mc_0(IasISetup *setup)
{
  IasAudioDeviceParams sourceParams;
  sourceParams.clockType = IasAudio::eIasClockReceived;
  sourceParams.dataFormat = IasAudio::eIasFormatInt16;
  sourceParams.name = "plug:avb:mc_0";
  sourceParams.numChannels = 6;
  sourceParams.numPeriods = 3;
  sourceParams.periodSize = 192;
  sourceParams.samplerate = 48000;
  IasAudioSourceDevicePtr avbMC;
  IasISetup::IasResult setupRes = IasSetupHelper::createAudioSourceDevice(setup, sourceParams, sourceId++, &avbMC);
  if (setupRes != IasISetup::eIasOk)
  {
    cerr << "Error creating " << sourceParams.name << ": " << toString(setupRes) << endl;
  }
}

void createSourceAlsaHandler_avb_mc_1(IasISetup *setup)
{
  IasAudioDeviceParams sourceParams;
  sourceParams.clockType = IasAudio::eIasClockReceived;
  sourceParams.dataFormat = IasAudio::eIasFormatInt16;
  sourceParams.name = "plug:avb:mc_1";
  sourceParams.numChannels = 6;
  sourceParams.numPeriods = 3;
  sourceParams.periodSize = 192;
  sourceParams.samplerate = 48000;
  IasAudioSourceDevicePtr avbMC;
  IasISetup::IasResult setupRes = IasSetupHelper::createAudioSourceDevice(setup, sourceParams, sourceId++, &avbMC);
  if (setupRes != IasISetup::eIasOk)
  {
    cerr << "Error creating " << sourceParams.name << ": " << toString(setupRes) << endl;
  }
}

void createSinkAlsaHandler_map_2_4(IasISetup *setup,
                                   IasRoutingZonePtr *rzn,
                                   IasAudioPortPtr *zonePortStereo,
                                   IasAudioPortPtr *zonePortMono,
                                   IasAudioPortPtr *sinkPort)
{
  IasAudioDeviceParams sinkParams;
  sinkParams.clockType = IasAudio::eIasClockReceived;
  sinkParams.dataFormat = IasAudio::eIasFormatInt16;
  sinkParams.name = "map_2_4";
  sinkParams.numChannels = 2;
  sinkParams.numPeriods = 4;
  sinkParams.periodSize = 192;
  sinkParams.samplerate = 48000;
  IasAudioSinkDevicePtr speaker;
  IasRoutingZonePtr speakerZone;


  IasISetup::IasResult result = setup->createAudioSinkDevice(sinkParams, &speaker);
  if (result != IasISetup::eIasOk)
  {
    cout << "Error creating speaker sink device" <<endl;
  }

  // Create sink port and add to sink device
  IasAudioPortParams sinkPortParams =
  {
   sinkParams.name + "_port",
   2,
   -1,
   eIasPortDirectionInput,
   0
  };

  result = setup->createAudioPort(sinkPortParams, sinkPort);
  IAS_ASSERT(result == IasISetup::eIasOk);
  result = setup->addAudioInputPort(speaker, *sinkPort);
  IAS_ASSERT(result == IasISetup::eIasOk);

  // Create a routing zone
  IasRoutingZoneParams routingZoneParams =
  {
   sinkParams.name + "_rz"
  };
  IasRoutingZonePtr myRoutingZone = nullptr;
  result = setup->createRoutingZone(routingZoneParams, &myRoutingZone);
  IAS_ASSERT(result == IasISetup::eIasOk);

  // Link the routing zone with the sink
  result = setup->link(myRoutingZone, speaker);
  IAS_ASSERT(result == IasISetup::eIasOk);

  // Create a zone port
  IasAudioPortParams zonePortParamsStereo =
  {
   sinkParams.name + "_rznport_stereo",
   2,
   sinkId++,
   eIasPortDirectionInput,
   0
  };

  // Create a zone port
  IasAudioPortParams zonePortParamsMono =
  {
   sinkParams.name + "_rznport_mono",
   1,
   sinkId++,
   eIasPortDirectionInput,
   0
  };


  result = setup->createAudioPort(zonePortParamsStereo, zonePortStereo);
  IAS_ASSERT(result == IasISetup::eIasOk);
  result = setup->createAudioPort(zonePortParamsMono, zonePortMono);
  IAS_ASSERT(result == IasISetup::eIasOk);
  result = setup->addAudioInputPort(myRoutingZone, *zonePortStereo);
  IAS_ASSERT(result == IasISetup::eIasOk);
  result = setup->addAudioInputPort(myRoutingZone, *zonePortMono);
  IAS_ASSERT(result == IasISetup::eIasOk);

  IAS_ASSERT(rzn != nullptr);
  *rzn = myRoutingZone;
}

void createSinkPlugin_Stereo0(IasISetup *setup, IasRoutingZonePtr *rzn)
{
  IasAudioDeviceParams sinkParams;
  sinkParams.clockType = IasAudio::eIasClockProvided;
  sinkParams.dataFormat = IasAudio::eIasFormatInt16;
  sinkParams.name = "stereoOut24";
  sinkParams.numChannels = 2;
  sinkParams.numPeriods = 4;
  sinkParams.periodSize = 96;
  sinkParams.samplerate = 24000;
  IasAudioSinkDevicePtr stereo;
  IasISetup::IasResult setupRes = IasSetupHelper::createAudioSinkDevice(setup, sinkParams, sinkId++, &stereo,rzn);
  if (setupRes != IasISetup::eIasOk)
  {
    cerr << "Error creating " << sinkParams.name << ": " << toString(setupRes) << endl;
  }
}

void createSinkPlugin_Stereo1(IasISetup *setup, IasRoutingZonePtr *rzn)
{
  IasAudioDeviceParams sinkParams;
  sinkParams.clockType = IasAudio::eIasClockProvided;
  sinkParams.dataFormat = IasAudio::eIasFormatInt16;
  sinkParams.name = "stereoOut16";
  sinkParams.numChannels = 2;
  sinkParams.numPeriods = 4;
  sinkParams.periodSize = 192;
  sinkParams.samplerate = 16000;
  IasAudioSinkDevicePtr stereo;
  IasISetup::IasResult setupRes = IasSetupHelper::createAudioSinkDevice(setup, sinkParams, sinkId++, &stereo,rzn);
  if (setupRes != IasISetup::eIasOk)
  {
    cerr << "Error creating " << sinkParams.name << ": " << toString(setupRes) << endl;
  }
}

void createSinkPlugin_Stereo2(IasISetup *setup, IasRoutingZonePtr *rzn)
{
  IasAudioDeviceParams sinkParams;
  sinkParams.clockType = IasAudio::eIasClockProvided;
  sinkParams.dataFormat = IasAudio::eIasFormatInt16;
  sinkParams.name = "stereoOut48_m1";
  sinkParams.numChannels = 2;
  sinkParams.numPeriods = 4;
  sinkParams.periodSize = 192;
  sinkParams.samplerate = 48000;
  IasAudioSinkDevicePtr stereo;
  IasISetup::IasResult setupRes = IasSetupHelper::createAudioSinkDevice(setup, sinkParams, sinkId++, &stereo,rzn);
  if (setupRes != IasISetup::eIasOk)
  {
    cerr << "Error creating " << sinkParams.name << ": " << toString(setupRes) << endl;
  }
}

void createSinkPlugin_Stereo3(IasISetup *setup, IasRoutingZonePtr *rzn)
{
  IasAudioDeviceParams sinkParams;
  sinkParams.clockType = IasAudio::eIasClockProvided;
  sinkParams.dataFormat = IasAudio::eIasFormatInt16;
  sinkParams.name = "stereoOut48_m2";
  sinkParams.numChannels = 2;
  sinkParams.numPeriods = 4;
  sinkParams.periodSize = 384;
  sinkParams.samplerate = 48000;
  IasAudioSinkDevicePtr stereo;
  IasISetup::IasResult setupRes = IasSetupHelper::createAudioSinkDevice(setup, sinkParams, sinkId++, &stereo,rzn);
  if (setupRes != IasISetup::eIasOk)
  {
    cerr << "Error creating " << sinkParams.name << ": " << toString(setupRes) << endl;
  }
}

void createSinkAlsaHandler_avb_stereo_0(IasISetup *setup, IasRoutingZonePtr *rzn)
{
  IasAudioDeviceParams sinkParams;
  sinkParams.clockType = IasAudio::eIasClockReceived;
  sinkParams.dataFormat = IasAudio::eIasFormatInt16;
  sinkParams.name = "plug:avb:stereo_0";
  sinkParams.numChannels = 2;
  sinkParams.numPeriods = 3;
  sinkParams.periodSize = 192;
  sinkParams.samplerate = 48000;
  IasAudioSinkDevicePtr avbStereo0;
  IasRoutingZonePtr avbStereo0Zone;
  IasISetup::IasResult setupRes = IasSetupHelper::createAudioSinkDevice(setup, sinkParams, sinkId++, &avbStereo0, &avbStereo0Zone);
  if (setupRes != IasISetup::eIasOk)
  {
    cerr << "Error creating " << sinkParams.name << ": " << toString(setupRes) << endl;
  }
  IAS_ASSERT(rzn != nullptr);
  *rzn = avbStereo0Zone;
}

void createSinkAlsaHandler_avb_stereo_1(IasISetup *setup, IasRoutingZonePtr *rzn)
{
  IasAudioDeviceParams sinkParams;
  sinkParams.clockType = IasAudio::eIasClockReceived;
  sinkParams.dataFormat = IasAudio::eIasFormatInt16;
  sinkParams.name = "plug:avb:stereo_1";
  sinkParams.numChannels = 2;
  sinkParams.numPeriods = 3;
  sinkParams.periodSize = 192;
  sinkParams.samplerate = 48000;
  IasAudioSinkDevicePtr avbStereo1;
  IasRoutingZonePtr avbStereo1Zone;
  IasISetup::IasResult setupRes = IasSetupHelper::createAudioSinkDevice(setup, sinkParams, sinkId++, &avbStereo1, &avbStereo1Zone);
  if (setupRes != IasISetup::eIasOk)
  {
    cerr << "Error creating " << sinkParams.name << ": " << toString(setupRes) << endl;
  }
  IAS_ASSERT(rzn != nullptr);
  *rzn = avbStereo1Zone;
}

void createSinkAlsaHandler_avb_mc_0(IasISetup *setup, IasRoutingZonePtr *rzn)
{
  IasAudioDeviceParams sinkParams;
  sinkParams.clockType = IasAudio::eIasClockReceived;
  sinkParams.dataFormat = IasAudio::eIasFormatInt16;
  sinkParams.name = "plug:avb:mc_0";
  sinkParams.numChannels = 6;
  sinkParams.numPeriods = 3;
  sinkParams.periodSize = 192;
  sinkParams.samplerate = 48000;
  IasAudioSinkDevicePtr avbMC0;
  IasRoutingZonePtr avbMC0Zone;
  IasISetup::IasResult setupRes = IasSetupHelper::createAudioSinkDevice(setup, sinkParams, sinkId++, &avbMC0, &avbMC0Zone);
  if (setupRes != IasISetup::eIasOk)
  {
    cerr << "Error creating " << sinkParams.name << ": " << toString(setupRes) << endl;
  }
  IAS_ASSERT(rzn != nullptr);
  *rzn = avbMC0Zone;
}

void createSinkAlsaHandler_avb_mc_1(IasISetup *setup, IasRoutingZonePtr *rzn)
{
  IasAudioDeviceParams sinkParams;
  sinkParams.clockType = IasAudio::eIasClockReceived;
  sinkParams.dataFormat = IasAudio::eIasFormatInt16;
  sinkParams.name = "plug:avb:mc_1";
  sinkParams.numChannels = 6;
  sinkParams.numPeriods = 3;
  sinkParams.periodSize = 192;
  sinkParams.samplerate = 48000;
  IasAudioSinkDevicePtr avbMC1;
  IasRoutingZonePtr avbMC1Zone;
  IasISetup::IasResult setupRes = IasSetupHelper::createAudioSinkDevice(setup, sinkParams, sinkId++, &avbMC1, &avbMC1Zone);
  if (setupRes != IasISetup::eIasOk)
  {
    cerr << "Error creating " << sinkParams.name << ": " << toString(setupRes) << endl;
  }
  IAS_ASSERT(rzn != nullptr);
  *rzn = avbMC1Zone;
}

void staticSetup(IasSmartX *smartx)
{
  IasISetup *setup = smartx->setup();

  // Create the source devices
  createSourceSmartXplugin_stereo0(setup);
  createSourceSmartXplugin_stereo1(setup);
  createSourceSmartXplugin_mono(setup);
  createSourceSmartXplugin_44100(setup);
  if(isHost == false)
  {
    createSourceAlsaHandler_tuner(setup);
    createSourceAlsaHandler_aux(setup);
    createSourceAlsaHandler_mic(setup);
  }
  if(noAvb == false)
  {
    createSourceAlsaHandler_avb_stereo_0(setup);
    createSourceAlsaHandler_avb_stereo_1(setup);
    createSourceAlsaHandler_avb_mc_0(setup);
    createSourceAlsaHandler_avb_mc_1(setup);
  }

#if BT_ACTIVE
  // BT is not working yet. FW crashes when BT devices are being opened
  // without an active clock.
  if(isHost == false)
  {
    createSourceAlsaHandler_bt_dl(setup);
  }
#endif

  // Create the sink devices
  IasRoutingZonePtr avbstereo0 = nullptr;
  IasRoutingZonePtr avbstereo1 = nullptr;
  IasRoutingZonePtr avbmc0 = nullptr;
  IasRoutingZonePtr avbmc1 = nullptr;
#if BT_ACTIVE
  IasRoutingZonePtr bt_ul = nullptr;
#endif
  IasRoutingZonePtr map_2_4 = nullptr;
  IasRoutingZonePtr rz_stereo = nullptr;
  IasRoutingZonePtr rz_stereo1 = nullptr;
  IasRoutingZonePtr rz_stereo2 = nullptr;
  IasRoutingZonePtr rz_stereo3 = nullptr;
  IasAudioPortPtr zoneInPortStereo = nullptr;
  IasAudioPortPtr zoneInPortMono = nullptr;
  IasAudioPortPtr zoneOutPort = nullptr;
  createSinkAlsaHandler_map_2_4(setup, &map_2_4,&zoneInPortStereo, &zoneInPortMono, &zoneOutPort);
  createSinkPlugin_Stereo0(setup,&rz_stereo);
  createSinkPlugin_Stereo1(setup,&rz_stereo1);
  createSinkPlugin_Stereo2(setup,&rz_stereo2);
  createSinkPlugin_Stereo3(setup,&rz_stereo3);
  if(noAvb == false)
  {
    createSinkAlsaHandler_avb_stereo_0(setup, &avbstereo0);
    createSinkAlsaHandler_avb_stereo_1(setup, &avbstereo1);
    createSinkAlsaHandler_avb_mc_0(setup, &avbmc0);
    createSinkAlsaHandler_avb_mc_1(setup, &avbmc1);
  }

  setup->addDerivedZone(map_2_4, rz_stereo);
  setup->addDerivedZone(map_2_4, rz_stereo1);
  setup->addDerivedZone(map_2_4, rz_stereo2);
  setup->addDerivedZone(map_2_4, rz_stereo3);

#if BT_ACTIVE
  createSinkAlsaHandler_bt_ul(setup, &bt_ul);
#else
  (void)map_2_4;
#endif

  if (avbstereo0 != nullptr && avbstereo1 != nullptr &&
      avbmc0 != nullptr && avbmc1 != nullptr )
  {
    IasISetup::IasResult setupRes = setup->addDerivedZone(avbstereo0, avbstereo1);
    if (setupRes != IasISetup::eIasOk)
    {
      cerr << "Error adding derived zone avbstereo1 to base zone avbstereo0: " << toString(setupRes) << endl;
    }
    setupRes = setup->addDerivedZone(avbstereo0, avbmc0);
    if (setupRes != IasISetup::eIasOk)
    {
      cerr << "Error adding derived zone avbmc0 to base zone avbstereo0: " << toString(setupRes) << endl;
    }
    setupRes = setup->addDerivedZone(avbstereo0, avbmc1);
    if (setupRes != IasISetup::eIasOk)
    {
      cerr << "Error adding derived zone avbmc1 to base zone avbstereo0: " << toString(setupRes) << endl;
    }
  }
#if BT_ACTIVE
  if (map_2_4 != nullptr && bt_ul != nullptr)
  {
    IasISetup::IasResult setupRes = setup->addDerivedZone(map_2_4, bt_ul);
    if (setupRes != IasISetup::eIasOk)
    {
      cerr << "Error adding derived zone bt_ul to base zone map_2_4: " << toString(setupRes) << endl;
    }
  }
#endif
  IasSmartX::IasResult smxres = smartx->start();
  // We are not interested in the result of the start. Details about what can be started and what not can be found
  // in the DLT. A printout here is just distracting.
  (void)smxres;
}

IasISetup::IasResult connect(IasIRouting *routing, int sourceId, int sinkId)
{
  cout << "Connect Source Id=" << sourceId << " Sink Id=" << sinkId << endl;
  if (routing->connect(sourceId, sinkId) == IasIRouting::eIasOk)
  {
    cout << "Successfully connected Source Id=" << sourceId << " with Sink Id=" << sinkId << endl;
    return IasISetup::eIasOk;
  }
  else
  {
    cerr << "Error connecting Source Id=" << sourceId << " with Sink Id=" << sinkId << endl;
    return IasISetup::eIasFailed;
  }
}

IasISetup::IasResult disconnect(IasIRouting *routing, int sourceId, int sinkId)
{
  cout << "Disconnect Source Id=" << sourceId << " Sink Id=" << sinkId << endl;
  if (routing->disconnect(sourceId, sinkId) == IasIRouting::eIasOk)
  {
    cout << "Successfully disconnected Source Id=" << sourceId << " with Sink Id=" << sinkId << endl;
    return IasISetup::eIasOk;
  }
  else
  {
    cerr << "Error disconnecting Source Id=" << sourceId << " with Sink Id=" << sinkId << endl;
    return IasISetup::eIasFailed;
  }
}

using ProcessResult = std::pair<int, int>;


ProcessResult executeProcess(const char **cmd)
{
  int result = 0, status = 0;

#ifdef ENABLE_TIMEOUT
  int timeout_ms = 1000;
#endif

  const pid_t p = fork();

  if (p == 0)
  {
    result = execvp(cmd[0], (char **)cmd);
    if (result != 0)
    {
      std::cout << "Child process execve failed" << std::endl;
      return std::make_pair(result, status);
    }
  }

  while (waitpid(p , &status , WNOHANG) == 0)
  {
#ifdef ENABLE_TIMEOUT
    if(timeout_ms != 0)
    {
      if (--timeout < 0)
      {
        return std::make_pair(-1, 0);
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
#endif
  }

  if (1 != WIFEXITED(status) || 0 != WEXITSTATUS(status))
  {
    return std::make_pair(-1, WEXITSTATUS(status) );
  }

  return std::make_pair(0,0);
}


std::vector<ProcessResult> runProcesses(IasSmartX* smartX, const int numberOfProcesses, int sourceId, int sinkId, const char **cmd)
{
  connect(smartX->routing(), sourceId, sinkId);

  std::vector<std::future<ProcessResult>> processes;
  std::vector<ProcessResult>              processesResults;

  for(int i = 0; i < numberOfProcesses; i++)
  {
    processes.emplace_back(std::async(std::launch::async, executeProcess, cmd));
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  for(auto& proc : processes)
  {
    processesResults.emplace_back(proc.get());
  }

  disconnect(smartX->routing(), sourceId, sinkId);

  return processesResults;
}


TEST (ParallelPlayTest, RunBothAplay)
{
  DLT_REGISTER_APP("XBAR", "SmartXbar example");
  DLT_VERBOSE_MODE();

  IasSmartX *smartx = IasSmartX::create();
  EXPECT_NE(smartx, nullptr);

  if (smartx->isAtLeast(SMARTX_API_MAJOR, SMARTX_API_MINOR, SMARTX_API_PATCH) == false)
  {
    cerr << "SmartX API version does not match" << endl;
    IasSmartX::destroy(smartx);
    ASSERT_TRUE(false);
  }
  IasISetup *setup = smartx->setup();
  EXPECT_NE(setup, nullptr);

  IasIRouting *routing = smartx->routing();
  EXPECT_NE(routing, nullptr);

  const auto runAplays = [&](const int numberOfProcesses, int sourceId, int sinkId, const char **cmd)
  {
    return runProcesses(smartx, numberOfProcesses, sourceId, sinkId, cmd);
  };

  staticSetup(smartx);

  // Remove all shared mutexes if exist
  boost::interprocess::named_mutex::remove("smartx_mono_p_mtx");
  boost::interprocess::named_mutex::remove("smartx_stereo0_p_mtx");

  //
  // Run both aplay on mono device
  //
  const char* aplayMono[] = {
   "aplay",
   "-Dplug:smartx:mono",
   ADD_ROOT("sin400_mono.wav"),
   nullptr
  };
  const int monoSourceId = 2;
  const int monoSinkId = 1;
  int numOfThreads {};

  numOfThreads = 2;
  auto results = runAplays(numOfThreads, monoSourceId, monoSinkId, aplayMono);

  EXPECT_EQ(numOfThreads, results.size());
  for(std::size_t i = 0; i< results.size(); i++)
  {
    if(i == 0) // only first prosess should be finished correct
    {
      EXPECT_EQ(results[i].first,  0);
      EXPECT_EQ(results[i].second, 0);
    }
    else
    {
      EXPECT_NE(results[i].first,  0);
      EXPECT_NE(results[i].second, 0);
    }
  }
  results.clear();

  const char* aplayStereo[] = {
   "aplay",
   "-Dplug:smartx:stereo0",
   ADD_ROOT("sin400_stereo.wav"),
   nullptr
  };

  const int stereoSourceId = 0;
  const int stereoSinkId = 0;

  //
  // Run both aplay on stereo device
  //
  results = runAplays(numOfThreads, stereoSourceId, stereoSinkId, aplayStereo);

  EXPECT_EQ(numOfThreads, results.size());
  for(std::size_t i = 0; i< results.size(); i++)
  {
    if(i == 0)
    {
      EXPECT_EQ(results[i].first,  0);
      EXPECT_EQ(results[i].second, 0);
    }
    else
    {
      EXPECT_NE(results[i].first,  0);
      EXPECT_NE(results[i].second, 0);
    }
  }
  results.clear();


  //
  // Run multiple aplay on stereo device
  //
  numOfThreads = 5;
  results = runAplays(5, stereoSourceId, stereoSinkId, aplayStereo);

  EXPECT_EQ(numOfThreads, results.size());
  for(std::size_t i = 0; i< results.size(); i++)
  {
    if(i == 0)
    {
      EXPECT_EQ(results[i].first,  0);
      EXPECT_EQ(results[i].second, 0);
    }
    else
    {
      EXPECT_NE(results[i].first,  0);
      EXPECT_NE(results[i].second, 0);
    }
  }
  results.clear();


  IasSmartX::IasResult smres = smartx->stop();
  if(smres == IasSmartX::eIasOk)
  {
    IasSmartX::destroy(smartx);
  }
  else
  {
    cout << "Error stopping SmartX" <<endl;
    ASSERT_TRUE(false);
  }
}

int main(int argc, char* argv[])
{
  setenv("DLT_INITIAL_LOG_LEVEL", "::6", true);
  DLT_REGISTER_APP("TST", "Test Application");
  //DLT_ENABLE_LOCAL_PRINT();
  ::testing::InitGoogleTest(&argc, argv);
  int result = RUN_ALL_TESTS();
  DLT_UNREGISTER_APP();

  printf("test complete!\n");

  return result;
}

