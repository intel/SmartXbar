/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasAlsaHandler.hpp
 * @date   2015
 * @brief
 */

#ifndef IASALSAHANDLER_HPP_
#define IASALSAHANDLER_HPP_


#include "audio/common/IasAudioCommonTypes.hpp"
#include "alsahandler/IasAlsaHandlerWorkerThread.hpp"


// Forward declarations (usually defined in alsa/asoundlib.h).
typedef struct _snd_pcm    snd_pcm_t;
typedef struct _snd_output snd_output_t;


namespace IasAudio {

class IasAudioRingBuffer;

class IAS_AUDIO_PUBLIC IasAlsaHandler
{
  public:
    /**
     * @brief  Result type of the class IasAlsaHandler.
     */
    enum IasResult
    {
      eIasOk,               //!< Ok, Operation successful
      eIasInvalidParam,     //!< Invalid parameter, e.g., out of range or NULL pointer
      eIasInitFailed,       //!< Initialization of the component failed
      eIasNotInitialized,   //!< Component has not been initialized appropriately
      eIasAlsaError,        //!< Error reported by the ALSA device
      eIasTimeOut,          //!< Time out occured
      eIasRingBufferError,  //!< Error reported by the ring buffer
      eIasFailed,           //!< Other error (operation failed)
    };

    /**
     * @brief  Type definition of the direction in which the ALSA handler operates.
     */
    enum IasDirection
    {
      eIasDirectionUndef = 0, //!< Undefined
      eIasDirectionCapture,   //!< Capture direction
      eIasDirectionPlayback,  //!< Playback direction
    };

    /**
     * @brief Constructor.
     *
     * @param[in] params  Pointer (shared pointer) to the audio device configuration.
     */
    IasAlsaHandler(IasAudioDeviceParamsPtr params);

    /**
     * @brief Destructor, virtual by default.
     */
    virtual ~IasAlsaHandler();

    /**
     * @brief Initialization function.
     *
     * @param[in] deviceType The device type of the created audio device to which the ALSA handler belongs
     *
     * @returns IasAlsaHandler::IasResult::eIasOk on success, otherwise an error code.
     */
    IasResult init(IasDeviceType deviceType);

    /**
     * @brief Start the ALSA handler.
     *
     * @returns IasAlsaHandler::IasResult::eIasOk on success, otherwise an error code.
     */
    IasResult start();

    /**
     * @brief Stop the ALSA handler.
     */
    void stop();

    /**
     * @brief Get a handle to the ring buffer used by the AlsaHandler.
     *
     * @returns    IasAlsaHandler::IasResult::eIasOk on success, otherwise an error code.
     * @param[out] ringBuffer  Returns the handle to the ring buffer.
     */
    IasResult getRingBuffer(IasAudioRingBuffer **ringBuffer) const;

    /**
     * @brief Get a the period size used by the AlsaHandler.
     *
     * @returns    IasAlsaHandler::IasResult::eIasOk on success, otherwise an error code.
     * @param[out] periodSize  Returns the period size.
     */
    IasResult getPeriodSize(uint32_t *periodSize) const;

    /**
     * @brief Let the ALSA handler operate in non-blocking mode.
     *
     * In non-blocking mode, the method updateAvailable() does not wait for the output buffer
     * to provide enough free PCM frames for one period. The default behavior of the ALSA
     * handler (if this function is not called) is the blocking mode.
     *
     * @returns                 IasAlsaHandler::IasResult::eIasOk on success, otherwise an error code.
     *
     * @param[in] isNonBlocking Activates (if true) or deactivates (if false) the non-blocking mode.
     */
    IasResult setNonBlockMode(bool isNonBlocking);

    /**
     * @brief Reset the states of the ALSA handler
     *
     * This is mainly used to reset the sample rate converter states inside the ALSA handler.
     * It can be enhanced to do further resets of internal states if required.
     */
    void reset();

  private:

    /**
     * @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasAlsaHandler(IasAlsaHandler const &other);

    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasAlsaHandler& operator=(IasAlsaHandler const &other);

    /**
     * @brief  Set the hardware parameters of an ASLA PCM device.
     *
     * @param[in,out] pcmHandle            Handle of the ALSA PCM device
     * @param[in]     format               Format of the PCM samples
     * @param[in]     rate                 Sample rate
     * @param[in]     channels             Number of channels
     * @param[in]     numPeriods           Number of periods that shall be buffered
     * @param[in]     period_size          Length of each period, expressed in PCM frames
     * @param[out]    actualBufferSize     Returned actual buffer size
     * @param[out]    actualPeriodSize     Returned actual period length
     * @param[in,out] sndLogger            Handle to the debug output for ALSA
     */
    int set_hwparams(snd_pcm_t                *pcmHandle,
                     IasAudioCommonDataFormat  dataFormat,
                     unsigned int              rate,
                     unsigned int              channels,
                     unsigned int              numPeriods,
                     unsigned int              period_size,
                     int32_t               *actualBufferSize,
                     int32_t               *actualPeriodSize);

    int set_swparams(snd_pcm_t   *pcmHandle,
                     int32_t   bufferSize,
                     int32_t   periodSize);

    DltContext                   *mLog;                //!< Handle for the DLT log object
    IasAudioDeviceParamsPtr       mParams;             //!< Pointer to device params
    IasDeviceType                 mDeviceType;         //!< Device type: eIasDeviceTypeSource or eIasDeviceTypeSink
    IasAudioRingBuffer           *mRingBuffer;         //!< Handle of the ring buffer
    IasAudioRingBuffer           *mRingBufferAsrc;     //!< Handle of the ring buffer (jitter buffer for ASRC)
    snd_pcm_t                    *mAlsaHandle;         //!< Handle of the ALSA device
    uint32_t                   mBufferSize;         //!< Total buffer size (i.e., mPeriodSize * mParams->numPeriods)
    uint32_t                   mPeriodSize;         //!< Period size, expressed in PCM frames
    uint32_t                   mPeriodTime;         //!< Period time, expressed in microseconds
    snd_output_t                 *mSndLogger;          //!< Handle of the debug output for ALSA
    int32_t                    mTimeout;            //!< Timeout [ms] for snd_pcm_wait, or -1 for infinity
    uint32_t                   mTimevalUSecLast;    //!< Time stamp [us] when last call of updateAvailable() was completed
    bool                     mIsAsynchronous;     //!< true, if asynchronous sample rate conversion needs to be performed
    IasAlsaHandlerWorkerThreadPtr mWorkerThread;       //!< Handle for the workerthread object
};


/**
 * @brief Function to get a IasAlsaHandler::IasResult as string.
 *
 * @return String carrying the result message.
 */
std::string toString(const IasAlsaHandler::IasResult& type);


} //namespace IasAudio

#endif /* IASALSAHANDLER_HPP_ */
