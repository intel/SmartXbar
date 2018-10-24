/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/*
 * alsa_plugin_open_once.cpp
 */

#include <thread>
#include <future>
#include "gtest/gtest.h"
#include "internal/audio/common/IasAudioLogging.hpp"
#include "audio/smartx/IasSmartX.hpp"
#include "audio/smartx/IasSetupHelper.hpp"
#include "alsa/asoundlib.h"

namespace IasAudio {

class IasAlsaPluginOpenOnce : public ::testing::Test
{
  public:
    static void SetUpTestCase()
    {
      system("exec rm -r /run/smartx/*");
      system("exec rm -r /dev/shm/smartx*");
      mSmartX = IasSmartX::create();
      ASSERT_TRUE(mSmartX != nullptr);
      IasISetup *setup = mSmartX->setup();
      IasAudioSourceDevicePtr source = nullptr;
      IasAudioDeviceParams params;
      params.clockType = eIasClockProvided;
      params.dataFormat = eIasFormatInt16;
      params.name = "mySource";
      params.numChannels = 2;
      params.numPeriods = 4;
      params.periodSize = 192;
      params.samplerate = 48000;
      IasISetup::IasResult setres =  IasSetupHelper::createAudioSourceDevice(setup, params, 123, &source);
      ASSERT_EQ(IasISetup::eIasOk, setres);
    }

    static void TearDownTestCase()
    {
      IasSmartX::destroy(mSmartX);
    }

  protected:

    virtual void SetUp()
    {

    }

    virtual void TearDown()
    {

    }

    // Member variables
    static IasSmartX *mSmartX;
};

IasSmartX *IasAlsaPluginOpenOnce::mSmartX = nullptr;


TEST_F(IasAlsaPluginOpenOnce, open_close_same_thread)
{
  snd_pcm_t *hPcm = nullptr;

  for (int count=0; count<1000; count++)
  {
    int ret = snd_pcm_open(&hPcm, "smartx:mySource", SND_PCM_STREAM_PLAYBACK, 0);
    ASSERT_EQ(0, ret);
    ret = snd_pcm_open(&hPcm, "smartx:mySource", SND_PCM_STREAM_PLAYBACK, 0);
    ASSERT_EQ(-EBUSY, ret);
    ret = snd_pcm_close(hPcm);
    ASSERT_EQ(0, ret);
  }
}

int closeHandle(snd_pcm_t *hPcm)
{
  return snd_pcm_close(hPcm);
}

TEST_F(IasAlsaPluginOpenOnce, open_close_different_thread)
{
  snd_pcm_t *hPcm = nullptr;
  int ret = snd_pcm_open(&hPcm, "smartx:mySource", SND_PCM_STREAM_PLAYBACK, 0);
  ASSERT_EQ(0, ret);
  std::packaged_task<int(snd_pcm_t*)> taskCloseHandle(closeHandle);
  std::future<int> result = taskCloseHandle.get_future();
  std::thread threadTask(std::move(taskCloseHandle), hPcm);
  threadTask.join();
  ASSERT_EQ(0, result.get());
  ret = snd_pcm_open(&hPcm, "smartx:mySource", SND_PCM_STREAM_PLAYBACK, 0);
  ASSERT_EQ(0, ret);
}


} // namespace IasAudio


using namespace IasAudio;

int main(int argc, char* argv[])
{

  IasAudio::IasAudioLogging::registerDltApp(true);

  IasAudioLogging::addDltContextItem("SXP", DLT_LOG_VERBOSE, DLT_TRACE_STATUS_ON);

  ::testing::InitGoogleTest(&argc, argv);
  int result = RUN_ALL_TESTS();
  return result;
}
