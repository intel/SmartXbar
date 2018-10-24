/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file IasRamp.cpp
 * @date 2012
 * @brief the implementation of the ramp generator
 */

#include <math.h>
#include "helper/IasRamp.hpp"
#include <stdio.h>

namespace IasAudio {

IasRamp::IasRamp(uint32_t sampleFreq, uint32_t frameSize)
  :mSampleFreq(sampleFreq)
  ,mFrameSize(frameSize)
{
  mStartValue    = 0.0f;
  mEndValue      = 0.0f;
  mRampTime      = 0;
  mDelta         = 0.0f;
  mIncrementMult = 0.0f;
  mIncrementAdd  = 0.0f;
  mCurrentValue  = 0.0f;
  mNumRampValues = 0;
  mRampSetActive = false;
  mRampLinear    = true;
}

IasRamp::~IasRamp()
{

}

IasRampErrors IasRamp::setTimedRamp(float startValue, float endValue,
                                    uint32_t rampTime, IasRampShapes fadingShape )
{
  if( mSampleFreq == 0 || mFrameSize == 0)
  {
    return eIasRampErrorInitParams;
  }
  if( rampTime == 0 )
  {
    return eIasRampErrorZeroFadeTime;
  }
  if(fadingShape >= eIasMaxNumRampShapes )
  {
    return eIasRampErrorUnknownRampShape;
  }
  if(fadingShape == eIasRampShapeLinear)
  {
    setTimedRampLinear(startValue, endValue, rampTime);
  }
  else if(fadingShape == eIasRampShapeExponential)
  {
    setTimedRampExponential(startValue, endValue, rampTime);
  }
  return eIasRampErrorNoError;
}


void IasRamp::setTimedRampLinear(float startValue, float endValue, uint32_t rampTime)
{

  mStartValue = startValue;
  mEndValue   = endValue;
  mRampTime   = rampTime;

  mDelta = (mEndValue - mStartValue);
  if(mDelta != 0)
  {
    mNumRampValues = static_cast<uint32_t>(floor(static_cast<float>(mRampTime) * 0.001f *
                                                    static_cast<float>(mSampleFreq)));
    mIncrementAdd  = mDelta / static_cast<float>(mNumRampValues);
  }
  else
  {
    mNumRampValues = 0;
    mIncrementAdd = 0;
  }
  mCurrentValue = mStartValue;
  mIncrementMult = 1.0f;
  mRampLinear = true;
  mRampSetActive = true;

}


void IasRamp::setTimedRampExponential(float startValue, float endValue, uint32_t rampTime)
{
  float temp=0;
  float tempEnd, tempStart;
  mStartValue = startValue;
  mEndValue   = endValue;
  mRampTime   = rampTime;

  if(startValue != endValue)
  {
    if(startValue == 0)
    {
      tempStart = 6.309573444801930e-8f; //this is -144 dB
      mEndValue = endValue;
      mDelta = mEndValue - tempStart;
      mNumRampValues = static_cast<uint32_t>(floor(static_cast<float>(mRampTime) * 0.001f *
                                                      static_cast<float>(mSampleFreq)));
      mStartValue = startValue;
      mCurrentValue = tempStart;

      temp = (1.0f)/((float)mNumRampValues);
      mIncrementMult = (double) pow((float)fabs(mEndValue/tempStart),temp);
    }
    else if(endValue == 0)
    {
      tempEnd =  6.309573444801930e-8f; //this is -144 dB
      mStartValue = startValue;
      mDelta = tempEnd - mStartValue;
      mNumRampValues = static_cast<uint32_t>(floor(static_cast<float>(mRampTime) * 0.001f *
                                                      static_cast<float>(mSampleFreq)));
      mEndValue = endValue;
      mCurrentValue = mStartValue;

      temp = (1.0f)/((float)mNumRampValues);
      mIncrementMult = (double) pow((float)fabs(tempEnd/mStartValue),temp);
    }
    else
    {
      mStartValue = startValue;
      mEndValue = endValue;
      mDelta = mEndValue - mStartValue;
      mNumRampValues = static_cast<uint32_t>(floor(static_cast<float>(mRampTime) * 0.001f *
                                                      static_cast<float>(mSampleFreq)));
      mCurrentValue = mStartValue;
      temp = (1.0f)/((float)mNumRampValues);
      mIncrementMult = (double) pow((float)fabs(mEndValue/mStartValue),temp);
    }
  }
  else
  {
    mNumRampValues = 0;
    mDelta = 0;
    mIncrementMult = 0;
  }
  mRampLinear = false;
  mRampSetActive = true;
  mIncrementAdd = 0.0f;
}



IasRampErrors IasRamp::getRampValues(float* data, uint32_t channelIndex,
                                     uint32_t numChannels, uint32_t* numValuesLeftToRamp)
{

  if ( (data != NULL) &&
       (numValuesLeftToRamp != NULL) &&
       (channelIndex < cIasNumChannelsPerBundle) &&
       (numChannels <= cIasNumChannelsPerBundle))
  {
    if( mRampSetActive == true)
    {
      float* work_data = & (data[channelIndex]);
      uint32_t i = 0;
      uint32_t loopSize = 0;

      if(mNumRampValues >= mFrameSize)
      {
        loopSize = mFrameSize;
      }
      else
      {
        loopSize = mNumRampValues;
      }
      i= 0;

      //writing the increments to the bundle
      if(loopSize != 0)
      {
        while(i < (loopSize-1))
        {
          mCurrentValue += mIncrementAdd;
          mCurrentValue *= mIncrementMult;
          if((mCurrentValue > mEndValue) && (mStartValue < mEndValue))
          {
            mCurrentValue = mEndValue;
          }
          else if((mCurrentValue < mEndValue) && (mStartValue > mEndValue))
          {
            mCurrentValue = mEndValue;
          }
          for(uint32_t j=0;j<numChannels;j++)
          {
            *work_data++ = static_cast<float>( mCurrentValue);
          }
          work_data += (4-numChannels);
          mNumRampValues--;
          i++;
        }
        //last steps
        mNumRampValues--;
        mCurrentValue += mIncrementAdd;
        mCurrentValue *= mIncrementMult;
        if((mCurrentValue > mEndValue) && (mStartValue < mEndValue))
        {
          mCurrentValue = mEndValue;
        }
        else if((mCurrentValue < mEndValue) && (mStartValue > mEndValue))
        {
          mCurrentValue = mEndValue;
        }
        if(mNumRampValues == 0)
        {
          if(mCurrentValue != mEndValue)
          {
            mCurrentValue = mEndValue;
          }
        }
        for(uint32_t j=0;j<numChannels;j++)
        {
          *work_data++ = static_cast<float>(mCurrentValue);
        }
        work_data += (4-numChannels);
        i++;
      }

      //if there are still some bundle values to fill, check if the desired value was reached
      //if not, set to desired value
      if(i< mFrameSize)
      {
        if (mCurrentValue != mEndValue)
        {
          mCurrentValue = mEndValue;
        }
      }
      //fill up the rest of the bundle
      while(i < mFrameSize)
      {
        for(uint32_t j=0;j<numChannels;j++)
        {
          *work_data++ = static_cast<float>(mCurrentValue);
        }
        work_data += (4-numChannels);
        i++;
      }
      *numValuesLeftToRamp = mNumRampValues;

      return eIasRampErrorNoError;
    }
    else
    {
      return eIasRampErrorRampNotSet;
    }
  }
  else
  {
    return eIasRampErrorDataPointer;
  }
}


IasRampErrors IasRamp::getRampValues(float* data)
{
  if (data == NULL)
  {
    return eIasRampErrorDataPointer;
  }
  if( mRampSetActive == false)
  {
    return eIasRampErrorRampNotSet;
  }

  uint32_t i = 0;
  uint32_t loopSize = mNumRampValues;

  if(mNumRampValues >= mFrameSize)
  {
    loopSize = mFrameSize;
  }

  //writing the increments to the bundle
  if(loopSize > 0)
  {
    while(i < loopSize)
    {
      mCurrentValue += mIncrementAdd;
      mCurrentValue *= mIncrementMult;
      mNumRampValues--;

      if((mCurrentValue > mEndValue && mStartValue < mEndValue) ||
         (mCurrentValue < mEndValue && mStartValue > mEndValue) ||
         (mNumRampValues == 0 && mCurrentValue != mEndValue))
      {
        mCurrentValue = mEndValue;
      }
      *data++ = static_cast<float>( mCurrentValue);
      i++;
    }
  }

  //if there are still some bundle values to fill, check if the desired value was reached
  //if not, set to desired value
  if(i < mFrameSize)
  {
    if (mCurrentValue != mEndValue)
    {
      mCurrentValue = mEndValue;
    }
  }
  //fill up the rest of the bundle
  while(i < mFrameSize)
  {
    *data++ = static_cast<float>(mCurrentValue);
    i++;
  }

  return eIasRampErrorNoError;
}


IasRampErrors IasRamp::getRampNewValue(float* data)
{
  if ( data != NULL )
  {
    if(mNumRampValues)
    {
      mCurrentValue += mIncrementAdd;
      mCurrentValue *= mIncrementMult;
      *data = static_cast<float>(mCurrentValue);
      mNumRampValues--;
    }
    if(mNumRampValues == 0)
    {
      *data = mEndValue;
    }
    return eIasRampErrorNoError;
  }
  else
  {
    return eIasRampErrorDataPointer;
  }
}

} // namespace IasAudio
