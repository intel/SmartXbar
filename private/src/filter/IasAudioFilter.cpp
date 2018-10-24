/*
  * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file    IasAudioFilter.cpp
 * @brief   Audio Filter Component.
 * @date    September 05, 2012
 */

#include "filter/IasAudioFilter.hpp"
 // #define IAS_ASSERT()
#include <math.h>
#include <iostream>
#include <xmmintrin.h>
#include <emmintrin.h>
#include <malloc.h>
#include <string.h>

#ifdef __INTEL_COMPILER
#define INTEL_COMPILER 1
#else
#define INTEL_COMPILER 0
#endif

/*
 *  Switches for SSE optimization
 */
#define USE_SSE          1

/*
 *  Switches for overriding the automatic decision what precision shall be used.
 */
#define ENFORCE_FLOAT32  0
#define ENFORCE_FLOAT64  0


namespace IasAudio {

/*
 * These are the parameter ranges, which are used to decide if double precision is needed
 */
const uint32_t cIasPeakFilterFreqBorder  = 300;  //Peak filter with f < 300 Hz need double precision
const float cIasPeakFilterQualBorder = 1.0f; //Peak filter with Q > 1.0 need double precision
const uint32_t cIasShelvFilterFreqBorder = 200;  //Shelving filter with f <= 200 Hz need double precision
const uint32_t cIasHPFilterFreqBorder    = 200;  //Highpass filter with f < 200 Hz need double precision
const uint32_t cIasLPFilterFreqBorder    = 200;  //Lowpass filter with f < 200 Hz need double precision


/*
 * Define minimum and maximum values for the filter parameters.
 */
const IasAudioFilterParams cIasAudioFilterParamsMinValues = {
  10,                          // freq
  0.001f,                      // gain (linear, not in dB)
  0.01f,                       // quality
  eIasFilterTypeFlat,          // type
  1,                           // order
  1                            // section
};

const IasAudioFilterParams cIasAudioFilterParamsMaxValues = {
  9999999,                     // freq, invalid, use the Nyquist rate instead!
  1000.0f,                     // gain (linear, not in dB)
  100.0f,                      // quality
  eIasFilterTypeHighShelving,  // type
  20,                          // order
  10                           // section, must not be greater than order/2
};


/*
 *  @brief Public helper function for verifying that a given
 *  filter parameter set is valid.
 */
int32_t  IasAudioFilterCheckParams(IasAudioFilterParams const &filterParams,
                                      uint32_t                 sampleFreq)
{
  if (filterParams.type == eIasFilterTypeFlat)
  {
    // Flat filter type does not need any further parameters, which need to be checked.
    return 0;
  }

  if ((filterParams.freq < cIasAudioFilterParamsMinValues.freq) ||
      (filterParams.freq > (sampleFreq >> 1)))
  {
    return -1;
  }

  switch (filterParams.type)
  {
    case eIasFilterTypePeak:
      if ((filterParams.gain < cIasAudioFilterParamsMinValues.gain) ||
          (filterParams.gain > cIasAudioFilterParamsMaxValues.gain) ||
          (filterParams.quality < cIasAudioFilterParamsMinValues.quality) ||
          (filterParams.quality > cIasAudioFilterParamsMaxValues.quality) ||
          (filterParams.order != 2))
      {
        return -1;
      }
      break;
    case eIasFilterTypeBandpass:
      if ((filterParams.quality < cIasAudioFilterParamsMinValues.quality) ||
          (filterParams.quality > cIasAudioFilterParamsMaxValues.quality) ||
          (filterParams.order != 2))
      {
        return -1;
      }
      break;
    case eIasFilterTypeLowpass:
    case eIasFilterTypeHighpass:
      if ((filterParams.order < cIasAudioFilterParamsMinValues.order) ||
          (filterParams.order > cIasAudioFilterParamsMaxValues.order))
      {
        return -1;
      }
      // for higher-order filters, we have to verify that section is valid.
      if (( filterParams.order > 2) &&
          ((filterParams.section < 1) ||
           (filterParams.section > ((filterParams.order+1) >> 1))))
      {
        return -1;
      }
      break;
    case eIasFilterTypeLowShelving:
    case eIasFilterTypeHighShelving:
      if ((filterParams.gain < cIasAudioFilterParamsMinValues.gain) ||
          (filterParams.gain > cIasAudioFilterParamsMaxValues.gain) ||
          (filterParams.order < 1) ||
          (filterParams.order > 2))
      {
        return -1;
      }
      break;
    default:
      return -1;
      break;
  }

  return 0;
}


/*!
 *  @brief Internal helper function for calculating the pre-warped normalized
 *         frequency, according to  K = tan(pi*fc/fs)
 */
static float calculatePreWarpedFrequency(float fc,
                                                float fs)
{
  const float cPi = static_cast<float>(M_PI);
  return tanf(cPi * fc / fs);
}


/*!
 *  @brief Internal helper function for calculating the parameter alpha,
 *  which is required for the Butterworth filter design.
 *
 *  The parameter alpha depends on the filter order and of the filter
 *  section. See also
 *  http://de.wikipedia.org/wiki/Butterworth-Filter#Koeffizienten
 *
 *  @return               Error value, -1 in case of invalid parameters.
 *  @param[out]  alpha    Result of this function.
 *  @param[in]   order    Filter order, must be >= 2.
 *  @param[in]   section  Filter section, must be >= 1 or >=2, depending
 *                        on whether the filter order is even or odd.
 */
static int32_t calculateAlphaButterworth(float *alpha,
                                            uint32_t   order,
                                            uint32_t   section)
{
  const float cPi = static_cast<float>(M_PI);

  IAS_ASSERT(order > 1);
  if (order == 2)
  {
    // 2nd order filter
    *alpha = 2.0f * cosf(cPi * ((float)(2-1))/(static_cast<float>(2*2)));
  }
  else if (order & 1)
  {
    // filter order is odd
    if ((section < 2) || (2*section > (order+1)))
    {
      return -1;
    }
    *alpha = 2.0f * cosf(cPi * ((float)(section-1))/(static_cast<float>(order)));
  }
  else
  {
    // filter order is even
    if ((section < 1) || (2*section > order))
    {
      return -1;
    }
    *alpha = 2.0f * cosf(cPi * ((float)(2*section-1))/(static_cast<float>(2*order)));
  }
  return 0;
}


/*!
 * @brief Internal helper function for calculating the coefficients of a
 *        Biquad filter (1st or 2nd order IIR filter).
 *
 * The function calculates the coefficients b0, b1, b2, a1, and a2,
 * as a function of the filter parameters. The coefficients are written
 * into the buffer that is indicated by @a coeff. In order to support the
 * idea of interleaved audio data, the coefficients are not stored
 * contiguously. Instead, the parameter @a offset declares the distance
 * between two coefficients. Use @a coeff=1, if the coefficients shall
 * be stored coherently, or @a coeff=cIasNumChannelsPerBundle, if the
 * architecture is based on bundles.
 *
 * @return               Error value, -1 in case of invalid parameters, 0 on success.
 * @param[out]  coeffs   Pointer to the buffer, where the updated coefficients
 *                       shall be written to. The buffer must be able to
 *                       carry (cIasNumCoeffsBiquad-1)*offset+1 coefficients.
 * @param[in]   offset   Space between two coefficients.
 * @param[in]   K        Pre-warped frequency, K = tan(pi*fc/fs).
 * @param[in]   V        Gain of the (peak or shelving) filter,
 *                       in linear representation (i.e., not in dB).
 * @param[in]   Q        Filter quality (in case of a peak filter).
 * @param[in]   type     Filter type (peak, low-pass, high-pass, ...).
 * @param[in]   order    Filter order, must be >= 1.
 *                       For higher-order filters, this function implements
 *                       only a pair of two poles and a pair of two zeros,
 *                       depending on the parameter @a section.
 * @param[in]   section  Filter section, must be >= 1 or >=2, depending
 *                       on whether the filter order is even or odd. This
 *                       parameter is ignored, if the filter order is <= 2.
 */

static int32_t calculateBiquadCoeff(float        *coeffs,
                                       double        *coeffs64,
                                       uint32_t          offset,
                                       float         K,
                                       float         V,
                                       float         Q,
                                       IasAudioFilterTypes  type,
                                       uint32_t          order,
                                       uint32_t          section)
{
  float alpha;
  int32_t   status;

  switch(type)
  {
    case eIasFilterTypeFlat: // ===============================================
      coeffs[0]          = 1.0f;
      coeffs[offset]     = 0.0f;
      coeffs[2*offset]   = 0.0f;
      coeffs[3*offset]   = 0.0f;
      coeffs[4*offset]   = 0.0f;
      break;

    case eIasFilterTypePeak:  // ==============================================
      // Re-check the filter parameters (this has already been done by IasAudioFilterCheckParams())
      IAS_ASSERT(Q >= cIasAudioFilterParamsMinValues.quality);
      IAS_ASSERT(Q <= cIasAudioFilterParamsMaxValues.quality);
      IAS_ASSERT(V >= cIasAudioFilterParamsMinValues.gain);
      IAS_ASSERT(V <= cIasAudioFilterParamsMaxValues.gain);
      IAS_ASSERT(order == 2);

      if (V >= 1.0)
      {
        float denominator = 1.0f/(1.0f + K/Q + K*K);
        coeffs[0]        = (1.0f + (V/Q)*K + K*K) * denominator;
        coeffs[offset]   = 2.0f*(K*K-1.0f) * denominator;
        coeffs[2*offset] = (1.0f - (V/Q)*K + K*K) * denominator;
        coeffs[3*offset] = 2.0f*(K*K-1.0f) * denominator;
        coeffs[4*offset] = (1.0f - K/Q + K*K) * denominator;
      }
      else
      {
        // Note that our definition of V is different to Zoelzer's definition.
        float denominator = 1.0f / (1.0f + K/(V*Q) + K*K);
        coeffs[0]        = (1.0f + K/Q + K*K) * denominator;
        coeffs[offset]   = 2.0f*(K*K-1.0f) * denominator;
        coeffs[2*offset] = (1.0f - K/Q + K*K) * denominator;
        coeffs[3*offset] = 2.0f*(K*K-1.0f) * denominator;
        coeffs[4*offset] = (1.0f-K/(V*Q) + K*K) * denominator;
      }
      break;

    case eIasFilterTypeBandpass:  // ==========================================
      // Re-check the filter parameters (this has already been done by IasAudioFilterCheckParams())
      IAS_ASSERT(Q >= cIasAudioFilterParamsMinValues.quality);
      IAS_ASSERT(Q <= cIasAudioFilterParamsMaxValues.quality);
      IAS_ASSERT(order == 2);

      {
        float denominator = 1.0f/(1.0f + K/Q + K*K);
        coeffs[0]          =  (K/Q) * denominator;
        coeffs[offset]     =  0.0f;
        coeffs[2*offset]   = -(K/Q) * denominator;
        coeffs[3*offset]   =  2.0f*(K*K-1.0f) * denominator;
        coeffs[4*offset]   =  (1.0f - K/Q + K*K) * denominator;
      }
      break;

    case eIasFilterTypeLowpass: // ============================================
      if ((order == 1) || ((order & 1) && (section == 1)))
      {
        // 1st order filter or 1st section of an odd-order filter
        float denominator = 1.0f/(K + 1.0f);
        coeffs[0]        = K * denominator;           // b0
        coeffs[offset]   = K * denominator;           // b1
        coeffs[2*offset] = 0.0f;                      // b2
        coeffs[3*offset] = (K - 1.0f) * denominator;  // a1
        coeffs[4*offset] = 0;                         // a2
      }
      else
      {
        status = calculateAlphaButterworth(&alpha, order, section);
        if (status)
        {
          return status;
        }
        float denominator = 1.0f / (1.0f + alpha*K + K*K);
        coeffs[0]        = K*K * denominator;                     // b0
        coeffs[offset]   = 2.0f*K*K * denominator;                // b1
        coeffs[2*offset] = K*K * denominator;                     // b2
        coeffs[3*offset] = 2.0f*(K*K-1.0f) * denominator;         // a1
        coeffs[4*offset] = (1.0f - alpha*K + K*K) * denominator;  // a2
      }
      break;

    case eIasFilterTypeHighpass: // ===========================================
      if ((order == 1) || ((order & 1) && (section == 1)))
      {
        // 1st order filter or 1st section of an odd-order filter
        float denominator = 1.0f/(K + 1.0f);
        coeffs[0]        =  denominator;              // b0
        coeffs[offset]   = -denominator;              // b1
        coeffs[2*offset] = 0.0f;                      // b2
        coeffs[3*offset] = (K - 1.0f) * denominator;  // a1
        coeffs[4*offset] = 0;                         // a2
      }
      else
      {
        status = calculateAlphaButterworth(&alpha, order, section);
        if (status)
        {
          return status;
        }
        float denominator = 1.0f / (1.0f + alpha*K + K*K);
        coeffs[0]        = denominator;                           // b0
        coeffs[offset]   = -2.0f*denominator;                     // b1
        coeffs[2*offset] = denominator;                           // b2
        coeffs[3*offset] = 2.0f*(K*K-1.0f) * denominator;         // a1
        coeffs[4*offset] = (1.0f - alpha*K + K*K) * denominator;  // a2
      }
      break;
    case eIasFilterTypeLowShelving: // ========================================
      // Re-check the filter parameters (this has already been done by IasAudioFilterCheckParams())
      IAS_ASSERT(V >= cIasAudioFilterParamsMinValues.gain);
      IAS_ASSERT(V <= cIasAudioFilterParamsMaxValues.gain);

      if (order == 1)
      {
        if (V >= 1.0)
        {
          float denominator = 1.0f/(K + 1.0f);
          coeffs[0]        = (K*V + 1.0f) * denominator;
          coeffs[offset]   = (K*V - 1.0f) * denominator;
          coeffs[2*offset] = 0.0f;
          coeffs[3*offset] = (K - 1.0f) * denominator;
          coeffs[4*offset] = 0.0f;
        }
        else
        {
          float denominator = 1.0f / (K/V + 1.0f);
          coeffs[0]        = (K + 1.0f) * denominator;
          coeffs[offset]   = (K - 1.0f) * denominator;
          coeffs[2*offset] = 0.0f;
          coeffs[3*offset] = (K/V - 1.0f) * denominator;
          coeffs[4*offset] = 0.0f;
        }
      }
      else if (order == 2)
      {
        float const cSqrt2 = sqrtf(2.0f);
        float const cSqrtV = sqrtf(V);
        if (V >= 1.0)
        {
          float denominator = 1.0f/(1.0f + cSqrt2*K + K*K);
          coeffs[0]        = (1.0f + cSqrt2*cSqrtV*K + V*K*K) * denominator;
          coeffs[offset]   = 2.0f*(V*K*K - 1.0f) * denominator;
          coeffs[2*offset] = (1.0f - cSqrt2*cSqrtV*K + V*K*K) * denominator;
          coeffs[3*offset] = 2.0f*(K*K-1.0f) * denominator;
          coeffs[4*offset] = (1.0f - cSqrt2*K + K*K) * denominator;
        }
        else
        {
          // Note that our definition of V is different to Zoelzer's definition.
          float denominator = 1.0f / (1.0f + cSqrt2*K/cSqrtV + K*K/V);
          coeffs[0]        = (1.0f + cSqrt2*K + K*K) * denominator;
          coeffs[offset]   = 2.0f*(K*K-1.0f) * denominator;
          coeffs[2*offset] = (1.0f - cSqrt2*K + K*K) * denominator;
          coeffs[3*offset] = 2.0f*(K*K/V-1.0f) * denominator;
          coeffs[4*offset] = (1.0f - cSqrt2*K/cSqrtV + K*K/V) * denominator;
        }
      }
      else
      {
        return -1;
      }
      break;
    case eIasFilterTypeHighShelving: // =======================================
      // Re-check the filter parameters (this has already been done by IasAudioFilterCheckParams())
      IAS_ASSERT(V >= cIasAudioFilterParamsMinValues.gain);
      IAS_ASSERT(V <= cIasAudioFilterParamsMaxValues.gain);

      if (order == 1)
      {
        if (V >= 1.0)
        {
          float denominator = 1.0f/(K + 1.0f);
          coeffs[0]        = (K + V) * denominator;
          coeffs[offset]   = (K - V) * denominator;
          coeffs[2*offset] = 0.0f;
          coeffs[3*offset] = (K - 1.0f) * denominator;
          coeffs[4*offset] = 0.0f;
        }
        else
        {
          float denominator = 1.0f / (K + 1.0f/V);
          coeffs[0]        = (K + 1.0f) * denominator;
          coeffs[offset]   = (K - 1.0f) * denominator;
          coeffs[2*offset] = 0.0f;
          coeffs[3*offset] = (K - 1.0f/V) * denominator;
          coeffs[4*offset] = 0.0f;
        }
      }
      else if (order == 2)
      {
        float const cSqrt2 = sqrtf(2.0f);
        float const cSqrtV = sqrtf(V);
        if (V >= 1.0)
        {
          float denominator = 1.0f/(1.0f + cSqrt2*K + K*K);
          coeffs[0]        = (V + cSqrt2*cSqrtV*K + K*K) * denominator;
          coeffs[offset]   = 2.0f*(K*K - V) * denominator;
          coeffs[2*offset] = (V - cSqrt2*cSqrtV*K + K*K) * denominator;
          coeffs[3*offset] = 2.0f*(K*K-1.0f) * denominator;
          coeffs[4*offset] = (1.0f - cSqrt2*K + K*K) * denominator;
        }
        else
        {
          // Note that our definition of V is different to Zoelzer's definition.
          float denominator1 = 1.0f / (1.0f/V + cSqrt2*K/cSqrtV + K*K);
          float denominator2 = 1.0f / (1.0f + cSqrt2*K*cSqrtV + K*K*V);
          coeffs[0]        = (1.0f + cSqrt2*K + K*K) * denominator1;
          coeffs[offset]   = 2.0f*(K*K-1.0f) * denominator1;
          coeffs[2*offset] = (1.0f - cSqrt2*K + K*K) * denominator1;
          coeffs[3*offset] = 2.0f*(K*K*V-1.0f) * denominator2;
          coeffs[4*offset] = (1.0f - cSqrt2*K*cSqrtV + K*K*V) * denominator2;
        }
      }
      else
      {
        return -1;
      }
      break;
    default: // ===============================================================
      return -1;
      break;
  }

  coeffs64[0]        = static_cast<double>(coeffs[0]);
  coeffs64[offset]   = static_cast<double>(coeffs[offset]);
  coeffs64[2*offset] = static_cast<double>(coeffs[2*offset]);
  coeffs64[3*offset] = static_cast<double>(coeffs[3*offset]);
  coeffs64[4*offset] = static_cast<double>(coeffs[4*offset]);

  return 0;
}


/*
 * ===================================
 * Methods of the class IasAudioFilter
 * ===================================
 */

// Constructor
IasAudioFilter::IasAudioFilter(uint32_t sampleFreq,uint32_t frameLength)
  :mSampleFreq(sampleFreq)
  ,mFrameLength(frameLength)
  ,mStateVarsBundle(NULL)
  ,mStateVars(NULL)
  ,mCoeffsBundle(NULL)
  ,mCoeffs(NULL)
  ,mStateVarsBundle64(NULL)
  ,mStateVars64(NULL)
  ,mCoeffsBundle64(NULL)
  ,mCoeffs64(NULL)
  ,mBundle(NULL)
  ,mCallback(NULL)
  ,mLogContext(IasAudioLogging::registerDltContext("_FIL", "Log of Audio Filter Module"))
{
}

// Destructor
IasAudioFilter::~IasAudioFilter()
{
  free(mStateVarsBundle);
  mStateVarsBundle = NULL;
  free(mStateVarsBundle64);
  mStateVarsBundle64 = NULL;
  free(mStateVars);
  mStateVars = NULL;
  free(mStateVars64);
  mStateVars64 = NULL;
  free(mCoeffsBundle);
  mCoeffsBundle = NULL;
  free(mCoeffsBundle64);
  mCoeffsBundle64 = NULL;
  free(mCoeffs);
  mCoeffs = NULL;
  free(mCoeffs64);
  mCoeffs64 = NULL;
  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, "IasAudioFilter::~IasAudioFilter: Deleted");
}

int32_t IasAudioFilter::updateGain(uint32_t channel, float gain)
{
  int32_t   status;

  if (channel >= cIasNumChannelsPerBundle)
  {
    return -1;
  }

  // Verify that the (new) filter parameters are valid.
  IasAudioFilterParams newParams;
  newParams.freq    = mFilterParams[channel].freq;
  newParams.gain    = gain;
  newParams.quality = mFilterParams[channel].quality;
  newParams.type    = mFilterParams[channel].type;
  newParams.order   = mFilterParams[channel].order;
  newParams.section = mFilterParams[channel].section;
  if (IasAudioFilterCheckParams(newParams, mSampleFreq) != 0)
  {
    return -1;
  }

  // Store the new filter parameters.
  mFilterParams[channel].gain = gain;

  // Prepare the update entry and push it into the queue.
  IasAudioFilterQueueEntry updateEntry;
  memset(&updateEntry, 0, sizeof(updateEntry));
  updateEntry.channel            = channel;
  updateEntry.rampedUpdate       = false;
  updateEntry.gainNew            = gain;
  updateEntry.useDoublePrecision = mFilterParamsInternal[channel].useDoublePrecision;
  updateEntry.clearStates        = false;

  status = calculateBiquadCoeff(updateEntry.biquadCoeffs32,
                                updateEntry.biquadCoeffs64,
                                1,
                                mFilterParamsInternal[channel].preWarpedFreq,
                                gain,
                                mFilterParams[channel].quality,
                                mFilterParams[channel].type,
                                mFilterParams[channel].order,
                                mFilterParams[channel].section);
  filterUpdateQueue.push(updateEntry);

  return status;
}


int32_t IasAudioFilter::rampGain(uint32_t  channel,
                                    float gain,
                                    uint64_t  callbackUserData)
{
  if (channel >= cIasNumChannelsPerBundle)
  {
    return -1;
  }

  // Verify that the (new) filter parameters are valid.
  IasAudioFilterParams newParams;
  newParams.freq    = mFilterParams[channel].freq;
  newParams.gain    = gain;
  newParams.quality = mFilterParams[channel].quality;
  newParams.type    = mFilterParams[channel].type;
  newParams.order   = mFilterParams[channel].order;
  newParams.section = mFilterParams[channel].section;
  if (IasAudioFilterCheckParams(newParams, mSampleFreq) != 0)
  {
    return -1;
  }

  // Store the new filter parameters.
  mFilterParams[channel].gain = gain;

  // Prepare the update entry and push it into the queue.
  IasAudioFilterQueueEntry updateEntry;
  memset(&updateEntry, 0, sizeof(updateEntry));
  updateEntry.channel          = channel;
  updateEntry.rampedUpdate     = true;
  updateEntry.gainNew          = gain;
  updateEntry.preWarpedFreq    = mFilterParamsInternal[channel].preWarpedFreq;
  updateEntry.quality          = mFilterParams[channel].quality;
  updateEntry.type             = mFilterParams[channel].type;
  updateEntry.order            = mFilterParams[channel].order;
  updateEntry.section          = mFilterParams[channel].section;
  updateEntry.factorRampUp     = mFilterParamsInternal[channel].factorRampUp;
  updateEntry.factorRampDown   = mFilterParamsInternal[channel].factorRampDown;
  updateEntry.callbackUserData = callbackUserData;
  filterUpdateQueue.push(updateEntry);

  return 0;
}


int32_t IasAudioFilter::setChannelFilter(uint32_t channel, IasAudioFilterParams const * params )
{
  int32_t status = 0;

  if (params == NULL)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR,
                    "IasAudioFilter::setChannelFilter: Error: params pointer is NULL");
    return -1;
  }
  if (channel >= cIasNumChannelsPerBundle)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR,
                    "IasAudioFilter::setChannelFilter: Error: channel ID (", channel,
                    ") exceeds cIasNumChannelsPerBundle");
    return -1;
  }

  // Verify that the filter parameters are valid.
  if (IasAudioFilterCheckParams(*params, mSampleFreq) != 0)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR,
                    "IasAudioFilter::setChannelFilter: Error: invalid filter parameters: ",
                    "type=",    static_cast<uint32_t>(params->type),
                    "freq=",    params->freq,
                    "gain=",    params->gain,
                    "quality=", params->quality,
                    "order=",   params->order);
    return -1;
  }

  float preWarpedFreq;
  preWarpedFreq = calculatePreWarpedFrequency(static_cast<float>(params->freq),
                                              static_cast<float>(mSampleFreq));

  // Decide whether this filter shall be calculated using double precision
  bool useDoublePrecision = false;
  switch (params->type)
  {
    case eIasFilterTypePeak:
      if((params->freq < cIasPeakFilterFreqBorder ) || (params->quality > cIasPeakFilterQualBorder))
      {
        useDoublePrecision = true;
      }
      break;
    case eIasFilterTypeLowpass:
      if(params->freq < cIasLPFilterFreqBorder)
      {
        useDoublePrecision = true;
      }
      break;
    case eIasFilterTypeHighpass:
      if(params->freq < cIasHPFilterFreqBorder)
      {
        useDoublePrecision = true;
      }
      break;
    case eIasFilterTypeHighShelving:
      if((params->freq <= cIasShelvFilterFreqBorder) && (params->order == 2))
      {
        useDoublePrecision = true;
      }
      break;
    case eIasFilterTypeLowShelving:
      if((params->freq <= cIasShelvFilterFreqBorder) && (params->order == 2))
      {
        useDoublePrecision = true;
      }
      break;
    default:
      break;
  }

  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, "Double precision for channel ", channel," is ", useDoublePrecision);

  // Store the new filter parameters.
  mFilterParams[channel] = *params;
  mFilterParamsInternal[channel].preWarpedFreq = preWarpedFreq;
  mFilterParamsInternal[channel].useDoublePrecision = useDoublePrecision;

  // Prepare the update entry and push it into the queue.
  IasAudioFilterQueueEntry updateEntry;
  memset(&updateEntry, 0, sizeof(updateEntry));
  updateEntry.channel            = channel;
  updateEntry.rampedUpdate       = false;
  updateEntry.gainNew            = params->gain;
  updateEntry.useDoublePrecision = useDoublePrecision;
  updateEntry.clearStates        = true;
  status = calculateBiquadCoeff(updateEntry.biquadCoeffs32,
                                updateEntry.biquadCoeffs64,
                                1,
                                mFilterParamsInternal[channel].preWarpedFreq,
                                mFilterParams[channel].gain,
                                mFilterParams[channel].quality,
                                mFilterParams[channel].type,
                                mFilterParams[channel].order,
                                mFilterParams[channel].section);
  filterUpdateQueue.push(updateEntry);

  return status;
}


/*
 *  Set the gradient that is used for ramping the filter gain.
 */
int32_t IasAudioFilter::setRampGradient(uint32_t channel, float gradient)
{
  float factorRampUp;

  if ( (channel >= cIasNumChannelsPerBundle) || (gradient < 0.01f) || (gradient > 6.0f) )
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR,
                    "IasAudioFilter::setChannelFilter: Error: invalid channel or gradient: ",
                    "channel=", channel,
                    "gradient=", gradient);
    return -1;
  }

  factorRampUp = powf(10.0f, gradient*0.05f);
  mFilterParamsInternal[channel].factorRampUp   = factorRampUp;
  mFilterParamsInternal[channel].factorRampDown = 1.0f/factorRampUp;

  return 0;
}


/*
 *  This is the process() function.
 */
void IasAudioFilter::calculate()
{
  uint32_t channel;
  IasAudioFilterQueueEntry updateEntry;
  memset(&updateEntry, 0, sizeof(updateEntry));

  // Process all updates from the queue.
  while(filterUpdateQueue.try_pop(updateEntry))
  {
    channel = updateEntry.channel;
    if (updateEntry.rampedUpdate)
    {
      // Ramped update: adopt all parameters that are required for ramping the filter gain.
      mProcessingParams[channel].gainTarget       = updateEntry.gainNew;
      mProcessingParams[channel].preWarpedFreq    = updateEntry.preWarpedFreq;
      mProcessingParams[channel].quality          = updateEntry.quality;
      mProcessingParams[channel].type             = updateEntry.type;
      mProcessingParams[channel].order            = updateEntry.order;
      mProcessingParams[channel].section          = updateEntry.section;
      mProcessingParams[channel].factorRampUp     = updateEntry.factorRampUp;
      mProcessingParams[channel].factorRampDown   = updateEntry.factorRampDown;
      mProcessingParams[channel].callbackUserData = updateEntry.callbackUserData;
      mProcessingParams[channel].isRamping        = true;
    }
    else
    {
      // Immediate update: copy the new filter coefficients into the working coefficient set.
      float* coeffsNew      = updateEntry.biquadCoeffs32;
      double* coeffsNew64    = updateEntry.biquadCoeffs64;
      float* coeffsActual   = mCoeffsBundle+channel;
      double* coeffsActual64 = mCoeffsBundle64+channel;
      for (uint32_t cnt=0; cnt<cIasNumCoeffsBiquad; cnt++)
      {
        *coeffsActual   = *coeffsNew;
        *coeffsActual64 = *coeffsNew64;
        coeffsNew      += 1;
        coeffsActual   += cIasNumChannelsPerBundle;
        coeffsNew64    += 1;
        coeffsActual64 += cIasNumChannelsPerBundle;
      }

      // If requested: reset the state variables for this channel to zero.
      if (updateEntry.clearStates)
      {
        float* stateVars      = mStateVarsBundle+channel;
        double* stateVars64    = mStateVarsBundle64+channel;
        for (uint32_t cnt=0; cnt<cIasNumStateVarsBiquad; cnt++)
        {
          *stateVars   = 0.0f;
          *stateVars64 = 0.0;
          stateVars   += cIasNumChannelsPerBundle;
          stateVars64 += cIasNumChannelsPerBundle;
        }
      }

      // If the ramp generator for this channel was still active, execute the
      // callback function with the updated gain setting so that the software
      // alyer above knows that the ramp has been finished now.
      if (mProcessingParams[channel].isRamping && mCallback)
      {
        mCallback->gainRampingFinished(channel, updateEntry.gainNew, mProcessingParams[channel].callbackUserData);
      }

      // Store the updated params.
      mProcessingParams[channel].gainCurrent = updateEntry.gainNew;
      mProcessingParams[channel].useDoublePrecision = updateEntry.useDoublePrecision;
      mProcessingParams[channel].isRamping   = false;
    }
  }

  // Check for each channel, whether the filter gain has to be ramped up/down.
  for (uint32_t channel=0; channel<cIasNumChannelsPerBundle; channel++)
  {
    if (mProcessingParams[channel].isRamping)
    {
      float gainTarget  = mProcessingParams[channel].gainTarget;

      if (mProcessingParams[channel].gainCurrent <= gainTarget)
      {
        mProcessingParams[channel].gainCurrent = mProcessingParams[channel].gainCurrent*mProcessingParams[channel].factorRampUp;
        if (mProcessingParams[channel].gainCurrent >= gainTarget * 0.999f)
        {
          mProcessingParams[channel].gainCurrent = gainTarget;
          mProcessingParams[channel].isRamping = false;
          if (mCallback)
          {
            mCallback->gainRampingFinished(channel, gainTarget, mProcessingParams[channel].callbackUserData);
          }
        }
        (void)calculateBiquadCoeff(mCoeffsBundle+channel,
                                   mCoeffsBundle64+channel,
                                   cIasNumChannelsPerBundle,
                                   mProcessingParams[channel].preWarpedFreq,
                                   mProcessingParams[channel].gainCurrent,
                                   mProcessingParams[channel].quality,
                                   mProcessingParams[channel].type,
                                   mProcessingParams[channel].order,
                                   mProcessingParams[channel].section);
      }
      else  // now we have the case that (mProcessingParams[channel].gainCurrent > gainTarget)
      {
        mProcessingParams[channel].gainCurrent = mProcessingParams[channel].gainCurrent*mProcessingParams[channel].factorRampDown;
        if (mProcessingParams[channel].gainCurrent <= gainTarget * 1.001f)
        {
          mProcessingParams[channel].gainCurrent = gainTarget;
          mProcessingParams[channel].isRamping = false;
          if (mCallback)
          {
            mCallback->gainRampingFinished(channel, gainTarget, mProcessingParams[channel].callbackUserData);
          }
        }
        (void)calculateBiquadCoeff(mCoeffsBundle+channel,
                                   mCoeffsBundle64+channel,
                                   cIasNumChannelsPerBundle,
                                   mProcessingParams[channel].preWarpedFreq,
                                   mProcessingParams[channel].gainCurrent,
                                   mProcessingParams[channel].quality,
                                   mProcessingParams[channel].type,
                                   mProcessingParams[channel].order,
                                   mProcessingParams[channel].section);
      }
    }
  }

  bool useDoublePrecision = ( mProcessingParams[0].useDoublePrecision ||
                                   mProcessingParams[1].useDoublePrecision ||
                                   mProcessingParams[2].useDoublePrecision ||
                                   mProcessingParams[3].useDoublePrecision);

#if ENFORCE_FLOAT32
  useDoublePrecision = false;
#endif
#if ENFORCE_FLOAT64
  useDoublePrecision = true;
#endif

#if USE_SSE

  if(!useDoublePrecision)
  {
    /*
     * Variables that are required for both SSE-implmentation (Intel and Gnu compiler)
     */
    __m128       *bundleData = (__m128*)(mBundle->getAudioDataPointer());
    __m128 const *coeffs     = (__m128*)mCoeffsBundle;
    __m128       *vars       = (__m128*)mStateVarsBundle;;
    __m128        output;

    // Coefficients
    __m128 const b0 = coeffs[0];
    __m128 const b1 = coeffs[1];
    __m128 const b2 = coeffs[2];
    __m128 const a1 = coeffs[3];
    __m128 const a2 = coeffs[4];

    // States
    __m128 x1 = vars[0]; // x(n-1)
    __m128 x2 = vars[1]; // x(n-2)
    __m128 y1 = vars[2]; // y(n-1)
    __m128 y2 = vars[3]; // y(n-2)


#if INTEL_COMPILER
  /*
   * For the Intel compiler we use 8x loop unrollment (controlled by pragma).
   * The number of samples per frame must be a multiple of 8.
   */
    IAS_ASSERT((mFrameLength & 0x07) == 0);
    #pragma ivdep
    #pragma vector aligned
    #pragma unroll(8)
    for(uint32_t i=0; i < mFrameLength; i++)
    {
      output =  (_mm_sub_ps(_mm_add_ps(_mm_mul_ps(*bundleData,   b0),    // x(n)*b0
                                       _mm_add_ps(_mm_mul_ps(x1, b1),    // x(n-1)*b1
                                                  _mm_mul_ps(x2, b2))),  // x(n-2)*b2
                                                  _mm_add_ps(           _mm_mul_ps(y1, a1),    // y(n-1)*a1
                                                                        _mm_mul_ps(y2, a2)))); // y(n-2)*a2

      x2 = x1;                //x(n-2) = x(n-1)
      x1 = *bundleData;       //x(n-1) = x(n)
      y2 = y1;                //y(n-2) = y(n-1)
      y1 = output;            //y(n-1) = y(n)
      *bundleData = output;
      bundleData++;
    }

    // Save states for the next frame to be processed.
    vars[0] = x1;
    vars[1] = x2;
    vars[2] = y1;
    vars[3] = y2;
#else
    /*
     * For the Gnu compiler, we use 4x loop unrollment (manual unrollment).
     * The number of samples per frame must be a multiple of 4.
     */
    IAS_ASSERT((mFrameLength & 0x03) == 0);
    for(uint32_t i=0; i< mFrameLength; i+=4)
    {
      // 1st sample
      output =  (_mm_sub_ps(_mm_add_ps(_mm_mul_ps(*bundleData,   b0),    // x(n)*b0
                                       _mm_add_ps(_mm_mul_ps(x1, b1),    // x(n-1)*b1
                                                  _mm_mul_ps(x2, b2))),  // x(n-2)*b2
                                                  _mm_add_ps(           _mm_mul_ps(y1, a1),    // y(n-1)*a1
                                                                        _mm_mul_ps(y2, a2)))); // y(n-2)*a2

      x2 = x1;                //x(n-2) = x(n-1)
      x1 = *bundleData;       //x(n-1) = x(n)
      y2 = y1;                //y(n-2) = y(n-1)
      y1 = output;            //y(n-1) = y(n)
      *bundleData = output;
      bundleData++;

      // 2nd sample
      output =  (_mm_sub_ps(_mm_add_ps(_mm_mul_ps(*bundleData,   b0),    // x(n)*b0
                                       _mm_add_ps(_mm_mul_ps(x1, b1),    // x(n-1)*b1
                                                  _mm_mul_ps(x2, b2))),  // x(n-2)*b2
                                                  _mm_add_ps(           _mm_mul_ps(y1, a1),    // y(n-1)*a1
                                                                        _mm_mul_ps(y2, a2)))); // y(n-2)*a2

      x2 = x1;                //x(n-2) = x(n-1)
      x1 = *bundleData;       //x(n-1) = x(n)
      y2 = y1;                //y(n-2) = y(n-1)
      y1 = output;            //y(n-1) = y(n)
      *bundleData = output;
      bundleData++;

      // 3rd sample
      output =  (_mm_sub_ps(_mm_add_ps(_mm_mul_ps(*bundleData,   b0),    // x(n)*b0
                                       _mm_add_ps(_mm_mul_ps(x1, b1),    // x(n-1)*b1
                                                  _mm_mul_ps(x2, b2))),  // x(n-2)*b2
                                                  _mm_add_ps(           _mm_mul_ps(y1, a1),    // y(n-1)*a1
                                                                        _mm_mul_ps(y2, a2)))); // y(n-2)*a2

      x2 = x1;                //x(n-2) = x(n-1)
      x1 = *bundleData;       //x(n-1) = x(n)
      y2 = y1;                //y(n-2) = y(n-1)
      y1 = output;            //y(n-1) = y(n)
      *bundleData = output;
      bundleData++;

      // 4th sample
      output =  (_mm_sub_ps(_mm_add_ps(_mm_mul_ps(*bundleData,   b0),    // x(n)*b0
                                       _mm_add_ps(_mm_mul_ps(x1, b1),    // x(n-1)*b1
                                                  _mm_mul_ps(x2, b2))),  // x(n-2)*b2
                                                  _mm_add_ps(           _mm_mul_ps(y1, a1),    // y(n-1)*a1
                                                                        _mm_mul_ps(y2, a2)))); // y(n-2)*a2

      x2 = x1;                //x(n-2) = x(n-1)
      x1 = *bundleData;       //x(n-1) = x(n)
      y2 = y1;                //y(n-2) = y(n-1)
      y1 = output;            //y(n-1) = y(n)
      *bundleData = output;
      bundleData++;
    }

    // Save states for the next frame to be processed.
    vars[0] = x1;
    vars[1] = x2;
    vars[2] = y1;
    vars[3] = y2;
#endif
  }
  else // doublePrecision -> SSE2
  {
    /*
     * SSE2-optimized variant using double-precision.
     *
     * The SSE2 is used here to process two channels in parallel. The input
     * samples are converted from Float32 to Float64, before the filter core
     * is executed. After the filter, the output samples have to be converted
     * from Float64 to Float32.
     */
    __m128        *bundleData   = (__m128*)(mBundle->getAudioDataPointer());
    __m128d        bundleDataDP;
    __m128d const *coeffs       = (__m128d*)mCoeffsBundle64;
    __m128d       *vars         = (__m128d*)mStateVarsBundle64;
    __m128d        outputDP;
    __m128         outputSP_1;
    __m128         outputSP_2;

    // Coefficients
    __m128d const b0_0 = coeffs[0];
    __m128d const b0_1 = coeffs[1];
    __m128d const b1_0 = coeffs[2];
    __m128d const b1_1 = coeffs[3];
    __m128d const b2_0 = coeffs[4];
    __m128d const b2_1 = coeffs[5];
    __m128d const a1_0 = coeffs[6];
    __m128d const a1_1 = coeffs[7];
    __m128d const a2_0 = coeffs[8];
    __m128d const a2_1 = coeffs[9];

    // States
    __m128d x1_0 = vars[0]; // x(n-1)
    __m128d x1_1 = vars[1]; // x(n-1)
    __m128d x2_0 = vars[2]; // x(n-2)
    __m128d x2_1 = vars[3]; // x(n-2)
    __m128d y1_0 = vars[4]; // y(n-1)
    __m128d y1_1 = vars[5]; // y(n-1)
    __m128d y2_0 = vars[6]; // y(n-2)
    __m128d y2_1 = vars[7]; // y(n-2)

    /*
     * Variant #2 of double-precision SSE: innermost loop with loop unrollment of 2.
     */
    // Verify that mFrameLength is even.
    IAS_ASSERT((mFrameLength & 0x01) == 0);
    for(uint32_t i=0; i< mFrameLength; i+=2)
    {
      // 1st and 2nd channel, 1st sample.
      // Convert the two lower single-precision (SP) floating-point values to double precision (DP).
      bundleDataDP = _mm_cvtps_pd(*bundleData);

      outputDP =  (_mm_sub_pd(_mm_add_pd(_mm_mul_pd(bundleDataDP,    b0_0),    // x(n)*b0
                                         _mm_add_pd(_mm_mul_pd(x1_0, b1_0),    // x(n-1)*b1
                                                    _mm_mul_pd(x2_0, b2_0))),  // x(n-2)*b2
                              _mm_add_pd(           _mm_mul_pd(y1_0, a1_0),    // y(n-1)*a1
                                                    _mm_mul_pd(y2_0, a2_0)))); // y(n-2)*a2

      x2_0 = x1_0;         //x(n-2) = x(n-1)
      x1_0 = bundleDataDP; //x(n-1) = x(n)
      y2_0 = y1_0;         //y(n-2) = y(n-1)
      y1_0 = outputDP;     //y(n-1) = y(n)

      // Convert the two double precision output values to SP float
      // and store them in the lower portion of outputSP_1
      outputSP_1 = _mm_cvtpd_ps(outputDP);

      // 3rd and 4th channel, 1st sample.
      // Convert the two higher single-precision (SP) floating-point values to double precision (DP).
      bundleDataDP = _mm_cvtps_pd((__m128)(_mm_srli_si128((__m128i)(*bundleData),8)));

      outputDP =  (_mm_sub_pd(_mm_add_pd(_mm_mul_pd(bundleDataDP,    b0_1),    // x(n)*b0
                                         _mm_add_pd(_mm_mul_pd(x1_1, b1_1),    // x(n-1)*b1
                                                    _mm_mul_pd(x2_1, b2_1))),  // x(n-2)*b2
                              _mm_add_pd(           _mm_mul_pd(y1_1, a1_1),    // y(n-1)*a1
                                                    _mm_mul_pd(y2_1, a2_1)))); // y(n-2)*a2

      x2_1 = x1_1;         //x(n-2) = x(n-1)
      x1_1 = bundleDataDP; //x(n-1) = x(n)
      y2_1 = y1_1;         //y(n-2) = y(n-1)
      y1_1 = outputDP;     //y(n-1) = y(n)

      // Convert the two double precision output values to SP float
      // and store them in the higher portion of outputSP_2
      outputSP_2 = (__m128)(_mm_slli_si128((__m128i)_mm_cvtpd_ps(outputDP),8));

      // Collect all four SP results and store them into the output buffer.
      *bundleData = _mm_add_ps(outputSP_1, outputSP_2);

      bundleData++;

      // 1st and 2nd channel, 2nd sample.
      // Convert the two lower single-precision (SP) floating-point values to double precision (DP).
      bundleDataDP = _mm_cvtps_pd(*bundleData);

      outputDP =  (_mm_sub_pd(_mm_add_pd(_mm_mul_pd(bundleDataDP,    b0_0),    // x(n)*b0
                                         _mm_add_pd(_mm_mul_pd(x1_0, b1_0),    // x(n-1)*b1
                                                    _mm_mul_pd(x2_0, b2_0))),  // x(n-2)*b2
                              _mm_add_pd(           _mm_mul_pd(y1_0, a1_0),    // y(n-1)*a1
                                                    _mm_mul_pd(y2_0, a2_0)))); // y(n-2)*a2

      x2_0 = x1_0;         //x(n-2) = x(n-1)
      x1_0 = bundleDataDP; //x(n-1) = x(n)
      y2_0 = y1_0;         //y(n-2) = y(n-1)
      y1_0 = outputDP;     //y(n-1) = y(n)

      // Convert the two double precision output values to SP float
      // and store them in the lower portion of outputSP_1
      outputSP_1 = _mm_cvtpd_ps(outputDP);

      // 3rd and 4th channel, 2nd sample.
      // Convert the two higher single-precision (SP) floating-point values to double precision (DP).
      bundleDataDP = _mm_cvtps_pd((__m128)(_mm_srli_si128((__m128i)(*bundleData),8)));

      outputDP =  (_mm_sub_pd(_mm_add_pd(_mm_mul_pd(bundleDataDP,    b0_1),    // x(n)*b0
                                         _mm_add_pd(_mm_mul_pd(x1_1, b1_1),    // x(n-1)*b1
                                                    _mm_mul_pd(x2_1, b2_1))),  // x(n-2)*b2
                              _mm_add_pd(           _mm_mul_pd(y1_1, a1_1),    // y(n-1)*a1
                                                    _mm_mul_pd(y2_1, a2_1)))); // y(n-2)*a2

      x2_1 = x1_1;         //x(n-2) = x(n-1)
      x1_1 = bundleDataDP; //x(n-1) = x(n)
      y2_1 = y1_1;         //y(n-2) = y(n-1)
      y1_1 = outputDP;     //y(n-1) = y(n)

      // Convert the two double precision output values to SP float
      // and store them in the higher portion of outputSP_2
      outputSP_2 = (__m128)(_mm_slli_si128((__m128i)_mm_cvtpd_ps(outputDP),8));

      // Collect all four SP results and store them into the output buffer.
      *bundleData = _mm_add_ps(outputSP_1, outputSP_2);

      bundleData++;
    }

    // Save states for the next frame to be processed.
    vars[0] = x1_0;
    vars[1] = x1_1;
    vars[2] = x2_0;
    vars[3] = x2_1;
    vars[4] = y1_0;
    vars[5] = y1_1;
    vars[6] = y2_0;
    vars[7] = y2_1;
    /*
     * end of the SSE2 variant.
     */
  }

#else // USE_SSE

  if (!useDoublePrecision)
  {
    /*
     * Generic C++ implementation (without SSE optimizations) using single precision
     */
    float const *coeffs     = mCoeffsBundle;
    float       *vars       = mStateVarsBundle;
    float        output;

    // Loop over all channels that belong to this bundle.
    for (uint32_t chan=0; chan < cIasNumChannelsPerBundle; chan++)
    {
      float       * __restrict bundleData = mBundle->getAudioDataPointer() + chan;

      // Coefficients
      float const b0 = coeffs[chan];
      float const b1 = coeffs[chan + 1*cIasNumChannelsPerBundle];
      float const b2 = coeffs[chan + 2*cIasNumChannelsPerBundle];
      float const a1 = coeffs[chan + 3*cIasNumChannelsPerBundle];
      float const a2 = coeffs[chan + 4*cIasNumChannelsPerBundle];

      // States
      float x1 = vars[chan]; // x(n-1)
      float x2 = vars[chan + 1*cIasNumChannelsPerBundle]; // x(n-2)
      float y1 = vars[chan + 2*cIasNumChannelsPerBundle]; // y(n-1)
      float y2 = vars[chan + 3*cIasNumChannelsPerBundle]; // y(n-2)

      // Loop over all samples that belong to this frame.
      for (uint32_t i=0; i < mFrameLength; i++)
      {
        output = *bundleData*b0 + x1*b1 + x2*b2 - (y1*a1 + y2*a2);

        x2 = x1;                //x(n-2) = x(n-1)
        x1 = *bundleData;       //x(n-1) = x(n)
        y2 = y1;                //y(n-2) = y(n-1)
        y1 = output;            //y(n-1) = y(n)
        *bundleData = output;
        bundleData+=4;
      }

      // Save states for the next frame to be processed.
      vars[chan] = x1;
      vars[chan + 1*cIasNumChannelsPerBundle] = x2;
      vars[chan + 2*cIasNumChannelsPerBundle] = y1;
      vars[chan + 3*cIasNumChannelsPerBundle] = y2;
    }
  }
  else
  {
    /*
     * Generic C++ implementation (without SSE optimizations) using double precision
     */
    double const *coeffs     = mCoeffsBundle64;
    double       *vars       = mStateVarsBundle64;
    double        output;
    double        input;

    // Loop over all channels that belong to this bundle.
    for (uint32_t chan=0; chan < cIasNumChannelsPerBundle; chan++)
    {
      float       * __restrict bundleData = mBundle->getAudioDataPointer() + chan;

      // Coefficients
      double const b0 = coeffs[chan];
      double const b1 = coeffs[chan + 1*cIasNumChannelsPerBundle];
      double const b2 = coeffs[chan + 2*cIasNumChannelsPerBundle];
      double const a1 = coeffs[chan + 3*cIasNumChannelsPerBundle];
      double const a2 = coeffs[chan + 4*cIasNumChannelsPerBundle];

      // States
      double x1 = vars[chan]; // x(n-1)
      double x2 = vars[chan + 1*cIasNumChannelsPerBundle]; // x(n-2)
      double y1 = vars[chan + 2*cIasNumChannelsPerBundle]; // y(n-1)
      double y2 = vars[chan + 3*cIasNumChannelsPerBundle]; // y(n-2)

      // Loop over all samples that belong to this frame.
      for (uint32_t i=0; i < mFrameLength; i++)
      {
        input  = static_cast<double>(*bundleData);
        output = input*b0 + x1*b1 + x2*b2 - (y1*a1 + y2*a2);

        x2 = x1;                //x(n-2) = x(n-1)
        x1 = input;             //x(n-1) = x(n)
        y2 = y1;                //y(n-2) = y(n-1)
        y1 = output;            //y(n-1) = y(n)
        *bundleData = static_cast<float>(output);
        bundleData+=4;
      }

      // Save states for the next frame to be processed.
      vars[chan] = x1;
      vars[chan + 1*cIasNumChannelsPerBundle] = x2;
      vars[chan + 2*cIasNumChannelsPerBundle] = y1;
      vars[chan + 3*cIasNumChannelsPerBundle] = y2;
    }
  }
#endif // USE_SSE
}


void IasAudioFilter::announceCallback(IasAudioFilterCallback* callback)
{
  mCallback = callback;
}


IasAudioProcessingResult IasAudioFilter::init()
{

  mStateVarsBundle   = (float*)memalign(128,cIasNumStateVarsBiquad*cIasNumChannelsPerBundle*sizeof(float));
  mStateVarsBundle64 = (double*)memalign(128,cIasNumStateVarsBiquad*cIasNumChannelsPerBundle*sizeof(double));
  if ((mStateVarsBundle == NULL) || (mStateVarsBundle64 == NULL))
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, "IasAudioFilter::init: not enough memory for memalign()");
    return eIasAudioProcNotEnoughMemory;
  }

  mStateVars   = (float**)memalign(128,cIasNumStateVarsBiquad*sizeof(float*));
  mStateVars64 = (double**)memalign(128,cIasNumStateVarsBiquad*sizeof(double*));
  if ((mStateVars == NULL) || (mStateVars64 == NULL))
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, "IasAudioFilter::init: not enough memory for memalign()");
    return eIasAudioProcNotEnoughMemory;
  }

  mCoeffsBundle   = (float*)memalign(128,cIasNumCoeffsBiquad*cIasNumChannelsPerBundle*sizeof(float));
  mCoeffsBundle64 = (double*)memalign(128,cIasNumCoeffsBiquad*cIasNumChannelsPerBundle*sizeof(double));
  if ((mCoeffsBundle == NULL) || (mCoeffsBundle64 == NULL))
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, "IasAudioFilter::init: not enough memory for memalign()");
    return eIasAudioProcNotEnoughMemory;
  }

  mCoeffs   = (float**)memalign(128, cIasNumCoeffsBiquad*sizeof(float*));
  mCoeffs64 = (double**)memalign(128, cIasNumCoeffsBiquad*sizeof(double*));
  if ((mCoeffs == NULL) || (mCoeffs64 == NULL))
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, "IasAudioFilter::init: not enough memory for memalign()");
    return eIasAudioProcNotEnoughMemory;
  }


  memset(mStateVarsBundle,       0, cIasNumStateVarsBiquad*cIasNumChannelsPerBundle*sizeof(float));
  memset(mCoeffsBundle,          0, cIasNumCoeffsBiquad*cIasNumChannelsPerBundle*sizeof(float));
  memset(mStateVarsBundle64,     0, cIasNumStateVarsBiquad*cIasNumChannelsPerBundle*sizeof(double));
  memset(mCoeffsBundle64,        0, cIasNumCoeffsBiquad*cIasNumChannelsPerBundle*sizeof(double));

  // Set the b0 coefficients for all channels to 1
  for(uint32_t i=0; i<cIasNumChannelsPerBundle;++i)
  {
    mCoeffsBundle[i]         = 1.0f;
    mCoeffsBundle64[i]       = 1.0;
  }

  float* tempVars = mStateVarsBundle;
  double* tempVars64 = mStateVarsBundle64;
  float* tempCoeffs = mCoeffsBundle;
  double* tempCoeffs64 = mCoeffsBundle64;

  for(uint32_t i=0; i<cIasNumCoeffsBiquad;++i)
  {
    mCoeffs[i]         = tempCoeffs;
    tempCoeffs        += cIasNumChannelsPerBundle;
    mCoeffs64[i]       = tempCoeffs64;
    tempCoeffs64      += cIasNumChannelsPerBundle;
  }

  for(uint32_t i=0; i<cIasNumStateVarsBiquad; ++i)
  {
    mStateVars[i] = tempVars;
    tempVars += cIasNumChannelsPerBundle;
    mStateVars64[i] = tempVars64;
    tempVars64 += cIasNumChannelsPerBundle;
  }

  for(uint32_t chan=0; chan<cIasNumChannelsPerBundle; chan++)
  {
    mProcessingParams[chan].isRamping = false;
    mProcessingParams[chan].useDoublePrecision = false;

    mFilterParams[chan].freq         = 1000;
    mFilterParams[chan].gain         = 1.0f;
    mFilterParams[chan].quality      = 1.0f;
    mFilterParams[chan].type         = eIasFilterTypeFlat;
    mFilterParams[chan].order        = 2;
    mFilterParams[chan].section      = 1;

    mFilterParamsInternal[chan].preWarpedFreq = calculatePreWarpedFrequency(static_cast<float>(mFilterParams[chan].freq),
                                                                            static_cast<float>(mSampleFreq));

    mFilterParamsInternal[chan].factorRampUp       = 1.02920f;  //  0.25 dB
    mFilterParamsInternal[chan].factorRampDown     = 0.97162f;  // -0.25 dB
    mFilterParamsInternal[chan].useDoublePrecision = false;

    if (IasAudioFilterCheckParams(mFilterParams[chan], mSampleFreq) != 0)
    {
      DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, "IasAudioFilter::init: error during IasAudioFilterCheckParams()");
      return eIasAudioProcInitializationFailed;
    }
  }
  this->reset();
  return eIasAudioProcOK;
}


int32_t IasAudioFilter::reset()
{
  for (uint32_t i=0; i<cIasNumStateVarsBiquad; ++i)
  {
    memset(mStateVars[i],  0, cIasNumChannelsPerBundle*sizeof(mStateVars[0][0]));
    memset(mStateVars64[i],0, cIasNumChannelsPerBundle*sizeof(mStateVars64[0][0]));
  }
  return 0;
}

} // namespace Ias
