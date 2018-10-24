/*
  * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file   IasAlsaHandler.cpp
 * @date   2015
 * @brief
 */

#include <string.h>
#include <time.h>
#include <alsa/asoundlib.h>

#include "internal/audio/common/IasAudioLogging.hpp"
#include "internal/audio/common/IasAlsaTypeConversion.hpp"
#include "internal/audio/common/audiobuffer/IasAudioRingBuffer.hpp"
#include "internal/audio/common/audiobuffer/IasAudioRingBufferFactory.hpp"
#include "alsahandler/IasAlsaHandlerWorkerThread.hpp"
#include "alsahandler/IasAlsaHandler.hpp"
#include "model/IasRoutingZone.hpp"

namespace IasAudio {

// String with class name used for printing DLT_LOG messages.
static const std::string cClassName = "IasAlsaHandler::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"
#define LOG_DEVICE "device=" + mParams->name + ":"

IasAlsaHandler::IasAlsaHandler(IasAudioDeviceParamsPtr params)
  :mLog(IasAudioLogging::registerDltContext("AHD", "ALSA Handler"))
  ,mParams(params)
  ,mDeviceType(eIasDeviceTypeUndef)
  ,mRingBuffer(nullptr)
  ,mRingBufferAsrc(nullptr)
  ,mAlsaHandle(nullptr)
  ,mBufferSize(0)
  ,mPeriodSize(0)
  ,mPeriodTime(0)
  ,mSndLogger(nullptr)
  ,mTimeout(-1)
  ,mTimevalUSecLast(0)
  ,mWorkerThread(nullptr)
{
  IAS_ASSERT(mParams != nullptr);
  mIsAsynchronous = (mParams->clockType == eIasClockReceivedAsync);
}


IasAlsaHandler::~IasAlsaHandler()
{
  this->stop();

  if (mWorkerThread != nullptr)
  {
    mWorkerThread.reset();
  }

  IasAudioRingBufferFactory* ringBufferFactory = IasAudioRingBufferFactory::getInstance();
  ringBufferFactory->destroyRingBuffer(mRingBuffer);
  mRingBuffer = nullptr;

  DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX, LOG_DEVICE);
}


IasAlsaHandler::IasResult IasAlsaHandler::getRingBuffer(IasAudioRingBuffer **ringBuffer) const
{
  if (ringBuffer == nullptr)
  {
    return eIasInvalidParam;
  }

  // The interface buffer, which is used by the switch matrix, is either the
  // mirror buffer (if the ALSA handler works synchronously) or the ASRC buffer
  // (if the ALSA handler works asynchronously).
  *ringBuffer = mIsAsynchronous ? mRingBufferAsrc : mRingBuffer;
  if (*ringBuffer == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Invalid nullptr pointer (due to missing initialization?)");
    return eIasNotInitialized;
  }
  return eIasOk;
}


IasAlsaHandler::IasResult IasAlsaHandler::getPeriodSize(uint32_t *periodSize) const
{
  if (periodSize == nullptr)
  {
    return eIasInvalidParam;
  }

  *periodSize = mPeriodSize;
  return eIasOk;
}

IasAlsaHandler::IasResult IasAlsaHandler::init(IasDeviceType deviceType)
{
  IAS_ASSERT(mParams != nullptr);

  if ((deviceType != eIasDeviceTypeSource) && (deviceType != eIasDeviceTypeSink))
  {
    return eIasInitFailed;
  }

  mDeviceType = deviceType;

  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_DEVICE, "Initialization of ALSA handler.");
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_DEVICE, "Stream parameters are:", mParams->samplerate, "Hz,", mParams->numChannels, "channels,", toString(mParams->dataFormat), ", periodSize:", mParams->periodSize, ", numPeriods:", mParams->numPeriods);

  int err = snd_output_stdio_attach(&mSndLogger, stdout, 0);
  if (err < 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error in snd_output_stdio_attach:", snd_strerror(err));
    return eIasInitFailed;
  }

  IasAudioRingBufferFactory* ringBufferFactory = IasAudioRingBufferFactory::getInstance();

  // Create the ring buffer that is connected to the ALSA device (mirror buffer).
  std::string ringBufName = "IasAlsaHandler_" + mParams->name;
  IasAudioCommonResult result = ringBufferFactory->createRingBuffer(&mRingBuffer,
                                                                    0, // period size, not required for type eIasRingBufferLocalMirror
                                                                    mParams->numPeriods,
                                                                    mParams->numChannels,
                                                                    mParams->dataFormat,
                                                                    eIasRingBufferLocalMirror,
                                                                    ringBufName);

  if ((mRingBuffer == nullptr) || (result != IasAudioCommonResult::eIasResultOk))
  {
    /**
     * @log The initialization parameter dataFormat of the IasAudioDeviceParams structure for device with name <NAME> is invalid.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error in IasAudioRingBufferFactory::createRingBuffer:", toString(result));
    return eIasInitFailed;
  }

  if (mIsAsynchronous)
  {
    if (mParams->numPeriodsAsrcBuffer < 1)
    {
      /**
       * @log The initialization parameter numPeriodsAsrcBuffer of the IasAudioDeviceParams structure for device with name <NAME> is invalid.
       */
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Invalid parameter numPeriodsAsrcBuffer (must be >= 1):", mParams->numPeriodsAsrcBuffer);
      return eIasInitFailed;
    }

    // Create the ASRC ring buffer (real buffer).
    std::string ringBufNameAsrc = "IasAlsaHandler_" + mParams->name + "_asrc";
    result = ringBufferFactory->createRingBuffer(&mRingBufferAsrc,
                                                 mParams->periodSize,
                                                 mParams->numPeriodsAsrcBuffer,
                                                 mParams->numChannels,
                                                 mParams->dataFormat,
                                                 eIasRingBufferLocalReal,
                                                 ringBufNameAsrc);
    if ((mRingBufferAsrc == nullptr) || (result != IasAudioCommonResult::eIasResultOk))
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error in IasAudioRingBufferFactory::createRingBuffer:", toString(result));
      return eIasInitFailed;
    }

    IasAlsaHandlerWorkerThread::IasAudioBufferParams deviceBufferParams(mRingBuffer, mParams->numChannels,
                                                                        mParams->dataFormat, mParams->periodSize,
                                                                        mParams->numPeriods);
    IasAlsaHandlerWorkerThread::IasAudioBufferParams asrcBufferParams(mRingBufferAsrc, mParams->numChannels,
                                                                      mParams->dataFormat, mParams->periodSize,
                                                                      mParams->numPeriodsAsrcBuffer);

    // Create an object containing the parameters for initializing the worker thread object.
    IasAlsaHandlerWorkerThread::IasAlsaHandlerWorkerThreadParamsPtr workerThreadParams
      = std::make_shared<IasAlsaHandlerWorkerThread::IasAlsaHandlerWorkerThreadParams>(mParams->name,
                                                                                       mParams->samplerate,
                                                                                       deviceBufferParams,
                                                                                       asrcBufferParams);

    // Create and initialize the worker thread object.
    if (mWorkerThread == nullptr)
    {
      mWorkerThread = std::make_shared<IasAlsaHandlerWorkerThread>(workerThreadParams);
      IAS_ASSERT(mWorkerThread != nullptr);
      IasAlsaHandlerWorkerThread::IasResult result = mWorkerThread->init(deviceType);
      IAS_ASSERT(result == IasAlsaHandlerWorkerThread::eIasOk);
      (void)result;
    }
  }
  return eIasOk;
}


IasAlsaHandler::IasResult IasAlsaHandler::start()
{
  if (mRingBuffer == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error, mRingBuffer is nullptr");
    return eIasNotInitialized;
  }

  snd_pcm_stream_t alsaStreamDirection;
  switch (mDeviceType)
  {
    case eIasDeviceTypeSource:
      alsaStreamDirection = SND_PCM_STREAM_CAPTURE;
      break;
    case eIasDeviceTypeSink:
      alsaStreamDirection = SND_PCM_STREAM_PLAYBACK;
      break;
    default:
      /**
       * @log Error when trying to open the ALSA device <NAME>.
       *      Either the name is incorrect or the device does not exist yet.
       */
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error, stream direction is not defined");
      return eIasInvalidParam;
  }

  int err = snd_pcm_open(&mAlsaHandle, mParams->name.c_str(), alsaStreamDirection, 0);
  if (err < 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error in snd_pcm_open:", snd_strerror(err));
    return eIasInvalidParam;
  }

  int32_t  actualBufferSize = 0;
  int32_t  actualPeriodSize = 0;
  err = set_hwparams(mAlsaHandle,
                     mParams->dataFormat, mParams->samplerate, mParams->numChannels,
                     mParams->numPeriods, mParams->periodSize,
                     &actualBufferSize, &actualPeriodSize);
  if (err < 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error in private function set_hwparams:", snd_strerror(err));
    return eIasInvalidParam;
  }

  err = set_swparams(mAlsaHandle, actualBufferSize, actualPeriodSize);
  if (err < 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error in private function set_swparams:", snd_strerror(err));
    return eIasInvalidParam;
  }

  mPeriodSize = static_cast<uint32_t>(actualPeriodSize);    // expressed in PCM frames
  mPeriodTime = static_cast<uint32_t>((static_cast<uint64_t>(mPeriodSize) * 1000000) / mParams->samplerate); // microseconds

  // Timeout after 10 * period size (expressed in milliseconds). This rather 'huge' timeout is required for systems were many real-time scheduled threads are running
  // and the system might not be fine-tuned correctly.
  mTimeout    = (10 * mPeriodTime) / 1000;

  mBufferSize = mParams->numPeriods * mPeriodTime;


  // Assign the ALSA device handle to the ring buffer.
  IasAudioRingBufferResult ringBufRes = mRingBuffer->setDeviceHandle(mAlsaHandle, mPeriodSize, mTimeout);
  if (ringBufRes != eIasRingBuffOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error in IasAudioRingBuffer::setDeviceHandle:", toString(ringBufRes));
    return eIasRingBufferError;
  }


  if (mIsAsynchronous)
  {
    if (mWorkerThread == nullptr)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error, ALSA handler has not been initialized (mWorkerThread == nullptr)");
      return eIasNotInitialized;
    }

    // reset the ASRC buffer, so we always start with a clean buffer in case we were stopped and started again in the same lifecycle
    mRingBufferAsrc->resetFromReader();

    // Start the worker thread.
    IasAlsaHandlerWorkerThread::IasResult ahwtResult = mWorkerThread->start();
    if (ahwtResult != IasAlsaHandlerWorkerThread::eIasOk)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error during IasAlsaHandlerWorkerThread::start:", toString(ahwtResult));
      return eIasFailed;
    }
  }
  return eIasOk;
}


void IasAlsaHandler::stop()
{
  // Stop the worker thread, if it exists.
  if (mWorkerThread != nullptr)
  {
    mWorkerThread->stop();
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_DEVICE, "asynchronous thread successfully stopped");
  }

  if (mAlsaHandle != nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_DEVICE, "Clear device handle and close pcm device");
    mRingBuffer->clearDeviceHandle();
    snd_pcm_close(mAlsaHandle);
    mAlsaHandle = nullptr;
  }
}


IasAlsaHandler::IasResult IasAlsaHandler::setNonBlockMode(bool isNonBlocking)
{
  if (mRingBuffer == nullptr)
  {
    return IasResult::eIasNotInitialized;
  }

  // Set the ring buffer behavior only if the ALSA handler is synchronous. For asyncrhonous
  // ALSA handlers, we need the blocking behavior, because otherwise the worker thread
  // won't be paused.
  if (!mIsAsynchronous)
  {
    IasAudioRingBufferResult ringBufRes = mRingBuffer->setNonBlockMode(isNonBlocking);
    if (ringBufRes != eIasRingBuffOk)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error in IasAudioRingBuffer::setNonBlockMode:", toString(ringBufRes));
      return eIasRingBufferError;
    }
  }
  return eIasOk;
}


/*
 * Private function: Set the hardware parameters of an ASLA PCM device.
 */
int IasAlsaHandler::set_hwparams(snd_pcm_t                *pcmHandle,
                                 IasAudioCommonDataFormat  dataFormat,
                                 unsigned int              rate,
                                 unsigned int              channels,
                                 unsigned int              numPeriods,
                                 unsigned int              period_size,
                                 int32_t               *actualBufferSize,
                                 int32_t               *actualPeriodSize)
{
  int err = 0;
  int dir = 0;
  const int cResample = 1; // enable alsa-lib resampling

  snd_pcm_access_t  access = SND_PCM_ACCESS_MMAP_INTERLEAVED;
  snd_pcm_format_t  format = convertFormatIasToAlsa(dataFormat);

  snd_pcm_hw_params_t *hwparams;
  snd_pcm_hw_params_alloca(&hwparams);

  // Init complete hw params to be able to set them.
  err = snd_pcm_hw_params_any(pcmHandle, hwparams);
  if (err < 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error in snd_pcm_hw_params_any:", snd_strerror(err));
    return err;
  }

  // Set hardware resampling.
  err = snd_pcm_hw_params_set_rate_resample(pcmHandle, hwparams, cResample);
  if (err < 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error in snd_pcm_hw_params_set_rate_resample:", snd_strerror(err));
    return err;
  }

  // Find out what accestype the ALSA device supports.
  snd_pcm_access_mask_t *accessMask;
  snd_pcm_access_mask_alloca(&accessMask);
  err = snd_pcm_hw_params_get_access_mask(hwparams, accessMask);
  if (err < 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error in snd_pcm_hw_params_get_access_mask:", snd_strerror(err));
    return err;
  }

  // Check whether the ALSA device supports the requested access type.
  if (!(snd_pcm_access_mask_test(accessMask, access)))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error in snd_pcm_access_mask_test: requested access type is not supported");
    return -EINVAL;
  }

  // Set the access type.
  err = snd_pcm_hw_params_set_access(pcmHandle, hwparams, access);
  if (err < 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error in snd_pcm_hw_params_set_access:", snd_strerror(err));
    return err;
  }

  // Find out what sample format the ALSA device supports.
  snd_pcm_format_mask_t *formatMask;
  snd_pcm_format_mask_alloca(&formatMask);

  // Check whether the ALSA device supports the requested sample format.
  snd_pcm_hw_params_get_format_mask(hwparams, formatMask);

  // Check whether format is UNKNOWN, since snd_pcm_format_mask_test would crash in this case.
  if (format == SND_PCM_FORMAT_UNKNOWN)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error, unknown format: SND_PCM_FORMAT_UNKNOWN");
    return -EINVAL;
  }

  if (!(snd_pcm_format_mask_test(formatMask, format)))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error in snd_pcm_format_mask_test: requested format is not supported");
    return -EINVAL;
  }

  // Set the sample format.
  err = snd_pcm_hw_params_set_format(pcmHandle, hwparams, format);
  if (err < 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error in snd_pcm_hw_params_set_format:", snd_strerror(err));
    return err;
  }

  // Check which sample rates are supported by the ALSA device.
  uint32_t rate_min, rate_max;
  uint32_t adapted_samplerate;
  err = snd_pcm_hw_params_get_rate_min(hwparams, &rate_min, &dir);
  if (err != 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error in snd_pcm_hw_params_get_rate_min:", snd_strerror(err));
    return err;
  }
  err = snd_pcm_hw_params_get_rate_max(hwparams, &rate_max, &dir);
  if (err != 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error in snd_pcm_hw_params_get_rate_max:", snd_strerror(err));
    return err;
  }

  if ((rate < rate_min) || (rate > rate_max))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error: requested rate of",
                rate, "Hz is outside the range supported by the ALSA device [", rate_min, "Hz to", rate_max, "Hz ]");
    return -EINVAL;
  }

  // Set sample rate. We want the exact sample rate so set dir to zero
  dir = 0;
  err = snd_pcm_hw_params_set_rate(pcmHandle, hwparams, rate, dir);
  if (err < 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error in snd_pcm_hw_params_set_rate:", snd_strerror(err));
    return err;
  }

  // Get the actual set samplerate.
  err = snd_pcm_hw_params_get_rate(hwparams, &adapted_samplerate, &dir);
  if (err < 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error in snd_pcm_hw_params_get_rate:", snd_strerror(err));
    return err;
  }

  if (rate != adapted_samplerate)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_WARN, LOG_PREFIX, LOG_DEVICE, "The ALSA device does not support the desired sample rate of",
                rate, "Hz. Using", adapted_samplerate, "Hz instead.");
  }

  // Set the number of channels.
  err = snd_pcm_hw_params_set_channels(pcmHandle, hwparams, channels);
  if (err < 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "ALSA device does not support the requested number of", channels, "channels:", snd_strerror(err));
    return err;
  }

  uint32_t valmax, valmin;
  err = snd_pcm_hw_params_get_periods_max(hwparams, &valmax, &dir);
  if (err != 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error in snd_pcm_hw_params_get_periods_max:", snd_strerror(err));
    return err;
  }
  err = snd_pcm_hw_params_get_periods_min(hwparams, &valmin, &dir);
  if (err != 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error in snd_pcm_hw_params_get_periods_min:", snd_strerror(err));
    return err;
  }

  if (numPeriods < valmin)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Requested periods of", numPeriods, "is less than minimum of driver", valmin);
    return -EINVAL;
  }
  if (numPeriods > valmax)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Requested periods of", numPeriods, "is more than maximum of driver", valmax);
    return -EINVAL;
  }

  err = snd_pcm_hw_params_set_periods(pcmHandle, hwparams, numPeriods, 0);
  if (err < 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error in snd_pcm_hw_params_set_periods:", snd_strerror(err));
    return err;
  }

  uint32_t calculated_buffersize = period_size * numPeriods;
  err = snd_pcm_hw_params_set_buffer_size(pcmHandle, hwparams, calculated_buffersize);
  if (err < 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error in snd_pcm_hw_params_set_buffer_size:", snd_strerror(err));
    return err;
  }

  snd_pcm_uframes_t size;
  err = snd_pcm_hw_params_get_buffer_size(hwparams, &size);
  if (err < 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error in snd_pcm_hw_params_get_buffer_size:", snd_strerror(err));
    return err;
  }
  *actualBufferSize = static_cast<int32_t>(size);

  err = snd_pcm_hw_params_get_period_size(hwparams, &size, &dir);
  if (err < 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error in snd_pcm_hw_params_get_period_size:", snd_strerror(err));
    return err;
  }
  *actualPeriodSize = static_cast<int32_t>(size);

  // Set the hw params for the device
  err = snd_pcm_hw_params(pcmHandle, hwparams);
  if (err < 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error in snd_pcm_hw_params:", snd_strerror(err));
    snd_pcm_hw_params_dump(hwparams, mSndLogger);
    return err;
  }

  return 0;
}


/*
 * Private function: Set the software parameters of an ASLA PCM device.
 */
int IasAlsaHandler::set_swparams(snd_pcm_t   *pcmHandle,
                                 int32_t   bufferSize,
                                 int32_t   periodSize)
{
  int err;
  snd_pcm_sw_params_t *swparams;
  snd_pcm_sw_params_alloca(&swparams);

  // Get the current swparams.
  err = snd_pcm_sw_params_current(pcmHandle, swparams);
  if (err < 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error in snd_pcm_sw_params_current:", snd_strerror(err));
    return err;
  }
  // Start the transfer when the buffer is almost full: (bufferSize / avail_min) * avail_min
  err = snd_pcm_sw_params_set_start_threshold(pcmHandle, swparams, static_cast<snd_pcm_sframes_t>((bufferSize / periodSize) * periodSize));
  if (err < 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error in snd_pcm_sw_params_set_start_threshold:", snd_strerror(err));
    return err;
  }
  // Allow the transfer when at least periodSize samples can be processed.
  err = snd_pcm_sw_params_set_avail_min(pcmHandle, swparams, static_cast<snd_pcm_sframes_t>(periodSize));
  if (err < 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error in snd_pcm_sw_params_set_avail_min:", snd_strerror(err));
    return err;
  }
  // Write the parameters to the playback device.
  err = snd_pcm_sw_params(pcmHandle, swparams);
  if (err < 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error in snd_pcm_sw_params:", snd_strerror(err));
    return err;
  }
  return 0;
}

void IasAlsaHandler::reset()
{
  if (mWorkerThread)
  {
    mWorkerThread->reset();
  }
}

/*
 * Function to get a IasAlsaHandler::IasResult as string.
 */
#define STRING_RETURN_CASE(name) case name: return std::string(#name); break
#define DEFAULT_STRING(name) default: return std::string(name)
std::string toString(const IasAlsaHandler::IasResult& type)
{
  switch(type)
  {
    STRING_RETURN_CASE(IasAlsaHandler::eIasOk);
    STRING_RETURN_CASE(IasAlsaHandler::eIasInvalidParam);
    STRING_RETURN_CASE(IasAlsaHandler::eIasInitFailed);
    STRING_RETURN_CASE(IasAlsaHandler::eIasNotInitialized);
    STRING_RETURN_CASE(IasAlsaHandler::eIasAlsaError);
    STRING_RETURN_CASE(IasAlsaHandler::eIasTimeOut);
    STRING_RETURN_CASE(IasAlsaHandler::eIasRingBufferError);
    STRING_RETURN_CASE(IasAlsaHandler::eIasFailed);
    DEFAULT_STRING("Invalid IasAlsaHandler::IasResult => " + std::to_string(type));
  }
}


} // namespace IasAudio
