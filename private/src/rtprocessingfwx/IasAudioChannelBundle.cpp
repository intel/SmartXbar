/*
  * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file  IasAudioChannelBundle.cpp
 * @date  2012
 * @brief This is the implementation of the IasAudioChannelBundle class.
 */

#include <cstring>
#include <xmmintrin.h>
#include <emmintrin.h>
 // #define IAS_ASSERT()
#include "rtprocessingfwx/IasAudioChannelBundle.hpp"

/* Define SSE to enable optimization */
#define SSE 1

#ifdef __linux__
#define MS_VC  0
#else
#define MS_VC  1
#endif

#if !(MS_VC)
#include <malloc.h>
#endif


namespace IasAudio {

#if SSE
#if __INTEL_COMPILER
static const bool cIntelCompiler = true;
#else
static const bool cIntelCompiler = false;
#endif
#endif

IasAudioChannelBundle::IasAudioChannelBundle(uint32_t FrameLength)
  :mNumFreeChannels(cIasNumChannelsPerBundle)
  ,mFrameLength(FrameLength)
  ,mAudioData(nullptr)
  ,mAudioDataEnd(nullptr)
  ,mLogContext(IasAudioLogging::getDltContext("PFW"))
{

}

IasAudioChannelBundle::~IasAudioChannelBundle()
{
#if MS_VC
  _aligned_free(mAudioData);
#else
  free(mAudioData);
#endif
}

IasAudioProcessingResult IasAudioChannelBundle::init()
{
#if MS_VC
  mAudioData = (float*) _aligned_malloc(mFrameLength * sizeof(float) * mNumFreeChannels, 16);
#else

  #if INTEL_COMPILER
  if((mFrameLength % 8) != 0)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_FATAL, "IasAudioChannelBundle::init: The number of samples per frame must be a multiple of 8.");
    return eIasAudioProcInitializationFailed;
  }

  #else
  if((mFrameLength % 4) != 0)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_FATAL, "IasAudioChannelBundle::init: The number of samples per frame must be a multiple of 4.");
    return eIasAudioProcInitializationFailed;
  }

  #endif

  mAudioData = (float*) memalign(16 /*alignment 16Bytes = 4*Float32*/, mFrameLength * sizeof(float) * mNumFreeChannels /*channel per bundle*/);

#endif
  if (nullptr == mAudioData)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_FATAL, "IasAudioChannelBundle::init: No aligned memory received");
    return eIasAudioProcInitializationFailed;
  }
  std::memset(mAudioData, 0, mFrameLength * sizeof(float) * mNumFreeChannels);  // memalign() does not zero the memory!
  mAudioDataEnd = mAudioData + cIasNumChannelsPerBundle * mFrameLength;
  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, "IasAudioChannelBundle::init: Created audio channel bundle at", reinterpret_cast<uint64_t>(mAudioData),
                  "with length", mFrameLength * sizeof(float) * mNumFreeChannels);

  return eIasAudioProcOK;
}

void IasAudioChannelBundle::reset()
{
  mNumFreeChannels = cIasNumChannelsPerBundle;
}

void IasAudioChannelBundle::clear()
{
  std::memset(mAudioData, 0, mFrameLength * sizeof(float) * cIasNumChannelsPerBundle);
}

IasAudioProcessingResult IasAudioChannelBundle::reserveChannels(uint32_t numberChannels, uint32_t* startIndex)
{
  if (mNumFreeChannels >= numberChannels)
  {
    *startIndex = cIasNumChannelsPerBundle-mNumFreeChannels;
    mNumFreeChannels -= numberChannels;
    DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, "IasAudioChannelBundle::reserveChannels: Reserved", numberChannels,
                    "channels beginning from index", *startIndex,
                    "in audio channel bundle at", reinterpret_cast<uint64_t>(mAudioData));

    return eIasAudioProcOK;
  }
  else
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, "IasAudioChannelBundle::reserveChannels: No space to reserve", numberChannels,
                    "channels in audio channel bundle at", reinterpret_cast<uint64_t>(mAudioData));
    return eIasAudioProcNoSpaceLeft;
  }
}

void IasAudioChannelBundle::writeOneChannelFromNonInterleaved(uint32_t offset,
                                            const float *channel)
{
  for (float *audioData = mAudioData+offset; audioData < mAudioDataEnd; audioData += cIasNumChannelsPerBundle)
  {
    *audioData = *channel++;
  }
}

void IasAudioChannelBundle::writeTwoChannelsFromNonInterleaved(uint32_t offset,
                                             const float *channel0,
                                             const float *channel1)
{
#if SSE
  IAS_ASSERT((((uint64_t)mAudioData) & 0xf) == 0x0);
  IAS_ASSERT((offset == 0) || (offset == 2));

  // Check whether all input pointers are 16byte-aligned.
  // For the Intel compiler, this optimization does not seem to be appropriate.
  // Therefore, we use the generic implementation for the Intel compiler.
  if ((((((uint64_t)channel0) | ((uint64_t)channel1)) & 0x0f) == 0x0) &&
      (!cIntelCompiler))
  {
    // Efficient version with aligned load.
    __m128 matrixData[4];
    __m128 *bundleData = (__m128*)mAudioData;
    __m128 const *ch0  = (__m128*)channel0;
    __m128 const *ch1  = (__m128*)channel1;

    switch (offset)
    {
      case 0:
        for (uint32_t i = 0; i < mFrameLength; i+=4)
        {
          matrixData[0] = bundleData[i];
          matrixData[1] = bundleData[i+1];
          matrixData[2] = bundleData[i+2];
          matrixData[3] = bundleData[i+3];

          _MM_TRANSPOSE4_PS(matrixData[0], matrixData[1], matrixData[2], matrixData[3]);

          matrixData[0] = *ch0++;
          matrixData[1] = *ch1++;

          _MM_TRANSPOSE4_PS(matrixData[0], matrixData[1], matrixData[2], matrixData[3]);

          bundleData[i]   = matrixData[0];
          bundleData[i+1] = matrixData[1];
          bundleData[i+2] = matrixData[2];
          bundleData[i+3] = matrixData[3];
        }
        break;
      case 2:
        for (uint32_t i = 0; i < mFrameLength; i+=4)
        {
          matrixData[0] = bundleData[i];
          matrixData[1] = bundleData[i+1];
          matrixData[2] = bundleData[i+2];
          matrixData[3] = bundleData[i+3];

          _MM_TRANSPOSE4_PS(matrixData[0], matrixData[1], matrixData[2], matrixData[3]);

          matrixData[2] = *ch0++;
          matrixData[3] = *ch1++;

          _MM_TRANSPOSE4_PS(matrixData[0], matrixData[1], matrixData[2], matrixData[3]);

          bundleData[i]   = matrixData[0];
          bundleData[i+1] = matrixData[1];
          bundleData[i+2] = matrixData[2];
          bundleData[i+3] = matrixData[3];
        }
        break;
      default:
        break;
    }
  }
  else
  {
    // Generic implementation, which does not required aligned data.
    for(float *audioData = mAudioData+offset; audioData < mAudioDataEnd; audioData += cIasNumChannelsPerBundle)
    {
      audioData[0] = *channel0++;
      audioData[1] = *channel1++;
    }
  }
#else
  // This is the generic (non-optimized) code
  for(float *audioData = mAudioData+offset; audioData < mAudioDataEnd; audioData += cIasNumChannelsPerBundle)
  {
    audioData[0] = *channel0++;
    audioData[1] = *channel1++;
  }
#endif
}

void IasAudioChannelBundle::writeThreeChannelsFromNonInterleaved(uint32_t offset,
                                               const float *channel0,
                                               const float *channel1,
                                               const float *channel2)
{
  for (float *audioData = mAudioData+offset; audioData < mAudioDataEnd; audioData += cIasNumChannelsPerBundle)
  {
    audioData[0] = *channel0++;
    audioData[1] = *channel1++;
    audioData[2] = *channel2++;
  }
}

void IasAudioChannelBundle::writeFourChannelsFromNonInterleaved(const float *channel0,
                                              const float *channel1,
                                              const float *channel2,
                                              const float *channel3)
{
#if SSE
  IAS_ASSERT((((uint64_t)mAudioData) & 0xf) == 0x0);

  __m128 *bundleData =(__m128*)mAudioData;
  __m128 matrixData[4];

  // Check whether all input pointers are 16byte-aligned.
  if (((((uint64_t)channel0) |
        ((uint64_t)channel1) |
        ((uint64_t)channel2) |
        ((uint64_t)channel3)) & 0x0f) == 0x0)
  {
    // Efficient version with aligned load.
    __m128 const *ch0 =  (__m128*)channel0;
    __m128 const *ch1 =  (__m128*)channel1;
    __m128 const *ch2 =  (__m128*)channel2;
    __m128 const *ch3 =  (__m128*)channel3;

    for (uint32_t i = 0; i < mFrameLength; i+=4)
    {
      matrixData[0] = *ch0++;
      matrixData[1] = *ch1++;
      matrixData[2] = *ch2++;
      matrixData[3] = *ch3++;

      _MM_TRANSPOSE4_PS(matrixData[0], matrixData[1], matrixData[2], matrixData[3]);

      bundleData[i]   = matrixData[0];
      bundleData[i+1] = matrixData[1];
      bundleData[i+2] = matrixData[2];
      bundleData[i+3] = matrixData[3];
    }
  }
  else
  {
    // Less efficient version with unaligned load.
    for (uint32_t i = 0; i < mFrameLength; i+=4)
    {
      matrixData[0] = _mm_loadu_ps(channel0);
      matrixData[1] = _mm_loadu_ps(channel1);
      matrixData[2] = _mm_loadu_ps(channel2);
      matrixData[3] = _mm_loadu_ps(channel3);

      channel0 += 4;
      channel1 += 4;
      channel2 += 4;
      channel3 += 4;

      _MM_TRANSPOSE4_PS(matrixData[0], matrixData[1], matrixData[2], matrixData[3]);

      bundleData[i]   = matrixData[0];
      bundleData[i+1] = matrixData[1];
      bundleData[i+2] = matrixData[2];
      bundleData[i+3] = matrixData[3];
    }
  }

#else
  // This is the generic (non-optimized) code
  for (float *audioData = mAudioData; audioData < mAudioDataEnd; audioData += cIasNumChannelsPerBundle)
  {
    audioData[0] = *channel0++;
    audioData[1] = *channel1++;
    audioData[2] = *channel2++;
    audioData[3] = *channel3++;
  }
#endif
}

void IasAudioChannelBundle::writeTwoChannelsFromInterleaved(uint32_t offset,
                                                            uint32_t srcStride,
                                                            const float *channel)
{
  for(float *audioData = mAudioData+offset; audioData < mAudioDataEnd; audioData += cIasNumChannelsPerBundle)
  {
    audioData[0] = *channel;
    audioData[1] = *(channel+1);
    channel += srcStride;
  }
}

void IasAudioChannelBundle::writeThreeChannelsFromInterleaved(uint32_t offset,
                                                              uint32_t srcStride,
                                                              const float *channel)
{
  for (float *audioData = mAudioData+offset; audioData < mAudioDataEnd; audioData += cIasNumChannelsPerBundle)
  {
    audioData[0] = *channel;
    audioData[1] = *(channel+1);
    audioData[2] = *(channel+2);
    channel += srcStride;
  }
}

void IasAudioChannelBundle::writeFourChannelsFromInterleaved(uint32_t srcStride,
                                                             const float *channel)
{
  for (float *audioData = mAudioData; audioData < mAudioDataEnd; audioData += cIasNumChannelsPerBundle)
  {
    audioData[0] = *channel;
    audioData[1] = *(channel+1);
    audioData[2] = *(channel+2);
    audioData[3] = *(channel+3);
    channel += srcStride;
  }
}

void IasAudioChannelBundle::readOneChannel(uint32_t offset, float *channel) const
{
  IAS_ASSERT(channel != nullptr);
  for (float *audioData = mAudioData+offset; audioData < mAudioDataEnd; audioData += cIasNumChannelsPerBundle)
  {
    *channel++ = *audioData;
  }
}

void IasAudioChannelBundle::readTwoChannels(uint32_t offset,
                                            float *channel0,
                                            float *channel1) const
{
  IAS_ASSERT(channel0 != nullptr);
  IAS_ASSERT(channel1 != nullptr);

#if SSE
  IAS_ASSERT((((uint64_t)mAudioData) & 0xf) == 0x0);
  IAS_ASSERT((offset == 0) || (offset == 2));

  __m128 *bundleData =(__m128*)mAudioData;
  __m128 matrixData[4];

  // Check whether all output pointers are 16byte-aligned.
  if (((((uint64_t)channel0) |
        ((uint64_t)channel1)) & 0x0f) == 0x0)
  {
    // Efficient version with aligned load.
    __m128 *ch0 =  (__m128*)channel0;
    __m128 *ch1 =  (__m128*)channel1;

    switch (offset)
    {
      case 0:
        for (uint32_t i = 0; i < mFrameLength; i+=4)
        {
          matrixData[0] = bundleData[i];
          matrixData[1] = bundleData[i+1];
          matrixData[2] = bundleData[i+2];
          matrixData[3] = bundleData[i+3];

          _MM_TRANSPOSE4_PS(matrixData[0], matrixData[1], matrixData[2], matrixData[3]);

          *ch0++ = matrixData[0];
          *ch1++ = matrixData[1];
        }
        break;
      case 2:
        for (uint32_t i = 0; i < mFrameLength; i+=4)
        {
          matrixData[0] = bundleData[i];
          matrixData[1] = bundleData[i+1];
          matrixData[2] = bundleData[i+2];
          matrixData[3] = bundleData[i+3];

          _MM_TRANSPOSE4_PS(matrixData[0], matrixData[1], matrixData[2], matrixData[3]);

          *ch0++ = matrixData[2];
          *ch1++ = matrixData[3];
        }
        break;
      default:
        break;
    }
  }
  else
  {
    // Less efficient version with unaligned write.
    switch (offset)
    {
      case 0:
        for (uint32_t i = 0; i < mFrameLength; i+=4)
        {
          matrixData[0] = bundleData[i];
          matrixData[1] = bundleData[i+1];
          matrixData[2] = bundleData[i+2];
          matrixData[3] = bundleData[i+3];

          _MM_TRANSPOSE4_PS(matrixData[0], matrixData[1], matrixData[2], matrixData[3]);

          _mm_storeu_ps(channel0, matrixData[0]);
          _mm_storeu_ps(channel1, matrixData[1]);

          channel0 += 4;
          channel1 += 4;
        }
        break;
      case 2:
        for (uint32_t i = 0; i < mFrameLength; i+=4)
        {
          matrixData[0] = bundleData[i];
          matrixData[1] = bundleData[i+1];
          matrixData[2] = bundleData[i+2];
          matrixData[3] = bundleData[i+3];

          _MM_TRANSPOSE4_PS(matrixData[0], matrixData[1], matrixData[2], matrixData[3]);

          _mm_storeu_ps(channel0, matrixData[2]);
          _mm_storeu_ps(channel1, matrixData[3]);

          channel0 += 4;
          channel1 += 4;
        }
        break;
      default:
        break;
    }
  }
#else
  // This is the generic (non-optimized) code
  for (float *audioData = mAudioData+offset; audioData < mAudioDataEnd; audioData += cIasNumChannelsPerBundle)
  {
    *channel0++ = audioData[0];
    *channel1++ = audioData[1];
  }
#endif
}

void IasAudioChannelBundle::readThreeChannels(uint32_t offset,
                                              float *channel0,
                                              float *channel1,
                                              float *channel2) const
{
  IAS_ASSERT(channel0 != nullptr);
  IAS_ASSERT(channel1 != nullptr);
  IAS_ASSERT(channel2 != nullptr);

  for (float *audioData = mAudioData+offset; audioData < mAudioDataEnd; audioData += cIasNumChannelsPerBundle)
  {
    *channel0++ = audioData[0];
    *channel1++ = audioData[1];
    *channel2++ = audioData[2];
  }
}

void IasAudioChannelBundle::readFourChannels(float *channel0,
                                             float *channel1,
                                             float *channel2,
                                             float *channel3) const
{
  IAS_ASSERT(channel0 != nullptr);
  IAS_ASSERT(channel1 != nullptr);
  IAS_ASSERT(channel2 != nullptr);
  IAS_ASSERT(channel3 != nullptr);

#if SSE
  IAS_ASSERT((((uint64_t)mAudioData) & 0xf) == 0x0);

  __m128 *bundleData =(__m128*)mAudioData;
  __m128 matrixData[4];

  // Check whether all output pointers are 16byte-aligned.
  if (((((uint64_t)channel0) |
        ((uint64_t)channel1) |
        ((uint64_t)channel2) |
        ((uint64_t)channel3)) & 0x0f) == 0x0)
  {
    // Efficient version with aligned load.
    __m128 *ch0 =  (__m128*)channel0;
    __m128 *ch1 =  (__m128*)channel1;
    __m128 *ch2 =  (__m128*)channel2;
    __m128 *ch3 =  (__m128*)channel3;

    for (uint32_t i = 0; i < mFrameLength; i+=4)
    {
      matrixData[0] = bundleData[i];
      matrixData[1] = bundleData[i+1];
      matrixData[2] = bundleData[i+2];
      matrixData[3] = bundleData[i+3];

      _MM_TRANSPOSE4_PS(matrixData[0], matrixData[1], matrixData[2], matrixData[3]);

      *ch0++ = matrixData[0];
      *ch1++ = matrixData[1];
      *ch2++ = matrixData[2];
      *ch3++ = matrixData[3];
    }
  }
  else
  {
    // Less efficient version with unaligned write.
    for (uint32_t i = 0; i < mFrameLength; i+=4)
    {
      matrixData[0] = bundleData[i];
      matrixData[1] = bundleData[i+1];
      matrixData[2] = bundleData[i+2];
      matrixData[3] = bundleData[i+3];

      _MM_TRANSPOSE4_PS(matrixData[0], matrixData[1], matrixData[2], matrixData[3]);

      _mm_storeu_ps(channel0, matrixData[0]);
      _mm_storeu_ps(channel1, matrixData[1]);
      _mm_storeu_ps(channel2, matrixData[2]);
      _mm_storeu_ps(channel3, matrixData[3]);

      channel0 += 4;
      channel1 += 4;
      channel2 += 4;
      channel3 += 4;
    }
  }
#else
  // This is the generic (non-optimized) code
  for (float *audioData = mAudioData; audioData < mAudioDataEnd; audioData += cIasNumChannelsPerBundle)
  {
    *channel0++ = audioData[0];
    *channel1++ = audioData[1];
    *channel2++ = audioData[2];
    *channel3++ = audioData[3];
  }
#endif
}

} // namespace IasAudio
