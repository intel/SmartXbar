/*
  * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/*
 * main.cpp
 *
 *  Created on: Sep 5, 2012
 *
 * This test application processes 4 input streams (coming from 4 WAVE
 * files) in such a way that a cascade of 2 filters are applied.
 * The first two test streams contain FFT noise, so that the output
 * files can be analyzed by a MATLAB script in order to plot the
 * frequency response of the filter cascade.
 *
 * Stream 3 and stream 4 contain music. Stream 3 is filtered by a peak
 * peak filter and high-shelving filter, whose filter gains are ramped
 * up and down between 0 dB and +12 dB. This test represents the use
 * case of a bass/treble filter. Stream 4 is filtered alternatingly by
 * high-pass and by low-pass filters.
 */

#include <gtest/gtest.h>
#include <string.h>
#include <sndfile.h>
#include <iostream>
#include <malloc.h>
#include <math.h>

#include "internal/audio/smartx_test_support/IasTimeStampCounter.hpp"
#include "filter/IasAudioFilter.hpp"

/*
 *  Set PROFILE to 1 to enable perf measurement
 */
#define PROFILE 0

/*
 * Use either pseudo noise (0) or sweep signals (1)
 */
#define USE_SWEEP 1

using namespace IasAudio;
using namespace std;

IasAudioFilterParams     filterParams;
static const uint32_t cIasFrameLength = 256;
static const uint32_t cNumStreams     =   4;
uint32_t              cIasSampleFreq;
bool useNfsPath = true;

int32_t main(int32_t argc, char **argv)
{
  (void)argc;
  (void)argv;
  DLT_REGISTER_APP("TST", "Test Application");
  ::testing::InitGoogleTest(&argc, argv);
  int result = RUN_ALL_TESTS();
  DLT_UNREGISTER_APP();
  return result;
}

namespace IasAudio {

class IasFilterTest : public ::testing::Test
{
  protected:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

/*
 * My implementation of a callback class that can be used by the IasAudioFilter.
 */
class  MyCallbackImplementation : public IasAudioFilterCallback
{
  public:
    inline MyCallbackImplementation()
      {
      }
    inline ~MyCallbackImplementation()
      {
      }
    inline virtual void gainRampingFinished(uint32_t  channel, float gain, uint64_t callbackUserData)
      {
        std::cout.precision(2);
        std::cout << "Ramping has reached target gain: " << fixed << gain
                  << " filter: " << callbackUserData << " channel: " << channel <<std::endl;
      }
};


/*
 * Main function.
 */
TEST_F(IasFilterTest, main_test)
{

  std::string nfsPath("");
  if (useNfsPath)
  {
    std::cout << "========================================================" << std::endl
              << "WAVE files are read from NFS directory."                  << std::endl
              << "Consider using option --no_nfs_paths to change this."     << std::endl
              << "========================================================" << std::endl;
    nfsPath = "/nfs/ka/disks/ias_organisation_disk001/teams/audio/TestWavRefFiles/2014-06-27/";
  }
  else
  {
    std::cout << "========================================================" << std::endl
              << "WAVE files are read from local working directory."        << std::endl
              << "Just don't use the option --no_nfs_paths to change this." << std::endl
              << "========================================================" << std::endl;
  }

  std::string inputFileName[cNumStreams];
  std::string outputFileName[cNumStreams];
  std::string referenceFileName[cNumStreams];


#if USE_SWEEP
  inputFileName[0]     = nfsPath + "sine_sweep_20Hz_20000Hz_60s_stereo_24bit.wav";
  inputFileName[1]     = nfsPath + "sine_sweep_20Hz_20000Hz_60s_stereo_24bit.wav";
  outputFileName[1]    = "filter_out1_sweep.wav";
  outputFileName[0]    = "filter_out2_sweep.wav";
  referenceFileName[0] = nfsPath + "reference_filterTest_out1_sweep.wav";
  referenceFileName[1] = nfsPath + "reference_filterTest_out2_sweep.wav";
#else
  inputFileName[0]     = nfsPath + "fft-noise_fft-noise_zeros_48000Hz.wav";
  inputFileName[1]     = nfsPath + "fft-noise_fft-noise_zeros_48000Hz.wav";
  outputFileName[0]    = "filter_out1.wav";
  outputFileName[1]    = "filter_out2.wav";
  referenceFileName[0] = nfsPath + "reference_filterTest_out1.wav";
  referenceFileName[1] = nfsPath + "reference_filterTest_out2.wav";
#endif
  inputFileName[2]     = nfsPath + "Grummelbass_48000Hz_60s.wav";
  inputFileName[3]     = nfsPath + "Grummelbass_48000Hz_60s.wav";
  outputFileName[2]    = "filter_out3.wav";
  outputFileName[3]    = "filter_out4.wav";
  referenceFileName[2] = nfsPath + "reference_filterTest_out3.wav";
  referenceFileName[3] = nfsPath + "reference_filterTest_out4.wav";


  uint32_t   nReadSamples[cNumStreams] = { 0, 0, 0, 0 };
  uint32_t   nWrittenSamples = 0;
  float  difference[cNumStreams]   = { 0.0f, 0.0f, 0.0f, 0.0f };

  float *ioData[cNumStreams];        // pointers to buffers with interleaved samples
  float *referenceData[cNumStreams]; // pointers to buffers with interleaved samples

  SNDFILE *inputFile1     = NULL;
  SNDFILE *inputFile2     = NULL;
  SNDFILE *inputFile3     = NULL;
  SNDFILE *inputFile4     = NULL;
  SNDFILE *outputFile1    = NULL;
  SNDFILE *outputFile2    = NULL;
  SNDFILE *outputFile3    = NULL;
  SNDFILE *outputFile4    = NULL;
  SNDFILE *referenceFile1 = NULL;
  SNDFILE *referenceFile2 = NULL;
  SNDFILE *referenceFile3 = NULL;
  SNDFILE *referenceFile4 = NULL;

  SNDFILE *inputFile[cNumStreams]     = { inputFile1,  inputFile2,  inputFile3,  inputFile4 };
  SNDFILE *outputFile[cNumStreams]    = { outputFile1, outputFile2, outputFile3, outputFile4 };
  SNDFILE *referenceFile[cNumStreams] = { referenceFile1, referenceFile2, referenceFile3, referenceFile4 };

  SF_INFO  inputFileInfo[cNumStreams];
  SF_INFO  outputFileInfo[cNumStreams];
  SF_INFO  referenceFileInfo[cNumStreams];

  // Open the input files carrying PCM data.
  for (uint32_t cntFiles = 0; cntFiles < cNumStreams; cntFiles++)
  {
    inputFileInfo[cntFiles].format = 0;
    inputFile[cntFiles] = sf_open(inputFileName[cntFiles].c_str(), SFM_READ, &inputFileInfo[cntFiles]);
    if (NULL == inputFile[cntFiles])
    {
      std::cout<<"Could not open file " << inputFileName[cntFiles] <<std::endl;
      ASSERT_FALSE(true);
    }
    if (inputFileInfo[cntFiles].channels != 2)
    {
      std::cout<<"File " << inputFileName[cntFiles] << " does not provide stereo PCM samples " << std::endl;
    }
  }

  if ((inputFileInfo[0].samplerate != inputFileInfo[1].samplerate) ||
      (inputFileInfo[0].samplerate != inputFileInfo[2].samplerate) ||
      (inputFileInfo[0].samplerate != inputFileInfo[3].samplerate))
  {
    std::cout<<"Sample rates of input files are not identical!" <<std::endl;
    ASSERT_FALSE(true);
  }

  // Open the output files carrying PCM data.
  for (uint32_t cntFiles = 0; cntFiles < cNumStreams; cntFiles++)
  {
    outputFileInfo[cntFiles].samplerate = inputFileInfo[cntFiles].samplerate;
    outputFileInfo[cntFiles].channels   = inputFileInfo[cntFiles].channels;
    outputFileInfo[cntFiles].format     = (SF_FORMAT_WAV | SF_FORMAT_PCM_32);
    outputFile[cntFiles] = sf_open(outputFileName[cntFiles].c_str(), SFM_WRITE, &outputFileInfo[cntFiles]);
    if (NULL == outputFile[cntFiles])
    {
      std::cout<<"Could not open file " << outputFileName[cntFiles] <<std::endl;
      ASSERT_FALSE(true);
    }
  }

  // Open the reference files carrying PCM data.
  for (uint32_t cntFiles = 0; cntFiles < cNumStreams; cntFiles++)
  {
    referenceFileInfo[cntFiles].format = 0;
    referenceFile[cntFiles] = sf_open(referenceFileName[cntFiles].c_str(), SFM_READ, &referenceFileInfo[cntFiles]);
    if (NULL == referenceFile[cntFiles])
    {
      std::cout<<"Could not open file " << referenceFileName[cntFiles] <<std::endl;
      ASSERT_FALSE(true);
    }
    if (referenceFileInfo[cntFiles].channels != inputFileInfo[cntFiles].channels)
    {
      std::cout<<"File " << referenceFileName[cntFiles] << ": number of channels does not match!" << std::endl;
      ASSERT_FALSE(true);
    }
  }

  cIasSampleFreq = inputFileInfo[0].samplerate;
  IasAudioFilter *filter1 = new IasAudioFilter(cIasSampleFreq,cIasFrameLength); // 1st filter in 1st bundle
  IasAudioFilter *filter2 = new IasAudioFilter(cIasSampleFreq,cIasFrameLength); // 2nd filter in 1st bundle
  IasAudioFilter *filter3 = new IasAudioFilter(cIasSampleFreq,cIasFrameLength); // 1st filter in 2nd bundle
  IasAudioFilter *filter4 = new IasAudioFilter(cIasSampleFreq,cIasFrameLength); // 2nd filter in 2nd bundle

  for (uint32_t cntFiles = 0; cntFiles < cNumStreams; cntFiles++)
  {
    uint32_t nChannels = static_cast<uint32_t>(inputFileInfo[cntFiles].channels);
    ioData[cntFiles]        = (float* )malloc(sizeof(float) * cIasFrameLength * nChannels);
    referenceData[cntFiles] = (float* )malloc(sizeof(float) * cIasFrameLength * nChannels);
  }

  IasAudioLogging::registerDltContext("_PFW", "4 test");
  IasAudioChannelBundle myBundle1(cIasFrameLength);
  myBundle1.init();
  IasAudioChannelBundle myBundle2(cIasFrameLength);
  myBundle2.init();

  filter1->init();
  filter1->setBundlePointer(&myBundle1);
  filter2->init();
  filter2->setBundlePointer(&myBundle1);
  filter3->init();
  filter3->setBundlePointer(&myBundle2);
  filter4->init();
  filter4->setBundlePointer(&myBundle2);

  MyCallbackImplementation  *myCallbackImplementation = new MyCallbackImplementation();
  filter3->announceCallback(myCallbackImplementation);
  filter4->announceCallback(myCallbackImplementation);

  // Set filter parameters for channel 0: 4th order Butterworth high-pass filter
  filterParams.freq    = 100;
  filterParams.order   = 4;
  filterParams.section = 1;
  filterParams.type    = eIasFilterTypeHighpass;
  if (filter1->setChannelFilter(0, &filterParams) != 0)
  {
    std::cout<<"Error while configuring first filter in channel 0." <<std::endl;
    ASSERT_FALSE(true);
  }

  filterParams.section = 2;
  if (filter2->setChannelFilter(0, &filterParams) != 0)
  {
    std::cout<<"Error while configuring second filter in channel 0." <<std::endl;
    ASSERT_FALSE(true);
  }

  // Set filter parameters for channel 1: two 2nd order peak filters at 50 Hz and at 5000 Hz.
  filterParams.freq    = 50;
  filterParams.gain    = pow(10.0f, 6.0f/20.0f);  // +6.0 dB
  filterParams.quality = 2.0f;
  filterParams.order   = 2;
  filterParams.type    = eIasFilterTypePeak;
  if (filter1->setChannelFilter(1, &filterParams) != 0)
  {
    std::cout<<"Error while configuring first filter in channel 1." <<std::endl;
    ASSERT_FALSE(true);
  }

  filterParams.gain    = pow(10.0f, -12.0f/20.0f);  // -12.0 dB;
  filterParams.freq    = 5000;
  if (filter2->setChannelFilter(1, &filterParams) != 0)
  {
    std::cout<<"Error while configuring second filter in channel 1." <<std::endl;
    ASSERT_FALSE(true);
  }

  // Set filter parameters for channel 2: 2nd order lo shelving filter at 100 Hz.
  filterParams.freq    = 100;
  filterParams.gain    = pow(10.0f, 12.0f/20.0f);  // 12.0 dB
  filterParams.quality = 2.0f;
  filterParams.order   = 2;
  filterParams.type    = eIasFilterTypeLowShelving;
  if (filter1->setChannelFilter(2, &filterParams) != 0)
  {
    std::cout<<"Error while configuring first filter in channel 2." <<std::endl;
    ASSERT_FALSE(true);
  }

  // Set filter parameters for channel 3: 2nd order hi shelving filter at 1000 Hz.
  filterParams.gain    = pow(10.0f, 6.0f/20.0f);  // 6.0 dB;
  filterParams.freq    = 1000;
  filterParams.type    = eIasFilterTypeHighShelving;
  if (filter2->setChannelFilter(3, &filterParams) != 0)
  {
    std::cout<<"Error while configuring second filter in channel 3." <<std::endl;
    ASSERT_FALSE(true);
  }

  // Set filter parameters for channels 4-7: 2nd order peak filter at 50 Hz.
  filterParams.gain    = pow(10.0f, 12.0f/20.0f);  // 12.0 dB;
  filterParams.freq    = 50;
  filterParams.quality = 1.0f;
  filterParams.type    = eIasFilterTypePeak;
  for (uint32_t chan=0; chan < 4; chan++)
  {
    if (filter3->setChannelFilter(chan, &filterParams) != 0)
    {
      std::cout<<"Error while configuring first filter in channel " << chan+4 << std::endl;
      ASSERT_FALSE(true);
    }
  }

  // Set filter parameters for channels 4-7: 2nd order hi shelving filter at 6000 Hz.
  filterParams.gain    = pow(10.0f, 12.0f/20.0f);  // 12.0 dB;
  filterParams.freq    = 6000;
  filterParams.type    = eIasFilterTypeHighShelving;
  for (uint32_t chan=0; chan < 4; chan++)
  {
    if (filter4->setChannelFilter(chan, &filterParams) != 0)
    {
      std::cout<<"Error while configuring second filter in channel " << chan+4 << std::endl;
      ASSERT_FALSE(true);
    }
  }

  filterParams.section = 1;
  filterParams.order   = 1;

  uint32_t cntFrames = 0;
  uint32_t const cSectionLength = 862;  // approximately 5 seconds

#if PROFILE
  IasAudio::IasTimeStampCounter *timeStampCounter = new IasAudio::IasTimeStampCounter();
  uint64_t  cyclesSum   = 0;
#endif

  do
  {
    if ((cntFrames % (4*cSectionLength)) == 0)
    {
      // Set filters for stream 3 and 4 to flat.
      filter3->rampGain(0, 1.0000f, 3);
      filter3->rampGain(1, 1.0000f, 3);
      filter4->rampGain(0, 1.0000f, 4);
      filter4->rampGain(1, 1.0000f, 4);

      filterParams.type    = eIasFilterTypeFlat;
      filter3->setChannelFilter(2, &filterParams);
      filter3->setChannelFilter(3, &filterParams);
    }
    else if ((cntFrames % (4*cSectionLength)) == cSectionLength)
    {
      // Set bass filter for stream 3 to +12.00 dB.
      // Set high-pass filter for stream 4.
      filter3->rampGain(0, 3.9811f, 3);
      filter3->rampGain(1, 3.9811f, 3);
      filter4->rampGain(0, 1.0000f, 4);
      filter4->rampGain(1, 1.0000f, 4);

      filterParams.freq    = 1500;
      filterParams.type    = eIasFilterTypeHighpass;
      filter3->setChannelFilter(2, &filterParams);
      filter3->setChannelFilter(3, &filterParams);
    }
    else if ((cntFrames % (4*cSectionLength)) == 2*cSectionLength)
    {
      // Set filters for stream 3 and 4 to flat.
      filter3->rampGain(0, 1.0000f, 3);
      filter3->rampGain(1, 1.0000f, 3);
      filter4->rampGain(0, 1.0000f, 4);
      filter4->rampGain(1, 1.0000f, 4);

      filterParams.type    = eIasFilterTypeFlat;
      filter3->setChannelFilter(2, &filterParams);
      filter3->setChannelFilter(3, &filterParams);
    }
    else if ((cntFrames % (4*cSectionLength)) == 3*cSectionLength)
    {
      // Set treble filter for stream 3 to +12.00 dB.
      // Set low-pass filter for stream 4.
      filter3->rampGain(0, 1.0000f, 3);
      filter3->rampGain(1, 1.0000f, 3);
      filter4->rampGain(0, 3.9811f, 4);
      filter4->rampGain(1, 3.9811f, 4);

      filterParams.freq    = 500;
      filterParams.type    = eIasFilterTypeLowpass;
      filter3->setChannelFilter(2, &filterParams);
      filter3->setChannelFilter(3, &filterParams);
    }

    // Read in all channels from the WAVE files. The interleaved samples are
    // stored within the linear vector ioData.
    for (uint32_t cntFiles = 0; cntFiles < cNumStreams; cntFiles++)
    {
      memset((void*)ioData[cntFiles], 0, inputFileInfo[cntFiles].channels * cIasFrameLength * sizeof(ioData[0][0]));
      nReadSamples[cntFiles] = (uint32_t)sf_readf_float(inputFile[cntFiles], ioData[cntFiles], cIasFrameLength);
    }

    float channel0[cIasFrameLength];
    float channel1[cIasFrameLength];
    float channel2[cIasFrameLength];
    float channel3[cIasFrameLength];
    float channel4[cIasFrameLength];
    float channel5[cIasFrameLength];
    float channel6[cIasFrameLength];
    float channel7[cIasFrameLength];

    uint32_t cnt1 = 0;
    uint32_t cnt2 = 0;
    uint32_t cnt3 = 0;
    uint32_t cnt4 = 0;
    for(uint32_t i=0; i< cIasFrameLength;++i )
    {
      channel0[i] = ioData[0][cnt1++];
      channel1[i] = ioData[0][cnt1++];
      channel2[i] = ioData[1][cnt2++];
      channel3[i] = ioData[1][cnt2++];
      channel4[i] = ioData[2][cnt3++];
      channel5[i] = ioData[2][cnt3++];
      channel6[i] = ioData[3][cnt4++];
      channel7[i] = ioData[3][cnt4++];
    }
    myBundle1.writeFourChannelsFromNonInterleaved(channel0, channel1, channel2, channel3);
    myBundle2.writeFourChannelsFromNonInterleaved(channel4, channel5, channel6, channel7);

#if PROFILE
    timeStampCounter->reset();
#endif

    filter1->calculate();
    filter2->calculate();
    filter3->calculate();
    filter4->calculate();

#if PROFILE
    uint32_t timeStamp = timeStampCounter->get();
    cyclesSum += static_cast<uint64_t>(timeStamp);
#endif

    myBundle1.readFourChannels(channel0,channel1, channel2, channel3);
    myBundle2.readFourChannels(channel4,channel5, channel6, channel7);

    cnt1 = 0;
    cnt2 = 0;
    cnt3 = 0;
    cnt4 = 0;
    for(uint32_t i=0; i< cIasFrameLength;++i )
    {
      ioData[0][cnt1++] = 0.50f*channel0[i];
      ioData[0][cnt1++] = 0.50f*channel1[i];
      ioData[1][cnt2++] = 0.25f*channel2[i];
      ioData[1][cnt2++] = 0.25f*channel3[i];
      ioData[2][cnt3++] = 0.25f*channel4[i];
      ioData[2][cnt3++] = 0.25f*channel5[i];
      ioData[3][cnt4++] = 0.50f*channel6[i];
      ioData[3][cnt4++] = 0.50f*channel7[i];
    }

    for (uint32_t cntFiles = 0; cntFiles < cNumStreams; cntFiles++)
    {
      // Write the interleaved PCM samples into the output WAVE file.
      nWrittenSamples = (uint32_t) sf_writef_float(outputFile[cntFiles], ioData[cntFiles], (sf_count_t)nReadSamples[cntFiles]);
      if (nWrittenSamples != nReadSamples[cntFiles])
      {
        std::cout<< "write to file " << outputFileName[cntFiles] << "failed" << std::endl;
        ASSERT_FALSE(true);
      }

      // Read in all channels from the reference file. The interleaved samples are
      // stored within the linear vector refData.
      uint32_t nReferenceSamples = static_cast<uint32_t>(sf_readf_float(referenceFile[cntFiles], referenceData[cntFiles], (sf_count_t)nReadSamples[cntFiles]));
      if (nReferenceSamples < nReadSamples[cntFiles])
      {
        std::cout << "Error while reading reference PCM samples from file " << referenceFileName[cntFiles] << std::endl;
        ASSERT_FALSE(true);
      }

      // Compare processed output samples with reference stream.
      for (uint32_t cnt = 0; cnt < nReadSamples[cntFiles]; cnt++)
      {
        uint32_t nChannels = static_cast<uint32_t>(inputFileInfo[cntFiles].channels);
        for (uint32_t channel = 0; channel < nChannels; channel++)
        {
          difference[cntFiles] = std::max(difference[cntFiles], (float)(fabs(ioData[cntFiles][cnt*nChannels+channel] -
                                                                                    referenceData[cntFiles][cnt*nChannels+channel])));
        }
      }
    }

    cntFrames++;

  } while ( (nReadSamples[0] == cIasFrameLength) ||
            (nReadSamples[1] == cIasFrameLength) ||
            (nReadSamples[2] == cIasFrameLength) ||
            (nReadSamples[3] == cIasFrameLength));

  //end of main processing loop, start cleanup.

#if PROFILE
  printf("Average load: %f clocks/frame \n", ((double)cyclesSum/cntFrames));
#endif

  // Close all files.
  for (uint32_t cntFiles = 0; cntFiles < cNumStreams; cntFiles++)
  {
    if (sf_close(inputFile[cntFiles]))
    {
      std::cout<< "close of file " << inputFileName[cntFiles] <<" failed" <<std::endl;
      ASSERT_FALSE(true);
    }
    if (sf_close(outputFile[cntFiles]))
    {
      std::cout<< "close of file " << outputFileName[cntFiles] <<" failed" <<std::endl;
      ASSERT_FALSE(true);
    }
    if (sf_close(referenceFile[cntFiles]))
    {
      std::cout<< "close of file " << referenceFileName[cntFiles] <<" failed" <<std::endl;
      ASSERT_FALSE(true);
    }
  }

  for (uint32_t cntFiles = 0; cntFiles < cNumStreams; cntFiles++)
  {
    free(ioData[cntFiles]);
    free(referenceData[cntFiles]);
  }

  /*
   * ***********************************************************************
   * Before we finish the test, we do some verifications that invalid filter
   * parameters are rejected. This is needed for a good branch coverage.
   * ***********************************************************************
   */
  filterParams.freq    = 100;
  filterParams.gain    = 1.0f;
  filterParams.quality = 1.0f;
  filterParams.type    = eIasFilterTypePeak;
  filterParams.order   = 2;
  filterParams.section = 1;
  if (filter1->setChannelFilter(0, &filterParams) != 0)
  {
    std::cout<<"Error while setting valid filter parameters." <<std::endl;
    ASSERT_FALSE(true);
  }

  if (filter1->setChannelFilter(4, &filterParams) == 0) // invalid channel
  {
    std::cout<<"Error: invalid parameters are not rejected (invalid channel)." <<std::endl;
    ASSERT_FALSE(true);
  }
  if (filter1->setChannelFilter(0, NULL) == 0) // invalid parameter pointer
  {
    std::cout<<"Error: invalid parameters are not rejected (invalid parameter pointer)." <<std::endl;
    ASSERT_FALSE(true);
  }
  filterParams.gain    = 0.0f; // invalid parameter
  if (filter1->setChannelFilter(0, &filterParams) == 0)
  {
    std::cout<<"Error: invalid parameters are not rejected (test with gain = 0)." <<std::endl;
    ASSERT_FALSE(true);
  }
  filterParams.gain    = 1000000.0f; // invalid parameter
  if (filter1->setChannelFilter(0, &filterParams) == 0)
  {
    std::cout<<"Error: invalid parameters are not rejected (test with gain = 1000000)." <<std::endl;
    ASSERT_FALSE(true);
  }
  filterParams.gain    = 1.0f; // valid parameter
  filterParams.quality = 0.0f; // invalid parameter
  if (filter1->setChannelFilter(0, &filterParams) == 0)
  {
    std::cout<<"Error: invalid parameters are not rejected (test with quality = 0)." <<std::endl;
    ASSERT_FALSE(true);
  }
  filterParams.quality = 1000000.0f; // invalid parameter
  if (filter1->setChannelFilter(0, &filterParams) == 0)
  {
    std::cout<<"Error: invalid parameters are not rejected (test with quality = 1000000)." <<std::endl;
    ASSERT_FALSE(true);
  }
  filterParams.type    = eIasFilterTypeBandpass;
  filterParams.quality = 0.0f; // invalid parameter
  if (filter1->setChannelFilter(0, &filterParams) == 0)
  {
    std::cout<<"Error: invalid parameters are not rejected (test with quality = 0)." <<std::endl;
    ASSERT_FALSE(true);
  }
  filterParams.quality = 1000000.0f; // invalid parameter
  if (filter1->setChannelFilter(0, &filterParams) == 0)
  {
    std::cout<<"Error: invalid parameters are not rejected (test with quality = 1000000)." <<std::endl;
    ASSERT_FALSE(true);
  }
  filterParams.type    = eIasFilterTypeHighpass;
  filterParams.quality = 1.0f;
  filterParams.order   = 0; // invalid parameter
  if (filter1->setChannelFilter(0, &filterParams) == 0)
  {
    std::cout<<"Error: invalid parameters are not rejected (test with order = 0)." <<std::endl;
    ASSERT_FALSE(true);
  }
  filterParams.order   = 1000; // invalid parameter
  if (filter1->setChannelFilter(0, &filterParams) == 0)
  {
    std::cout<<"Error: invalid parameters are not rejected (test with order = 1000)." <<std::endl;
    ASSERT_FALSE(true);
  }
  filterParams.order   = 4;
  filterParams.section = 0; // invalid parameter
  if (filter1->setChannelFilter(0, &filterParams) == 0)
  {
    std::cout<<"Error: invalid parameters are not rejected (test with section = 0)." <<std::endl;
    ASSERT_FALSE(true);
  }
  filterParams.section = 3; // invalid parameter
  if (filter1->setChannelFilter(0, &filterParams) == 0)
  {
    std::cout<<"Error: invalid parameters are not rejected (test with section too high)." <<std::endl;
    ASSERT_FALSE(true);
  }
  filterParams.type    = eIasFilterTypeHighShelving;
  filterParams.section = 1;
  filterParams.gain    = 0.0f; // invalid parameter
  filterParams.order   = 1;
  if (filter1->setChannelFilter(0, &filterParams) == 0)
  {
    std::cout<<"Error: invalid parameters are not rejected (test with gain = 0)." <<std::endl;
    ASSERT_FALSE(true);
  }
  filterParams.gain    = 1000000.0f; // invalid parameter
  if (filter1->setChannelFilter(0, &filterParams) == 0)
  {
    std::cout<<"Error: invalid parameters are not rejected (test with gain = 1000000)." <<std::endl;
    ASSERT_FALSE(true);
  }
  filterParams.gain    = 1.0f;
  filterParams.order   = 0; // invalid parameter
  if (filter1->setChannelFilter(0, &filterParams) == 0)
  {
    std::cout<<"Error: invalid parameters are not rejected (test with order = 0)." <<std::endl;
    ASSERT_FALSE(true);
  }
  filterParams.order   = 3; // invalid parameter
  if (filter1->setChannelFilter(0, &filterParams) == 0)
  {
    std::cout<<"Error: invalid parameters are not rejected (test with order too high)." <<std::endl;
    ASSERT_FALSE(true);
  }
  filterParams.type    = eIasFilterTypePeak;
  filterParams.order   = 2;
  if (filter1->updateGain(4, 1.0f) == 0) // invalid channel
  {
    std::cout<<"Error: invalid parameters are not rejected (test with invalid channel)." <<std::endl;
    ASSERT_FALSE(true);
  }
  if (filter1->updateGain(0, 1000000.0f) == 0) // invalid gain
  {
    std::cout<<"Error: invalid parameters are not rejected (test with invalid gain)." <<std::endl;
    ASSERT_FALSE(true);
  }
  if (filter1->rampGain(4, 1.0f, 0) == 0) // invalid channel
  {
    std::cout<<"Error: invalid parameters are not rejected (test with invalid channel)." <<std::endl;
    ASSERT_FALSE(true);
  }
  if (filter1->rampGain(0, 1000000.0f, 0) == 0) // invalid gain
  {
    std::cout<<"Error: invalid parameters are not rejected (test with invalid gain)." <<std::endl;
    ASSERT_FALSE(true);
  }
  if (filter1->setRampGradient(4, 2.0f) == 0) // invalid channel
  {
    std::cout<<"Error: invalid parameters are not rejected (test with invalid channel)." <<std::endl;
    ASSERT_FALSE(true);
  }

  filterParams.freq  = 10;
  filterParams.order = 2;
  filterParams.type  = eIasFilterTypeLowpass;
  if (filter1->setChannelFilter(0, &filterParams) != 0)
  {
    std::cout<<"Error: unable to set parameters that need double precision filters." <<std::endl;
    ASSERT_FALSE(true);
  }
  filterParams.type  = eIasFilterTypeHighShelving;
  if (filter1->setChannelFilter(0, &filterParams) != 0)
  {
    std::cout<<"Error: unable to set parameters that need double precision filters." <<std::endl;
    ASSERT_FALSE(true);
  }
  filterParams.order = 1;
  if (filter1->setChannelFilter(0, &filterParams) != 0)
  {
    std::cout<<"Error: unable to set parameters that need double precision filters." <<std::endl;
    ASSERT_FALSE(true);
  }
  filterParams.type  = eIasFilterTypeLowShelving;
  if (filter1->setChannelFilter(0, &filterParams) != 0)
  {
    std::cout<<"Error: unable to set parameters that need double precision filters." <<std::endl;
    ASSERT_FALSE(true);
  }

  /*
   * ***********************************************************************
   * End of the branch-coverage tests.
   * ***********************************************************************
   */

  delete filter1;
  delete filter2;
  delete filter3;
  delete filter4;
  delete myCallbackImplementation;

  printf("All frames have been processed.\n");

  for (uint32_t cntFiles = 0; cntFiles < cNumStreams; cntFiles++)
  {
    float differenceDB = 20.0f*log10(std::max(difference[cntFiles], (float)(1e-20)));
    printf("Stream %d (%d channels): peak difference between processed output data and reference: %7.2f dBFS\n",
           cntFiles+1, inputFileInfo[cntFiles].channels, differenceDB);
  }

  for (uint32_t cntFiles = 0; cntFiles < cNumStreams; cntFiles++)
  {
    float differenceDB = 20.0f*log10(std::max(difference[cntFiles], (float)(1e-20)));
    float thresholdDB  = -48.0f;

    if (differenceDB > thresholdDB)
    {
      printf("Error: Peak difference exceeds threshold of %7.2f dBFS.\n", thresholdDB);
      ASSERT_FALSE(true);
    }
  }
  printf("Test has been passed.\n");
}
}
