/*
  * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file    IasAudioFilter.hpp
 * @brief   Audio Filter Library.
 * @date    October 19, 2012
 */

#ifndef IASAUDIOFILTER_HPP_
#define IASAUDIOFILTER_HPP_

// disable conversion warnings for tbb
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#include "tbb/tbb.h"
// turn the warnings back on
#pragma GCC diagnostic pop


#include "rtprocessingfwx/IasAudioChannelBundle.hpp"
#include "audio/smartx/rtprocessingfwx/IasProcessingTypes.hpp" // typedef IasAudioFilterTypes
#include "IasAudioFilterCallback.hpp"

namespace IasAudio {

static const uint32_t cIasNumStateVarsBiquad = 4;
static const uint32_t cIasNumCoeffsBiquad = 5;


/*!
 *  @brief Struct defining the parameters of the filter stage.
 *
 *  The filter stage is implemented as a Biquad filter (2nd order IIR
 *  filter in Direct Form 1). Higher-order Butterworth filters can be
 *  implemented by cascading several stages of this Biquad. In this case,
 *  the parameter @a order has to be set accordingly (e.g. order=5 for a
 *  5th order Butterworh filter) and the parameter @a section has to be
 *  iterated from 1 to 3 for the three filter sections.
 */
struct IasAudioFilterParams
{
  uint32_t         freq;         //!< cut-off frequency or mid frequency of the filter
  float        gain;         //!< gain (linear, not in dB), required only for peak and shelving filters
  float        quality;      //!< quality, required only for band-pass and peak filters
  IasAudioFilterTypes type;         //!< filter type; the type IasAudioFilterTypes is defined in IasProcessingTypes.hpp
  uint32_t         order;        //!< filter order
  uint32_t         section;      //!< section that shall be implemented, required only for higher-order filters (order>2)
};


/*!
 *  @brief Struct defining the additional (internal) parameters of the filter stage.
 *
 *  These parameters act as a supplement to IasAudioFilterParams.
 */
struct IasAudioFilterParamsInternal
{
  float  preWarpedFreq;      //!< Pre-warped, normalized cut-off frequency, K = tan(pi*fc/fs).
  float  factorRampUp;       //!< Factor for increasing the filter gain.
  float  factorRampDown;     //!< Factor for decreasing the filter gain.
  bool     useDoublePrecision; //!< True, if double precision shall be used.
};


/*!
 *  @brief Struct comprising all channel-specific variables that are used by
 *         the processing thread, i.e., by the method IasAudioFilter::calculate.
 *         These variables must not be accessed by the command thread.
 */
struct IasAudioFilterProcessingParams
{
  float  gainCurrent;        //!< Current gain in linear representation (not in dB).
  float  gainTarget;         //!< Target gain, if gain ramping is used.
  float  preWarpedFreq;      //!< Pre-warped frequency, tan(pi*fc/fs)
  float  quality;            //!< Filter quality (Q value).
  IasAudioFilterTypes type;         //!< Filter type.
  uint32_t   order;              //!< Filter order.
  uint32_t   section;            //!< Section, only for higher-order filters (order > 2).
  float  factorRampUp;       //!< Factor for increasing the filter gain.
  float  factorRampDown;     //!< Factor for decreasing the filter gain.
  uint64_t   callbackUserData;   //!< User data that will be returned by the callback function.
  bool     useDoublePrecision; //!< True, if double precision shall be used.
  bool     isRamping;          //!< True, while gain ramping is ongoing.
};


/*!
 *  @brief Struct comprising all parameters that are required for a queued
 *         update of one filter stage.
 */
struct IasAudioFilterQueueEntry
{
  // Member variables that are used for all types of updates.
  uint32_t   channel;            //!< Index of the channel whose filter shall be updated.
  bool     rampedUpdate;       //!< Ramped update or immediate update.
  float  gainNew;            //!< New gain (after the update).

  // Member variables that are used only for ramped updates.
  float  preWarpedFreq;      //!< Pre-warped frequency, tan(pi*fc/fs)
  float  quality;            //!< Filter quality (Q value).
  IasAudioFilterTypes type;         //!< Filter type.
  uint32_t   order;              //!< Filter order.
  uint32_t   section;            //!< Section, only for higher-order filters (order > 2).
  float  factorRampUp;       //!< Factor for increasing the filter gain.
  float  factorRampDown;     //!< Factor for decreasing the filter gain.
  uint64_t   callbackUserData;   //!< User data that will be returned by the callback function.

  // Member variables that are used only for non-ramped (immediate) updates.
  float  biquadCoeffs32[cIasNumCoeffsBiquad]; //!< Biquad coefficients as Float32.
  double  biquadCoeffs64[cIasNumCoeffsBiquad]; //!< Biquad coefficients as Float64.
  bool     useDoublePrecision;                  //!< True, if double precision shall be used.
  bool     clearStates;                         //!< True, if the filter variables shall be cleared.
};


/*
 *  @brief Minimum values for the filter parameters.
 */
extern IAS_AUDIO_PUBLIC const IasAudioFilterParams cIasAudioFilterParamsMinValues;


/*
 *  @brief Maximum values for the filter parameters.
 */
extern IAS_AUDIO_PUBLIC const IasAudioFilterParams cIasAudioFilterParamsMaxValues;


/*!
 *  @brief Public helper function for verifying that a given filter
 *         parameter set is valid.
 *
 *  The parameter filterParams.section is not verified by this function,
 *  because this parameter is not used for software layers above the
 *  core equalizer.
 *
 *  @return                   Error value, -1 in case of invalid parameters.
 *  @param[in]  filterParams  Filter parameters that shall be verified.
 *  @param[in]  sampleFreq    Sampling rate, expressed in Hz.
 */
int32_t  IasAudioFilterCheckParams(IasAudioFilterParams const &filterParams,
                                      uint32_t                 sampleFreq);


/*!
 *  @class IasEqualizerCore
 *  @brief This class implements the Audio Filter Module.
 */
class IAS_AUDIO_PUBLIC IasAudioFilter
{
  public:
    /*!
     *  @brief Constructor.
     */
    IasAudioFilter(uint32_t sampleFreq, uint32_t frameLength);

    /*!
     *  @brief Destructor, virtual by default.
     */
    virtual ~IasAudioFilter();

    /*!
     * @brief Initialization function, must be called after the filter has been created.
     */
    IasAudioProcessingResult init();

    /*!
     * @brief Reset function.
     *
     * This function clears all filter state variables. The filter parameters are not touched.
     */
    int32_t reset();

    /*!
     * @brief Execute the filter for one frame.
     */
    void calculate();

    /*!
     * @brief Tell the filter on which bundle it shall operate on.
     */
    inline void setBundlePointer(IasAudioChannelBundle* bundle){mBundle = bundle;}

    /*!
     * @brief Set the filter parameters of one single channel.
     *
     * The filter parameters are set immediately, i.e., the parameters are not
     * ramped. Furthermore, the filter state variables of the channel are cleared
     * in order to guarantee the stability of the updated filter.
     * The other channels of the bundle are not touched.
     *
     * If a ramped update of the filter gain is still ongoing while this function is
     * called, the announced callback function is executed in order to signalize that
     * the ramp has been terminated due to the immediate parameter update.
     *
     * @return             Error value.
     * @retval 0           Success!
     * @retval -1          Invalid parameters.
     *
     * @param[in] channel  Channel, whose parameters shall be set. Must be < 4.
     * @param[in] params   Filter parameters, which shall be used.
     */
    int32_t setChannelFilter(uint32_t channel, IasAudioFilterParams const * params );

    /*!
     * @brief Update the gain of the filter.
     *
     * The use of this function is reasonable only for peak filters or shelving
     * filters. For any other filter type, this function does not have any effect.
     *
     * In contrast to the function IasAudioFilter::rampGain, this function does
     * not apply a ramped (smoothed) update of the filter gain. Instead, the
     * gain is updated immediately. This function does not clear the filter states.
     *
     * If a ramped update of the filter gain is still ongoing while this function is
     * called, the announced callback function is executed in order to signalize that
     * the ramp has been terminated due to the immediate gain update.
     *
     * @return             Error value.
     * @retval 0           Success!
     * @retval -1          Invalid parameters.
     *
     * @param[in] channel  Channel, whose gain shall be updated. Must be < 4.
     * @param[in] gain     Gain, in linear representation (not in dB).
     */
    int32_t updateGain(uint32_t channel, float gain);

    /*!
     * @brief Ramp the gain of the filter conitunously till the specified
     *        target gain is reached.
     *
     * The use of this function is reasonable only for peak filters or shelving
     * filters. For any other filter type, this function does not have any effect.
     *
     * During the ramped update of the filter gain, the filter states are not
     * cleared in order to guarantee a smooth audio experience.
     *
     * If a callback function has been declared before (by means of the function
     * IasAudioFilter::announceCallback), the announced callback function will be
     * executed as soon as the ramp generator reaches the specified gain (target
     * gain).
     *
     * @return                     Error value.
     * @retval 0                   Success!
     * @retval -1                  Invalid parameters.
     *
     * @param[in] channel          Channel, whose gain shall be updated. Must be < 4.
     * @param[in] gain             Target gain, in linear representation (not in dB).
     * @param[in] callbackUserData UserData, which will be returned when the callback
     *                             function is executed. The caller can use this field
     *                             to identify which filter has caused the callback.
     */
    int32_t rampGain(uint32_t  channel,
                        float gain,
                        uint64_t  callbackUserData);

    /*!
     * @brief Set the gradient that is used for ramping the filter gain.
     *
     * The gradient specifies by how many dB the filter gain is ramped up/down
     * for each audio frame.
     *
     * @return              Error value.
     * @retval 0            Success!
     * @retval -1           Invalid parameters.
     *
     * @param[in] channel   Channel, whose ramp gardient shall be updated.
     *                      Must be < 4.
     * @param[in] gradient  Gain increase/decrease, expressed in dB per frame.
     *                      Must be between 0.01 dB and 6.0 dB.
     */
    int32_t setRampGradient(uint32_t channel, float gradient);

    /*!
     * @brief Announce the callback function that shall be executed if the
     *        gain ramping has reached its target value.
     *
     * @param[in] callback  Pointer to the callback class. Or NULL if the
     *                      callback mechanism shall be deactivated.
     */
    void announceCallback(IasAudioFilterCallback* callback);


  private:
    /*!
     *  @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasAudioFilter(IasAudioFilter const &other); //lint !e1704

    /*!
     *  @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasAudioFilter& operator=(IasAudioFilter const &other); //lint !e1704

    uint32_t                     mSampleFreq;
    uint32_t                     mFrameLength;
    float                   *mStateVarsBundle;
    float                  **mStateVars;
    float                   *mCoeffsBundle;
    float                  **mCoeffs;
    double                   *mStateVarsBundle64;
    double                  **mStateVars64;
    double                   *mCoeffsBundle64;
    double                  **mCoeffs64;
    IasAudioChannelBundle          *mBundle;
    IasAudioFilterProcessingParams  mProcessingParams[cIasNumChannelsPerBundle];
    IasAudioFilterParams            mFilterParams[cIasNumChannelsPerBundle];
    IasAudioFilterParamsInternal    mFilterParamsInternal[cIasNumChannelsPerBundle];
    IasAudioFilterCallback         *mCallback;
    tbb::concurrent_queue<IasAudioFilterQueueEntry> filterUpdateQueue;
    DltContext                  *mLogContext;                       //!< The log context for the audio filter
};

} //namespace IasAudio

#endif /* IASAUDIOFILTER_HPP_ */
