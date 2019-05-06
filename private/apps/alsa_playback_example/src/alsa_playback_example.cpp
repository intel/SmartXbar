/*
 * Copyright (C) 2019 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file   alsa_playback_example.cpp
 * @date   2015
 * @brief  This file demonstrates how a player application can use the alsa-smartx-plugin
 */

#include <iostream>
#include <alsa/asoundlib.h>
#include <sndfile.h>
#include <time.h>

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

int32_t play(SNDFILE *inputFile, snd_pcm_t *pcm)
{
  const uint32_t cFrameLength = 1920;
  const uint32_t cNrChannels = 2;

  sf_count_t numberFramesRead = 0;
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

  // The snd_pcm_start() function has to be called after the first period of PCM
  // frames have been written to the buffer of the ALSA sink device. This flag
  // keeps in mind whether we have already called the the snd_pcm_start() function.
  bool alreadyStarted = false;

  err = clock_gettime(CLOCK_MONOTONIC, &startTime);
  if (err < 0)
  {
    return err;
  }
  int64_t totalNumberFramesWritten = 0;
  snd_pcm_uframes_t offset;
  snd_pcm_sframes_t commitres;
  const snd_pcm_channel_area_t *areas;
  int16_t *samples = new int16_t[cFrameLength*cNrChannels];
  do
  {
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
    cout << "numberFramesAvail: " << numberFramesAvail << endl;
    if (static_cast<uint32_t>(numberFramesAvail) < cFrameLength)
    {
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
      continue;
    }

    numberFramesRead = sf_readf_short(inputFile, samples, cFrameLength);
    cout << "Number frames read from file: " << numberFramesRead << endl;
    totalNumberFrames += numberFramesRead;

    frames = static_cast<snd_pcm_uframes_t>(numberFramesRead);
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

    snd_pcm_channel_area_t srcArea[2];
    srcArea[0].addr = srcArea[1].addr = samples;
    srcArea[0].first = 0;
    srcArea[1].first = static_cast<uint32_t>(sizeof(*samples)*8);
    srcArea[0].step = srcArea[1].step = static_cast<uint32_t>(sizeof(*samples)*cNrChannels*8);
    snd_pcm_areas_copy(areas, offset, srcArea, 0, cNrChannels, cFrameLength, SND_PCM_FORMAT_S16_LE);

    commitres = snd_pcm_mmap_commit(pcm, offset, frames);
    if (commitres < 0 || (snd_pcm_uframes_t)commitres != frames)
    {
      if ((err = xrun_recovery(pcm, commitres >= 0 ? -EPIPE : static_cast<int32_t>(commitres))) < 0) {
        cout << "snd_pcm_mmap_commit error: " << snd_strerror(err) << endl;
        delete[] samples;
        return err;
      }
    }

    // Start the ALSA sink device after the first period of PCM frames has been written.
    if (!alreadyStarted)
    {
      err = snd_pcm_start(pcm);
      if (err < 0)
      {
        return err;
      }
      alreadyStarted = true;
    }

    totalNumberFramesWritten += frames;
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
    cout << totalNumberFramesWritten << " frames written to ALSA device" << endl;
    cout << delay << " returned by snd_pcm_delay()" << endl;
    int64_t playPos = totalNumberFramesWritten - delay;
    cout << playPos << " frames / " << playPos/48 << " ms is current play position" << endl;
    cout << timeSinceStart << " ms elapsed since snd_pcm_start" << endl;
    if (frames > 0 && frames < static_cast<snd_pcm_uframes_t>(numberFramesRead))
    {
      cout << "Short write (expected " <<  sizeof(samples) << ", wrote " << frames << ")" << endl;
    }
  }
  while (numberFramesRead != 0);
  cout << "Total number of frames read from file: " << totalNumberFrames << endl;

  err = snd_pcm_drain(pcm);
  if (err < 0)
  {
    delete[] samples;
    return err;
  }
  numberFramesAvail = snd_pcm_avail_update(pcm);
  cout << "Number frames available in buffer to fill by writing after end of playback: " << numberFramesAvail << endl;

  delete[] samples;
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
  SNDFILE *inputFile;
  SF_INFO inputFileInfo;
  snd_pcm_t *pcm;
  int32_t err;
  string alsaDevice = "smartx:stereo0";

  if (argc == 1 || argc > 2)
  {
    cout << "Usage: alsa_playback_example YOUR_STEREO_WAV_FILE.wav" << endl;
    exit(0);
  }
  string inputFileName = argv[1];
  inputFile = sf_open(inputFileName.c_str(), SFM_READ, &inputFileInfo);
  err = snd_pcm_open(&pcm, alsaDevice.c_str(), SND_PCM_STREAM_PLAYBACK, 0);
  if (err < 0)
  {
    cout << "Error during snd_pcm_open: " << snd_strerror(err) << endl;
    return err;
  }

  if (inputFile && pcm)
  {
    play(inputFile, pcm);
  }
  if (inputFile)
  {
    sf_close(inputFile);
  }
  if (pcm)
  {
    snd_pcm_close(pcm);
  }
  return 0;
}
