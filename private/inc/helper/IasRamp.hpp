/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file IasRamp.hpp
 * @date 2012
 * @brief the header for the ramp generator
 */

#ifndef IASRAMP_HPP_
#define IASRAMP_HPP_


#include "rtprocessingfwx/IasAudioChannelBundle.hpp"
#include "audio/smartx/rtprocessingfwx/IasProcessingTypes.hpp"

namespace IasAudio {

/**
 * @brief enum for the possible return values
 */
typedef enum
{
  eIasRampErrorNoError = 0,
  eIasRampErrorDataPointer,
  eIasRampErrorRampNotSet,
  eIasRampErrorZeroFadeTime,
  eIasRampErrorZeroDelta,
  eIasRampErrorUnknownRampShape,
  eIasRampErrorInitParams
}IasRampErrors;

/** \class IasRamp
 *  This is the class for the ramp generator
 */
class IAS_AUDIO_PUBLIC IasRamp
{
  public:
    /*!
     *  @brief Constructor.
     */
    IasRamp(uint32_t sampleFreq, uint32_t frameSize);

    /*!
     *  @brief Destructor, virtual by default.
     */
    virtual ~IasRamp();

    /**
     * @brief the function returns the current ramp value as linear value
     * @param currentValue pointer for the current value
     */
    inline void getCurrentValue(float* currentValue)
    {
      *currentValue = static_cast<float>(mCurrentValue);
    }

    /*
     * @brief This public function sets the parameter for a time based fade ramp
     *        It also activates the ramp so that the ramp values can be ordered by any other module
     *
     * @param startValue the value the fade ramp begins
     * @param endValue   the destination value of the ramp when it's finished
     * @param rampTime   the duration of the ramp in ms (when it has reached its end value)
     * @param fadingSHape the shape of the fade ramp (linear or exponential)
     *
     * @return nonzero in case of a problem.
     */
    IasRampErrors setTimedRamp(float startValue, float endValue,
                               uint32_t rampTime, IasRampShapes fadingShape );

    /*
     * @brief This function returns a frame of ramp values as part of the active ramp. It also returns the
     *        number of samples that still have to be ramped to finish the ramping.
     *
     * @param data                 The data pointer passed to the function. It gets filled with ramp values.
     *                             The pointer points to an array of the size of a bundle
     * @param numSamplesLeftToRamp the number of samples that still will be ramped
     *
     * @return nonzero in case of a problem
     */
    IasRampErrors getRampValues(float* data, uint32_t channelIndex,
                                uint32_t numChannels, uint32_t* numValuesLeftToRamp);

    /*
     * @brief This function returns a frame of ramp values as part of the active ramp.
     *
     * @param data                 The data pointer passed to the function. It gets filled with ramp values.
     *                             The pointer points to an array of the size of a frame
     *
     * @return nonzero in case of a problem
     */
    IasRampErrors getRampValues(float* data);

    /*
     * @brief This function returns one sample of a ramp.If no ramp is active, the desired end value is returned
     *
     * @param data                 The data pointer passed to the function. It gets filled with the ramp value.
     *                             The pointer points to a single data sample
     *
     * @return nonzero in case of a problem
     */
    IasRampErrors getRampNewValue(float* data);

    inline uint32_t getNumSamples2Ramp(){return mNumRampValues;};

  private:
    /*!
     *  @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasRamp(IasRamp const &other); //lint !e1704

    /*!
     *  @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasRamp& operator=(IasRamp const &other); //lint !e1704

    /*
     * @brief This function sets the parameter for time based linear fade ramp
     *
     * @param startValue the value the fade ramp begins
     * @param endValue   the destination value of the ramp when it's finished
     * @param rampTime   the duration of the ramp in ms (when it has reached its end value)
     *
     * @return nonzero in case of a problem.
     */
    void setTimedRampLinear(float startValue, float endValue, uint32_t rampTime);

    /*
     * @brief This function sets the parameter for time based exponential fade ramp
     *
     * @param startValue the value the fade ramp begins
     * @param endValue   the destination value of the ramp when it's finished
     * @param rampTime   the duration of the ramp in ms (when it has reached its end value)
     *
     * @return nonzero in case of a problem.
     */
    void setTimedRampExponential(float startValue, float endValue, uint32_t rampTime);


    uint32_t  mSampleFreq;
    uint32_t  mFrameSize;
    float mStartValue;
    float mEndValue;
    uint32_t  mRampTime;
    float mDelta;
    float mIncrementAdd;
    double mIncrementMult;
    double mCurrentValue;
    uint32_t  mNumRampValues;
    bool    mRampSetActive;
    bool    mRampLinear;
};

} //namespace IasAudio

#endif /* IASRAMP_HPP_ */
