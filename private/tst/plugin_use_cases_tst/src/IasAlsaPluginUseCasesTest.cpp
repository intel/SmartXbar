/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasAlsaPluginUseCasesTest.cpp
 * @date   2016
 * @brief
 */

#include <iostream>
#include <alsa/asoundlib.h>
#include <gtest/gtest.h>
#include <sndfile.h>
#include <time.h>
#include <thread>

#include "internal/audio/common/IasAudioLogging.hpp"
#include "audio/smartx/IasSmartX.hpp"
#include "audio/smartx/IasISetup.hpp"
#include "audio/smartx/IasIRouting.hpp"
#include "audio/smartx/IasIDebug.hpp"
#include "audio/smartx/IasSetupHelper.hpp"

// Global variable that describes whether WAVE files are taken from an absolute
// NFS path (default) or from the working directory (can be selected by command
// line option). This variable need to be global, because it is used for the
// data exchange between the functions main() and mainFunction().
static bool  useNfsPath = true;
static std::string nfsPath("");

#ifndef PLUGHW_DEVICE_NAME
#define PLUGHW_DEVICE_NAME  "plughw:31,0"
#endif

#ifndef SMARTX_CONFIG_DIR
#define SMARTX_CONFIG_DIR "./"
#endif

static std::string defaultDevice(PLUGHW_DEVICE_NAME);

#ifndef NFS_PATH
#define NFS_PATH "/nfs/ka/disks/ias_organisation_disk001/teams/audio/TestWavRefFiles/2015-05-27/"
#endif

using namespace std;

namespace IasAudio {

using IasTestParam = std::tuple<int32_t, std::string>;

static const char* policyToString(int policy)
{
  if (policy == SCHED_FIFO)
  {
    return "SCHED_FIFO";
  }
  else if (policy == SCHED_RR)
  {
    return "SCHED_RR";
  }
  else
  {
    return "SCHED_OTHER";
  }
}

static void setThreadSchedParams()
{
  char threadName[16];
  pthread_getname_np(pthread_self(), threadName, sizeof(threadName));
  struct sched_param params;
  params.sched_priority = 20;
  int policy = SCHED_FIFO;
  int result = pthread_setschedparam(pthread_self(), policy, &params);
  if(result == 0)
  {
    result = pthread_getschedparam(pthread_self(), &policy, &params);
    if(result == 0)
    {
      std::cout << "Thread=" << threadName << ": Scheduling parameters set. Policy=" << policyToString(policy) << " Priority=" << params.sched_priority << std::endl;
    }
  }
  else
  {
    std::cout << "Thread=" << threadName << ": Scheduling parameters couldn't be set. Policy=" << policyToString(policy) << " Priority=" << params.sched_priority << std::endl;
  }
}

class IasAlsaPluginUseCasesTest : public ::testing::TestWithParam<IasTestParam>
{
protected:
  IasAlsaPluginUseCasesTest()
    :mInputFile1(nullptr)
    ,mInputFileInfo1()
    ,mInputFile2(nullptr)
    ,mInputFileInfo2()
    ,mInputFile3(nullptr)
    ,mInputFileInfo3()
    ,mOutputFile1(nullptr)
    ,mOutputFileInfo1()
    ,mSamples(nullptr)
    ,mSamplesInterleaved(nullptr)
    ,mSndOut(nullptr)
    ,mPcm(nullptr)
    ,mPlaybackFinished(false)
    ,mBaseZone(nullptr)
    ,mDerivedZone(nullptr)
  {}
  virtual ~IasAlsaPluginUseCasesTest() {}

  // Sets up the test fixture.
  virtual void SetUp()
  {
    auto testCase = GetParam();
    const int32_t sink = std::get<0>(testCase);
    if (sink == 125)
    {
      setenv("SMARTX_CFG_DIR", std::string(SMARTX_CONFIG_DIR).c_str(), true);
    }
    else
    {
      // Remove previously added definition
      // char array is chosen to avoid compiler warning for deprecated conversion between const char* an char*
      char envVarName[] = "SMARTX_CFG_DIR";
      putenv(envVarName);
    }

    mPlaybackFinished = false;
    cout << "useNfsPath: " << useNfsPath << endl;

    if (useNfsPath)
    {
      cout << "useNfsPath has been set" << endl;
      nfsPath = NFS_PATH;
      cout << "nfs path: " << nfsPath << endl;
    }
    mSmartx = IasSmartX::create();
    ASSERT_TRUE(mSmartx != nullptr);
    IasISetup *setup = mSmartx->setup();
    ASSERT_TRUE(setup != nullptr);
    IasAudioDeviceParams sinkParams;
    sinkParams.name = defaultDevice;
    sinkParams.clockType = eIasClockReceived;
    sinkParams.dataFormat = eIasFormatInt16;
    sinkParams.numChannels = 2;
    sinkParams.numPeriods = 4;
    sinkParams.periodSize = 192;
    sinkParams.samplerate = 48000;
    IasAudioSinkDevicePtr sinkDevice = nullptr;
    IasRoutingZonePtr rzn = nullptr;
    IasISetup::IasResult setres;
    setres = IasSetupHelper::createAudioSinkDevice(setup, sinkParams, 123, &sinkDevice, &rzn);
    ASSERT_EQ(IasISetup::eIasOk, setres);

    sinkParams.name = "capturestereo48";
    sinkParams.clockType = eIasClockProvided;
    sinkParams.dataFormat = eIasFormatInt16;
    sinkParams.numChannels = 2;
    sinkParams.numPeriods = 4;
    sinkParams.periodSize = 192;
    sinkParams.samplerate = 48000;
    IasAudioSinkDevicePtr sinkDevice48 = nullptr;
    IasRoutingZonePtr rznSmartX48 = nullptr;
    setres = IasSetupHelper::createAudioSinkDevice(setup, sinkParams, 124, &sinkDevice48, &rznSmartX48);
    ASSERT_EQ(IasISetup::eIasOk, setres);

    setres = setup->addDerivedZone(rzn, rznSmartX48);
    ASSERT_EQ(IasISetup::eIasOk, setres);

    // The same device as above, however with doubled periodSize to make sure runner threads get used.
    // For runner threads to run, a config option needs to be enabled in IasConfigFile (see smartx_api tests)
    // and periodSizeMultiple in IasRoutingZoneWorkerThread needs to be different than 1.
    // Note that device name is different too.
    sinkParams.name = "capturestereo48rt";
    sinkParams.clockType = eIasClockProvided;
    sinkParams.dataFormat = eIasFormatInt16;
    sinkParams.numChannels = 2;
    sinkParams.numPeriods = 4;
    sinkParams.periodSize = 384;
    sinkParams.samplerate = 48000;
    IasAudioSinkDevicePtr sinkDevice48rt = nullptr;
    IasRoutingZonePtr rznSmartX48rt = nullptr;
    setres = IasSetupHelper::createAudioSinkDevice(setup, sinkParams, 125, &sinkDevice48rt, &rznSmartX48rt);
    ASSERT_EQ(IasISetup::eIasOk, setres);

    setres = setup->addDerivedZone(rzn, rznSmartX48rt);
    ASSERT_EQ(IasISetup::eIasOk, setres);
    mBaseZone = rzn;
    mDerivedZone = rznSmartX48rt;

    IasAudioDeviceParams sourceParams;
    sourceParams.name = "stereo48";
    sourceParams.clockType = eIasClockProvided;
    sourceParams.dataFormat = eIasFormatInt16;
    sourceParams.numChannels = 2;
    sourceParams.numPeriods = 3;
    sourceParams.periodSize = 1920;
    sourceParams.samplerate = 48000;
    IasAudioSourceDevicePtr sourceDevice48 = nullptr;
    setres = IasSetupHelper::createAudioSourceDevice(setup, sourceParams, 123, &sourceDevice48);
    ASSERT_EQ(IasISetup::eIasOk, setres);

    sourceParams.name = "stereo44";
    sourceParams.clockType = eIasClockProvided;
    sourceParams.dataFormat = eIasFormatInt16;
    sourceParams.numChannels = 1;
    sourceParams.numPeriods = 3;
    sourceParams.periodSize = 1920;
    sourceParams.samplerate = 44100;
    IasAudioSourceDevicePtr sourceDevice44 = nullptr;
    setres = IasSetupHelper::createAudioSourceDevice(setup, sourceParams, 124, &sourceDevice44);
    ASSERT_EQ(IasISetup::eIasOk, setres);

    IasSmartX::IasResult smxres;
    smxres = mSmartx->start();
    ASSERT_EQ(IasSmartX::eIasOk, smxres);


    std::string inputFileName = nfsPath + "sin_440_LR_1.wav";
    mInputFile1 = sf_open(inputFileName.c_str(), SFM_READ, &mInputFileInfo1);
    ASSERT_TRUE(mInputFile1 != NULL);

    inputFileName = nfsPath + "sin_440_LR_1_48k.wav";
    mInputFile2 = sf_open(inputFileName.c_str(), SFM_READ, &mInputFileInfo2);
    ASSERT_TRUE(mInputFile2 != NULL);

    inputFileName = nfsPath + "stereo_sample_48000Hz_1s_16bit_PCM.wav";
    mInputFile3 = sf_open(inputFileName.c_str(), SFM_READ, &mInputFileInfo3);
    ASSERT_TRUE(mInputFile3 != NULL);

    memset(&mOutputFileInfo1, 0, sizeof(mOutputFileInfo1));
    mOutputFileInfo1.channels = 2;
    mOutputFileInfo1.samplerate = 48000;
    mOutputFileInfo1.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
    std::string outputFileName = "/tmp/plugin_use_cases_output1.wav";
    mOutputFile1 = sf_open(outputFileName.c_str(), SFM_WRITE, &mOutputFileInfo1);
    ASSERT_TRUE(mOutputFile1 != NULL);

    ASSERT_EQ(0, snd_output_stdio_attach(&mSndOut, stdout, 0));
  }

  // Tears down the test fixture.
  virtual void TearDown()
  {
    if (mInputFile1)
    {
       EXPECT_EQ(0, sf_close(mInputFile1));
    }
    if (mInputFile2)
    {
       EXPECT_EQ(0, sf_close(mInputFile2));
    }
    if (mInputFile3)
    {
       EXPECT_EQ(0, sf_close(mInputFile3));
    }
    if (mOutputFile1)
    {
       EXPECT_EQ(0, sf_close(mOutputFile1));
    }


    IasSmartX::IasResult smxres = mSmartx->stop();
    ASSERT_EQ(IasSmartX::eIasOk, smxres);
    mSmartx->setup()->deleteDerivedZone(mBaseZone, mDerivedZone);
    IasSmartX::destroy(mSmartx);
  }
  void setHwParams(snd_pcm_t *pcm,
                   snd_pcm_uframes_t periodSize,
                   snd_pcm_uframes_t bufferSize,
                   uint32_t rate,
                   uint32_t nrChannels,
                   snd_pcm_format_t format,
                   snd_pcm_access_t access);
  void setSwParams(snd_pcm_t *pcm,
                   snd_pcm_uframes_t avail_min,
                   snd_pcm_uframes_t startThreshold);

  void record(const std::string sinkName);
  void recordBlockingMode(const std::string sinkName);
  void recordBlockingModeRetries(const std::string sinkName);
  void recordBlockingModeNoData(const std::string sinkName);
  void record_TC8(const std::string sinkName);
  void recordBlockingMode_TC9(const std::string sinkName);


  // Member variables
  SNDFILE            *mInputFile1;
  SF_INFO             mInputFileInfo1;
  SNDFILE            *mInputFile2;
  SF_INFO             mInputFileInfo2;
  SNDFILE            *mInputFile3;
  SF_INFO             mInputFileInfo3;
  SNDFILE            *mOutputFile1;
  SF_INFO             mOutputFileInfo1;
  int16_t         *mSamples;
  int16_t         *mSamplesInterleaved;
  snd_output_t       *mSndOut;
  snd_pcm_t          *mPcm;
  bool                mPlaybackFinished;
  IasRoutingZonePtr   mBaseZone;
  IasRoutingZonePtr   mDerivedZone;

  static IasSmartX    *mSmartx;
};

INSTANTIATE_TEST_CASE_P(rznData, IasAlsaPluginUseCasesTest, testing::Values(
		IasTestParam(124, "capturestereo48"),
		IasTestParam(125, "capturestereo48rt")
		));

IasSmartX *IasAlsaPluginUseCasesTest::mSmartx = nullptr;

////////////////////////////////////////////////////////////////////////////////
// HELPER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////
void IasAlsaPluginUseCasesTest::setHwParams(
  snd_pcm_t *pcm,
  snd_pcm_uframes_t periodSize,
  snd_pcm_uframes_t bufferSize,
  uint32_t rate,
  uint32_t nrChannels,
  snd_pcm_format_t format,
  snd_pcm_access_t access)
{
  (void)bufferSize;
  snd_pcm_hw_params_t *hwParams;
  snd_pcm_hw_params_alloca(&hwParams);

  int err = snd_pcm_hw_params_any(pcm, hwParams);
  if (err < 0)
  {
    cout << "Error during snd_pcm_hw_params_any" << snd_strerror(err) << endl;
  }
  ASSERT_GE(err, 0);

  int dir = 0;
  uint32_t currentRate = rate;
  err = snd_pcm_hw_params_set_rate_near(pcm, hwParams, &currentRate, 0);
  if (err < 0)
  {
    cout << "Error during snd_pcm_hw_params_set_rate_near: " << snd_strerror(err) << endl;
  }
  else
  {
    cout << "Sample rate set to: " << currentRate << endl;
  }
  ASSERT_EQ(0, err);
  snd_pcm_uframes_t tmpPeriodSize = periodSize;
  err = snd_pcm_hw_params_set_period_size_near(pcm, hwParams, &tmpPeriodSize, &dir);
  if (err < 0)
  {
    cout << "Error during snd_pcm_hw_params_set_period_size_near: " << snd_strerror(err) << endl;
  }
  ASSERT_EQ(0, err);
  cout << "Set period size to " << tmpPeriodSize << endl;
  uint32_t periods = 32;
  dir = 0;
  err = snd_pcm_hw_params_set_periods_near(pcm, hwParams, &periods, &dir);
  if (err < 0)
  {
    cout << "Error during snd_pcm_hw_params_set_periods_near: " << snd_strerror(err) << endl;
  }
  else
  {
    cout << "Set periods to: " << periods << endl;
  }
  ASSERT_EQ(0, err);

  err = snd_pcm_hw_params_set_channels(pcm, hwParams, nrChannels);
  if (err < 0)
  {
    cout << "Error during snd_pcm_hw_params_set_channels: " << snd_strerror(err) << endl;
  }
  ASSERT_EQ(0, err);
  err = snd_pcm_hw_params_set_format(pcm, hwParams, format);
  if (err < 0)
  {
    cout << "Error during snd_pcm_hw_params_set_format: " << snd_strerror(err) << endl;
  }
  ASSERT_EQ(0, err);
  err = snd_pcm_hw_params_set_access(pcm, hwParams, access);
  if (err < 0)
  {
    cout << "Error during snd_pcm_hw_params_set_access: " << snd_strerror(err) << endl;
  }
  ASSERT_EQ(0, err);

  err = snd_pcm_hw_params(pcm, hwParams);
  if (err < 0)
  {
    cout << "Error during snd_pcm_hw_params: " << snd_strerror(err) << endl;
  }
  ASSERT_EQ(0, err);
}

void IasAlsaPluginUseCasesTest::setSwParams(
  snd_pcm_t *pcm,
  snd_pcm_uframes_t avail_min,
  snd_pcm_uframes_t startThreshold)
{
  snd_pcm_sw_params_t *swParams;
  snd_pcm_sw_params_alloca(&swParams);
  int err = snd_pcm_sw_params_current(pcm, swParams);
  if (err < 0)
  {
    cout << "Error during snd_pcm_sw_params_current: " << snd_strerror(err) << endl;
  }
  ASSERT_EQ(0, err);
  err = snd_pcm_sw_params_set_avail_min(pcm, swParams, avail_min);
  if (err < 0)
  {
    cout << "Error during snd_pcm_sw_params_set_avail_min: " << snd_strerror(err) << endl;
  }
  ASSERT_EQ(0, err);
  err = snd_pcm_sw_params_set_start_threshold(pcm, swParams, startThreshold);
  if (err < 0)
  {
    cout << "Error during snd_pcm_sw_params_set_start_threshold: " << snd_strerror(err) << endl;
  }
  ASSERT_EQ(0, err);


  err = snd_pcm_sw_params(pcm, swParams);
  if (err < 0)
  {
    cout << "Error during snd_pcm_sw_params: " << snd_strerror(err) << endl;
  }
  ASSERT_EQ(0, err);

  cout << "SW Setup" << endl;
  cout << "***********************" << endl;
  snd_pcm_dump_sw_setup(pcm, mSndOut);
  cout << "***********************" << endl;
}


/*
 * @brief Thread for recording from an ALSA capture device in non-blocking mode.
 */
void IasAlsaPluginUseCasesTest::record(const std::string sinkName)
{
  const uint32_t cFrameLength = 192;
  const uint32_t cNrChannels = 2;
  int16_t *sampleBuffer = new int16_t[cFrameLength*cNrChannels];
  snd_pcm_sframes_t frames = 1;
  snd_pcm_uframes_t numberFramesToRead;
  struct timespec startTime;
  int32_t err;
  snd_pcm_sframes_t totalNumberFramesRead = 0;
  sf_count_t totalNumberFrames = 0;
  snd_pcm_t *hPcmCapture = nullptr;
  std::string alsaDevice = "smartx:" + sinkName;

  setThreadSchedParams();
  cout << "=================> Started recording thread" << endl;
  err = snd_pcm_open(&hPcmCapture, alsaDevice.c_str(), SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK);
  if (err < 0)
  {
    cout << "[record]Error during snd_pcm_open of capture device: " << snd_strerror(err) << endl;
  }
  ASSERT_EQ(0, err);
  setHwParams(hPcmCapture, cFrameLength, 4*cFrameLength, 48000, cNrChannels, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED);
  setSwParams(hPcmCapture, cFrameLength, 16*cFrameLength);
  snd_pcm_dump(hPcmCapture, mSndOut);

  snd_pcm_state_t pcmState = snd_pcm_state(hPcmCapture);
  if (pcmState != SND_PCM_STATE_RUNNING)
  {
    cout << "[record]ALSA capture device state before start == " << snd_pcm_state_name(pcmState) << endl;
    err = clock_gettime(CLOCK_MONOTONIC, &startTime);
    ASSERT_EQ(0, err);
    err = snd_pcm_start(hPcmCapture);
    ASSERT_EQ(0, err);
    pcmState = snd_pcm_state(hPcmCapture);
    cout << "[record]ALSA capture device state after start == " << snd_pcm_state_name(pcmState) << endl;
  }

  do
  {
    err = snd_pcm_wait(hPcmCapture, 100);
    if (err == 0)
    {
      cout << "======================> [record] TIMEOUT" << endl;
      break;
    }
    struct timespec startRead;
    err = clock_gettime(CLOCK_MONOTONIC, &startRead);
    ASSERT_EQ(0, err);
    numberFramesToRead = cFrameLength;
    frames = snd_pcm_readi(hPcmCapture, sampleBuffer, numberFramesToRead);
    struct timespec stopRead;
    err = clock_gettime(CLOCK_MONOTONIC, &stopRead);
    ASSERT_EQ(0, err);
    long readDuration = (stopRead.tv_nsec - startRead.tv_nsec) / 1000000;
    readDuration += (stopRead.tv_sec - startRead.tv_sec) * 1000;
    if (frames > 0 && readDuration > 0)
    {
      cout << "[record]snd_pcm_readi duration " << readDuration << " ms" << endl;
    }

    if (frames > 0)
    {
      totalNumberFramesRead += frames;
      snd_pcm_sframes_t delay;
      err = snd_pcm_delay(hPcmCapture, &delay);
      ASSERT_EQ(0, err);
      struct timespec curTime;
      err = clock_gettime(CLOCK_MONOTONIC, &curTime);
      ASSERT_EQ(0, err);
      long timeSinceStart = (curTime.tv_nsec - startTime.tv_nsec) / 1000000;
      timeSinceStart += (curTime.tv_sec - startTime.tv_sec) * 1000;
      cout << "[record]" << totalNumberFramesRead << " frames read by snd_pcm_readi()" << endl;
      cout << "[record]" << delay << " returned by snd_pcm_delay()" << endl;
      snd_pcm_sframes_t playPos = totalNumberFramesRead - delay;
      cout << "[record]" << playPos << " frames / " << playPos/48 << " ms is current play position" << endl;
      cout << "[record]" << timeSinceStart << " ms elapsed since snd_pcm_start" << endl;
      cout << "[record]" << "ALSA device is too slow " << playPos/48 - timeSinceStart << endl;
    }
    else if ((frames == -EAGAIN) || (frames == 0))
    {
      frames = 0;
    }
    else if (frames < 0)
    {
      frames = snd_pcm_recover(hPcmCapture, static_cast<int32_t>(frames), 0);
      if (frames < 0) {
        cout << "[record]Recovery from snd_pcm_readi failed: " << snd_strerror(err) << endl;
        break;
      }
    }

    if (frames > 0 && frames < static_cast<snd_pcm_sframes_t>(numberFramesToRead))
    {
      cout << "[record]Short read (expected " <<  numberFramesToRead << ", read " << frames << ")" << endl;
    }
    if (frames > 0)
    {
      sf_count_t numberFramesWritten;
      numberFramesWritten = sf_writef_short(mOutputFile1, sampleBuffer, frames);
      cout << "[record]Number frames written to file: " << numberFramesWritten << endl;
      totalNumberFrames += numberFramesWritten;
    }
  }
  while (true);

  delete[] sampleBuffer;
  snd_pcm_close(hPcmCapture);
  cout << "=================> End recording thread" << endl;
}


/*
 * @brief Thread for recording from an ALSA capture device in non-blocking mode.
 *
 * This is used in TC8 to verify that there is no problem when trying to go on
 * capturing after there was a scheduling issue, a stop and restart or something like this.
 * It shall test that there is no issue when the ALSA sink device buffer "jumps" from empty (frame-pointer==0)
 * and full (frame-pointer==0).
 */
static bool endThread = false;
void IasAlsaPluginUseCasesTest::record_TC8(const std::string sinkName)
{
  const uint32_t cFrameLength = 192;
  const uint32_t cNrChannels = 2;
  int16_t *sampleBuffer = new int16_t[cFrameLength*cNrChannels];
  snd_pcm_sframes_t frames = 1;
  snd_pcm_uframes_t numberFramesToRead;
  struct timespec startTime;
  int32_t err;
  snd_pcm_sframes_t totalNumberFramesRead = 0;
  sf_count_t totalNumberFrames = 0;
  snd_pcm_t *hPcmCapture = nullptr;
  std::string alsaDevice = "smartx:" + sinkName;
  uint32_t cntPeriods = 0;

  setThreadSchedParams();
  cout << "=================> Started recording thread" << endl;
  err = snd_pcm_open(&hPcmCapture, alsaDevice.c_str(), SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK);
  if (err < 0)
  {
    cout << "[record]Error during snd_pcm_open of capture device: " << snd_strerror(err) << endl;
  }
  ASSERT_EQ(0, err);
  setHwParams(hPcmCapture, cFrameLength, 4*cFrameLength, 48000, cNrChannels, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED);
  setSwParams(hPcmCapture, cFrameLength, 16*cFrameLength);
  snd_pcm_dump(hPcmCapture, mSndOut);

  snd_pcm_state_t pcmState = snd_pcm_state(hPcmCapture);
  if (pcmState != SND_PCM_STATE_RUNNING)
  {
    cout << "[record]ALSA capture device state before start == " << snd_pcm_state_name(pcmState) << endl;
    err = clock_gettime(CLOCK_MONOTONIC, &startTime);
    ASSERT_EQ(0, err);
    err = snd_pcm_start(hPcmCapture);
    ASSERT_EQ(0, err);
    pcmState = snd_pcm_state(hPcmCapture);
    cout << "[record]ALSA capture device state after start == " << snd_pcm_state_name(pcmState) << endl;
  }

  bool firstRun = true;
  do
  {
    cntPeriods++;
    if ((cntPeriods % 100) == 0)
    {
      cout << "************** Sleep now for 100 ms" << endl << endl << endl;
      usleep(100*1000);
      cout << "**************Back from sleep" << endl << endl << endl;
    }
    if (firstRun == false)
    {
      err = snd_pcm_wait(hPcmCapture, 100);
      cout << "[record]snd_pcm_wait returned=" << err << endl;
      if (mPlaybackFinished == false)
      {
        ASSERT_EQ(1, err);
      }
    }
    else
    {
      firstRun = false;
    }
    struct timespec startRead;
    err = clock_gettime(CLOCK_MONOTONIC, &startRead);
    ASSERT_EQ(0, err);
    numberFramesToRead = cFrameLength;
    frames = snd_pcm_readi(hPcmCapture, sampleBuffer, numberFramesToRead);
    struct timespec stopRead;
    err = clock_gettime(CLOCK_MONOTONIC, &stopRead);
    ASSERT_EQ(0, err);
    long readDuration = (stopRead.tv_nsec - startRead.tv_nsec) / 1000000;
    readDuration += (stopRead.tv_sec - startRead.tv_sec) * 1000;
    if (frames > 0 && readDuration > 0)
    {
      cout << "[record]snd_pcm_readi duration " << readDuration << " ms" << endl;
    }

    if (frames > 0)
    {
      totalNumberFramesRead += frames;
      snd_pcm_sframes_t delay;
      err = snd_pcm_delay(hPcmCapture, &delay);
      ASSERT_EQ(0, err);
      struct timespec curTime;
      err = clock_gettime(CLOCK_MONOTONIC, &curTime);
      ASSERT_EQ(0, err);
      long timeSinceStart = (curTime.tv_nsec - startTime.tv_nsec) / 1000000;
      timeSinceStart += (curTime.tv_sec - startTime.tv_sec) * 1000;
      cout << "[record]" << totalNumberFramesRead << " frames read by snd_pcm_readi()" << endl;
      cout << "[record]" << delay << " returned by snd_pcm_delay()" << endl;
      snd_pcm_sframes_t playPos = totalNumberFramesRead - delay;
      cout << "[record]" << playPos << " frames / " << playPos/48 << " ms is current play position" << endl;
      cout << "[record]" << timeSinceStart << " ms elapsed since snd_pcm_start" << endl;
      cout << "[record]" << "ALSA device is too slow " << playPos/48 - timeSinceStart << endl;
    }
    else if ((frames == -EAGAIN) || (frames == 0))
    {
      frames = 0;
    }
    else if (frames < 0)
    {
      frames = snd_pcm_recover(hPcmCapture, static_cast<int32_t>(frames), 0);
      if (frames < 0) {
        cout << "[record]Recovery from snd_pcm_readi failed: " << snd_strerror(err) << endl;
        break;
      }
    }

    if (frames > 0 && frames < static_cast<snd_pcm_sframes_t>(numberFramesToRead))
    {
      cout << "[record]Short read (expected " <<  numberFramesToRead << ", read " << frames << ")" << endl;
    }
    if (frames > 0)
    {
      sf_count_t numberFramesWritten;
      numberFramesWritten = sf_writef_short(mOutputFile1, sampleBuffer, frames);
      cout << "[record]Number frames written to file: " << numberFramesWritten << endl;
      totalNumberFrames += numberFramesWritten;
    }
  }
  while (endThread == false);

  delete[] sampleBuffer;
  snd_pcm_close(hPcmCapture);
  cout << "=================> End recording thread" << endl;
}

/*
 * @brief Thread for recording from an ALSA capture device in blocking mode.
 *
 * The capture process is paused from time to time in order to verify that
 * after a pause the snd_pcm_wait() method again blocks appropriately.
 */
void IasAlsaPluginUseCasesTest::recordBlockingMode(const std::string sinkName)
{
  const uint32_t cFrameLength = 192;
  const uint32_t cNrChannels = 2;
  int16_t *sampleBuffer = new int16_t[cFrameLength*cNrChannels];
  snd_pcm_sframes_t frames = 1;
  snd_pcm_uframes_t numberFramesToRead;
  struct timespec startTime;
  int32_t err;
  snd_pcm_sframes_t totalNumberFramesRead = 0;
  sf_count_t totalNumberFrames = 0;
  snd_pcm_t *hPcmCapture = nullptr;
  std::string alsaDevice = "smartx:" + sinkName;
  uint32_t cntPeriods = 0;

  setThreadSchedParams();
  cout << "=================> Started recording thread" << endl;
  err = snd_pcm_open(&hPcmCapture, alsaDevice.c_str(), SND_PCM_STREAM_CAPTURE, 0 /*blocking mode*/);
  if (err < 0)
  {
    cout << "[record]Error during snd_pcm_open of capture device: " << snd_strerror(err) << endl;
  }
  ASSERT_EQ(0, err);
  setHwParams(hPcmCapture, cFrameLength, 4*cFrameLength, 48000, cNrChannels, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED);
  setSwParams(hPcmCapture, cFrameLength, 16*cFrameLength);
  snd_pcm_dump(hPcmCapture, mSndOut);

  snd_pcm_state_t pcmState = snd_pcm_state(hPcmCapture);
  if (pcmState != SND_PCM_STATE_RUNNING)
  {
    cout << "[record]ALSA capture device state before start == " << snd_pcm_state_name(pcmState) << endl;
    err = clock_gettime(CLOCK_MONOTONIC, &startTime);
    ASSERT_EQ(0, err);
    err = snd_pcm_start(hPcmCapture);
    ASSERT_EQ(0, err);
    pcmState = snd_pcm_state(hPcmCapture);
    cout << "[record]ALSA capture device state after start == " << snd_pcm_state_name(pcmState) << endl;
  }

  do
  {
    // After every 100 periods, sleep for 100 ms. During this time, the SmartXbar will
    // fill up the ring buffer at the capture device. By means of this we can verify
    // that after the sleep the snd_pcm_wait function will again block appropriately.
    cntPeriods++;
    if ((cntPeriods % 100) == 0)
    {
      cout << endl << "[record]>>>>> Now sleeping for 100 ms <<<<<" << endl << endl;
      usleep(100*1000);
    }

    struct timespec startWait;
    err = clock_gettime(CLOCK_MONOTONIC, &startWait);
    ASSERT_EQ(0, err);

    snd_pcm_sframes_t avail = snd_pcm_avail_update(hPcmCapture);
    cout << "[record] avail = " << avail << endl;
    err = snd_pcm_wait(hPcmCapture, 100);
    if (err == 0)
    {
      cout << "======================> [record] TIMEOUT" << endl;
      break;
    }

    struct timespec stopWait;
    err = clock_gettime(CLOCK_MONOTONIC, &stopWait);
    ASSERT_EQ(0, err);

    long waitDuration = (stopWait.tv_nsec - startWait.tv_nsec) / 1000;
    waitDuration += (stopWait.tv_sec - startWait.tv_sec) * 1000000;
    cout << "[record]snd_pcm_wait duration " << waitDuration << " us" << endl;

    struct timespec startRead;
    err = clock_gettime(CLOCK_MONOTONIC, &startRead);
    ASSERT_EQ(0, err);
    numberFramesToRead = cFrameLength;
    frames = snd_pcm_readi(hPcmCapture, sampleBuffer, numberFramesToRead);
    struct timespec stopRead;
    err = clock_gettime(CLOCK_MONOTONIC, &stopRead);
    ASSERT_EQ(0, err);
    long readDuration = (stopRead.tv_nsec - startRead.tv_nsec) / 1000;
    readDuration += (stopRead.tv_sec - startRead.tv_sec) * 1000000;
    cout << "[record]snd_pcm_readi duration " << readDuration << " us" << endl;

    // Altough we use the blocking mode for snd_pcm_readi(), the blocking must happen always
    // in snd_pcm_wait() and never in snd_pcm_readi()
    if (readDuration > 200)
    {
      cout << "[record]snd_pcm_readi took longer than 200 us." << endl;
    }

    if (frames > 0)
    {
      totalNumberFramesRead += frames;
      snd_pcm_sframes_t delay;
      err = snd_pcm_delay(hPcmCapture, &delay);
      ASSERT_EQ(0, err);
      struct timespec curTime;
      err = clock_gettime(CLOCK_MONOTONIC, &curTime);
      ASSERT_EQ(0, err);
      long timeSinceStart = (curTime.tv_nsec - startTime.tv_nsec) / 1000000;
      timeSinceStart += (curTime.tv_sec - startTime.tv_sec) * 1000;
      cout << "[record]" << totalNumberFramesRead << " frames read by snd_pcm_readi()" << endl;
      cout << "[record]" << delay << " returned by snd_pcm_delay()" << endl;
      snd_pcm_sframes_t playPos = totalNumberFramesRead - delay;
      cout << "[record]" << playPos << " frames / " << playPos/48 << " ms is current play position" << endl;
      cout << "[record]" << timeSinceStart << " ms elapsed since snd_pcm_start" << endl;
      cout << "[record]" << "ALSA device is too slow " << playPos/48 - timeSinceStart << endl;
    }
    else if ((frames == -EAGAIN) || (frames == 0))
    {
      frames = 0;
    }
    else if (frames < 0)
    {
      frames = snd_pcm_recover(hPcmCapture, static_cast<int32_t>(frames), 0);
      if (frames < 0) {
        cout << "[record]Recovery from snd_pcm_readi failed: " << snd_strerror(err) << endl;
        break;
      }
    }

    if (frames > 0 && frames < static_cast<snd_pcm_sframes_t>(numberFramesToRead))
    {
      cout << "[record]Short read (expected " <<  numberFramesToRead << ", read " << frames << ")" << endl;
    }
    if (frames > 0)
    {
      sf_count_t numberFramesWritten;
      numberFramesWritten = sf_writef_short(mOutputFile1, sampleBuffer, frames);
      cout << "[record]Number frames written to file: " << numberFramesWritten << endl;
      totalNumberFrames += numberFramesWritten;
    }
  }
  while (true);

  delete[] sampleBuffer;
  snd_pcm_close(hPcmCapture);
  cout << "=================> End recording thread" << endl;
}

/*
 * @brief Thread for recording from an ALSA capture device in blocking mode.
 *
 * The capture process is paused from time to time in order to verify that
 * after a pause the snd_pcm_wait() method again blocks appropriately.
 */
void IasAlsaPluginUseCasesTest::recordBlockingModeRetries(const std::string sinkName)
{
  const uint32_t cFrameLength = 192;
  const uint32_t cNrChannels = 2;
  int16_t *sampleBuffer = new int16_t[cFrameLength*cNrChannels];
  snd_pcm_sframes_t frames = 1;
  snd_pcm_uframes_t numberFramesToRead;
  struct timespec startTime;
  int32_t err;
  snd_pcm_sframes_t totalNumberFramesRead = 0;
  sf_count_t totalNumberFrames = 0;
  snd_pcm_t *hPcmCapture = nullptr;
  string alsaDevice = "smartx:" + sinkName;
  uint32_t cntPeriods = 0;

  setThreadSchedParams();
  cout << "=================> Started recording thread" << endl;
  err = snd_pcm_open(&hPcmCapture, alsaDevice.c_str(), SND_PCM_STREAM_CAPTURE, 0 /*blocking mode*/);
  if (err < 0)
  {
    cout << "[record]Error during snd_pcm_open of capture device: " << snd_strerror(err) << endl;
  }
  ASSERT_EQ(0, err);
  setHwParams(hPcmCapture, cFrameLength, 4*cFrameLength, 48000, cNrChannels, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED);
  setSwParams(hPcmCapture, cFrameLength, 16*cFrameLength);
  snd_pcm_dump(hPcmCapture, mSndOut);

  snd_pcm_state_t pcmState = snd_pcm_state(hPcmCapture);
  if (pcmState != SND_PCM_STATE_RUNNING)
  {
    cout << "[record]ALSA capture device state before start == " << snd_pcm_state_name(pcmState) << endl;
    err = clock_gettime(CLOCK_MONOTONIC, &startTime);
    ASSERT_EQ(0, err);
    err = snd_pcm_start(hPcmCapture);
    ASSERT_EQ(0, err);
    pcmState = snd_pcm_state(hPcmCapture);
    cout << "[record]ALSA capture device state after start == " << snd_pcm_state_name(pcmState) << endl;
  }

  uint32_t retries = 3;
  do
  {
    // After every 100 periods, sleep for 100 ms. During this time, the SmartXbar will
    // fill up the ring buffer at the capture device. By means of this we can verify
    // that after the sleep the snd_pcm_wait function will again block appropriately.
    cntPeriods++;
    if ((cntPeriods % 100) == 0)
    {
      cout << endl << "[record]>>>>> Now sleeping for 100 ms <<<<<" << endl << endl;
      usleep(100*1000);
    }

    struct timespec startWait;
    err = clock_gettime(CLOCK_MONOTONIC, &startWait);
    ASSERT_EQ(0, err);

    snd_pcm_sframes_t avail = snd_pcm_avail_update(hPcmCapture);
    cout << "[record] avail = " << avail << endl;
    err = snd_pcm_wait(hPcmCapture, 100);
    if (err == 0)
    {
      cout << "======================> [record] TIMEOUT" << endl;
      retries--;
      if (retries == 0)
      {
        break;
      }
      else
      {
        continue;
      }
    }

    struct timespec stopWait;
    err = clock_gettime(CLOCK_MONOTONIC, &stopWait);
    ASSERT_EQ(0, err);

    long waitDuration = (stopWait.tv_nsec - startWait.tv_nsec) / 1000;
    waitDuration += (stopWait.tv_sec - startWait.tv_sec) * 1000000;
    cout << "[record]snd_pcm_wait duration " << waitDuration << " us" << endl;

    struct timespec startRead;
    err = clock_gettime(CLOCK_MONOTONIC, &startRead);
    ASSERT_EQ(0, err);
    numberFramesToRead = cFrameLength;
    frames = snd_pcm_readi(hPcmCapture, sampleBuffer, numberFramesToRead);
    struct timespec stopRead;
    err = clock_gettime(CLOCK_MONOTONIC, &stopRead);
    ASSERT_EQ(0, err);
    long readDuration = (stopRead.tv_nsec - startRead.tv_nsec) / 1000;
    readDuration += (stopRead.tv_sec - startRead.tv_sec) * 1000000;
    cout << "[record]snd_pcm_readi duration " << readDuration << " us" << endl;

    // Altough we use the blocking mode for snd_pcm_readi(), the blocking must happen always
    // in snd_pcm_wait() and never in snd_pcm_readi()
    if (readDuration > 200)
    {
      cout << "[record]snd_pcm_readi took longer than 200 us." << endl;
    }

    if (frames > 0)
    {
      totalNumberFramesRead += frames;
      snd_pcm_sframes_t delay;
      err = snd_pcm_delay(hPcmCapture, &delay);
      ASSERT_EQ(0, err);
      struct timespec curTime;
      err = clock_gettime(CLOCK_MONOTONIC, &curTime);
      ASSERT_EQ(0, err);
      long timeSinceStart = (curTime.tv_nsec - startTime.tv_nsec) / 1000000;
      timeSinceStart += (curTime.tv_sec - startTime.tv_sec) * 1000;
      cout << "[record]" << totalNumberFramesRead << " frames read by snd_pcm_readi()" << endl;
      cout << "[record]" << delay << " returned by snd_pcm_delay()" << endl;
      snd_pcm_sframes_t playPos = totalNumberFramesRead - delay;
      cout << "[record]" << playPos << " frames / " << playPos/48 << " ms is current play position" << endl;
      cout << "[record]" << timeSinceStart << " ms elapsed since snd_pcm_start" << endl;
      cout << "[record]" << "ALSA device is too slow " << playPos/48 - timeSinceStart << endl;
    }
    else if ((frames == -EAGAIN) || (frames == 0))
    {
      frames = 0;
    }
    else if (frames < 0)
    {
      frames = snd_pcm_recover(hPcmCapture, static_cast<int32_t>(frames), 0);
      if (frames < 0) {
        cout << "[record]Recovery from snd_pcm_readi failed: " << snd_strerror(err) << endl;
        break;
      }
    }

    if (frames > 0 && frames < static_cast<snd_pcm_sframes_t>(numberFramesToRead))
    {
      cout << "[record]Short read (expected " <<  numberFramesToRead << ", read " << frames << ")" << endl;
    }
    if (frames > 0)
    {
      sf_count_t numberFramesWritten;
      numberFramesWritten = sf_writef_short(mOutputFile1, sampleBuffer, frames);
      cout << "[record]Number frames written to file: " << numberFramesWritten << endl;
      totalNumberFrames += numberFramesWritten;
    }
  }
  while (true);

  delete[] sampleBuffer;
  snd_pcm_close(hPcmCapture);
  cout << "=================> End recording thread" << endl;
}

/*
 * @brief Thread for recording from an ALSA capture device in blocking mode.
 *
 * The capture process does only start and stop the device but not read data.
 * This is used to check if the event handling works correctly.
 */
void IasAlsaPluginUseCasesTest::recordBlockingModeNoData(const std::string sinkName)
{
  const uint32_t cFrameLength = 192;
  const uint32_t cNrChannels = 2;
  struct timespec startTime;
  int32_t err;
  snd_pcm_t *hPcmCapture = nullptr;
  string alsaDevice = "smartx:" + sinkName;

  setThreadSchedParams();
  cout << "=================> Started recording thread" << endl;
  err = snd_pcm_open(&hPcmCapture, alsaDevice.c_str(), SND_PCM_STREAM_CAPTURE, 0 /*blocking mode*/);
  if (err < 0)
  {
    cout << "[record]Error during snd_pcm_open of capture device: " << snd_strerror(err) << endl;
  }
  ASSERT_EQ(0, err);
  setHwParams(hPcmCapture, cFrameLength, 4*cFrameLength, 48000, cNrChannels, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED);
  setSwParams(hPcmCapture, cFrameLength, 16*cFrameLength);
  snd_pcm_dump(hPcmCapture, mSndOut);

  snd_pcm_state_t pcmState = snd_pcm_state(hPcmCapture);
  if (pcmState != SND_PCM_STATE_RUNNING)
  {
    cout << "[record]ALSA capture device state before start == " << snd_pcm_state_name(pcmState) << endl;
    err = clock_gettime(CLOCK_MONOTONIC, &startTime);
    ASSERT_EQ(0, err);
    err = snd_pcm_start(hPcmCapture);
    ASSERT_EQ(0, err);
    pcmState = snd_pcm_state(hPcmCapture);
    cout << "[record]ALSA capture device state after start == " << snd_pcm_state_name(pcmState) << endl;
  }

  snd_pcm_drop(hPcmCapture);
  snd_pcm_close(hPcmCapture);
  cout << "=================> End recording thread" << endl;
}


/*
 * @brief Thread for recording from an ALSA capture device in blocking mode.
 *
 * The capture process is paused from time to time in order to verify that
 * after a pause the snd_pcm_wait() method again blocks appropriately.
 */
void IasAlsaPluginUseCasesTest::recordBlockingMode_TC9(const std::string sinkName)
{
  const uint32_t cFrameLength = 192;
  const uint32_t cNrChannels = 2;
  int16_t *sampleBuffer = new int16_t[cFrameLength*cNrChannels];
  snd_pcm_sframes_t frames = 1;
  snd_pcm_uframes_t numberFramesToRead;
  struct timespec startTime;
  int32_t err;
  snd_pcm_sframes_t totalNumberFramesRead = 0;
  sf_count_t totalNumberFrames = 0;
  snd_pcm_t *hPcmCapture = nullptr;
  std::string alsaDevice = "smartx:" + sinkName;
  uint32_t cntPeriods = 0;

  setThreadSchedParams();
  cout << "=================> Started recording thread" << endl;
  err = snd_pcm_open(&hPcmCapture, alsaDevice.c_str(), SND_PCM_STREAM_CAPTURE, 0 /*blocking mode*/);
  if (err < 0)
  {
    cout << "[record]Error during snd_pcm_open of capture device: " << snd_strerror(err) << endl;
  }
  ASSERT_EQ(0, err);
  setHwParams(hPcmCapture, cFrameLength, 4*cFrameLength, 48000, cNrChannels, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED);
  setSwParams(hPcmCapture, cFrameLength, 16*cFrameLength);
  snd_pcm_dump(hPcmCapture, mSndOut);

  snd_pcm_state_t pcmState = snd_pcm_state(hPcmCapture);
  if (pcmState != SND_PCM_STATE_RUNNING)
  {
    cout << "[record]ALSA capture device state before start == " << snd_pcm_state_name(pcmState) << endl;
    err = clock_gettime(CLOCK_MONOTONIC, &startTime);
    ASSERT_EQ(0, err);
    err = snd_pcm_start(hPcmCapture);
    ASSERT_EQ(0, err);
    pcmState = snd_pcm_state(hPcmCapture);
    cout << "[record]ALSA capture device state after start == " << snd_pcm_state_name(pcmState) << endl;
  }

  do
  {
    // After every 100 periods, sleep for 100 ms. During this time, the SmartXbar will
    // fill up the ring buffer at the capture device. By means of this we can verify
    // that after the sleep the snd_pcm_wait function will again block appropriately.
    cntPeriods++;
    if ((cntPeriods % 100) == 0)
    {
      cout << endl << "[record]>>>>> Now closing the device and finish recording <<<<<" << endl << endl;
      break;
    }

    struct timespec startWait;
    err = clock_gettime(CLOCK_MONOTONIC, &startWait);
    ASSERT_EQ(0, err);

    snd_pcm_sframes_t avail = snd_pcm_avail_update(hPcmCapture);
    cout << "[record] avail = " << avail << endl;
    err = snd_pcm_wait(hPcmCapture, 100);
    if (err == 0)
    {
      cout << "======================> [record] TIMEOUT" << endl;
      break;
    }

    struct timespec stopWait;
    err = clock_gettime(CLOCK_MONOTONIC, &stopWait);
    ASSERT_EQ(0, err);

    long waitDuration = (stopWait.tv_nsec - startWait.tv_nsec) / 1000;
    waitDuration += (stopWait.tv_sec - startWait.tv_sec) * 1000000;
    cout << "[record]snd_pcm_wait duration " << waitDuration << " us" << endl;

    struct timespec startRead;
    err = clock_gettime(CLOCK_MONOTONIC, &startRead);
    ASSERT_EQ(0, err);
    numberFramesToRead = cFrameLength;
    frames = snd_pcm_readi(hPcmCapture, sampleBuffer, numberFramesToRead);
    struct timespec stopRead;
    err = clock_gettime(CLOCK_MONOTONIC, &stopRead);
    ASSERT_EQ(0, err);
    long readDuration = (stopRead.tv_nsec - startRead.tv_nsec) / 1000;
    readDuration += (stopRead.tv_sec - startRead.tv_sec) * 1000000;
    cout << "[record]snd_pcm_readi duration " << readDuration << " us" << endl;

    // Altough we use the blocking mode for snd_pcm_readi(), the blocking must happen always
    // in snd_pcm_wait() and never in snd_pcm_readi()
    if (readDuration > 200)
    {
      cout << "[record]snd_pcm_readi took longer than 200 us." << endl;
    }

    if (frames > 0)
    {
      totalNumberFramesRead += frames;
      snd_pcm_sframes_t delay;
      err = snd_pcm_delay(hPcmCapture, &delay);
      ASSERT_EQ(0, err);
      struct timespec curTime;
      err = clock_gettime(CLOCK_MONOTONIC, &curTime);
      ASSERT_EQ(0, err);
      long timeSinceStart = (curTime.tv_nsec - startTime.tv_nsec) / 1000000;
      timeSinceStart += (curTime.tv_sec - startTime.tv_sec) * 1000;
      cout << "[record]" << totalNumberFramesRead << " frames read by snd_pcm_readi()" << endl;
      cout << "[record]" << delay << " returned by snd_pcm_delay()" << endl;
      snd_pcm_sframes_t playPos = totalNumberFramesRead - delay;
      cout << "[record]" << playPos << " frames / " << playPos/48 << " ms is current play position" << endl;
      cout << "[record]" << timeSinceStart << " ms elapsed since snd_pcm_start" << endl;
      cout << "[record]" << "ALSA device is too slow " << playPos/48 - timeSinceStart << endl;
    }
    else if ((frames == -EAGAIN) || (frames == 0))
    {
      frames = 0;
    }
    else if (frames < 0)
    {
      frames = snd_pcm_recover(hPcmCapture, static_cast<int32_t>(frames), 0);
      if (frames < 0) {
        cout << "[record]Recovery from snd_pcm_readi failed: " << snd_strerror(err) << endl;
        break;
      }
    }

    if (frames > 0 && frames < static_cast<snd_pcm_sframes_t>(numberFramesToRead))
    {
      cout << "[record]Short read (expected " <<  numberFramesToRead << ", read " << frames << ")" << endl;
    }
    if (frames > 0)
    {
      sf_count_t numberFramesWritten;
      numberFramesWritten = sf_writef_short(mOutputFile1, sampleBuffer, frames);
      cout << "[record]Number frames written to file: " << numberFramesWritten << endl;
      totalNumberFrames += numberFramesWritten;
    }
  }
  while (true);

  delete[] sampleBuffer;
  snd_pcm_close(hPcmCapture);
  cout << "=================> End recording thread" << endl;
}

////////////////////////////////////////////////////////////////////////////////
// TEST CASES
////////////////////////////////////////////////////////////////////////////////

/*****************************************
 * Testcase:    TC1
 * Description:
 * Play wave file in non-blocking mode until finished. Checking
 * delay value and add time measurement like
 * some media players do.
 *
 * Period size: 2048 frames
 * Buffer size: 3 * period size frames
 * Sample rate: 48 kHz
 */
TEST_P(IasAlsaPluginUseCasesTest, TC1)
{
  const uint32_t cFrameLength = 1920;
  const uint32_t cNrChannels = 2;
  mSamples = new int16_t[cFrameLength*cNrChannels];

  int32_t err;

  IasIRouting *routing = mSmartx->routing();
  ASSERT_TRUE(routing != nullptr);
  IasIRouting::IasResult roucon = routing->connect(123, 123);
  ASSERT_EQ(IasIRouting::eIasOk, roucon);


  std::string alsaDevice = "smartx:stereo48";
  sf_count_t numberFramesRead;
  sf_count_t totalNumberFrames = 0;
  err = snd_pcm_open(&mPcm, alsaDevice.c_str(), SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
  if (err < 0)
  {
    cout << "Error during snd_pcm_open: " << snd_strerror(err) << endl;
  }
  ASSERT_EQ(0, err);
  // Try to open device again, to see whether it fails as expected with EBUSY
  snd_pcm_t *dummyPcm = nullptr;
  err = snd_pcm_open(&dummyPcm, alsaDevice.c_str(), SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
  ASSERT_EQ(-EBUSY, err);

  setHwParams(mPcm, cFrameLength, 3*cFrameLength, 48000, cNrChannels, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED);
  setSwParams(mPcm, cFrameLength, 16*cFrameLength);
  snd_pcm_dump(mPcm, mSndOut);

  struct timespec clockRes;
  err = clock_getres(CLOCK_MONOTONIC, &clockRes);
  ASSERT_EQ(0, err);
  cout << "Clock resolution: " << clockRes.tv_sec << " sec:" << clockRes.tv_nsec << " nsec" << endl;

  snd_pcm_sframes_t frames = 1;
  snd_pcm_sframes_t numberFramesAvail;
  struct timespec startTime;
  err = clock_gettime(CLOCK_MONOTONIC, &startTime);
  ASSERT_EQ(0, err);
  snd_pcm_sframes_t totalNumberFramesWritten = 0;
  do
  {
    if (frames > 0)
    {
      numberFramesRead = sf_readf_short(mInputFile2, mSamples, cFrameLength);
      cout << "Number frames read from file: " << numberFramesRead << endl;
      totalNumberFrames += numberFramesRead;
      if (numberFramesRead > 0 && numberFramesRead < cFrameLength)
      {
        // The rest of the buffer not filled with samples from file will be filled with zeros
        uint32_t rest = static_cast<uint32_t>(cFrameLength - numberFramesRead);
        memset(&mSamples[numberFramesRead*cNrChannels], 0, sizeof(int16_t)*rest*cNrChannels);
        // Set the numberFramesRead variable to cFrameLength to fill a complete period size buffer
        numberFramesRead = cFrameLength;
      }
    }
    struct timespec startWrite;
    err = clock_gettime(CLOCK_MONOTONIC, &startWrite);
    ASSERT_EQ(0, err);
    frames = snd_pcm_writei(mPcm, mSamples, numberFramesRead);
    struct timespec stopWrite;
    err = clock_gettime(CLOCK_MONOTONIC, &stopWrite);
    ASSERT_EQ(0, err);
    long writeDuration = (stopWrite.tv_nsec - startWrite.tv_nsec) / 1000000;
    writeDuration += (stopWrite.tv_sec - startWrite.tv_sec) * 1000;
    if (frames > 0 && writeDuration > 0)
    {
      cout << "snd_pcm_writei duration " << writeDuration << " ms" << endl;
    }

    if (frames > 0)
    {
      totalNumberFramesWritten += frames;
      snd_pcm_sframes_t delay;
      err = snd_pcm_delay(mPcm, &delay);
      ASSERT_EQ(0, err);
      struct timespec curTime;
      err = clock_gettime(CLOCK_MONOTONIC, &curTime);
      ASSERT_EQ(0, err);
      long timeSinceStart = (curTime.tv_nsec - startTime.tv_nsec) / 1000000;
      timeSinceStart += (curTime.tv_sec - startTime.tv_sec) * 1000;
      cout << totalNumberFramesWritten << " frames written to snd_pcm_writei()" << endl;
      cout << delay << " returned by snd_pcm_delay()" << endl;
      snd_pcm_sframes_t playPos = totalNumberFramesWritten - delay;
      cout << playPos << " frames / " << playPos/48 << " ms is current play position" << endl;
      cout << timeSinceStart << " ms elapsed since snd_pcm_start" << endl;
      cout << "ALSA device is too slow " << playPos/48 - timeSinceStart << endl;
    }
    else if ((frames == -EAGAIN) || (frames == 0))
    {
      snd_pcm_state_t pcmState = snd_pcm_state(mPcm);
      if (pcmState != SND_PCM_STATE_RUNNING)
      {
        cout << "ALSA device state before start == " << snd_pcm_state_name(pcmState) << endl;
        cout << "Buffer completely filled, now start the pcm device" << endl;
        err = clock_gettime(CLOCK_MONOTONIC, &startTime);
        ASSERT_EQ(0, err);
        err = snd_pcm_start(mPcm);
        ASSERT_EQ(0, err);
        pcmState = snd_pcm_state(mPcm);
        cout << "ALSA device state after start == " << snd_pcm_state_name(pcmState) << endl;
      }
      frames = 0;
    }
    else if (frames < 0)
    {
      frames = snd_pcm_recover(mPcm, static_cast<int32_t>(frames), 0);
      if (frames < 0) {
        cout << "snd_pcm_writei failed: " << snd_strerror(err) << endl;
        break;
      }
    }

    if (frames > 0 && frames < numberFramesRead)
    {
      cout << "Short write (expected " <<  sizeof(mSamples) << ", wrote " << frames << ")" << endl;
    }
  }
  while (numberFramesRead != 0);
  cout << "Total number of frames read from file: " << totalNumberFrames << endl;

  snd_pcm_state_t pcmState = snd_pcm_state(mPcm);
  cout << "ALSA device state at the end == " << snd_pcm_state_name(pcmState) << endl;
  ASSERT_EQ(SND_PCM_STATE_RUNNING, pcmState);

  err = snd_pcm_nonblock(mPcm, 0);
  ASSERT_EQ(0, err);
  err = snd_pcm_drain(mPcm);
  ASSERT_EQ(0, err);
  numberFramesAvail = snd_pcm_avail(mPcm);
  cout << "Number frames available in buffer to fill by writing after end of playback: " << numberFramesAvail << endl;
  numberFramesAvail = snd_pcm_avail_update(mPcm);
  cout << "snd_pcm_avail_update returned: " << numberFramesAvail << endl;

  delete[] mSamples;
  snd_pcm_close(mPcm);
  roucon = routing->disconnect(123, 123);
  ASSERT_EQ(IasIRouting::eIasOk, roucon);
}

/*****************************************
 * Testcase:    TC2
 * Description:
 * Play wave file in non-blocking mode until finished. Checking
 * delay value and add time measurement like
 * some media players do.
 * The block size writing to the ALSA device (2000) is different than the period size (2048)
 * to enforce a partially filled buffer
 *
 * Period size: 2048 frames
 * Buffer size: 3 * period size frames
 * Sample rate: 48 kHz
 */
TEST_P(IasAlsaPluginUseCasesTest, TC2)
{
  const uint32_t cFrameLength = 2048;
  const uint32_t cNrChannels = 2;
  mSamples = new int16_t[cFrameLength*cNrChannels];

  int32_t err;

  IasIRouting *routing = mSmartx->routing();
  ASSERT_TRUE(routing != nullptr);
  IasIRouting::IasResult roucon = routing->connect(123, 123);
  ASSERT_EQ(IasIRouting::eIasOk, roucon);

  std::string recPortName = defaultDevice + "_port";

  mSmartx->debug()->startRecord("out_port", recPortName, 10);

  std::string alsaDevice = "smartx:stereo48";
  sf_count_t numberFramesRead = 0;
  sf_count_t totalNumberFrames = 0;
  err = snd_pcm_open(&mPcm, alsaDevice.c_str(), SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
  if (err < 0)
  {
    cout << "Error during snd_pcm_open: " << snd_strerror(err) << endl;
  }
  ASSERT_EQ(0, err);

  setHwParams(mPcm, cFrameLength, 3*cFrameLength, 48000, cNrChannels, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED);
  setSwParams(mPcm, cFrameLength, 16*cFrameLength);
  snd_pcm_dump(mPcm, mSndOut);

  struct timespec clockRes;
  err = clock_getres(CLOCK_MONOTONIC, &clockRes);
  ASSERT_EQ(0, err);
  cout << "Clock resolution: " << clockRes.tv_sec << " sec:" << clockRes.tv_nsec << " nsec" << endl;

  snd_pcm_sframes_t frames = 1;
  snd_pcm_sframes_t numberFramesAvail;
  struct timespec startTime;
  err = clock_gettime(CLOCK_MONOTONIC, &startTime);
  ASSERT_EQ(0, err);
  snd_pcm_sframes_t totalNumberFramesWritten = 0;
  const uint32_t cFractFrameLength = 2000;
  sf_seek(mInputFile2, 0, SEEK_SET);
  int16_t *write = mSamples;
  static bool alreadyPrinted = false;
  do
  {
    if (numberFramesRead == 0)
    {
      numberFramesRead = sf_readf_short(mInputFile2, mSamples, cFractFrameLength);
      cout << "Number frames read from file: " << numberFramesRead << endl;
      totalNumberFrames += numberFramesRead;
      if (numberFramesRead > 0 && numberFramesRead < cFractFrameLength)
      {
        // The rest of the buffer not filled with samples from file will be filled with zeros
        uint32_t rest = static_cast<uint32_t>(cFrameLength - numberFramesRead);
        memset(&mSamples[numberFramesRead*cNrChannels], 0, sizeof(int16_t)*rest*cNrChannels);
        // Set the numberFramesRead variable to cFrameLength to fill a complete period size buffer
        numberFramesRead = cFractFrameLength;
      }
      else if (numberFramesRead == 0)
      {
        break;
      }
    }
    struct timespec startWrite;
    err = clock_gettime(CLOCK_MONOTONIC, &startWrite);
    ASSERT_EQ(0, err);
    frames = snd_pcm_writei(mPcm, write, numberFramesRead);
    if (alreadyPrinted == false)
    {
      cout << "Wrote block of " << numberFramesRead << " to snd_pcm_writei" << endl;
    }
    struct timespec stopWrite;
    err = clock_gettime(CLOCK_MONOTONIC, &stopWrite);
    ASSERT_EQ(0, err);
    long writeDuration = (stopWrite.tv_nsec - startWrite.tv_nsec) / 1000000;
    writeDuration += (stopWrite.tv_sec - startWrite.tv_sec) * 1000;
    if (frames > 0 && writeDuration > 0)
    {
      cout << "snd_pcm_writei duration " << writeDuration << " ms" << endl;
    }

    if (frames > 0)
    {
      alreadyPrinted = false;
      numberFramesRead -= frames;
      if (numberFramesRead > 0)
      {
        write += frames;
      }
      else if (numberFramesRead == 0)
      {
        write = mSamples;
      }

      totalNumberFramesWritten += frames;
      snd_pcm_sframes_t delay;
      err = snd_pcm_delay(mPcm, &delay);
      ASSERT_EQ(0, err);
      struct timespec curTime;
      err = clock_gettime(CLOCK_MONOTONIC, &curTime);
      ASSERT_EQ(0, err);
      long timeSinceStart = (curTime.tv_nsec - startTime.tv_nsec) / 1000000;
      timeSinceStart += (curTime.tv_sec - startTime.tv_sec) * 1000;
      cout << totalNumberFramesWritten << " frames written to snd_pcm_writei()" << endl;
      cout << delay << " returned by snd_pcm_delay()" << endl;
      snd_pcm_sframes_t playPos = totalNumberFramesWritten - delay;
      cout << playPos << " frames / " << playPos/48 << " ms is current play position" << endl;
      cout << timeSinceStart << " ms elapsed since snd_pcm_start" << endl;
      cout << "ALSA device is too slow " << playPos/48 - timeSinceStart << endl;
    }
    else if ((frames == -EAGAIN) || (frames == 0))
    {
      snd_pcm_state_t pcmState = snd_pcm_state(mPcm);
      if (pcmState != SND_PCM_STATE_RUNNING)
      {
        cout << "ALSA device state before start == " << snd_pcm_state_name(pcmState) << endl;
        cout << "Buffer completely filled, now start the pcm device" << endl;
        err = clock_gettime(CLOCK_MONOTONIC, &startTime);
        ASSERT_EQ(0, err);
        err = snd_pcm_start(mPcm);
        ASSERT_EQ(0, err);
        pcmState = snd_pcm_state(mPcm);
        cout << "ALSA device state after start == " << snd_pcm_state_name(pcmState) << endl;
      }
      else
      {
        if (alreadyPrinted == false)
        {
          cout << "ALSA device already started, but no frames could be written" << endl;
          alreadyPrinted = true;
        }
      }
      frames = 0;
    }
    else if (frames < 0)
    {
      frames = snd_pcm_recover(mPcm, static_cast<int32_t>(frames), 0);
      if (frames < 0) {
        cout << "snd_pcm_writei failed: " << snd_strerror(err) << endl;
        break;
      }
    }

    if (frames > 0 && frames < numberFramesRead)
    {
      cout << "Short write (expected " <<  numberFramesRead << ", wrote " << frames << ")" << endl;
    }
  }
  while (true);
  cout << "Total number of frames read from file: " << totalNumberFrames << endl;

  snd_pcm_state_t pcmState = snd_pcm_state(mPcm);
  cout << "ALSA device state at the end == " << snd_pcm_state_name(pcmState) << endl;
  ASSERT_EQ(SND_PCM_STATE_RUNNING, pcmState);

  err = snd_pcm_nonblock(mPcm, 0);
  ASSERT_EQ(0, err);
  err = snd_pcm_drain(mPcm);
  ASSERT_EQ(0, err);
  numberFramesAvail = snd_pcm_avail(mPcm);
  cout << "Number frames available in buffer to fill by writing after end of playback: " << numberFramesAvail << endl;
  numberFramesAvail = snd_pcm_avail_update(mPcm);
  cout << "snd_pcm_avail_update returned: " << numberFramesAvail << endl;

  delete[] mSamples;
  snd_pcm_close(mPcm);

  mSmartx->debug()->stopProbing(recPortName);
  roucon = routing->disconnect(123, 123);
  ASSERT_EQ(IasIRouting::eIasOk, roucon);
}

/*****************************************
 * Testcase:    TC4
 * Description:
 * Play wave file in non-blocking mode until finished. Checking
 * delay value and add time measurement like
 * some media players do.
 *
 * Period size: 1920 frames
 * Buffer size: 3 * period size frames
 * Sample rate: 48 kHz
 */
TEST_P(IasAlsaPluginUseCasesTest, TC4)
{
  const uint32_t cFrameLength = 1920;
  const uint32_t cNrChannels = 2;
  mSamples = new int16_t[cFrameLength*cNrChannels];

  int32_t err;

  IasIRouting *routing = mSmartx->routing();
  ASSERT_TRUE(routing != nullptr);
  IasIRouting::IasResult roucon = routing->connect(123, 123);
  ASSERT_EQ(IasIRouting::eIasOk, roucon);

  std::string alsaDevice = "smartx:stereo48";
  sf_count_t numberFramesRead;
  sf_count_t totalNumberFrames = 0;
  err = snd_pcm_open(&mPcm, alsaDevice.c_str(), SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
  if (err < 0)
  {
    cout << "Error during snd_pcm_open: " << snd_strerror(err) << endl;
  }
  ASSERT_EQ(0, err);

  setHwParams(mPcm, cFrameLength, 3*cFrameLength, 48000, cNrChannels, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED);
  setSwParams(mPcm, cFrameLength, 16*cFrameLength);
  snd_pcm_dump(mPcm, mSndOut);

  struct timespec clockRes;
  err = clock_getres(CLOCK_MONOTONIC, &clockRes);
  ASSERT_EQ(0, err);
  cout << "Clock resolution: " << clockRes.tv_sec << " sec:" << clockRes.tv_nsec << " nsec" << endl;

  snd_pcm_sframes_t frames = 1;
  snd_pcm_sframes_t numberFramesAvail;
  struct timespec startTime;
  err = clock_gettime(CLOCK_MONOTONIC, &startTime);
  ASSERT_EQ(0, err);
  snd_pcm_sframes_t totalNumberFramesWritten = 0;
  sf_seek(mInputFile3, 0, SEEK_SET);
  // Prefill buffer
  do
  {
    numberFramesRead = sf_readf_short(mInputFile3, mSamples, cFrameLength);
    cout << "Number frames read from file: " << numberFramesRead << endl;
    totalNumberFrames += numberFramesRead;
    frames = snd_pcm_writei(mPcm, mSamples, numberFramesRead);
    if (frames > 0)
    {
      totalNumberFramesWritten += frames;
    }
  }
  while (frames > 0);
  if ((frames == -EAGAIN) || (frames == 0))
  {
    snd_pcm_state_t pcmState = snd_pcm_state(mPcm);
    if (pcmState != SND_PCM_STATE_RUNNING)
    {
      cout << "ALSA device state before start == " << snd_pcm_state_name(pcmState) << endl;
      cout << "Buffer completely filled, now start the pcm device" << endl;
      err = clock_gettime(CLOCK_MONOTONIC, &startTime);
      ASSERT_EQ(0, err);
      err = snd_pcm_start(mPcm);
      ASSERT_EQ(0, err);
      pcmState = snd_pcm_state(mPcm);
      cout << "ALSA device state after start == " << snd_pcm_state_name(pcmState) << endl;
    }
  }

  do
  {
    if (frames > 0)
    {
      numberFramesRead = sf_readf_short(mInputFile3, mSamples, cFrameLength);
      cout << "Number frames read from file: " << numberFramesRead << endl;
      totalNumberFrames += numberFramesRead;
      if (numberFramesRead > 0 && numberFramesRead < cFrameLength)
      {
        // The rest of the buffer not filled with samples from file will be filled with zeros
        uint32_t rest = static_cast<uint32_t>(cFrameLength - numberFramesRead);
        memset(&mSamples[numberFramesRead*cNrChannels], 0, sizeof(int16_t)*rest*cNrChannels);
        // Set the numberFramesRead variable to cFrameLength to fill a complete period size buffer
        numberFramesRead = cFrameLength;
      }
    }
    err = snd_pcm_wait(mPcm, 5000);
    if (err == 0)
    {
      cout << "======================> TIMEOUT" << endl;
    }
    struct timespec startWrite;
    err = clock_gettime(CLOCK_MONOTONIC, &startWrite);
    ASSERT_EQ(0, err);
    frames = snd_pcm_writei(mPcm, mSamples, numberFramesRead);
    struct timespec stopWrite;
    err = clock_gettime(CLOCK_MONOTONIC, &stopWrite);
    ASSERT_EQ(0, err);
    long writeDuration = (stopWrite.tv_nsec - startWrite.tv_nsec) / 1000000;
    writeDuration += (stopWrite.tv_sec - startWrite.tv_sec) * 1000;
    if (frames > 0 && writeDuration > 0)
    {
      cout << "snd_pcm_writei duration " << writeDuration << " ms" << endl;
    }

    if (frames > 0)
    {
      totalNumberFramesWritten += frames;
      snd_pcm_sframes_t delay;
      err = snd_pcm_delay(mPcm, &delay);
      ASSERT_EQ(0, err);
      struct timespec curTime;
      err = clock_gettime(CLOCK_MONOTONIC, &curTime);
      ASSERT_EQ(0, err);
      long timeSinceStart = (curTime.tv_nsec - startTime.tv_nsec) / 1000000;
      timeSinceStart += (curTime.tv_sec - startTime.tv_sec) * 1000;
      cout << totalNumberFramesWritten << " frames written to snd_pcm_writei()" << endl;
      cout << delay << " returned by snd_pcm_delay()" << endl;
      snd_pcm_sframes_t playPos = totalNumberFramesWritten - delay;
      cout << playPos << " frames / " << playPos/48 << " ms is current play position" << endl;
      cout << timeSinceStart << " ms elapsed since snd_pcm_start" << endl;
      cout << "ALSA device is too slow " << playPos/48 - timeSinceStart << endl;
    }
    else if ((frames == -EAGAIN) || (frames == 0))
    {
      frames = 0;
    }
    else if (frames < 0)
    {
      frames = snd_pcm_recover(mPcm, static_cast<int32_t>(frames), 0);
      if (frames < 0) {
        cout << "snd_pcm_writei failed: " << snd_strerror(err) << endl;
        break;
      }
    }

    if (frames > 0 && frames < numberFramesRead)
    {
      cout << "Short write (expected " <<  sizeof(mSamples) << ", wrote " << frames << ")" << endl;
    }
  }
  while (numberFramesRead != 0);
  cout << "Total number of frames read from file: " << totalNumberFrames << endl;

  snd_pcm_state_t pcmState = snd_pcm_state(mPcm);
  cout << "ALSA device state at the end == " << snd_pcm_state_name(pcmState) << endl;
  ASSERT_EQ(SND_PCM_STATE_RUNNING, pcmState);

  err = snd_pcm_nonblock(mPcm, 0);
  ASSERT_EQ(0, err);
  err = snd_pcm_drain(mPcm);
  ASSERT_EQ(0, err);
  numberFramesAvail = snd_pcm_avail(mPcm);
  cout << "Number frames available in buffer to fill by writing after end of playback: " << numberFramesAvail << endl;
  numberFramesAvail = snd_pcm_avail_update(mPcm);
  cout << "snd_pcm_avail_update returned: " << numberFramesAvail << endl;

  delete[] mSamples;
  snd_pcm_close(mPcm);
  roucon = routing->disconnect(123, 123);
  ASSERT_EQ(IasIRouting::eIasOk, roucon);
}

/*****************************************
 * Testcase:    TC5
 * Description:
 * Play wave file in non-blocking mode until finished. Checking
 * delay value and add time measurement like
 * some media players do.
 *
 * Period size: 1920 frames
 * Buffer size: 3 * period size frames
 * Sample rate: 48 kHz
 */
TEST_P(IasAlsaPluginUseCasesTest, TC5)
{
  auto testCase = GetParam();
  const int32_t sink = std::get<0>(testCase);
  const std::string sinkName = std::get<1>(testCase);
  const uint32_t cFrameLength = 1920;
  const uint32_t cNrChannels = 2;
  mSamples = new int16_t[cFrameLength*cNrChannels];

  int32_t err;
  IasIRouting::IasResult roucon;

  IasIRouting *routing = mSmartx->routing();
  ASSERT_TRUE(routing != nullptr);
  roucon = routing->connect(123, 123);
  ASSERT_EQ(IasIRouting::eIasOk, roucon);
  roucon = routing->connect(123, sink);
  ASSERT_EQ(IasIRouting::eIasOk, roucon);

  // Spawn a new thread for recording on the smartx plugin called smartx:stereo48
  std::thread recordThread(&IasAudio::IasAlsaPluginUseCasesTest_TC5_Test::record, this, sinkName);

  std::string alsaDevice = "smartx:stereo48";
  sf_count_t numberFramesRead;
  sf_count_t totalNumberFrames = 0;
  err = snd_pcm_open(&mPcm, alsaDevice.c_str(), SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
  if (err < 0)
  {
    cout << "Error during snd_pcm_open: " << snd_strerror(err) << endl;
  }
  ASSERT_EQ(0, err);

  setHwParams(mPcm, cFrameLength, 3*cFrameLength, 48000, cNrChannels, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED);
  setSwParams(mPcm, cFrameLength, 16*cFrameLength);
  snd_pcm_dump(mPcm, mSndOut);

  struct timespec clockRes;
  err = clock_getres(CLOCK_MONOTONIC, &clockRes);
  ASSERT_EQ(0, err);
  cout << "Clock resolution: " << clockRes.tv_sec << " sec:" << clockRes.tv_nsec << " nsec" << endl;

  snd_pcm_sframes_t frames = 1;
  snd_pcm_sframes_t numberFramesAvail;
  struct timespec startTime;
  err = clock_gettime(CLOCK_MONOTONIC, &startTime);
  ASSERT_EQ(0, err);
  snd_pcm_sframes_t totalNumberFramesWritten = 0;
  sf_seek(mInputFile3, 0, SEEK_SET);
  // Prefill buffer
  do
  {
    numberFramesRead = sf_readf_short(mInputFile3, mSamples, cFrameLength);
    cout << "Number frames read from file: " << numberFramesRead << endl;
    totalNumberFrames += numberFramesRead;
    frames = snd_pcm_writei(mPcm, mSamples, numberFramesRead);
    if (frames > 0)
    {
      totalNumberFramesWritten += frames;
    }
  }
  while (frames > 0);
  if ((frames == -EAGAIN) || (frames == 0))
  {
    snd_pcm_state_t pcmState = snd_pcm_state(mPcm);
    if (pcmState != SND_PCM_STATE_RUNNING)
    {
      cout << "ALSA device state before start == " << snd_pcm_state_name(pcmState) << endl;
      cout << "Buffer completely filled, now start the pcm device" << endl;
      err = clock_gettime(CLOCK_MONOTONIC, &startTime);
      ASSERT_EQ(0, err);
      err = snd_pcm_start(mPcm);
      ASSERT_EQ(0, err);
      pcmState = snd_pcm_state(mPcm);
      cout << "ALSA device state after start == " << snd_pcm_state_name(pcmState) << endl;
    }
  }

  do
  {
    if (frames > 0)
    {
      numberFramesRead = sf_readf_short(mInputFile3, mSamples, cFrameLength);
      cout << "Number frames read from file: " << numberFramesRead << endl;
      totalNumberFrames += numberFramesRead;
      if (numberFramesRead > 0 && numberFramesRead < cFrameLength)
      {
        // The rest of the buffer not filled with samples from file will be filled with zeros
        uint32_t rest = static_cast<uint32_t>(cFrameLength - numberFramesRead);
        memset(&mSamples[numberFramesRead*cNrChannels], 0, sizeof(int16_t)*rest*cNrChannels);
        // Set the numberFramesRead variable to cFrameLength to fill a complete period size buffer
        numberFramesRead = cFrameLength;
      }
    }
    err = snd_pcm_wait(mPcm, 5000);
    if (err == 0)
    {
      cout << "======================> TIMEOUT" << endl;
    }
    struct timespec startWrite;
    err = clock_gettime(CLOCK_MONOTONIC, &startWrite);
    ASSERT_EQ(0, err);
    frames = snd_pcm_writei(mPcm, mSamples, numberFramesRead);
    struct timespec stopWrite;
    err = clock_gettime(CLOCK_MONOTONIC, &stopWrite);
    ASSERT_EQ(0, err);
    long writeDuration = (stopWrite.tv_nsec - startWrite.tv_nsec) / 1000000;
    writeDuration += (stopWrite.tv_sec - startWrite.tv_sec) * 1000;
    if (frames > 0 && writeDuration > 0)
    {
      cout << "snd_pcm_writei duration " << writeDuration << " ms" << endl;
    }

    if (frames > 0)
    {
      totalNumberFramesWritten += frames;
      snd_pcm_sframes_t delay;
      err = snd_pcm_delay(mPcm, &delay);
      ASSERT_EQ(0, err);
      struct timespec curTime;
      err = clock_gettime(CLOCK_MONOTONIC, &curTime);
      ASSERT_EQ(0, err);
      long timeSinceStart = (curTime.tv_nsec - startTime.tv_nsec) / 1000000;
      timeSinceStart += (curTime.tv_sec - startTime.tv_sec) * 1000;
      cout << totalNumberFramesWritten << " frames written to snd_pcm_writei()" << endl;
      cout << delay << " returned by snd_pcm_delay()" << endl;
      snd_pcm_sframes_t playPos = totalNumberFramesWritten - delay;
      cout << playPos << " frames / " << playPos/48 << " ms is current play position" << endl;
      cout << timeSinceStart << " ms elapsed since snd_pcm_start" << endl;
      cout << "ALSA device is too slow " << playPos/48 - timeSinceStart << endl;
    }
    else if ((frames == -EAGAIN) || (frames == 0))
    {
      frames = 0;
    }
    else if (frames < 0)
    {
      frames = snd_pcm_recover(mPcm, static_cast<int32_t>(frames), 0);
      if (frames < 0) {
        cout << "snd_pcm_writei failed: " << snd_strerror(err) << endl;
        break;
      }
    }

    if (frames > 0 && frames < numberFramesRead)
    {
      cout << "Short write (expected " <<  sizeof(mSamples) << ", wrote " << frames << ")" << endl;
    }
  }
  while (numberFramesRead != 0);
  cout << "Total number of frames read from file: " << totalNumberFrames << endl;

  snd_pcm_state_t pcmState = snd_pcm_state(mPcm);
  cout << "ALSA device state at the end == " << snd_pcm_state_name(pcmState) << endl;
  ASSERT_EQ(SND_PCM_STATE_RUNNING, pcmState);

  err = snd_pcm_nonblock(mPcm, 0);
  ASSERT_EQ(0, err);
  err = snd_pcm_drain(mPcm);
  ASSERT_EQ(0, err);
  numberFramesAvail = snd_pcm_avail(mPcm);
  cout << "Number frames available in buffer to fill by writing after end of playback: " << numberFramesAvail << endl;
  numberFramesAvail = snd_pcm_avail_update(mPcm);
  cout << "snd_pcm_avail_update returned: " << numberFramesAvail << endl;

  delete[] mSamples;
  snd_pcm_close(mPcm);
  roucon = routing->disconnect(123, 123);
  ASSERT_EQ(IasIRouting::eIasOk, roucon);
  roucon = routing->disconnect(123, sink);
  ASSERT_EQ(IasIRouting::eIasOk, roucon);
  mSmartx->stop();
  recordThread.join();
  mSmartx->start();
}

/*****************************************
 * Testcase:    TC6
 * Description:
 * Play wave file in non-blocking mode until finished.
 * Record from capture device (provided by SmartXbar) using blocking mode.
 * Verifying whether snd_pcm_wait() for capture device works appropriately
 * (must block appropriately even after a pause of the recording thread).
 *
 * Period size: 1920 frames
 * Buffer size: 3 * period size frames
 * Sample rate: 48 kHz
 */
TEST_P(IasAlsaPluginUseCasesTest, TC6)
{
  auto testCase = GetParam();
  const int32_t sink = std::get<0>(testCase);
  const std::string sinkName = std::get<1>(testCase);
  const uint32_t cFrameLength = 1920;
  const uint32_t cNrChannels = 2;
  mSamples = new int16_t[cFrameLength*cNrChannels];

  int32_t err;
  IasIRouting::IasResult roucon;

  IasIRouting *routing = mSmartx->routing();
  ASSERT_TRUE(routing != nullptr);
  roucon = routing->connect(123, 123);
  ASSERT_EQ(IasIRouting::eIasOk, roucon);
  roucon = routing->connect(123, sink);
  ASSERT_EQ(IasIRouting::eIasOk, roucon);

  // Spawn a new thread for recording on the smartx plugin called smartx:stereo48
  std::thread recordThread(&IasAudio::IasAlsaPluginUseCasesTest_TC6_Test::recordBlockingMode, this, sinkName);

  std::string alsaDevice = "smartx:stereo48";
  sf_count_t numberFramesRead;
  sf_count_t totalNumberFrames = 0;
  err = snd_pcm_open(&mPcm, alsaDevice.c_str(), SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
  if (err < 0)
  {
    cout << "Error during snd_pcm_open: " << snd_strerror(err) << endl;
  }
  ASSERT_EQ(0, err);

  setHwParams(mPcm, cFrameLength, 3*cFrameLength, 48000, cNrChannels, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED);
  setSwParams(mPcm, cFrameLength, 16*cFrameLength);
  snd_pcm_dump(mPcm, mSndOut);

  struct timespec clockRes;
  err = clock_getres(CLOCK_MONOTONIC, &clockRes);
  ASSERT_EQ(0, err);
  cout << "Clock resolution: " << clockRes.tv_sec << " sec:" << clockRes.tv_nsec << " nsec" << endl;

  snd_pcm_sframes_t frames = 1;
  snd_pcm_sframes_t numberFramesAvail;
  struct timespec startTime;
  err = clock_gettime(CLOCK_MONOTONIC, &startTime);
  ASSERT_EQ(0, err);
  snd_pcm_sframes_t totalNumberFramesWritten = 0;
  sf_seek(mInputFile3, 0, SEEK_SET);
  // Prefill buffer
  do
  {
    numberFramesRead = sf_readf_short(mInputFile3, mSamples, cFrameLength);
    cout << "Number frames read from file: " << numberFramesRead << endl;
    totalNumberFrames += numberFramesRead;
    frames = snd_pcm_writei(mPcm, mSamples, numberFramesRead);
    if (frames > 0)
    {
      totalNumberFramesWritten += frames;
    }
  }
  while (frames > 0);
  if ((frames == -EAGAIN) || (frames == 0))
  {
    snd_pcm_state_t pcmState = snd_pcm_state(mPcm);
    if (pcmState != SND_PCM_STATE_RUNNING)
    {
      cout << "ALSA device state before start == " << snd_pcm_state_name(pcmState) << endl;
      cout << "Buffer completely filled, now start the pcm device" << endl;
      err = clock_gettime(CLOCK_MONOTONIC, &startTime);
      ASSERT_EQ(0, err);
      err = snd_pcm_start(mPcm);
      ASSERT_EQ(0, err);
      pcmState = snd_pcm_state(mPcm);
      cout << "ALSA device state after start == " << snd_pcm_state_name(pcmState) << endl;
    }
  }

  bool timeoutDuringWait = false;
  do
  {
    if (frames > 0)
    {
      numberFramesRead = sf_readf_short(mInputFile3, mSamples, cFrameLength);
      cout << "Number frames read from file: " << numberFramesRead << endl;
      totalNumberFrames += numberFramesRead;
      if (numberFramesRead > 0 && numberFramesRead < cFrameLength)
      {
        // The rest of the buffer not filled with samples from file will be filled with zeros
        uint32_t rest = static_cast<uint32_t>(cFrameLength - numberFramesRead);
        memset(&mSamples[numberFramesRead*cNrChannels], 0, sizeof(int16_t)*rest*cNrChannels);
        // Set the numberFramesRead variable to cFrameLength to fill a complete period size buffer
        numberFramesRead = cFrameLength;
      }
    }
    err = snd_pcm_wait(mPcm, 5000);
    if (err == 0)
    {
      timeoutDuringWait = 1;
      cout << "======================> TIMEOUT" << endl;
      // wait until the recording thread has been finished
      usleep(5*1000*1000);
      // Mark this test as non-successful
      EXPECT_TRUE(false);
    }
    struct timespec startWrite;
    err = clock_gettime(CLOCK_MONOTONIC, &startWrite);
    ASSERT_EQ(0, err);
    frames = snd_pcm_writei(mPcm, mSamples, numberFramesRead);
    struct timespec stopWrite;
    err = clock_gettime(CLOCK_MONOTONIC, &stopWrite);
    ASSERT_EQ(0, err);
    long writeDuration = (stopWrite.tv_nsec - startWrite.tv_nsec) / 1000000;
    writeDuration += (stopWrite.tv_sec - startWrite.tv_sec) * 1000;
    if (frames > 0 && writeDuration > 0)
    {
      cout << "snd_pcm_writei duration " << writeDuration << " ms" << endl;
    }

    if (frames > 0)
    {
      totalNumberFramesWritten += frames;
      snd_pcm_sframes_t delay;
      err = snd_pcm_delay(mPcm, &delay);
      ASSERT_EQ(0, err);
      struct timespec curTime;
      err = clock_gettime(CLOCK_MONOTONIC, &curTime);
      ASSERT_EQ(0, err);
      long timeSinceStart = (curTime.tv_nsec - startTime.tv_nsec) / 1000000;
      timeSinceStart += (curTime.tv_sec - startTime.tv_sec) * 1000;
      cout << totalNumberFramesWritten << " frames written to snd_pcm_writei()" << endl;
      cout << delay << " returned by snd_pcm_delay()" << endl;
      snd_pcm_sframes_t playPos = totalNumberFramesWritten - delay;
      cout << playPos << " frames / " << playPos/48 << " ms is current play position" << endl;
      cout << timeSinceStart << " ms elapsed since snd_pcm_start" << endl;
      cout << "ALSA device is too slow " << playPos/48 - timeSinceStart << endl;
    }
    else if ((frames == -EAGAIN) || (frames == 0))
    {
      frames = 0;
    }
    else if (frames < 0)
    {
      frames = snd_pcm_recover(mPcm, static_cast<int32_t>(frames), 0);
      if (frames < 0) {
        cout << "snd_pcm_writei failed: " << snd_strerror(err) << endl;
        break;
      }
    }

    if (frames > 0 && frames < numberFramesRead)
    {
      cout << "Short write (expected " <<  sizeof(mSamples) << ", wrote " << frames << ")" << endl;
    }
  }
  while ((numberFramesRead != 0) && !timeoutDuringWait);
  cout << "Total number of frames read from file: " << totalNumberFrames << endl;

  snd_pcm_state_t pcmState = snd_pcm_state(mPcm);
  cout << "ALSA device state at the end == " << snd_pcm_state_name(pcmState) << endl;
  ASSERT_EQ(SND_PCM_STATE_RUNNING, pcmState);

  err = snd_pcm_nonblock(mPcm, 0);
  ASSERT_EQ(0, err);
  err = snd_pcm_drain(mPcm);
  ASSERT_EQ(0, err);
  numberFramesAvail = snd_pcm_avail(mPcm);
  cout << "Number frames available in buffer to fill by writing after end of playback: " << numberFramesAvail << endl;
  numberFramesAvail = snd_pcm_avail_update(mPcm);
  cout << "snd_pcm_avail_update returned: " << numberFramesAvail << endl;

  delete[] mSamples;
  snd_pcm_close(mPcm);
  roucon = routing->disconnect(123, 123);
  ASSERT_EQ(IasIRouting::eIasOk, roucon);
  roucon = routing->disconnect(123, sink);
  ASSERT_EQ(IasIRouting::eIasOk, roucon);
  mSmartx->stop();
  recordThread.join();
  mSmartx->start();
}

/*****************************************
 * Testcase:    TC7
 * Description:
 * Replicate TC1 with non-interleaved access.
 * Play wave file in non-blocking mode until finished. Checking
 * delay value and add time measurement like
 * some media players do.
 *
 * Period size: 2048 frames
 * Buffer size: 3 * period size frames
 * Sample rate: 48 kHz
 */
TEST_P(IasAlsaPluginUseCasesTest, TC7)
{
  const uint32_t cFrameLength = 1920;
  const uint32_t cNrChannels = 2;
  mSamples = new int16_t[cFrameLength*cNrChannels];
  mSamplesInterleaved = new int16_t[cFrameLength*cNrChannels];
  void* buffers[2];
  buffers[0] = &mSamples[0];
  buffers[1] = &mSamples[cFrameLength];


  int32_t err;

  IasIRouting *routing = mSmartx->routing();
  ASSERT_TRUE(routing != nullptr);
  IasIRouting::IasResult roucon = routing->connect(123, 123);
  ASSERT_EQ(IasIRouting::eIasOk, roucon);


  std::string alsaDevice = "smartx:stereo48";
  sf_count_t numberFramesRead;
  sf_count_t totalNumberFrames = 0;
  err = snd_pcm_open(&mPcm, alsaDevice.c_str(), SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
  if (err < 0)
  {
    cout << "Error during snd_pcm_open: " << snd_strerror(err) << endl;
  }
  ASSERT_EQ(0, err);

  setHwParams(mPcm, cFrameLength, 3*cFrameLength, 48000, cNrChannels, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_NONINTERLEAVED);
  setSwParams(mPcm, cFrameLength, 16*cFrameLength);
  snd_pcm_dump(mPcm, mSndOut);

  struct timespec clockRes;
  err = clock_getres(CLOCK_MONOTONIC, &clockRes);
  ASSERT_EQ(0, err);
  cout << "Clock resolution: " << clockRes.tv_sec << " sec:" << clockRes.tv_nsec << " nsec" << endl;

  snd_pcm_sframes_t frames = 1;
  snd_pcm_sframes_t numberFramesAvail;
  struct timespec startTime;
  err = clock_gettime(CLOCK_MONOTONIC, &startTime);
  ASSERT_EQ(0, err);
  snd_pcm_sframes_t totalNumberFramesWritten = 0;
  do
  {
    if (frames > 0)
    {
      numberFramesRead = sf_readf_short(mInputFile2, mSamplesInterleaved, cFrameLength);
      cout << "Number frames read from file: " << numberFramesRead << endl;
      totalNumberFrames += numberFramesRead;
      if (numberFramesRead > 0 && numberFramesRead < cFrameLength)
      {
        // The rest of the buffer not filled with samples from file will be filled with zeros
        uint32_t rest = static_cast<uint32_t>(cFrameLength - numberFramesRead);
        memset(&mSamplesInterleaved[numberFramesRead*cNrChannels], 0, sizeof(int16_t)*rest*cNrChannels);
        // Set the numberFramesRead variable to cFrameLength to fill a complete period size buffer
        numberFramesRead = cFrameLength;
      }
      // Rearrange from interleaved to non-interleaved
      for (uint32_t channel=0; channel<cNrChannels; ++channel)
      {
        for (uint32_t index=0; index<cFrameLength; ++index)
        {
          mSamples[index + (channel*cFrameLength)] = mSamplesInterleaved[index*cNrChannels + channel];
        }
      }
    }
    struct timespec startWrite;
    err = clock_gettime(CLOCK_MONOTONIC, &startWrite);
    ASSERT_EQ(0, err);
    frames = snd_pcm_writen(mPcm, buffers, numberFramesRead);
    struct timespec stopWrite;
    err = clock_gettime(CLOCK_MONOTONIC, &stopWrite);
    ASSERT_EQ(0, err);
    long writeDuration = (stopWrite.tv_nsec - startWrite.tv_nsec) / 1000000;
    writeDuration += (stopWrite.tv_sec - startWrite.tv_sec) * 1000;
    if (frames > 0 && writeDuration > 0)
    {
      cout << "snd_pcm_writen duration " << writeDuration << " ms" << endl;
    }

    if (frames > 0)
    {
      totalNumberFramesWritten += frames;
      snd_pcm_sframes_t delay;
      err = snd_pcm_delay(mPcm, &delay);
      ASSERT_EQ(0, err);
      struct timespec curTime;
      err = clock_gettime(CLOCK_MONOTONIC, &curTime);
      ASSERT_EQ(0, err);
      long timeSinceStart = (curTime.tv_nsec - startTime.tv_nsec) / 1000000;
      timeSinceStart += (curTime.tv_sec - startTime.tv_sec) * 1000;
      cout << totalNumberFramesWritten << " frames written to snd_pcm_writen()" << endl;
      cout << delay << " returned by snd_pcm_delay()" << endl;
      snd_pcm_sframes_t playPos = totalNumberFramesWritten - delay;
      cout << playPos << " frames / " << playPos/48 << " ms is current play position" << endl;
      cout << timeSinceStart << " ms elapsed since snd_pcm_start" << endl;
      cout << "ALSA device is too slow " << playPos/48 - timeSinceStart << endl;
    }
    else if ((frames == -EAGAIN) || (frames == 0))
    {
      snd_pcm_state_t pcmState = snd_pcm_state(mPcm);
      if (pcmState != SND_PCM_STATE_RUNNING)
      {
        cout << "ALSA device state before start == " << snd_pcm_state_name(pcmState) << endl;
        cout << "Buffer completely filled, now start the pcm device" << endl;
        err = clock_gettime(CLOCK_MONOTONIC, &startTime);
        ASSERT_EQ(0, err);
        err = snd_pcm_start(mPcm);
        ASSERT_EQ(0, err);
        pcmState = snd_pcm_state(mPcm);
        cout << "ALSA device state after start == " << snd_pcm_state_name(pcmState) << endl;
      }
      frames = 0;
    }
    else if (frames < 0)
    {
      frames = snd_pcm_recover(mPcm, static_cast<int32_t>(frames), 0);
      if (frames < 0) {
        cout << "snd_pcm_writen failed: " << snd_strerror(err) << endl;
        break;
      }
    }

    if (frames > 0 && frames < numberFramesRead)
    {
      cout << "Short write (expected " <<  sizeof(mSamples) << ", wrote " << frames << ")" << endl;
    }
  }
  while (numberFramesRead != 0);
  cout << "Total number of frames read from file: " << totalNumberFrames << endl;

  snd_pcm_state_t pcmState = snd_pcm_state(mPcm);
  cout << "ALSA device state at the end == " << snd_pcm_state_name(pcmState) << endl;
  ASSERT_EQ(SND_PCM_STATE_RUNNING, pcmState);

  err = snd_pcm_nonblock(mPcm, 0);
  ASSERT_EQ(0, err);
  err = snd_pcm_drain(mPcm);
  ASSERT_EQ(0, err);
  numberFramesAvail = snd_pcm_avail(mPcm);
  cout << "Number frames available in buffer to fill by writing after end of playback: " << numberFramesAvail << endl;
  numberFramesAvail = snd_pcm_avail_update(mPcm);
  cout << "snd_pcm_avail_update returned: " << numberFramesAvail << endl;

  delete[] mSamples;
  delete[] mSamplesInterleaved;
  snd_pcm_close(mPcm);
  roucon = routing->disconnect(123, 123);
  ASSERT_EQ(IasIRouting::eIasOk, roucon);
}

/*****************************************
 * Testcase:    TC8
 * Description:
 * Play wave file in non-blocking mode until finished.
 * Record from capture device (provided by SmartXbar) using blocking mode.
 * Verifying whether snd_pcm_wait() for capture device works appropriately
 * (must block appropriately even after a pause of the recording thread).
 * It may also not react with a timeout, because it cannot differentiate between
 * buffer empty and buffer full in the alsa-lib.
 *
 * Period size: 1920 frames
 * Buffer size: 3 * period size frames
 * Sample rate: 48 kHz
 */
TEST_P(IasAlsaPluginUseCasesTest, TC8)
{
  auto testCase = GetParam();
  const int32_t sink = std::get<0>(testCase);
  const std::string sinkName = std::get<1>(testCase);
  const uint32_t cFrameLength = 1920;
  const uint32_t cNrChannels = 2;
  mSamples = new int16_t[cFrameLength*cNrChannels];

  int32_t err;
  IasIRouting::IasResult roucon;

  IasIRouting *routing = mSmartx->routing();
  ASSERT_TRUE(routing != nullptr);
  roucon = routing->connect(123, 123);
  ASSERT_EQ(IasIRouting::eIasOk, roucon);
  roucon = routing->connect(123, sink);
  ASSERT_EQ(IasIRouting::eIasOk, roucon);

  // Spawn a new thread for recording on the smartx plugin called smartx:capturestereo48(rt)
  std::thread recordThread(&IasAudio::IasAlsaPluginUseCasesTest_TC8_Test::record_TC8, this, sinkName);

  std::string alsaDevice = "smartx:stereo48";
  sf_count_t numberFramesRead;
  sf_count_t totalNumberFrames = 0;
  err = snd_pcm_open(&mPcm, alsaDevice.c_str(), SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
  if (err < 0)
  {
    cout << "Error during snd_pcm_open: " << snd_strerror(err) << endl;
  }
  ASSERT_EQ(0, err);

  setHwParams(mPcm, cFrameLength, 3*cFrameLength, 48000, cNrChannels, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED);
  setSwParams(mPcm, cFrameLength, 16*cFrameLength);
  snd_pcm_dump(mPcm, mSndOut);

  struct timespec clockRes;
  err = clock_getres(CLOCK_MONOTONIC, &clockRes);
  ASSERT_EQ(0, err);
  cout << "Clock resolution: " << clockRes.tv_sec << " sec:" << clockRes.tv_nsec << " nsec" << endl;

  snd_pcm_sframes_t frames = 1;
  snd_pcm_sframes_t numberFramesAvail;
  struct timespec startTime;
  err = clock_gettime(CLOCK_MONOTONIC, &startTime);
  ASSERT_EQ(0, err);
  snd_pcm_sframes_t totalNumberFramesWritten = 0;
  sf_seek(mInputFile3, 0, SEEK_SET);
  // Prefill buffer
  do
  {
    numberFramesRead = sf_readf_short(mInputFile3, mSamples, cFrameLength);
    cout << "Number frames read from file: " << numberFramesRead << endl;
    totalNumberFrames += numberFramesRead;
    frames = snd_pcm_writei(mPcm, mSamples, numberFramesRead);
    if (frames > 0)
    {
      totalNumberFramesWritten += frames;
    }
  }
  while (frames > 0);
  if ((frames == -EAGAIN) || (frames == 0))
  {
    snd_pcm_state_t pcmState = snd_pcm_state(mPcm);
    if (pcmState != SND_PCM_STATE_RUNNING)
    {
      cout << "ALSA device state before start == " << snd_pcm_state_name(pcmState) << endl;
      cout << "Buffer completely filled, now start the pcm device" << endl;
      err = clock_gettime(CLOCK_MONOTONIC, &startTime);
      ASSERT_EQ(0, err);
      err = snd_pcm_start(mPcm);
      ASSERT_EQ(0, err);
      pcmState = snd_pcm_state(mPcm);
      cout << "ALSA device state after start == " << snd_pcm_state_name(pcmState) << endl;
    }
  }

  bool timeoutDuringWait = false;
  do
  {
    if (frames > 0)
    {
      numberFramesRead = sf_readf_short(mInputFile3, mSamples, cFrameLength);
      cout << "Number frames read from file: " << numberFramesRead << endl;
      totalNumberFrames += numberFramesRead;
      if (numberFramesRead > 0 && numberFramesRead < cFrameLength)
      {
        // The rest of the buffer not filled with samples from file will be filled with zeros
        uint32_t rest = static_cast<uint32_t>(cFrameLength - numberFramesRead);
        memset(&mSamples[numberFramesRead*cNrChannels], 0, sizeof(int16_t)*rest*cNrChannels);
        // Set the numberFramesRead variable to cFrameLength to fill a complete period size buffer
        numberFramesRead = cFrameLength;
      }
    }
    err = snd_pcm_wait(mPcm, 5000);
    if (err == 0)
    {
      timeoutDuringWait = 1;
      cout << "======================> TIMEOUT" << endl;
      // wait until the recording thread has been finished
      usleep(5*1000*1000);
      // Mark this test as non-successful
      EXPECT_TRUE(false);
    }
    struct timespec startWrite;
    err = clock_gettime(CLOCK_MONOTONIC, &startWrite);
    ASSERT_EQ(0, err);
    frames = snd_pcm_writei(mPcm, mSamples, numberFramesRead);
    struct timespec stopWrite;
    err = clock_gettime(CLOCK_MONOTONIC, &stopWrite);
    ASSERT_EQ(0, err);
    long writeDuration = (stopWrite.tv_nsec - startWrite.tv_nsec) / 1000000;
    writeDuration += (stopWrite.tv_sec - startWrite.tv_sec) * 1000;
    if (frames > 0 && writeDuration > 0)
    {
      cout << "snd_pcm_writei duration " << writeDuration << " ms" << endl;
    }

    if (frames > 0)
    {
      totalNumberFramesWritten += frames;
      snd_pcm_sframes_t delay;
      err = snd_pcm_delay(mPcm, &delay);
      ASSERT_EQ(0, err);
      struct timespec curTime;
      err = clock_gettime(CLOCK_MONOTONIC, &curTime);
      ASSERT_EQ(0, err);
      long timeSinceStart = (curTime.tv_nsec - startTime.tv_nsec) / 1000000;
      timeSinceStart += (curTime.tv_sec - startTime.tv_sec) * 1000;
      cout << totalNumberFramesWritten << " frames written to snd_pcm_writei()" << endl;
      cout << delay << " returned by snd_pcm_delay()" << endl;
      snd_pcm_sframes_t playPos = totalNumberFramesWritten - delay;
      cout << playPos << " frames / " << playPos/48 << " ms is current play position" << endl;
      cout << timeSinceStart << " ms elapsed since snd_pcm_start" << endl;
      cout << "ALSA device is too slow " << playPos/48 - timeSinceStart << endl;
    }
    else if ((frames == -EAGAIN) || (frames == 0))
    {
      frames = 0;
    }
    else if (frames < 0)
    {
      frames = snd_pcm_recover(mPcm, static_cast<int32_t>(frames), 0);
      if (frames < 0) {
        cout << "snd_pcm_writei failed: " << snd_strerror(err) << endl;
        break;
      }
    }

    if (frames > 0 && frames < numberFramesRead)
    {
      cout << "Short write (expected " <<  sizeof(mSamples) << ", wrote " << frames << ")" << endl;
    }
  }
  while ((numberFramesRead != 0) && !timeoutDuringWait);
  mPlaybackFinished = true;
  cout << "Total number of frames read from file: " << totalNumberFrames << endl;

  snd_pcm_state_t pcmState = snd_pcm_state(mPcm);
  cout << "ALSA device state at the end == " << snd_pcm_state_name(pcmState) << endl;
  ASSERT_EQ(SND_PCM_STATE_RUNNING, pcmState);

  err = snd_pcm_nonblock(mPcm, 0);
  ASSERT_EQ(0, err);
  err = snd_pcm_drain(mPcm);
  ASSERT_EQ(0, err);
  numberFramesAvail = snd_pcm_avail(mPcm);
  cout << "Number frames available in buffer to fill by writing after end of playback: " << numberFramesAvail << endl;
  numberFramesAvail = snd_pcm_avail_update(mPcm);
  cout << "snd_pcm_avail_update returned: " << numberFramesAvail << endl;

  delete[] mSamples;
  snd_pcm_close(mPcm);
  roucon = routing->disconnect(123, 123);
  ASSERT_EQ(IasIRouting::eIasOk, roucon);
  roucon = routing->disconnect(123, sink);
  ASSERT_EQ(IasIRouting::eIasOk, roucon);
  mSmartx->stop();
  endThread = true;
  recordThread.join();
  mSmartx->start();
}

/*****************************************
 * Testcase:    TC9
 * Description:
 * Main intent of TC9 is to force a call to IasRoutingZoneWorkerThread::clearConversionBuffers()
 *
 * Play wave file in non-blocking mode until finished.
 * Record from capture device (provided by SmartXbar) using blocking mode.
 * Verifying whether snd_pcm_wait() for capture device works appropriately
 * (must block appropriately even after a pause of the recording thread).
 * It may also not react with a timeout, because it cannot differentiate between
 * buffer empty and buffer full in the alsa-lib.
 *
 * Period size: 1920 frames
 * Buffer size: 3 * period size frames
 * Sample rate: 48 kHz
 */
TEST_P(IasAlsaPluginUseCasesTest, TC9)
{
  auto testCase = GetParam();
  const int32_t sink = std::get<0>(testCase);
  const std::string sinkName = std::get<1>(testCase);
  const uint32_t cFrameLength = 1920;
  const uint32_t cNrChannels = 2;
  mSamples = new int16_t[cFrameLength*cNrChannels];

  int32_t err;
  IasIRouting::IasResult roucon;

  IasIRouting *routing = mSmartx->routing();
  ASSERT_TRUE(routing != nullptr);
  roucon = routing->connect(123, 123);
  ASSERT_EQ(IasIRouting::eIasOk, roucon);
  roucon = routing->connect(123, sink);
  ASSERT_EQ(IasIRouting::eIasOk, roucon);

  // Spawn a new thread for recording on the smartx plugin called smartx:capturestereo48(rt)
  std::thread recordThread(&IasAudio::IasAlsaPluginUseCasesTest_TC9_Test::recordBlockingMode_TC9, this, sinkName);

  std::string alsaDevice = "smartx:stereo48";
  sf_count_t numberFramesRead;
  sf_count_t totalNumberFrames = 0;
  err = snd_pcm_open(&mPcm, alsaDevice.c_str(), SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
  if (err < 0)
  {
    cout << "Error during snd_pcm_open: " << snd_strerror(err) << endl;
  }
  ASSERT_EQ(0, err);

  setHwParams(mPcm, cFrameLength, 3*cFrameLength, 48000, cNrChannels, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED);
  setSwParams(mPcm, cFrameLength, 16*cFrameLength);
  snd_pcm_dump(mPcm, mSndOut);

  struct timespec clockRes;
  err = clock_getres(CLOCK_MONOTONIC, &clockRes);
  ASSERT_EQ(0, err);
  cout << "Clock resolution: " << clockRes.tv_sec << " sec:" << clockRes.tv_nsec << " nsec" << endl;

  snd_pcm_sframes_t frames = 1;
  snd_pcm_sframes_t numberFramesAvail;
  struct timespec startTime;
  err = clock_gettime(CLOCK_MONOTONIC, &startTime);
  ASSERT_EQ(0, err);
  snd_pcm_sframes_t totalNumberFramesWritten = 0;
  sf_seek(mInputFile3, 0, SEEK_SET);
  // Prefill buffer
  do
  {
    numberFramesRead = sf_readf_short(mInputFile3, mSamples, cFrameLength);
    cout << "Number frames read from file: " << numberFramesRead << endl;
    totalNumberFrames += numberFramesRead;
    frames = snd_pcm_writei(mPcm, mSamples, numberFramesRead);
    if (frames > 0)
    {
      totalNumberFramesWritten += frames;
    }
  }
  while (frames > 0);
  if ((frames == -EAGAIN) || (frames == 0))
  {
    snd_pcm_state_t pcmState = snd_pcm_state(mPcm);
    if (pcmState != SND_PCM_STATE_RUNNING)
    {
      cout << "ALSA device state before start == " << snd_pcm_state_name(pcmState) << endl;
      cout << "Buffer completely filled, now start the pcm device" << endl;
      err = clock_gettime(CLOCK_MONOTONIC, &startTime);
      ASSERT_EQ(0, err);
      err = snd_pcm_start(mPcm);
      ASSERT_EQ(0, err);
      pcmState = snd_pcm_state(mPcm);
      cout << "ALSA device state after start == " << snd_pcm_state_name(pcmState) << endl;
    }
  }

  bool timeoutDuringWait = false;
  do
  {
    if (frames > 0)
    {
      numberFramesRead = sf_readf_short(mInputFile3, mSamples, cFrameLength);
      cout << "Number frames read from file: " << numberFramesRead << endl;
      totalNumberFrames += numberFramesRead;
      if (numberFramesRead > 0 && numberFramesRead < cFrameLength)
      {
        // The rest of the buffer not filled with samples from file will be filled with zeros
        uint32_t rest = static_cast<uint32_t>(cFrameLength - numberFramesRead);
        memset(&mSamples[numberFramesRead*cNrChannels], 0, sizeof(int16_t)*rest*cNrChannels);
        // Set the numberFramesRead variable to cFrameLength to fill a complete period size buffer
        numberFramesRead = cFrameLength;
      }
    }
    err = snd_pcm_wait(mPcm, 5000);
    if (err == 0)
    {
      timeoutDuringWait = 1;
      cout << "======================> TIMEOUT" << endl;
      // wait until the recording thread has been finished
      usleep(5*1000*1000);
      // Mark this test as non-successful
      EXPECT_TRUE(false);
    }
    struct timespec startWrite;
    err = clock_gettime(CLOCK_MONOTONIC, &startWrite);
    ASSERT_EQ(0, err);
    frames = snd_pcm_writei(mPcm, mSamples, numberFramesRead);
    struct timespec stopWrite;
    err = clock_gettime(CLOCK_MONOTONIC, &stopWrite);
    ASSERT_EQ(0, err);
    long writeDuration = (stopWrite.tv_nsec - startWrite.tv_nsec) / 1000000;
    writeDuration += (stopWrite.tv_sec - startWrite.tv_sec) * 1000;
    if (frames > 0 && writeDuration > 0)
    {
      cout << "snd_pcm_writei duration " << writeDuration << " ms" << endl;
    }

    if (frames > 0)
    {
      totalNumberFramesWritten += frames;
      snd_pcm_sframes_t delay;
      err = snd_pcm_delay(mPcm, &delay);
      ASSERT_EQ(0, err);
      struct timespec curTime;
      err = clock_gettime(CLOCK_MONOTONIC, &curTime);
      ASSERT_EQ(0, err);
      long timeSinceStart = (curTime.tv_nsec - startTime.tv_nsec) / 1000000;
      timeSinceStart += (curTime.tv_sec - startTime.tv_sec) * 1000;
      cout << totalNumberFramesWritten << " frames written to snd_pcm_writei()" << endl;
      cout << delay << " returned by snd_pcm_delay()" << endl;
      snd_pcm_sframes_t playPos = totalNumberFramesWritten - delay;
      cout << playPos << " frames / " << playPos/48 << " ms is current play position" << endl;
      cout << timeSinceStart << " ms elapsed since snd_pcm_start" << endl;
      cout << "ALSA device is too slow " << playPos/48 - timeSinceStart << endl;
    }
    else if ((frames == -EAGAIN) || (frames == 0))
    {
      frames = 0;
    }
    else if (frames < 0)
    {
      frames = snd_pcm_recover(mPcm, static_cast<int32_t>(frames), 0);
      if (frames < 0) {
        cout << "snd_pcm_writei failed: " << snd_strerror(err) << endl;
        break;
      }
    }

    if (frames > 0 && frames < numberFramesRead)
    {
      cout << "Short write (expected " <<  sizeof(mSamples) << ", wrote " << frames << ")" << endl;
    }
  }
  while ((numberFramesRead != 0) && !timeoutDuringWait);
  mPlaybackFinished = true;
  cout << "Total number of frames read from file: " << totalNumberFrames << endl;

  snd_pcm_state_t pcmState = snd_pcm_state(mPcm);
  cout << "ALSA device state at the end == " << snd_pcm_state_name(pcmState) << endl;
  ASSERT_EQ(SND_PCM_STATE_RUNNING, pcmState);

  err = snd_pcm_nonblock(mPcm, 0);
  ASSERT_EQ(0, err);
  err = snd_pcm_drain(mPcm);
  ASSERT_EQ(0, err);
  numberFramesAvail = snd_pcm_avail(mPcm);
  cout << "Number frames available in buffer to fill by writing after end of playback: " << numberFramesAvail << endl;
  numberFramesAvail = snd_pcm_avail_update(mPcm);
  cout << "snd_pcm_avail_update returned: " << numberFramesAvail << endl;

  delete[] mSamples;
  snd_pcm_close(mPcm);
  roucon = routing->disconnect(123, 123);
  ASSERT_EQ(IasIRouting::eIasOk, roucon);
  roucon = routing->disconnect(123, sink);
  ASSERT_EQ(IasIRouting::eIasOk, roucon);
  mSmartx->stop();
  endThread = true;
  recordThread.join();
  mSmartx->start();
}

/*****************************************
 * Testcase:    TC10
 * Description:
 * Play wave file in non-blocking mode until finished.
 * Record from capture device (provided by SmartXbar) using blocking mode.
 * In the first iteration we only open the capture device, start, stop
 * and close the device without reading data. Then we try to do the standard
 * capturing process and check whether we have issues with the events being sent
 * from the SmartXClient to the real-time thread.
 *
 * Period size: 1920 frames
 * Buffer size: 3 * period size frames
 * Sample rate: 48 kHz
 */
TEST_P(IasAlsaPluginUseCasesTest, TC10)
{
  auto testCase = GetParam();
  const int32_t sink = std::get<0>(testCase);
  const std::string sinkName = std::get<1>(testCase);
  const uint32_t cFrameLength = 1920;
  const uint32_t cNrChannels = 2;
  mSamples = new int16_t[cFrameLength*cNrChannels];

  int32_t err;
  IasIRouting::IasResult roucon;

  IasIRouting *routing = mSmartx->routing();
  ASSERT_TRUE(routing != nullptr);
  roucon = routing->connect(123, 123);
  ASSERT_EQ(IasIRouting::eIasOk, roucon);
  roucon = routing->connect(123, sink);
  ASSERT_EQ(IasIRouting::eIasOk, roucon);

  // Spawn a new thread for recording on the smartx plugin called smartx:capturestereo48(rt)
  std::thread recordThread(&IasAudio::IasAlsaPluginUseCasesTest_TC10_Test::recordBlockingModeNoData, this, sinkName);
  recordThread.join();
  recordThread = std::thread(&IasAudio::IasAlsaPluginUseCasesTest_TC10_Test::recordBlockingModeRetries, this, sinkName);

  string alsaDevice = "smartx:stereo48";
  sf_count_t numberFramesRead;
  sf_count_t totalNumberFrames = 0;
  err = snd_pcm_open(&mPcm, alsaDevice.c_str(), SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
  if (err < 0)
  {
    cout << "Error during snd_pcm_open: " << snd_strerror(err) << endl;
  }
  ASSERT_EQ(0, err);

  setHwParams(mPcm, cFrameLength, 3*cFrameLength, 48000, cNrChannels, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED);
  setSwParams(mPcm, cFrameLength, 16*cFrameLength);
  snd_pcm_dump(mPcm, mSndOut);

  struct timespec clockRes;
  err = clock_getres(CLOCK_MONOTONIC, &clockRes);
  ASSERT_EQ(0, err);
  cout << "Clock resolution: " << clockRes.tv_sec << " sec:" << clockRes.tv_nsec << " nsec" << endl;

  snd_pcm_sframes_t frames = 1;
  snd_pcm_sframes_t numberFramesAvail;
  struct timespec startTime;
  err = clock_gettime(CLOCK_MONOTONIC, &startTime);
  ASSERT_EQ(0, err);
  snd_pcm_sframes_t totalNumberFramesWritten = 0;
  sf_seek(mInputFile3, 0, SEEK_SET);
  // Prefill buffer
  do
  {
    numberFramesRead = sf_readf_short(mInputFile3, mSamples, cFrameLength);
    cout << "Number frames read from file: " << numberFramesRead << endl;
    totalNumberFrames += numberFramesRead;
    frames = snd_pcm_writei(mPcm, mSamples, numberFramesRead);
    if (frames > 0)
    {
      totalNumberFramesWritten += frames;
    }
  }
  while (frames > 0);
  if ((frames == -EAGAIN) || (frames == 0))
  {
    snd_pcm_state_t pcmState = snd_pcm_state(mPcm);
    if (pcmState != SND_PCM_STATE_RUNNING)
    {
      cout << "ALSA device state before start == " << snd_pcm_state_name(pcmState) << endl;
      cout << "Buffer completely filled, now start the pcm device" << endl;
      err = clock_gettime(CLOCK_MONOTONIC, &startTime);
      ASSERT_EQ(0, err);
      err = snd_pcm_start(mPcm);
      ASSERT_EQ(0, err);
      pcmState = snd_pcm_state(mPcm);
      cout << "ALSA device state after start == " << snd_pcm_state_name(pcmState) << endl;
    }
  }

  bool timeoutDuringWait = false;
  do
  {
    if (frames > 0)
    {
      numberFramesRead = sf_readf_short(mInputFile3, mSamples, cFrameLength);
      cout << "Number frames read from file: " << numberFramesRead << endl;
      totalNumberFrames += numberFramesRead;
      if (numberFramesRead > 0 && numberFramesRead < cFrameLength)
      {
        // The rest of the buffer not filled with samples from file will be filled with zeros
        uint32_t rest = static_cast<uint32_t>(cFrameLength - numberFramesRead);
        memset(&mSamples[numberFramesRead*cNrChannels], 0, sizeof(int16_t)*rest*cNrChannels);
        // Set the numberFramesRead variable to cFrameLength to fill a complete period size buffer
        numberFramesRead = cFrameLength;
      }
    }
    err = snd_pcm_wait(mPcm, 5000);
    if (err == 0)
    {
      timeoutDuringWait = 1;
      cout << "======================> TIMEOUT" << endl;
      // wait until the recording thread has been finished
      usleep(5*1000*1000);
      // Mark this test as non-successful
      EXPECT_TRUE(false);
    }
    struct timespec startWrite;
    err = clock_gettime(CLOCK_MONOTONIC, &startWrite);
    ASSERT_EQ(0, err);
    frames = snd_pcm_writei(mPcm, mSamples, numberFramesRead);
    struct timespec stopWrite;
    err = clock_gettime(CLOCK_MONOTONIC, &stopWrite);
    ASSERT_EQ(0, err);
    long writeDuration = (stopWrite.tv_nsec - startWrite.tv_nsec) / 1000000;
    writeDuration += (stopWrite.tv_sec - startWrite.tv_sec) * 1000;
    if (frames > 0 && writeDuration > 0)
    {
      cout << "snd_pcm_writei duration " << writeDuration << " ms" << endl;
    }

    if (frames > 0)
    {
      totalNumberFramesWritten += frames;
      snd_pcm_sframes_t delay;
      err = snd_pcm_delay(mPcm, &delay);
      ASSERT_EQ(0, err);
      struct timespec curTime;
      err = clock_gettime(CLOCK_MONOTONIC, &curTime);
      ASSERT_EQ(0, err);
      long timeSinceStart = (curTime.tv_nsec - startTime.tv_nsec) / 1000000;
      timeSinceStart += (curTime.tv_sec - startTime.tv_sec) * 1000;
      cout << totalNumberFramesWritten << " frames written to snd_pcm_writei()" << endl;
      cout << delay << " returned by snd_pcm_delay()" << endl;
      snd_pcm_sframes_t playPos = totalNumberFramesWritten - delay;
      cout << playPos << " frames / " << playPos/48 << " ms is current play position" << endl;
      cout << timeSinceStart << " ms elapsed since snd_pcm_start" << endl;
      cout << "ALSA device is too slow " << playPos/48 - timeSinceStart << endl;
    }
    else if ((frames == -EAGAIN) || (frames == 0))
    {
      frames = 0;
    }
    else if (frames < 0)
    {
      frames = snd_pcm_recover(mPcm, static_cast<int32_t>(frames), 0);
      if (frames < 0) {
        cout << "snd_pcm_writei failed: " << snd_strerror(err) << endl;
        break;
      }
    }

    if (frames > 0 && frames < numberFramesRead)
    {
      cout << "Short write (expected " <<  sizeof(mSamples) << ", wrote " << frames << ")" << endl;
    }
  }
  while ((numberFramesRead != 0) && !timeoutDuringWait);
  mPlaybackFinished = true;
  cout << "Total number of frames read from file: " << totalNumberFrames << endl;

  snd_pcm_state_t pcmState = snd_pcm_state(mPcm);
  cout << "ALSA device state at the end == " << snd_pcm_state_name(pcmState) << endl;
  ASSERT_EQ(SND_PCM_STATE_RUNNING, pcmState);

  err = snd_pcm_nonblock(mPcm, 0);
  ASSERT_EQ(0, err);
  err = snd_pcm_drain(mPcm);
  ASSERT_EQ(0, err);
  numberFramesAvail = snd_pcm_avail(mPcm);
  cout << "Number frames available in buffer to fill by writing after end of playback: " << numberFramesAvail << endl;
  numberFramesAvail = snd_pcm_avail_update(mPcm);
  cout << "snd_pcm_avail_update returned: " << numberFramesAvail << endl;

  delete[] mSamples;
  snd_pcm_close(mPcm);
  roucon = routing->disconnect(123, 123);
  ASSERT_EQ(IasIRouting::eIasOk, roucon);
  roucon = routing->disconnect(123, sink);
  ASSERT_EQ(IasIRouting::eIasOk, roucon);
  mSmartx->stop();
  endThread = true;
  recordThread.join();
  mSmartx->start();
}



} // namespace IasAudio


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
  setenv("DLT_INITIAL_LOG_LEVEL", "::6", true);
  DLT_REGISTER_APP("TST", "Test Application");
  //DLT_ENABLE_LOCAL_PRINT();

  ::testing::InitGoogleTest(&argc, argv);

  // By default, all WAVE files are read from NFS. The command-line option
  // "--no_nfs_paths" can be used to enforce that files are read locally.
  useNfsPath = true;

  // Parse the command line for the keyword "--no_nfs_paths".
  for (int cnt = 1; cnt < argc ; cnt++)
  {
    if (strcmp(argv[cnt], "--no_nfs_paths") == 0)
    {
      useNfsPath = false;
    }
    else if (strcmp(argv[cnt], "-D") == 0)
    {
      if (argc > cnt)
      {
        defaultDevice = argv[cnt+1];
      }
      else
      {
        cout << "Missing device name after -D" << endl;
      }
    }
  }

  std::cout << "Use device " << defaultDevice << " as master audio device"      << std::endl
            << "You can use the option -D ALSA_DEVICE_NAME to change this." << std::endl;

  if (useNfsPath)
  {
    std::cout << "====================================================================" << std::endl
              << "WAVE files are read from NFS directory."                              << std::endl
              << "Consider using option --no_nfs_paths to read from current directory." << std::endl
              << "====================================================================" << std::endl;
  }
  else
  {
    std::cout << "========================================================" << std::endl
              << "WAVE files are read from local working directory."        << std::endl
              << "Just don't use the option --no_nfs_paths to change this." << std::endl
              << "========================================================" << std::endl;
  }

  std::cout << "useNfsPath: " << useNfsPath << std::endl;

  IasAudio::setThreadSchedParams();

  return RUN_ALL_TESTS();
}
