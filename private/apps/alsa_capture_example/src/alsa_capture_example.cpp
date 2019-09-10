/*
 * Copyright (C) 2019 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file   alsa_capture_example.cpp
 * @date   2015
 * @brief  This file demonstrates how a capture application can use the alsa-smartx-plugin
 */

#include <iostream>
#include <alsa/asoundlib.h>
#include <sndfile.h>
#include <time.h>
#include <signal.h>

using namespace std;

static int32_t setHwParams(snd_pcm_t *pcm,
  snd_pcm_uframes_t periodSize,
  uint32_t rate,
  uint32_t nrChannels,
  snd_pcm_format_t format,
  snd_pcm_access_t access)
{
  snd_pcm_hw_params_t *hwParams;
  snd_pcm_hw_params_alloca(&hwParams);

  int err = snd_pcm_hw_params_any(pcm, hwParams);
  if (err < 0)
  {
    cout << "Error during snd_pcm_hw_params_any" << snd_strerror(err) << endl;
    return err;
  }

  err = snd_pcm_hw_params_set_rate(pcm, hwParams, rate, 0);
  if (err < 0)
  {
    cout << "Error during snd_pcm_hw_params_set_rate: " << snd_strerror(err) << endl;
    return err;
  }
  err = snd_pcm_hw_params_set_period_size(pcm, hwParams, periodSize, 0);
  if (err < 0)
  {
    cout << "Error during snd_pcm_hw_params_set_period_size: " << snd_strerror(err) << endl;
    return err;
  }
  cout << "Set period size to " << periodSize << endl;
  uint32_t periods = 4;
  err = snd_pcm_hw_params_set_periods(pcm, hwParams, periods, 0);
  if (err < 0)
  {
    cout << "Error during snd_pcm_hw_params_set_periods: " << snd_strerror(err) << endl;
    return err;
  }

  err = snd_pcm_hw_params_set_channels(pcm, hwParams, nrChannels);
  if (err < 0)
  {
    cout << "Error during snd_pcm_hw_params_set_channels: " << snd_strerror(err) << endl;
    return err;
  }
  err = snd_pcm_hw_params_set_format(pcm, hwParams, format);
  if (err < 0)
  {
    cout << "Error during snd_pcm_hw_params_set_format: " << snd_strerror(err) << endl;
    return err;
  }
  err = snd_pcm_hw_params_set_access(pcm, hwParams, access);
  if (err < 0)
  {
    cout << "Error during snd_pcm_hw_params_set_access: " << snd_strerror(err) << endl;
    return err;
  }

  err = snd_pcm_hw_params(pcm, hwParams);
  if (err < 0)
  {
    cout << "Error during snd_pcm_hw_params: " << snd_strerror(err) << endl;
    return err;
  }
  return err;
}

static int32_t setSwParams(
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
    return err;
  }
  err = snd_pcm_sw_params_set_avail_min(pcm, swParams, avail_min);
  if (err < 0)
  {
    cout << "Error during snd_pcm_sw_params_set_avail_min: " << snd_strerror(err) << endl;
    return err;
  }
  err = snd_pcm_sw_params_set_start_threshold(pcm, swParams, startThreshold);
  if (err < 0)
  {
    cout << "Error during snd_pcm_sw_params_set_start_threshold: " << snd_strerror(err) << endl;
    return err;
  }

  err = snd_pcm_sw_params(pcm, swParams);
  if (err < 0)
  {
    cout << "Error during snd_pcm_sw_params: " << snd_strerror(err) << endl;
    return err;
  }
  return err;
}

static int32_t xrun_recovery(snd_pcm_t *pcm, int32_t err)
{
  if (err == -EPIPE)
  {
    err = snd_pcm_prepare(pcm);
    if (err < 0)
    {
      cout << "Can't recover from underrun, prepare failed: " << snd_strerror(err) << endl;
    }
    return err;
  }
  else if (err == -ESTRPIPE)
  {
    while ((err = snd_pcm_resume(pcm)) == -EAGAIN)
    {
      sleep(1);
    }
    if (err < 0)
    {
      err = snd_pcm_prepare(pcm);
      if (err < 0)
      {
        cout << "Can't recover from suspend, prepare failed: " << snd_strerror(err) << endl;
      }
    }
    return err;
  }
  return err;
}

static bool endCapture = false;
static bool doSleep = false;

int32_t capture(SNDFILE *outputFile, snd_pcm_t *pcm)
{
  const uint32_t cFrameLength = 192;
  const uint32_t cNrChannels = 2;

  sf_count_t numberFramesWritten = 0;
  sf_count_t totalNumberFrames = 0;
  int32_t err;

  err = setHwParams(pcm, cFrameLength, 48000, cNrChannels, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_MMAP_INTERLEAVED);
  if (err < 0)
  {
    return err;
  }
  err = setSwParams(pcm, cFrameLength, 1);
  if (err < 0)
  {
    return err;
  }

  snd_output_t *sndOut;
  snd_output_stdio_attach(&sndOut, stdout, 0);
  snd_pcm_dump(pcm, sndOut);

  struct timespec clockRes;
  err = clock_getres(CLOCK_MONOTONIC, &clockRes);
  if (err < 0)
  {
    return err;
  }
  cout << "Clock resolution: " << clockRes.tv_sec << " sec:" << clockRes.tv_nsec << " nsec" << endl;

  snd_pcm_uframes_t frames;
  snd_pcm_sframes_t numberFramesAvail;
  struct timespec startTime;

  // The snd_pcm_start() function has to be called before data can be captured from the ALSA device.
  // This flag keeps in mind whether we have already called the the snd_pcm_start() function.
  bool alreadyStarted = false;

  err = clock_gettime(CLOCK_MONOTONIC, &startTime);
  if (err < 0)
  {
    return err;
  }
  int64_t totalNumberFramesRead = 0;
  snd_pcm_uframes_t offset;
  snd_pcm_sframes_t commitres;
  const snd_pcm_channel_area_t *areas;
  int16_t *samples = new int16_t[cFrameLength*cNrChannels];

  do
  {
    if (doSleep)
    {
      cout << "Wait now for 1 s" << endl;
      // Either do nothing,
      // or drop and then prepare, start
      // or pause and then resume
//      snd_pcm_drop(pcm);
//      snd_pcm_pause(pcm, 1);
      sleep(1);
      cout << "Returned from wait" << endl;
//      snd_pcm_prepare(pcm);
//      snd_pcm_start(pcm);
//      snd_pcm_pause(pcm, 0);
      doSleep = false;
    }
    numberFramesAvail = snd_pcm_avail_update(pcm);
    if (numberFramesAvail < 0)
    {
      err = xrun_recovery(pcm, static_cast<int32_t>(numberFramesAvail));
      if (err < 0)
      {
        cout << "snd_pcm_avail_update failed: " << snd_strerror(err) << endl;
        delete[] samples;
        return err;
      }
      continue;
    }
    if (static_cast<uint32_t>(numberFramesAvail) < cFrameLength)
    {
      // Start the ALSA sink device after the first period of PCM frames has been written.
      if (!alreadyStarted)
      {
        cout << "Start pcm device" << endl;
        err = snd_pcm_start(pcm);
        if (err < 0)
        {
          delete[] samples;
          return err;
        }
        alreadyStarted = true;
      }
      else
      {
        cout << "We have to wait because only " << numberFramesAvail << " frames are available" << endl;
        err = snd_pcm_wait(pcm, 4*cFrameLength);
        if (err < 0)
        {
          if ((err = xrun_recovery(pcm, err)) < 0)
          {
            cout << "snd_pcm_wait error: " << snd_strerror(err) << endl;
            delete[] samples;
            return err;
          }
        }
        else if (err == 0)
        {
          cout << "Timeout during snd_pcm_wait" << endl;
        }
        cout << "snd_pcm_wait returned: " << err << endl;
      }
      continue;
    }

    frames = static_cast<snd_pcm_uframes_t>(cFrameLength);
    err = snd_pcm_mmap_begin(pcm, &areas, &offset, &frames);
    if (err < 0)
    {
      if ((err = xrun_recovery(pcm, err)) < 0)
      {
        cout << "snd_pcm_mmap_begin error: " << snd_strerror(err) << endl;
        delete[] samples;
        return err;
      }
    }
    cout << "frames available to read=" << frames << endl;

    snd_pcm_channel_area_t sinkArea[2];
    sinkArea[0].addr = sinkArea[1].addr = samples;
    sinkArea[0].first = 0;
    sinkArea[1].first = static_cast<uint32_t>(sizeof(*samples)*8);
    sinkArea[0].step = sinkArea[1].step = static_cast<uint32_t>(sizeof(*samples)*cNrChannels*8);
    snd_pcm_areas_copy(sinkArea, 0, areas, offset, cNrChannels, cFrameLength, SND_PCM_FORMAT_S16_LE);

    commitres = snd_pcm_mmap_commit(pcm, offset, frames);
    if (commitres < 0 || (snd_pcm_uframes_t)commitres != frames)
    {
      if ((err = xrun_recovery(pcm, commitres >= 0 ? -EPIPE : static_cast<int32_t>(commitres))) < 0) {
        cout << "snd_pcm_mmap_commit error: " << snd_strerror(err) << endl;
        delete[] samples;
        return err;
      }
    }

    numberFramesWritten = sf_writef_short(outputFile, samples, frames);
    cout << "Number frames written to file: " << numberFramesWritten << endl;
    totalNumberFrames += numberFramesWritten;

    totalNumberFramesRead += frames;
    snd_pcm_sframes_t delay;
    err = snd_pcm_delay(pcm, &delay);
    if (err < 0)
    {
      delete[] samples;
      return err;
    }
    struct timespec curTime;
    err = clock_gettime(CLOCK_MONOTONIC, &curTime);
    if (err < 0)
    {
      delete[] samples;
      return err;
    }
    long timeSinceStart = (curTime.tv_nsec - startTime.tv_nsec) / 1000000;
    timeSinceStart += (curTime.tv_sec - startTime.tv_sec) * 1000;
    cout << totalNumberFramesRead << " frames read from ALSA device" << endl;
    cout << delay << " returned by snd_pcm_delay()" << endl;
    int64_t playPos = totalNumberFramesRead - delay;
    cout << playPos << " frames / " << playPos/48 << " ms is current play position" << endl;
    cout << timeSinceStart << " ms elapsed since snd_pcm_start" << endl;
    if (frames > 0 && frames < static_cast<snd_pcm_uframes_t>(numberFramesWritten))
    {
      cout << "Short write (expected " <<  sizeof(samples) << ", wrote " << frames << ")" << endl;
    }
  }
  while (numberFramesWritten != 0 && endCapture == false);
  cout << "Total number of frames written to file: " << totalNumberFrames << endl;

  delete[] samples;
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

static snd_pcm_t *pcm = nullptr;
static SNDFILE *outputFile = nullptr;

void signalHandler(int signum, siginfo_t *info, void *ptr)
{
  (void)info;
  (void)ptr;
  cout << endl << "Got signal " << strsignal(signum) << endl;
  if (signum == SIGUSR1)
  {
    cout << endl << "Shall sleep for 1 s" << endl;
    doSleep = true;
  }
  else if (signum == SIGINT)
  {
    cout << endl << "Finish capturing and do clean-up" << endl;
    endCapture = true;
  }
}

int main(int argc, char* argv[])
{
  SF_INFO outputFileInfo;
  int32_t err;
  string alsaDevice = "smartx:stereoOut48_m1";
  string outputFileName = "/tmp/alsa_capture_example.wav";

  if ((argc != 1) && (argc != 3) && (argc != 5))
  {
    cout << "Usage: alsa_capture_example -f YOUR_STEREO_WAV_FILE.wav -D ALSA_DEVICE" << endl;
    cout << "Default YOUR_STEREO_WAV_FILE.wav = " << outputFileName << endl;
    cout << "Default ALSA_DEVICE = " << alsaDevice << endl;
    exit(0);
  }
  for (int index=0; index<argc; ++index)
  {
    if (strncmp(argv[index], "-f", 2) == 0)
    {
      outputFileName = argv[index+1];
    }
    if (strncmp(argv[index], "-D", 2) == 0)
    {
      alsaDevice = argv[index+1];
    }
  }
  cout << "Using " << alsaDevice << " as ALSA capture device" << endl;
  cout << "Writing audio samples to " << outputFileName << endl;

  // Install a signal handler to catch a terminate signal to clean-up
  struct sigaction signalHandlerInfo;
  memset(&signalHandlerInfo, 0, sizeof(signalHandlerInfo));
  signalHandlerInfo.sa_flags = SA_SIGINFO;
  signalHandlerInfo.sa_sigaction = signalHandler;
  sigaction(SIGINT, &signalHandlerInfo, nullptr);
  sigaction(SIGUSR1, &signalHandlerInfo, nullptr);

  outputFileInfo.samplerate = 48000;
  outputFileInfo.channels = 2;
  outputFileInfo.format = SF_FORMAT_WAV|SF_FORMAT_PCM_16;
  outputFile = sf_open(outputFileName.c_str(), SFM_WRITE, &outputFileInfo);
  if (outputFile == nullptr)
  {
    cout << "Error during sf_open: " << sf_strerror(nullptr) << endl;
  }
  err = snd_pcm_open(&pcm, alsaDevice.c_str(), SND_PCM_STREAM_CAPTURE, 0);
  if (err < 0)
  {
    cout << "Error during snd_pcm_open: " << snd_strerror(err) << endl;
    return err;
  }

  if (outputFile && pcm)
  {
    capture(outputFile, pcm);
  }
  if (outputFile)
  {
    sf_close(outputFile);
  }
  if (pcm)
  {
    snd_pcm_close(pcm);
  }
  return 0;
}
