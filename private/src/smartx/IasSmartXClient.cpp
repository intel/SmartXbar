/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasSmartXClient.cpp
 * @date   2015
 * @brief
 */

#include "IasSmartXClient.hpp"
#include "internal/audio/common/alsa_smartx_plugin/IasAlsaHwConstraintsStatic.hpp"
#include "internal/audio/common/alsa_smartx_plugin/IasAlsaPluginIpc.hpp"

#include "internal/audio/common/IasAudioLogging.hpp"
#include "internal/audio/common/audiobuffer/IasAudioRingBuffer.hpp"
#include "smartx/IasThreadNames.hpp"
#include "smartx/IasConfigFile.hpp"

namespace IasAudio {

static const std::string cClassName = "IasSmartXClient::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"
#define LOG_DEVICE "device=" + mParams->name + ":"

IasSmartXClient::IasSmartXClient(IasAudioDeviceParamsPtr params)
  :mLog(IasAudioLogging::registerDltContext("SXC", "SmartX Client"))
  ,mParams(params)
  ,mShmConnection()
  ,mIpcThread(nullptr)
  ,mIsRunning(false)
  ,mInIpc(nullptr)
  ,mOutIpc(nullptr)
  ,mDeviceType(eIasDeviceTypeUndef)
  ,mIsEventQueueEnabled(false)
  ,mEventQueue()
  ,mDeviceState(eIasStopped)
  ,mSessionId(0)
{
}

IasSmartXClient::~IasSmartXClient()
{
  mIsRunning.store(false);
  if (mIpcThread != nullptr)
  {
    // Cancel the wait for condition variable
    pthread_cancel(mIpcThread->native_handle());
    mIpcThread->join();
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_DEVICE, "IPC thread successfully ended");
    delete mIpcThread;
  }
}

IasSmartXClient::IasResult IasSmartXClient::init(IasDeviceType deviceType)
{
  std::string postfix;
  if (deviceType == eIasDeviceTypeSink)
  {
    postfix = "_c";
  }
  else
  {
    postfix = "_p";
  }
  mDeviceType = deviceType;
  std::string fullName("smartx_" + mParams->name + postfix);
  const std::string &groupName = IasConfigFile::getInstance()->getShmGroupName();
  IasAudioCommonResult result = mShmConnection.createConnection(fullName, groupName);
  if (result != eIasResultOk)
  {
    return eIasFailed;
  }
  result = mShmConnection.createRingBuffer(mParams);
  if (result != eIasResultOk)
  {
    return eIasFailed;
  }
  mInIpc = mShmConnection.getInIpc();
  mOutIpc = mShmConnection.getOutIpc();
  setHwConstraints();
  mIsRunning.store(true);
  mIpcThread = new (std::nothrow) std::thread([this]{ipcThread();});
  IAS_ASSERT(mIpcThread != nullptr);
  return eIasOk;
}

IasSmartXClient::IasResult IasSmartXClient::getRingBuffer(IasAudioRingBuffer** ringBuffer)
{
  if (ringBuffer == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "ringBuffer == nullptr");
    return eIasFailed;
  }
  IasAudioRingBuffer *myRingBuffer = mShmConnection.getRingBuffer();
  if (myRingBuffer == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Not inititalized yet");
    return eIasFailed;
  }
  *ringBuffer = myRingBuffer;
  return eIasOk;
}


void IasSmartXClient::setHwConstraints()
{
  // Set the hardware device parameters.
  IasAlsaHwConstraintsStatic *constraints = mShmConnection.getAlsaHwConstraints();
  IAS_ASSERT(constraints != nullptr);
  IAS_ASSERT(mParams != nullptr);
  constraints->formats.list.push_back(mParams->dataFormat);
  constraints->access.list.push_back(eIasLayoutInterleaved);
  constraints->access.list.push_back(eIasLayoutNonInterleaved);
  constraints->channels.list.push_back(mParams->numChannels);
  constraints->rate.list.push_back(mParams->samplerate);
  uint32_t periodSizeBytes = mParams->periodSize * mParams->numChannels * toSize(mParams->dataFormat);
  constraints->period_size.list.push_back(periodSizeBytes);
  constraints->period_count.list.push_back(mParams->numPeriods);
  constraints->buffer_size.list.push_back(periodSizeBytes * mParams->numPeriods);
  constraints->isValid = true;
}

void IasSmartXClient::ipcThread()
{
  IasThreadNames::getInstance()->setThreadName(IasThreadNames::eIasStandard, "alsa-smartx-plugin communicator thread for smartx:" + mParams->name);

#ifdef __ANDROID__
  cancelable_pthread_init();
#endif // #ifdef __ANDROID__

  IasAudioCommonResult result;
  while (mIsRunning.load() == true)
  {
    // This call will block until either at least one package is received or
    // pthread_cancel is called in the destructor
    mInIpc->waitForPackage();
    while(mInIpc->packagesAvailable())
    {
      // Receive/Send types
      IasAudioIpcPluginControl inControl;
      IasAudioIpcPluginControlResponse outResponse;
      IasAudioIpcPluginInt32Data outInt32;
      IasAudioIpcPluginParamData inParams;

      // Check if the package is extractable
      if(eIasResultOk == mInIpc->pop_noblock<IasAudioIpcPluginControl>(&inControl))
      {
        // Check the contents of the control
        switch(inControl)
        {
          case eIasAudioIpcGetLatency:
          {
            outInt32.control = inControl;
            outInt32.response = 0;
            IasAudioRingBuffer *myRingBuffer = mShmConnection.getRingBuffer();
            // The ringbuffer is created in the init method. Init will fail if creation is not possible
            IAS_ASSERT(myRingBuffer != nullptr);
            uint32_t fillLevel = 0;
            IasAudioRingBufferResult rbres = myRingBuffer->updateAvailable(eIasRingBufferAccessRead, &fillLevel);
            // In this case we always have a real ringBuffer and the only things that could go wrong is passing illegal parameters to
            // the updateAvailable method. This is hardcoded here and thus always correct. And even if the call would fail the fillLevel
            // is still initialized to a valid value.
            IAS_ASSERT(rbres == eIasRingBuffOk);
            (void)rbres;
            outInt32.response = static_cast<int32_t>(fillLevel);
            result = mOutIpc->push<IasAudioIpcPluginInt32Data>(outInt32);
            DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX, LOG_DEVICE, "Received", toString(inControl), "control. FillLevel=", fillLevel);
            break;
          }
          case eIasAudioIpcStart:
            DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_DEVICE, "Received", toString(inControl), "control, session id=", mSessionId);
            outResponse.control = inControl;
            outResponse.response = eIasAudioIpcACK;
            result = mOutIpc->push<IasAudioIpcPluginControlResponse>(outResponse);
            putEventType(IasAudioDevice::eIasStart);
            break;
          case eIasAudioIpcStop:
            DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_DEVICE, "Received", toString(inControl), "control, session id=", mSessionId);
            outResponse.control = inControl;
            outResponse.response = eIasAudioIpcACK;
            result = mOutIpc->push<IasAudioIpcPluginControlResponse>(outResponse);
            if (mDeviceType == eIasDeviceTypeSource)
            {
              // Reset the shared memory of the ALSA device in case it is a playback device.
              // If it is a capture device, the reset is handled by the IasRoutingZoneWorkerThread
              // We have to call resetFromWriter here and not resetFromReader, because the only access that
              // could be active is from the reader side, from the buffer task. This read access locks
              // the read mutex. The resetFromWriter also tries to lock the read mutex, because the reset is
              // triggered from the writer side and thus we have to prohibit a concurrent access from the reader
              // side.
              mShmConnection.getRingBuffer()->resetFromWriter();
            }
            putEventType(IasAudioDevice::eIasStop);
            break;
          case eIasAudioIpcDrain:
            DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_DEVICE, "Received", toString(inControl), "control");
            outResponse.control = inControl;
            outResponse.response = eIasAudioIpcACK;
            result = mOutIpc->push<IasAudioIpcPluginControlResponse>(outResponse);
            break;
          case eIasAudioIpcPause:
            DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_DEVICE, "Received", toString(inControl), "control");
            outResponse.control = inControl;
            outResponse.response = eIasAudioIpcACK;
            result = mOutIpc->push<IasAudioIpcPluginControlResponse>(outResponse);
            break;
          case eIasAudioIpcResume:
            DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_DEVICE, "Received", toString(inControl), "control");
            outResponse.control = inControl;
            outResponse.response = eIasAudioIpcACK;
            result = mOutIpc->push<IasAudioIpcPluginControlResponse>(outResponse);
            break;
          default:
            DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_DEVICE, "Received", toString(inControl), "control");
            outResponse.control = inControl;
            outResponse.response = eIasAudioIpcNAK;
            result = mOutIpc->push<IasAudioIpcPluginControlResponse>(outResponse);
        }
        if (result != eIasResultOk)
        {
          /**
           * @log Message queue is full.
           */
          DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error sending response to", toString(inControl), "control =>", toString(result));
        }
      }
      else if(eIasResultOk == mInIpc->pop_noblock<IasAudioIpcPluginParamData>(&inParams))
      {
        if(inParams.control == eIasAudioIpcParameters)
        {
          DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_DEVICE, "Received parameters chosen by application");
          DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_DEVICE,
                      "numChannels", inParams.response.numChannels,
                      "sampleRate", inParams.response.sampleRate,
                      "periodSize", inParams.response.periodSize,
                      "numPeriods", inParams.response.numPeriods,
                      "dataFormat", toString(inParams.response.dataFormat)
                     );
          // As the application chose new parameters, we make this an indication for starting a new session
          // by incrementing the session id. This is later used to tag the events being created by the client
          // and send to the real-time thread in order to filter out outdated events being issued in an old session.
          mSessionId++;
          // Handle the wrap-around
          if (mSessionId < 0)
          {
            mSessionId = 0;
          }
          DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_DEVICE, "Use new session id", mSessionId, "now");
          outResponse.control = inParams.control;
          outResponse.response = eIasAudioIpcACK;
          result = mOutIpc->push<IasAudioIpcPluginControlResponse>(outResponse);
        }
        else
        {
          /**
           * @log Unexpected parameter received.
           */
          DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Unexpected IasAudioIpcPluginParamData control", static_cast<uint32_t>(inParams.control));
          outResponse.control = inParams.control;
          outResponse.response = eIasAudioIpcNAK;
          result = mOutIpc->push<IasAudioIpcPluginControlResponse>(outResponse);
        }
        if (result != eIasResultOk)
        {
          /**
           * @log Message queue is full.
           */
          DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error sending response to", toString(inParams.control), "control =>", toString(result));
        }
      }
      else
      {
        // Package was not expected;
        /**
         * @log Unexpected parameter received.
         */
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Unexpected IasAudioIpcPluginControl control", static_cast<uint32_t>(inParams.control));
        mInIpc->discardNext();
        outResponse.control = inParams.control;
        outResponse.response = eIasAudioIpcNAK;
        result = mOutIpc->push<IasAudioIpcPluginControlResponse>(outResponse);
        if (result != eIasResultOk)
        {
          /**
           * @log Message queue is full.
           */
          DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error sending response to", toString(inParams.control), "control =>", toString(result));
        }
      }
    }
  }
}

void IasSmartXClient::enableEventQueue(bool enable)
{
  mIsEventQueueEnabled = enable;
}

IasAudioDevice::IasEventType IasSmartXClient::getNextEventType()
{
  if (mIsEventQueueEnabled == false)
  {
    return IasAudioDevice::eIasNoEvent;
  }
  IasAudioDevice::IasEventType newEvent = IasAudioDevice::eIasNoEvent;
  do
  {
    IasEventItem eventItem = {IasAudioDevice::eIasNoEvent, -1};
    mEventQueue.try_pop(eventItem);
    if (mSessionId == eventItem.sessionId)
    {
      newEvent = eventItem.eventType;
      break;
    }
    else if (eventItem.sessionId != -1)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_DEVICE, "Removed outdated event", toString(eventItem.eventType), "with SessionId=", eventItem.sessionId, "current SessionId=", mSessionId);
    }
  }
  while(mEventQueue.empty() == false);

  return newEvent;
}

void IasSmartXClient::putEventType(IasAudioDevice::IasEventType eventType)
{
  if (mIsEventQueueEnabled == true)
  {
    // Ensure that we insert the event only in case of a device state change.
    // This filters out multiple stop events from being added to the event queue.
    if (mDeviceState == eIasStopped)
    {
      if (eventType == IasAudioDevice::eIasStart)
      {
        mEventQueue.push(IasEventItem{eventType, mSessionId});
        mDeviceState = eIasStarted;
      }
    }
    else
    {
      if (eventType == IasAudioDevice::eIasStop)
      {
        mEventQueue.push(IasEventItem{eventType, mSessionId});
        mDeviceState = eIasStopped;
      }
    }
  }
}


} //namespace IasAudio
