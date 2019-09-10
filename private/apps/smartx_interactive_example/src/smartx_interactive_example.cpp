/*
 * Copyright (C) 2019 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file   main.cpp
 * @date   2015
 * @brief
 */

#include <iostream>
#include <iomanip>
#include <boost/algorithm/string/split.hpp>
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
#include "audio/configparser/IasSmartXconfigParser.hpp"
#include "audio/configparser/IasSmartXDebugFacade.hpp"
//#include "core_libraries/foundation/IasDebug.hpp"
#include "audio/volumex/IasVolumeCmd.hpp"           // the header file of the volume plug-in module
#include "audio/mixerx/IasMixerCmd.hpp"             // the header file of the mixer plug-in module
#include <type_traits>

// If defined than BT DL and BT UL devices, zones and ports will be created and started
#define BT_ACTIVE 0


using namespace IasAudio;
using namespace std;
using namespace IasAudio::IasSetupHelper;

static const std::string cClassName = "main::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"

enum IasResult
{
  eIasOk,
  eIasFailed
};



struct IasCmdHelp
{
  std::string cmd;
  std::string brief;
  std::string syntax;
  std::string params;
  std::string details;
};

using IasCmdMap = std::map<std::string, IasCmdHelp>;
static IasCmdMap cmds;
static int32_t sourceId = 0;
static int32_t sinkId = 0;

static bool noAvb=false;
static bool isHost = false;
static bool btActive = false;
static bool isConfigParser = false;

void generateHelp()
{
  IasCmdHelp cmd;

  cmd.cmd     = "c";
  cmd.syntax  = "c <source id> <sink id>";
  cmd.brief   = "connect source id with sink id";
  cmd.params  = "source id:   source id of an output port\n"
                "sink id:     sink id of an input port";
  cmd.details = "connect an output port specified by the source id with an input port specified by the sink id";
  cmds[cmd.cmd] = cmd;

  cmd.cmd     = "d";
  cmd.syntax  = "d <source id> <sink id>";
  cmd.brief   = "disconnect source id from sink id";
  cmd.params  = "source id:   source id of an output port\n"
                "sink id:     sink id of an input port";
  cmd.details = "disconnect an output port specified by the source id from an input port specified by the sink id";
  cmds[cmd.cmd] = cmd;

  cmd.cmd     = "lip";
  cmd.syntax  = "lip";
  cmd.brief   = "list input ports";
  cmd.params  = "";
  cmd.details = "list all configured and available audio input ports and their id";
  cmds[cmd.cmd] = cmd;

  cmd.cmd     = "lop";
  cmd.syntax  = "lop";
  cmd.brief   = "list output ports";
  cmd.params  = "";
  cmd.details = "list all configured and available audio output ports and their id";
  cmds[cmd.cmd] = cmd;

  cmd.cmd     = "lpp";
  cmd.syntax  = "lpp";
  cmd.brief   = "list all probeable ports";
  cmd.params  = "";
  cmd.details = "list all probeable audio ports with some details";
  cmds[cmd.cmd] = cmd;

  cmd.cmd     = "gct";
  cmd.syntax  = "gct";
  cmd.brief   = "get connection table";
  cmd.params  = "";
  cmd.details = "list all active connections";
  cmds[cmd.cmd] = cmd;

  cmd.cmd     = "gsg";
  cmd.syntax  = "gsg";
  cmd.brief   = "get source groups";
  cmd.params  = "";
  cmd.details = "list all sources that are grouped together to a logical source";
  cmds[cmd.cmd] = cmd;

  cmd.cmd     = "gds";
  cmd.syntax  = "gds";
  cmd.brief   = "get dummy sources";
  cmd.params  = "";
  cmd.details = "list all sources that are not really connected, but the data is ignored to keep the ASLA devices running";
  cmds[cmd.cmd] = cmd;

  cmd.cmd     = "ipd";
  cmd.syntax  = "ipd <dataFilePrefix> <audioPortName> <numSeconds>";
  cmd.brief   = "inject probe data";
  cmd.params  = "dataFilePrefix:  the common part of the wave file name. Every wave file has to end with _chX.wav, where X is the channel number. The prefix must not have the _ at the end\n"
                "audioPortName:   the name of the audio port, where the data should be injected\n"
                "numSeconds:      the number of seconds to be injected";
  cmd.details = "injects data from one wave file per channel at the chosen audio port";
  cmds[cmd.cmd] = cmd;

  cmd.cmd     = "rpd";
  cmd.syntax  = "rpd <dataFilePrefix> <audioPortName> <numSeconds>";
  cmd.brief   = "record probe data";
  cmd.params  = "dataFilePrefix:  the common part of the wave file name. Every wave file ends with _chX.wav, where X is the channel number. The prefix must not have the _ at the end\n"
                "audioPortName:   the name of the audio port, where the data should be recorded\n"
                "numSeconds:      the number of seconds to be recorded";
  cmd.details = "records data to one wave file per channel at the chosen audio port";
  cmds[cmd.cmd] = cmd;

  cmd.cmd     = "spd";
  cmd.syntax  = "spd <audioPortName>";
  cmd.brief   = "stop probe data";
  cmd.params  = "audioPortName:   the name of the audio port, where the probing should stop";
  cmd.details = "stops any probing operation (inject or record) at the chosen audio port";
  cmds[cmd.cmd] = cmd;

  cmd.cmd     = "volume";
  cmd.syntax  = "volume <PinName> <VolumeLevel_dB/10>";
  cmd.brief   = "set volume for a special audio pin";
  cmd.params  = "PinName:   the name of the audio pin of the volume module\n"
                "VolumeLevel_dB/10: the volume level in dB/10, in the range of -1440 to 0\n";
  cmd.details = "the volume for the data associated with the pin name will be change to the new value with a fixed linear ramp\n";
  cmds[cmd.cmd] = cmd;

  cmd.cmd     = "mute";
  cmd.syntax  = "mute <PinName> <muteState>";
  cmd.brief   = "set mute state for a special audio pin";
  cmd.params  = "PinName:   the name of the audio pin of the volume module\n"
      "muteState: the mute state (1 -> muted , 0 -> not muted)";
  cmd.details = "the volume for the data associated with the pin name will be set linear zero or back to recent value\n";
  cmds[cmd.cmd] = cmd;

  cmd.cmd     = "loudness";
  cmd.syntax  = "loudness <PinName> <LoudnessState>";
  cmd.brief   = "turns loudness on or off";
  cmd.params  = "PinName:   the name of the audio pin of the volume module\n"
                "LoudnessState: loudness off (0) or on (!=0)\n";
  cmd.details = "the loudness for the data associated with the pin name will be turned off if LoudnessState== 0, else it will be turned on\n";
  cmds[cmd.cmd] = cmd;

  cmd.cmd     = "mixgain";
  cmd.syntax  = "mixgain <inputPinName> <gain_dB/10>";
  cmd.brief   = "set the gain for the input pin of the mixer";
  cmd.params  = "audioPinName: the name of the input pin, where the gain shall be applied to\n"
                "gain_dB/10:   the gain, expressed in dB/10, must be <=0, for example: -200 means -20.0 dB";
  cmd.details = "sets the gain for the input pin of the mixer";
  cmds[cmd.cmd] = cmd;

  cmd.cmd     = "mixfader";
  cmd.syntax  = "mixfader <inputPinName> <faderAttenuation_dB/10>";
  cmd.brief   = "set the fader value for an input pin of the mixer";
  cmd.params  = "inputPinName:  the name of the audio input pin, \n"
                "faderPosition: the fader position (front/rear). 0 will be Center, > 0 will attenuate the rear channels, < 0 will attenuate the front channels. All values are in dB/10 ";
  cmd.details = "set the fader value for an input pin of the mixer";
  cmds[cmd.cmd] = cmd;

  cmd.cmd     = "mixbalance";
  cmd.syntax  = "mixbalance <inputPinName> <balanceAttenuation_dB/10>";
  cmd.brief   = "set the balance value for an input pin of the mixer";
  cmd.params  = "inputPinName:  the name of the audio input pin, \n"
                "balance Position: the balance position (left/right). 0 will be Center, > 0 will attenuate the left channels, < 0 will attenuate the right channels. All values are in dB/10 ";
  cmd.details = "set the balance value for an input pin of the mixer";
  cmds[cmd.cmd] = cmd;

  cmd.cmd     = "enablemixer";
  cmd.syntax  = "enablemixer <flag>";
  cmd.brief   = "enable or disable the mixer";
  cmd.params  = "flag:  1 enables the mixer, 0 disables the mixer";
  cmd.details = "if the mixer is disabled, no output will be there anymore!";
  cmds[cmd.cmd] = cmd;

  cmd.cmd     = "q";
  cmd.syntax  = "q";
  cmd.brief   = "quit";
  cmd.params  = "";
  cmd.details = "quit the program";
  cmds[cmd.cmd] = cmd;

  cmd.cmd     = "addsrc";
  cmd.syntax  = "addsrc <clkType> <dataFormat> <name> <numChannels> <numPeriods> <periodSize> <sampleRate>";
  cmd.brief   = "Add source device";
  cmd.params  = "clockType: provided, received, receivedasync\n"
                "dataFormat: 16, 32, 32f\n"
                "name: name of source device\n"
                "numChannels: number of channels\n"
                "numPeriods: the number of periods that the ring buffer consists of\n"
                "periodSize: period size in frames\n"
                 "sampleRate: the sample rate in Hz, e.g. 48000";
  cmd.details = "Add source device";
  cmds[cmd.cmd] = cmd;

  cmd.cmd     = "addsink";
  cmd.syntax  = "addsink <clkType> <dataFormat> <name> <numChannels> <numPeriods> <periodSize> <sampleRate>";
  cmd.brief   = "Add sink device";
  cmd.params  = "clockType: provided, received, receivedasync\n"
                "dataFormat: 16, 32, 32f\n"
                "name: name of sink device\n"
                "numChannels: number of channels\n"
                "numPeriods: the number of periods that the ring buffer consists of\n"
                "periodSize: period size in frames\n"
                "sampleRate: the sample rate in Hz, e.g. 48000";
  cmd.details = "Add sink device";
  cmds[cmd.cmd] = cmd;

  cmd.cmd     = "delsink";
  cmd.syntax  = "delsink <name>";
  cmd.brief   = "Delete sink device";
  cmd.params  = "name: name of sink device";
  cmd.details = "Delete sink device by the name";
  cmds[cmd.cmd] = cmd;

  cmd.cmd     = "delsrc";
  cmd.syntax  = "delsrc <name>";
  cmd.brief   = "Delete source device";
  cmd.params  = "name: name of source device";
  cmd.details = "Delete source device by the name";
  cmds[cmd.cmd] = cmd;

  cmd.cmd     = "startsrc";
  cmd.syntax  = "startsrc <name>";
  cmd.brief   = "Start source device";
  cmd.params  = "name: name of source device";
  cmd.details = "Start source device by the name";
  cmds[cmd.cmd] = cmd;

  cmd.cmd     = "stopsrc";
  cmd.syntax  = "stopsrc <name>";
  cmd.brief   = "Stop source device";
  cmd.params  = "name: name of source device";
  cmd.details = "Stop source device by the name";
  cmds[cmd.cmd] = cmd;

  cmd.cmd     = "startrz";
  cmd.syntax  = "startrz <name>";
  cmd.brief   = "Start routing zone";
  cmd.params  = "name: name of routing zone, if name of device is 'default', the name of routing zone is 'default_rz'";
  cmd.details = "Start routing zone by the name";
  cmds[cmd.cmd] = cmd;

  cmd.cmd     = "stoprz";
  cmd.syntax  = "stoprz <name>";
  cmd.brief   = "Stop routing zone";
  cmd.params  = "name: name of routing zone, if name of device is 'default', the name of routing zone is 'default_rz'";
  cmd.details = "Stop routing zone by the name";
  cmds[cmd.cmd] = cmd;

  cmd.cmd     = "gentop";
  cmd.syntax  = "gentop";
  cmd.brief   = "Generate topology";
  cmd.params  = "";
  cmd.details = "Generate topology to xml file: ./topology.xml";
  cmds[cmd.cmd] = cmd;
}

/* convert string to ClockType*/
IasAudio::IasClockType toClockTypeFormat(const std::string& clockType)
{
  if (clockType == "provided")
    return IasAudio::IasClockType::eIasClockProvided;
  else if (clockType == "received")
    return IasAudio::IasClockType::eIasClockReceived;
  else if (clockType == "receivedasync")
    return IasAudio::IasClockType::eIasClockReceivedAsync;
  else
    return IasAudio::IasClockType::eIasClockUndef;
}

/* convert string to AudioCommonDataFormat */
IasAudio::IasAudioCommonDataFormat toDataFormat(const std::string& dataFormat)
{
  if (dataFormat == "16")
    return IasAudio::IasAudioCommonDataFormat::eIasFormatInt16;
  else if (dataFormat == "32")
    return IasAudio::IasAudioCommonDataFormat::eIasFormatInt32;
  else if (dataFormat == "32f")
    return IasAudio::IasAudioCommonDataFormat::eIasFormatFloat32;
  else
    return IasAudio::IasAudioCommonDataFormat::eIasFormatUndef;
}

/* read parameters from stream */
IasAudioDeviceParams readParams()
{
  IasAudioDeviceParams params;
  std::string clockType;
  std::string dataFormat;

  cin
  >> clockType
  >> dataFormat
  >> params.name
  >> params.numChannels
  >> params.numPeriods
  >> params.periodSize
  >> params.samplerate;

  params.clockType = toClockTypeFormat(clockType);
  params.dataFormat = toDataFormat(dataFormat);

  return params;
}

void printParams(const IasAudioDeviceParams& params, const int32_t id)
{
  cout
  << "Id          : " << id << endl
  << "Clock type  : " << toString(params.clockType) << endl
  << "Data format : " << toString(params.dataFormat) << endl
  << "Name        : " << params.name << endl
  << "Channels    : " << params.numChannels << endl
  << "Periods     : " << params.numPeriods << endl
  << "Period Size : " << params.periodSize << endl
  << "Sample Rate : " << params.samplerate << endl;
}

void addSourceDevice(IasISetup* setup)
{
  const IasAudioDeviceParams sourceParams { readParams() };
  IasAudioSourceDevicePtr sourceDevice;
  const auto id = sourceId++;

  const auto setupRes = IasSetupHelper::createAudioSourceDevice(setup, sourceParams, id, &sourceDevice);

  if (setupRes == IasISetup::eIasOk)
  {
    cout << "Succesfully created source device : " << IasSetupHelper::getDeviceName(sourceDevice) << endl;
    printParams(sourceParams, id);
  }

  else
  {
    cerr << "Error creating " << sourceParams.name << ": " << toString(setupRes) << endl;
  }
}

void addSinkDevice(IasISetup* setup)
{
  const IasAudioDeviceParams sinkParams { readParams() };
  IasAudioSinkDevicePtr sinkDevice;
  IasRoutingZonePtr rz;
  const auto id = sinkId++;

  const auto setupRes = IasSetupHelper::createAudioSinkDevice(setup, sinkParams, id, &sinkDevice, &rz);

  if (setupRes == IasISetup::eIasOk)
  {
    cout << "Succesfully created sink device : " << IasSetupHelper::getDeviceName(sinkDevice) << endl;
    printParams(sinkParams, id);
  }

  else
  {
    cerr << "Error creating " << sinkParams.name << ": " << toString(setupRes) << endl;
  }
}

IasIRouting::IasResult disconnect(IasIRouting* const routing, const std::string& name)
{
  const auto connections = routing->getActiveConnections();

  for (const auto& connection : connections)
  {
    auto&& sourcePort = connection.first;
    auto&&   sinkPort = connection.second;

    const auto sourceName = getPortName(sourcePort);
    const auto   sinkName = getPortName(sinkPort);

    if (((name + "_port") == sourceName) || ((name + "_rznport") == sinkName))
    {
      const auto ids = std::make_pair(getPortId(sourcePort), getPortId(sinkPort));

      if (routing->disconnect(ids.first, ids.second) == IasIRouting::eIasOk)
      {
        cout << "Successfully disconnected: Source: "
             << sourceName << "(id= " << ids.first << ") and Sink: "
             << sinkName << "(id= " << ids.second << ")" << endl;
        return IasIRouting::eIasOk;
      }

      else
      {
        cerr << "Error disconnecting: "
             << sourceName << "(id= " << ids.first << ") and Sink: "
             << sinkName << "(id= " << ids.second << ")" << endl;
        return IasIRouting::eIasFailed;
      }
    }
  }
  return IasIRouting::eIasOk;
}

void deleteSourceDevice(IasISetup* setup, IasIRouting* routing)
{
  std::string name;
  cin >> name;

  const auto disconnectResult = disconnect(routing, name);

  if(disconnectResult == IasIRouting::eIasOk)
  {
    const auto result = IasSetupHelper::deleteSourceDevice(setup, name);

    if (result == IasISetup::eIasOk)
    {
      cout << "Succesfully deleted sink device : " << name << endl;
    }

    else
    {
      cerr << "Error deleting " << name << ": " << toString(result) << endl;
    }
  }

  else
  {
    cerr << "Cannot delete source because of Error: \"" << name << "\" cannot be disconnected" << endl;
  }
}

void deleteSinkDevice(IasISetup* setup, IasIRouting* routing)
{
  std::string name;
  cin >> name;

  const auto disconnectResult = disconnect(routing, name);

  if(disconnectResult == IasIRouting::eIasOk)
  {
    const auto result = IasSetupHelper::deleteSinkDevice(setup, name);

    if (result == IasISetup::eIasOk)
    {
      cout << "Succesfully deleted sink device : " << name << endl;
    }

    else
    {
      cerr << "Error deleting " << name << ": " << toString(result) << endl;
    }
  }

  else
  {
    cerr << "Cannot delete sink because of Error: \"" << name << "\" cannot be disconnected" << endl;
  }

}

void startSourceDevice(IasISetup* setup)
{
  std::string name;
  cin >> name;

  auto sourceDevice = setup->getAudioSourceDevice(name);

  if (sourceDevice != nullptr)
  {
    const auto result = setup->startAudioSourceDevice(sourceDevice);

    if (result == IasISetup::eIasOk)
    {
      cout << "Succesfully started source device : " << name << endl;
    }

    else
    {
      cerr << "Error starting " << name << ": " << toString(result) << endl;
    }
  }

  else
  {
    cout << "Source Device " << name << " not found" << endl;
  }
}

void stopSourceDevice(IasISetup* setup)
{
  std::string name;
  cin >> name;

  auto sourceDevice = setup->getAudioSourceDevice(name);

  if (sourceDevice != nullptr)
  {
    setup->stopAudioSourceDevice(sourceDevice);
    cout << "Succesfully stopped source device : " << name << endl;
  }

  else
  {
    cout << "Source Device " << name << " not found" << endl;
  }
}

void startRoutingZone(IasISetup* setup)
{
  std::string name;
  cin >> name;

  auto rz = setup->getRoutingZone(name);

  if (rz != nullptr)
  {
    const auto result = setup->startRoutingZone(rz);

    if (result == IasISetup::eIasOk)
    {
      cout << "Succesfully started routing zone : " << IasSetupHelper::getRoutingZoneName(rz) << endl;
    }

    else
    {
      cerr << "Error starting " << IasSetupHelper::getRoutingZoneName(rz) << ": " << toString(result) << endl;
    }
  }

  else
  {
    cout << "Routing zone " << name << " not found" << endl;
  }
}

void stopRoutingZone(IasISetup *setup)
{
  std::string name;
  cin >> name;

  const auto rz = setup->getRoutingZone(name);

  if (rz != nullptr)
  {
    setup->stopRoutingZone(rz);
    cout << "Succesfully stopped routing zone : " << name << endl;
  }

  else
  {
    cout << "Routing zone " << name << " not found" << endl;
  }
}

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
  sourceParams.name = "hw:broxtongpmrb,3";
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
  sourceParams.name = "hw:broxtongpmrb,4";
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
  sourceParams.name = "hw:broxtongpmrb,2";
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


void createSourceAlsaHandler_bt_dl(IasISetup *setup)
{
  IasAudioDeviceParams sourceParams;
  sourceParams.clockType = IasAudio::eIasClockReceivedAsync;
  sourceParams.dataFormat = IasAudio::eIasFormatInt16;
  sourceParams.name = "hw:broxtongpmrb,7";
  sourceParams.numChannels = 1;
  sourceParams.numPeriods = 4;
  sourceParams.periodSize = 96;
  sourceParams.samplerate = 24000;
  IasAudioSourceDevicePtr bt_dl;
  IasISetup::IasResult setupRes = setup->createAudioSourceDevice(sourceParams, &bt_dl);
  if (setupRes != IasISetup::eIasOk)
  {
    cerr << "Error creating " << sourceParams.name << ": " << toString(setupRes) << endl;
  }

  // Create bt_dl
  IasAudioPortParams sourcePortParams;
  sourcePortParams.name = "bt_dl";
  sourcePortParams.numChannels = 1;
  sourcePortParams.id = sourceId++;
  sourcePortParams.direction = eIasPortDirectionOutput;
  sourcePortParams.index = 0;
  IasAudioPortPtr port = nullptr;
  setupRes = setup->createAudioPort(sourcePortParams, &port);
  if (setupRes != IasISetup::eIasOk)
  {
    cerr << "Error creating " << sourcePortParams.name << ": " << toString(setupRes) << endl;
  }
  setupRes = setup->addAudioOutputPort(bt_dl, port);
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

void createSinkPlugin_Mono0(IasISetup *setup, IasRoutingZonePtr *rzn)
{
  IasAudioDeviceParams sinkParams;
  sinkParams.clockType = IasAudio::eIasClockProvided;
  sinkParams.dataFormat = IasAudio::eIasFormatInt16;
  sinkParams.name = "monoOut24";
  sinkParams.numChannels = 1;
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



void createSinkAlsaHandler_bt_ul(IasISetup *setup, IasRoutingZonePtr *rzn)
{
  IasAudioDeviceParams sinkParams;
  sinkParams.clockType = IasAudio::eIasClockReceivedAsync;
  sinkParams.dataFormat = IasAudio::eIasFormatInt16;
  sinkParams.name = "hw:broxtongpmrb,7";
  sinkParams.numChannels = 1;
  sinkParams.numPeriods = 4;
  sinkParams.periodSize = 96;
  sinkParams.samplerate = 24000;
  IasAudioSinkDevicePtr bt_ul;
  IasRoutingZonePtr bt_ul_zone;
  IasISetup::IasResult setupRes = IasSetupHelper::createAudioSinkDevice(setup, sinkParams, sinkId++, &bt_ul, &bt_ul_zone);
  if (setupRes != IasISetup::eIasOk)
  {
    cerr << "Error creating " << sinkParams.name << ": " << toString(setupRes) << endl;
  }
  IAS_ASSERT(rzn != nullptr);
  *rzn = bt_ul_zone;
}

void createPipeline(IasISetup *setup,
                    IasRoutingZonePtr zone,
                    IasAudioPortPtr zoneInPortStereo,
                    IasAudioPortPtr zoneInPortMono,
                    IasAudioPortPtr zoneOutPort)
{
  // Create the pipeline
  IasPipelineParams pipelineParams;
  pipelineParams.name ="ExamplePipeline";
  pipelineParams.samplerate = 48000;
  pipelineParams.periodSize = 192;

  IasPipelinePtr pipeline = nullptr;
  IasISetup::IasResult res = setup->createPipeline(pipelineParams, &pipeline);
  if(res != IasISetup::eIasOk)
  {
    // Report error and exit the program, because many of the follwing commands will result
    // in a segmentation fault if the pipeline has not been created pipeline appropriately.
    cout << "Error creating example pipeline (please review DLT output for details)" <<endl;
    exit(1);
  }

  IasAudioPinPtr pipelineInputStereoPin = nullptr;
  IasAudioPinParams pipelineInputStereoParams;
  pipelineInputStereoParams.name = "pipeInputStereo";
  pipelineInputStereoParams.numChannels = 2;


  IasAudioPinPtr pipelineInputMonoPin = nullptr;
  IasAudioPinParams pipelineInputMonoParams;
  pipelineInputMonoParams.name = "pipeInputMono";
  pipelineInputMonoParams.numChannels = 1;


  res = setup->createAudioPin(pipelineInputStereoParams, &pipelineInputStereoPin);
  if(res != IasISetup::eIasOk)
  {
    cout << "Error creating pipeline input stereo pin" <<endl;
  }

  res = setup->createAudioPin(pipelineInputMonoParams, &pipelineInputMonoPin);
  if(res != IasISetup::eIasOk)
  {
    cout << "Error creating pipeline input mono pin " <<endl;
  }


  IasAudioPinPtr pipelineOutputPin = nullptr;
  IasAudioPinParams pipelineOutputParams;
  pipelineOutputParams.name = "pipeOutputStereo";
  pipelineOutputParams.numChannels = 2;

  res = setup->createAudioPin(pipelineOutputParams, &pipelineOutputPin);
  if(res != IasISetup::eIasOk)
  {
    cout << "Error creating pipeline output stereo pin" <<endl;
  }

  res = setup->addAudioInputPin(pipeline,pipelineInputStereoPin);
  if(res != IasISetup::eIasOk)
  {
    cout << "Error adding pipeline input stereo pin" <<endl;
  }
  res = setup->addAudioInputPin(pipeline,pipelineInputMonoPin);
  if(res != IasISetup::eIasOk)
  {
    cout << "Error adding pipeline input mono pin" <<endl;
  }
  res = setup->addAudioOutputPin(pipeline, pipelineOutputPin);
  if(res != IasISetup::eIasOk)
  {
    cout << "Error adding pipeline output stereo pin" <<endl;
  }

  IasProcessingModuleParams volumeModuleParams;
  volumeModuleParams.typeName     = "ias.volume";
  volumeModuleParams.instanceName = "VolumeLoudness";
  IasProcessingModulePtr volume = nullptr;

  IasAudioPinPtr volumeStereoPin = nullptr;
  IasAudioPinParams volumeStereoPinParams;
  volumeStereoPinParams.name = "Volume_InOutStereoPin";
  volumeStereoPinParams.numChannels = 2;

  IasAudioPinPtr volumeMonoPin = nullptr;
  IasAudioPinParams volumeMonoPinParams;
  volumeMonoPinParams.name = "Volume_InOutMonoPin";
  volumeMonoPinParams.numChannels = 1;


  res = setup->createAudioPin(volumeStereoPinParams,&volumeStereoPin);
  if(res != IasISetup::eIasOk)
  {
    cout << "Error creating volume stereo pin" <<endl;
  }

  res = setup->createAudioPin(volumeMonoPinParams,&volumeMonoPin);
  if(res != IasISetup::eIasOk)
  {
    cout << "Error creating volume mono pin" <<endl;
  }

  res = setup->createProcessingModule(volumeModuleParams, &volume);
  if(res != IasISetup::eIasOk)
  {
    cout << "Error creating volume module" <<endl;
  }

  IasProperties volumeProperties;
  volumeProperties.set<int32_t>("numFilterBands",2);
  IasStringVector activePins;
  activePins.push_back("Volume_InOutStereoPin");
  activePins.push_back("Volume_InOutMonoPin");
  volumeProperties.set("activePinsForBand.0", activePins);
  volumeProperties.set("activePinsForBand.1", activePins);
  int32_t tempVol = 0;
  int32_t tempGain = 0;
  IasInt32Vector ldGains;
  IasInt32Vector ldVolumes;
  for(uint32_t i=0; i< 8; i++)
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
  if(res != IasISetup::eIasOk)
  {
    cout << "Error adding volume module to pipeline" <<endl;
  }

  res = setup->addAudioInOutPin(volume, volumeStereoPin);
  if(res != IasISetup::eIasOk)
  {
    cout << "Error adding volume stereo pin" <<endl;
  }
  res = setup->addAudioInOutPin(volume, volumeMonoPin);
  if(res != IasISetup::eIasOk)
  {
    cout << "Error adding volume mono pin" <<endl;
  }

  // Setup the parameters for the mixer module and create the module
  IasProcessingModuleParams mixerModuleParams;
  mixerModuleParams.typeName     = "ias.mixer";
  mixerModuleParams.instanceName = "Mixer";
  IasProcessingModulePtr mixer = nullptr;
  res = setup->createProcessingModule(mixerModuleParams, &mixer);
  if(res != IasISetup::eIasOk)
  {
    cout << "Error creating mixer module" <<endl;
  }

  IasAudioPinParams mixerInputStereoPinParams;
  mixerInputStereoPinParams.name        = "Mixer_InputPin_stereo";
  mixerInputStereoPinParams.numChannels = 2;
  IasAudioPinPtr mixerInputStereoPin = nullptr;
  res = setup->createAudioPin(mixerInputStereoPinParams, &mixerInputStereoPin);
  if(res != IasISetup::eIasOk)
  {
    cout << "Error creating mixer input stereo pin" <<endl;
  }

  IasAudioPinParams mixerInputMonoPinParams;
  mixerInputMonoPinParams.name        = "Mixer_InputPin_mono";
  mixerInputMonoPinParams.numChannels = 1;
  IasAudioPinPtr mixerInputMonoPin = nullptr;
  res = setup->createAudioPin(mixerInputMonoPinParams, &mixerInputMonoPin);
  if(res != IasISetup::eIasOk)
  {
    cout << "Error creating mixer input mono pin" <<endl;
  }


  IasAudioPinParams mixerOutputPinParams;
  mixerOutputPinParams.name        = "Mixer_OutputPin_stereo";
  mixerOutputPinParams.numChannels = 2;
  IasAudioPinPtr mixerOutputPin = nullptr;
  res = setup->createAudioPin(mixerOutputPinParams, &mixerOutputPin);
  if(res != IasISetup::eIasOk)
  {
    cout << "Error creating mixer output stereo pin" <<endl;
  }

  // Setup the configuration parameters for the mixer module
  IasProperties mixerProperties;
  mixerProperties.set<int32_t>("defaultGain", -60);
  setup->setProperties(mixer, mixerProperties);

  // Add the mixer module to the pipeline
  res = setup->addProcessingModule(pipeline, mixer);
  if(res != IasISetup::eIasOk)
  {
    cout << "Error adding mixer module to pipeline" <<endl;
  }

  // Add pin mapping (from simplemeixerInputPin to simplemixerOutputPin) to simplemixer.
  res = setup->addAudioPinMapping(mixer, mixerInputStereoPin, mixerOutputPin);
  if(res != IasISetup::eIasOk)
  {
    cout << "Error adding mixer pin mapping from mixerInputStereoPin to mixerOutputPin" <<endl;
  }

  res = setup->addAudioPinMapping(mixer, mixerInputMonoPin, mixerOutputPin);
  if(res != IasISetup::eIasOk)
  {
    cout << "Error adding mixer pin mapping from mixerInputMonoPin to mixerOutputPin" <<endl;
  }

  res = setup->link(pipelineInputStereoPin, volumeStereoPin, eIasAudioPinLinkTypeImmediate);
  if(res != IasISetup::eIasOk)
  {
    cout << "Error linking pipe input to volume stereo input" <<endl;
  }

  res = setup->link(pipelineInputMonoPin, volumeMonoPin, eIasAudioPinLinkTypeImmediate);
  if(res != IasISetup::eIasOk)
  {
    cout << "Error linking pipe input to mono volume mono input" <<endl;
  }



  // Create the links: volumeStereoPin -> mixerInputPin |mixer| mixerOutputPin -> pipelineOutputPin
  res = setup->link(volumeStereoPin, mixerInputStereoPin ,eIasAudioPinLinkTypeImmediate);
  if(res != IasISetup::eIasOk)
  {
    cout << "Error linking volume stereo output to mixer stereo input" <<endl;
  }

  res = setup->link(volumeMonoPin, mixerInputMonoPin ,eIasAudioPinLinkTypeImmediate);
  if(res != IasISetup::eIasOk)
  {
    cout << "Error linking volume mono output to mixer mono input" <<endl;
  }

  res = setup->link(mixerOutputPin, pipelineOutputPin ,eIasAudioPinLinkTypeImmediate);
  if(res != IasISetup::eIasOk)
  {
    cout << "Error linking mixer output to pipe out" <<endl;
  }
//#endif

  res = setup->initPipelineAudioChain(pipeline);
  if(res != IasISetup::eIasOk)
  {
    cout << "Error init pipeline chain - Please check DLT output for additional info." <<endl;
  }

  res = setup->addPipeline(zone, pipeline);
  if(res != IasISetup::eIasOk)
  {
    cout << "Error adding pipeline to zone" <<endl;
  }


  // Link the routing zone input ports to the pipeline input pins.
  res = setup->link(zoneInPortStereo, pipelineInputStereoPin);
  if(res != IasISetup::eIasOk)
  {
    cout << "Error linking zone input stereo port to pipeline stereo port" <<endl;
  }

  res = setup->link(zoneInPortMono, pipelineInputMonoPin);
  if(res != IasISetup::eIasOk)
  {
    cout << "Error linking zone input mono port to pipeline mono port" <<endl;
  }

  res = setup->link(zoneOutPort, pipelineOutputPin);
  if(res != IasISetup::eIasOk)
  {
    cout << "Error linking zone output port to pipeline" <<endl;
  }

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

  if (isHost == false && btActive == true)
  {
    createSourceAlsaHandler_bt_dl(setup);
  }

  // Create the sink devices
  IasRoutingZonePtr avbstereo0 = nullptr;
  IasRoutingZonePtr avbstereo1 = nullptr;
  IasRoutingZonePtr avbmc0 = nullptr;
  IasRoutingZonePtr avbmc1 = nullptr;
  IasRoutingZonePtr bt_ul = nullptr;
  IasRoutingZonePtr map_2_4 = nullptr;
  IasRoutingZonePtr rz_stereo = nullptr;
  IasRoutingZonePtr rz_mono = nullptr;
  IasRoutingZonePtr rz_stereo1 = nullptr;
  IasRoutingZonePtr rz_stereo2 = nullptr;
  IasRoutingZonePtr rz_stereo3 = nullptr;
  IasAudioPortPtr zoneInPortStereo = nullptr;
  IasAudioPortPtr zoneInPortMono = nullptr;
  IasAudioPortPtr zoneOutPort = nullptr;
  createSinkAlsaHandler_map_2_4(setup, &map_2_4,&zoneInPortStereo, &zoneInPortMono, &zoneOutPort);
  createSinkPlugin_Mono0(setup,&rz_mono);
  createSinkPlugin_Stereo0(setup,&rz_stereo);
  createSinkPlugin_Stereo1(setup,&rz_stereo1);
  createSinkPlugin_Stereo2(setup,&rz_stereo2);
  createSinkPlugin_Stereo3(setup,&rz_stereo3);

  if (isHost == false && btActive == true)
  {
    createSinkAlsaHandler_bt_ul(setup, &bt_ul);
  }

  if(noAvb == false)
  {
    createSinkAlsaHandler_avb_stereo_0(setup, &avbstereo0);
    createSinkAlsaHandler_avb_stereo_1(setup, &avbstereo1);
    createSinkAlsaHandler_avb_mc_0(setup, &avbmc0);
    createSinkAlsaHandler_avb_mc_1(setup, &avbmc1);
  }

  createPipeline(setup, map_2_4, zoneInPortStereo, zoneInPortMono, zoneOutPort);

  setup->addDerivedZone(map_2_4, rz_stereo);
  setup->addDerivedZone(map_2_4, rz_stereo1);
  setup->addDerivedZone(map_2_4, rz_stereo2);
  setup->addDerivedZone(map_2_4, rz_stereo3);
  setup->addDerivedZone(map_2_4, rz_mono);
  if (isHost == false && btActive == true)
  {
    setup->addDerivedZone(map_2_4, bt_ul);
  }

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

  IasSmartX::IasResult smxres = smartx->start();
  // We are not interested in the result of the start. Details about what can be started and what not can be found
  // in the DLT. A printout here is just distracting.

  IasProperties volCmdProperties;
  volCmdProperties.set("cmd",static_cast<int32_t>(IasAudio::IasVolume::eIasSetVolume));

  IasIProcessing *processing = smartx->processing();
  IAS_ASSERT(processing != nullptr);

  std::string pin = "Volume_InOutStereoPin";
  int32_t vol = 0;
  volCmdProperties.set("pin",pin);
  volCmdProperties.set("volume",vol);
  IasAudio::IasInt32Vector ramp;
  ramp.push_back(100);
  ramp.push_back(0);
  volCmdProperties.set("ramp",ramp);
  IasProperties returnProps;
  processing->sendCmd("VolumeLoudness",volCmdProperties,returnProps);


  (void)smxres;
}

void connect(IasIRouting *routing)
{
  int32_t sourceId = -1;
  int32_t sinkId = -1;
  cin >> sourceId >> sinkId;
  cout << "Connect Source Id=" << sourceId << " Sink Id=" << sinkId << endl;
  if (routing->connect(sourceId, sinkId) == IasIRouting::eIasOk)
  {
    cout << "Successfully connected Source Id=" << sourceId << " with Sink Id=" << sinkId << endl;
  }
  else
  {
    cerr << "Error connecting Source Id=" << sourceId << " with Sink Id=" << sinkId << endl;
  }
}

void disconnect(IasIRouting *routing)
{
  int32_t sourceId = -1;
  int32_t sinkId = -1;
  cin >> sourceId >> sinkId;
  cout << "Disconnect Source Id=" << sourceId << " Sink Id=" << sinkId << endl;
  if (routing->disconnect(sourceId, sinkId) == IasIRouting::eIasOk)
  {
    cout << "Successfully disconnected Source Id=" << sourceId << " with Sink Id=" << sinkId << endl;
  }
  else
  {
    cerr << "Error disconnecting Source Id=" << sourceId << " with Sink Id=" << sinkId << endl;
  }
}

void printDetails(const std::string &cmd)
{
  cout <<  endl;
  cout << "Syntax:" << endl << cmds[cmd].syntax << endl << endl;
  cout << "Brief:" << endl << cmds[cmd].brief << endl << endl;
  if (!cmds[cmd].params.empty())
  {
    cout << "Parameters:" << endl << cmds[cmd].params << endl << endl;
  }
}

void printHelp()
{
  std::string what = "";
  if (cin.peek() != '\n')
  {
    cin >> what;
  }
  if (what == "")
  {
    cout << "Possible commands. Type '? <cmd>' for detailed description:" << endl << endl;
    cout << left;
    cout << setw(40) << "syntax" << setw(40) << "brief" << endl;
    IasCmdMap::iterator cmdIt;
    for (cmdIt=cmds.begin(); cmdIt!=cmds.end(); ++cmdIt)
    {
      cout << setw(40) << (*cmdIt).second.syntax << setw(40) << (*cmdIt).second.brief << endl;
    }
    cout << endl;
  }
  else
  {
    printDetails(what);
  }
}

void printCommandLineArgs()
{
  cout << "                                                         " << endl;
  cout << "Usage:                                                   " << endl;
  cout << "  ./smartx_interactive_example [OPTION...]               " << endl;
  cout << "                                                         " << endl;
  cout << "Help Options:                                            " << endl;
  cout << "  -h, --help               Show help                     " << endl;
  cout << "                                                         " << endl;
  cout << "Application Options:                                     " << endl;
  cout << "  -n, --noavb              no creation of avb devices    " << endl;
  cout << "  -H, --Host               program runs on a host machine" << endl;
  cout << "  -bt                      use bluetooth device, don't combine with -H" << endl;
  cout << "                                                             " << endl;
  cout << "  -C, --Configparser       use the config parser         " << endl;
}

void printUnknownOption(const char* unknownOption)
{
  cout << "                                        " << endl;
  cout << "Error: unknown option: " << unknownOption << endl;
  printCommandLineArgs();
}

void listProbePorts(const IasAudioPortVector &ports)
{
  cout << endl;
  cout << right << setw(15) << "Direction |";
  cout << right << setw(40) << "Port Name |";
  cout << right << setw(20) << "Number channels |";
  cout << right << setw(10) << "ID |" << endl;
  cout << right << setw(15+40+20+10) << setfill('-') << "" << endl;
  cout << setfill(' ');
  for (auto &port : ports)
  {
    cout << right << setw(13) << getPortDirection(port) << " |";
    cout << right << setw(38) << getPortName(port) << " |";
    cout << right << setw(18) << getPortNumChannels(port) << " |";
    cout << right << setw(8)<< getPortId(port) <<" |" <<endl;
  }
  cout << endl;
}

void listPorts(const IasAudioPortVector &ports)
{
  cout << endl;
  cout << right << setw(8) << "Id |";
  cout << right << setw(40) << "Port Name |";
  cout << right << setw(16) << "Number channels" << endl;
  cout << right << setw(8+40+30) << setfill('-') << "" << endl;
  cout << setfill(' ');
  for (auto &port : ports)
  {
    cout << right << setw(6) << getPortId(port) << " |";
    cout << right << setw(38) << getPortName(port) << " |";
    cout << right << setw(16) << getPortNumChannels(port) << endl;
  }
  cout << endl;
}

void listConnections(const IasConnectionVector &connections)
{
  cout << endl;
  cout << connections.size() <<" connections established!" << endl;
  cout << endl;
  cout << "Connection table:" <<endl <<endl;
  cout << right << setw(12) << "Source Id |";
  cout << right << setw(30) << "Source name |";
  cout << right << setw(12) << "Sink Id |";
  cout << right << setw(30) << "Sink name |";
  cout << right << setw(16) << "Number channels" <<endl;
  cout << right << setw(12+30+12+30+16) << setfill('-') << "" << endl;
  cout << setfill(' ');
  for (auto &connectionPair : connections)
  {

    cout << right << setw(10) <<getPortId(connectionPair.first) << " |";
    cout << right << setw(28) <<getPortName(connectionPair.first)<< " |";
    cout << right << setw(10) <<getPortId(connectionPair.second)<< " |";
    cout << right << setw(28) <<getPortName(connectionPair.second)<< " |";
    cout << right << setw(14) <<getPortNumChannels(connectionPair.second)<< " |" <<endl;
  }
  cout << endl;
}

void listSourceGoups(const IasSourceGroupMap &sourceGroups)
{
  cout << endl;
  IasSourceGroupMap::const_iterator it;
  if (sourceGroups.begin() == sourceGroups.end())
  {
    cout << "No source groups configured" << endl;
    return;
  }
  else
  {
    cout << setw(40) << "Source Group Name |";
    cout << setw(40) << "Source Ids" <<endl;
    cout << setw(80) << setfill('-') << "" <<endl;
    cout << setfill(' ');
    for(it=sourceGroups.begin();it!= sourceGroups.end();it++)
    {
       cout <<setw(38) << it->first << " |";
       cout << setw(35);
       std::set<int32_t>::iterator setIt;
       for (setIt=it->second.begin();setIt!=it->second.end();++setIt)
       {
         cout  <<*setIt <<", ";
       }
       cout <<endl;
    }
  }
}

void listDummySources(const IasDummySourcesSet &dummySources)
{
  if ( dummySources.begin() == dummySources.end())
  {
    cout << "No dummy connections" <<endl;
  }
  else
  {
    std::set<int32_t>::const_iterator it;
    cout << setw(20) << "Dummy Sources" <<endl;
    cout << setw(24) << setfill('-') << "" <<endl;
    cout << setfill(' ');
    for(it=dummySources.begin(); it!= dummySources.end();it++)
    {
      cout <<right <<setw(20) << *it <<endl;
    }
  }
}

void lip(IasISetup *setup)
{
  listPorts(setup->getAudioInputPorts());
}

void lop(IasISetup *setup)
{
  listPorts(setup->getAudioOutputPorts());
}

void lct(IasIRouting *routing)
{
  listConnections(routing->getActiveConnections());
}

void lsg(IasISetup *setup)
{
  listSourceGoups(setup->getSourceGroups());
}

void lds(IasIRouting *routing)
{
  listDummySources(routing->getDummySources());
}

void lpp(IasISetup *setup)
{
  listProbePorts(setup->getAudioPorts());
}

void inject(IasIDebug *debug)
{
  std::string dataFilePrefix = "";
  std::string audioPortName = "";
  uint32_t numSeconds = 0;
  cin >> dataFilePrefix >> audioPortName >> numSeconds;
  cout << "injecting data from filePrefix " << dataFilePrefix << " to audio port " <<audioPortName <<endl;
  debug->startInject(dataFilePrefix, audioPortName, numSeconds);
}

void record(IasIDebug *debug)
{
  std::string dataFilePrefix = "";
  std::string audioPortName = "";
  uint32_t numSeconds = 0;
  cin >> dataFilePrefix >> audioPortName >>numSeconds;
  cout << "recording data from audio port " << audioPortName << " to filePrefix " <<dataFilePrefix <<" for "<<numSeconds << " Seconds" <<endl;
  debug->startRecord(dataFilePrefix, audioPortName,numSeconds);
}

void stopProbe(IasIDebug *debug)
{
  std::string audioPortName = "";
  cin >> audioPortName;
  cout << "stop probing at port " <<audioPortName <<endl;
  debug->stopProbing(audioPortName);
}

void loudness(IasIProcessing *processing)
{
  int32_t loudness = 0;
  std::string pin;
  std::string loud;
  cin >> pin;
  cin >> loudness;

  IasProperties volCmdProperties;
  volCmdProperties.set("cmd",static_cast<int32_t>(IasAudio::IasVolume::eIasSetLoudness));
  volCmdProperties.set("pin",pin);
  if(loudness != 0)
  {
    loud="on";
  }
  else
  {
    loud="off";
  }
  volCmdProperties.set("loudness",loud);
  IasProperties returnProps;
  processing->sendCmd("VolumeLoudness",volCmdProperties,returnProps);
}

void mute(IasIProcessing *processing)
{
  int32_t mute = 0;
  std::string pin;
  cin >> pin;
  cin >> mute;
  if(mute != 0)
  {
    mute = 1;
  }
  IasProperties volCmdProperties;
  volCmdProperties.set("cmd",static_cast<int32_t>(IasAudio::IasVolume::eIasSetMuteState));
  volCmdProperties.set("pin",pin);
  IasAudio::IasInt32Vector params;
  params.push_back(static_cast<bool>(mute));
  params.push_back(0);
  params.push_back(0);
  volCmdProperties.set("params",params);
  IasProperties returnProps;
  processing->sendCmd("VolumeLoudness",volCmdProperties,returnProps);
}

void volume(IasIProcessing *processing)
{
  int32_t vol = 0;
  std::string pin;
  cin >> pin;
  cin >> vol;
  if (vol > 0)
  {
    cout << "Setting vol to max" <<endl;
    vol = 0;
  }
  if (vol < -1440)
  {
    cout << "Setting vol to min" <<endl;
    vol = -1440;
  }

  IasProperties volCmdProperties;
  volCmdProperties.set("cmd",static_cast<int32_t>(IasAudio::IasVolume::eIasSetVolume));

  volCmdProperties.set("pin",pin);
  volCmdProperties.set("volume",vol);
  IasAudio::IasInt32Vector ramp;
  ramp.push_back(100);
  ramp.push_back(0);
  volCmdProperties.set("ramp",ramp);
  IasProperties returnProps;
  processing->sendCmd("VolumeLoudness",volCmdProperties,returnProps);

}

void mixerFader(IasIProcessing *processing)
{
  int32_t fader = 0;
  std::string inputPin;
  cin >> inputPin;
  cin >> fader;


  IasProperties cmdProperties;
  cmdProperties.set<int32_t>("cmd", IasMixer::eIasSetFader);
  cmdProperties.set<std::string>("pin", inputPin);
  cmdProperties.set<int32_t>("fader", fader);
  IasProperties returnProps;
  processing->sendCmd("Mixer", cmdProperties, returnProps);
}

void mixerBalance(IasIProcessing *processing)
{
  int32_t balance = 0;
  std::string inputPin;
  cin >> inputPin;
  cin >> balance;

  IasProperties cmdProperties;
  cmdProperties.set<int32_t>("cmd",  IasMixer::eIasSetBalance);
  cmdProperties.set<std::string>("pin", inputPin);
  cmdProperties.set<int32_t>("balance", balance);
  IasProperties returnProps;
  processing->sendCmd("Mixer", cmdProperties, returnProps);
}

void mixerGain(IasIProcessing *processing)
{
  int32_t gain = 0;
  std::string inputPin;

  cin >> inputPin;
  cin >> gain;
  if (gain > 0)
  {
    gain = 0;
  }
  if (gain < -1440)
  {
    gain = -1440;
  }

  IasProperties cmdProperties;
  cmdProperties.set<int32_t>("cmd", IasAudio::IasMixer::eIasSetInputGainOffset);
  cmdProperties.set<std::string>("pin", inputPin);
  cmdProperties.set<int32_t>("gain", gain);
  IasProperties returnProps;
  processing->sendCmd("Mixer", cmdProperties, returnProps);
}

void enableMixer(IasIProcessing *processing)
{
  int32_t flag = 0;
  std::string moduleState;

  cin >> flag;
  if (flag == 0)
  {
    cout << "Setting simple mixer module state to off." << endl;
    moduleState = "off";
  }
  else
  {
    cout << "Setting simple mixer module state to on." << endl;
    moduleState = "on";
  }

  IasProperties cmdProperties;
  cmdProperties.set<int32_t>("cmd", IasMixer::eIasSetModuleState);
  cmdProperties.set<std::string>("moduleState", moduleState);
  IasProperties returnProps;
  processing->sendCmd("Mixer", cmdProperties, returnProps);

  IasProperties::IasResult result = returnProps.get<std::string>("moduleState", &moduleState);
  if (result == IasProperties::eIasOk)
  {
    cout << "New state of mixer module: " << moduleState << endl;
  }
}

void generateTopology(IasSmartX *smartx)
{
  IasSmartXDebugFacade facade(smartx);
  std::string xmlTopology;

  const auto res = facade.getSmartxTopology(xmlTopology);
  if(res == IasSmartXDebugFacade::IasResult::eIasOk)
  {
    cout << "Generate topology: SUCCESS" << std::endl;
  }
  else
  {
    cout << "Generate topology: FAILED, see dlt log for details" << std::endl;
  }
}


bool dispatch(IasSmartX *smartx)
{
  bool goOn = true;
  std::string cmd;
  cout << ">";
  std::string input;
  cin >> cmd;
  if      (cmd == "c") connect(smartx->routing());
  else if (cmd == "d") disconnect(smartx->routing());
  else if (cmd == "lip") lip(smartx->setup());
  else if (cmd == "lop") lop(smartx->setup());
  else if (cmd == "gct") lct(smartx->routing());
  else if (cmd == "gsg") lsg(smartx->setup());
  else if (cmd == "gds") lds(smartx->routing());
  else if (cmd == "ipd") inject(smartx->debug());
  else if (cmd == "rpd") record(smartx->debug());
  else if (cmd == "spd") stopProbe(smartx->debug());
  else if (cmd == "lpp") lpp(smartx->setup());
  else if (cmd == "volume") volume(smartx->processing());
  else if (cmd == "mute") mute(smartx->processing());
  else if (cmd == "loudness") loudness(smartx->processing());
  else if (cmd == "mixgain") mixerGain(smartx->processing());
  else if (cmd == "mixbalance") mixerBalance(smartx->processing());
  else if (cmd == "mixfader") mixerFader(smartx->processing());
  else if (cmd == "enablemixer") enableMixer(smartx->processing());
  else if (cmd == "addsrc") addSourceDevice(smartx->setup());
  else if (cmd == "addsink") addSinkDevice(smartx->setup());
  else if (cmd == "delsrc") deleteSourceDevice(smartx->setup(), smartx->routing());
  else if (cmd == "delsink") deleteSinkDevice(smartx->setup(), smartx->routing());
  else if (cmd == "startrz") startRoutingZone(smartx->setup());
  else if (cmd == "startsrc") startSourceDevice(smartx->setup());
  else if (cmd == "stoprz") stopRoutingZone(smartx->setup());
  else if (cmd == "stopsrc") stopSourceDevice(smartx->setup());
  else if (cmd == "gentop") generateTopology(smartx);
  else if (cmd == "?") printHelp();
  else if (cmd == "q") goOn = false;
  cin.ignore(1024, '\n');
  return goOn;
}

void printUsage()
{
  cout << "SmartX interactive command line interface" << endl;
  cout << "Enter '?' for help" << endl;
}

void parseCommandLineArgs(int argc, char* argv[])
{
  for (int index=1; index<argc; ++index)
  {
    if ((strncmp("-n", argv[index], strlen("-n")) == 0) ||
        (strncmp("--noavb", argv[index], strlen("--noavb")) == 0))
    {
      noAvb = true;
    }
    else if ((strncmp("-H", argv[index], strlen("-H")) == 0) ||
        (strncmp("--Host", argv[index], strlen("--Host")) == 0))
    {
      isHost = true;
    }
    else if((strncmp("-bt", argv[index], strlen("-bt")) == 0))
    {
      btActive = true;
    }
    else if ((strncmp("-C", argv[index], strlen("-C")) == 0) ||
        (strncmp("--Configparser", argv[index], strlen("--Configparser")) == 0))
    {
      isConfigParser = true;
      return;
    }
    else if ((strncmp("-h", argv[index], strlen("-h")) == 0) ||
        ((strncmp("--help", argv[index], strlen("--help")) == 0)))
    {
      printCommandLineArgs();
      exit(0);
    }
    else
    {
      printUnknownOption(argv[index]);
      exit(-1);
    }
  }
}

int main(int argc, char* argv[])
{
  parseCommandLineArgs(argc, argv);

  DLT_REGISTER_APP("XBAR", "SmartXbar example");
  DLT_VERBOSE_MODE();

  generateHelp();
  printUsage();

  IasSmartX *smartx = IasSmartX::create();
  if (smartx == nullptr)
  {
    return -1;
  }
  if (smartx->isAtLeast(SMARTX_API_MAJOR, SMARTX_API_MINOR, SMARTX_API_PATCH) == false)
  {
    cerr << "SmartX API version does not match" << endl;
    IasSmartX::destroy(smartx);
    return -1;
  }
  if (isConfigParser == true)
  {
    //Calling smartxConfigParser
    bool returnVal= IasAudio::parseConfig(smartx, argv[2]);
    if (returnVal == false)
    {
      cout << "ConfigParser Failed" << endl;
      return -1;
    }
  }
  else
  {
    IasISetup *setup = smartx->setup();
    IAS_ASSERT(setup != nullptr);
    IasIRouting *routing = smartx->routing();
    IAS_ASSERT(routing != nullptr);
    staticSetup(smartx);
  }

  // Instantiate a server for ADK Debug agent communication
  IasSmartXDebugFacade facade{smartx};
      //ToDo : Add support for Debug agent

  // Endless loop waiting for user input
  while (dispatch(smartx))
  {
    while (smartx->waitForEvent(200) == IasSmartX::eIasOk)
    {
      IasSmartX::IasResult smxres;
      IasEventPtr newEvent;
      smxres = smartx->getNextEvent(&newEvent);
      if (smxres == IasSmartX::eIasOk)
      {
        class : public IasEventHandler
        {
          virtual void receivedConnectionEvent(IasConnectionEvent* event)
          {
            cout << "Received connection event(" << toString(event->getEventType()) << ")" << endl;
            if (event->getSourceId() != -1)
            {
              cout << "  Source Id = " << event->getSourceId() << endl;
            }
            if (event->getSinkId() != -1)
            {
              cout << "  Sink Id = " << event->getSinkId() << endl;
            }
          }
          virtual void receivedSetupEvent(IasSetupEvent* event)
          {
            cout << "Received setup event(" << toString(event->getEventType()) << ")" << endl;
            if (event->getSourceDevice() != nullptr)
            {
              std::string name = getDeviceName(event->getSourceDevice());
              cout << "  Source Device = " << name << endl;
            }
            if (event->getSinkDevice() != nullptr)
            {
              std::string name = getDeviceName(event->getSinkDevice());
              cout << "  Sink Device = " << name << endl;
            }
          }
          virtual void receivedModuleEvent(IasModuleEvent* event)
          {
            const IasProperties eventProperties = event->getProperties();
            std::string typeName = "";
            std::string instanceName = "";
            int32_t eventType;
            eventProperties.get<std::string>("typeName", &typeName);
            eventProperties.get<std::string>("instanceName", &instanceName);
            eventProperties.get<int32_t>("eventType", &eventType);
            cout << "Received module event: "
                 << "typeName = " << typeName << ", "
                 << "instanceName = " << instanceName << std::endl;
            if (typeName == "ias.mixer")
            {
              if (eventType == static_cast<int32_t>(IasMixer::IasMixerEventTypes::eIasBalanceFinished))
              {
                int32_t balance;
                std::string pinName;
                IasProperties::IasResult result;
                result = eventProperties.get<int32_t>("balance", &balance);
                (void)result;
                result = eventProperties.get<std::string>("pin",  &pinName);
                (void)result;
                cout << "  Balance finished for pin: " << pinName.c_str()
                     << ", new balance is" <<balance <<endl;
              }
              if (eventType == static_cast<int32_t>(IasMixer::IasMixerEventTypes::eIasFaderFinished))
              {
                int32_t fader;
                std::string pinName;
                IasProperties::IasResult result;
                result = eventProperties.get<int32_t>("fader", &fader);
                (void)result;
                result = eventProperties.get<std::string>("pin",  &pinName);
                (void)result;
                cout << "  Fader finished for pin: " << pinName.c_str()
                     << ", new fader is" <<fader <<endl;
              }
              if (eventType == static_cast<int32_t>(IasMixer::IasMixerEventTypes::eIasInputGainOffsetFinished))
              {
                int32_t gainOffset;
                std::string pinName;
                IasProperties::IasResult result;
                result = eventProperties.get<int32_t>("gainOffset", &gainOffset);
                (void)result;
                result = eventProperties.get<std::string>("pin",  &pinName);
                (void)result;
                cout << "  Gainoffset finished for pin: " << pinName.c_str()
                     << ", new gain offset is" <<gainOffset  <<endl;
              }
            }
            else if (typeName == "ias.volume")
            {
              std::string  pinName;
              IasProperties::IasResult result;
              result = eventProperties.get<std::string>("pin",  &pinName);
              (void)result;
              switch (eventType)
              {
                case static_cast<int32_t>(IasVolume::eIasVolumeFadingFinished):
                  cout << "  volume fading finished for pin " << pinName << endl;
                  int32_t volume;
                  eventProperties.get<int32_t>("volume",&volume);
                  cout << "new volume is" << volume <<"dB/10" <<endl;
                  break;
                case static_cast<int32_t>(IasVolume::eIasLoudnessSwitchFinished):
                  cout << "  loudness switch finished for pin " << pinName << endl;
                  break;
                case static_cast<int32_t>(IasVolume::eIasSpeedControlledVolumeFinished):
                  cout << "  set speed controlled volume finished for pin " << pinName << endl;
                  break;
                case static_cast<int32_t>(IasVolume::eIasSetMuteStateFinished):
                  int32_t muteState;
                  cout << "  set mute state finished for pin " << pinName << endl;
                  eventProperties.get<int32_t>("muteState",&muteState);
                  cout << " mute state is " << muteState <<endl;
                  break;
                default:
                  cout << "  invalid event type" << endl;
              }
            }
          }
        } myEventHandler;
        newEvent->accept(myEventHandler);
      }
    }
  };

  IasSmartX::IasResult smres = smartx->stop();
  if(smres == IasSmartX::eIasOk)
  {
    IasSmartX::destroy(smartx);
  }
  else
  {
    cout << "Error stopping SmartX" <<endl;
  }
  return 0;
}
