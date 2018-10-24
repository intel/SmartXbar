/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasRoutingZoneWorkerThread.cpp
 * @date   2015
 * @brief
 */

#include <string>
#include <cmath>
#include <boost/algorithm/string/replace.hpp>

#include "internal/audio/common/helper/IasThread.hpp"
#include "internal/audio/common/IasAudioLogging.hpp"
#include "internal/audio/common/audiobuffer/IasAudioRingBuffer.hpp"
#include "internal/audio/common/helper/IasCopyAudioAreaBuffers.hpp"
#include "alsahandler/IasAlsaHandler.hpp"
#include "smartx/IasSmartXClient.hpp"
#include "switchmatrix/IasSwitchMatrix.hpp"
#include "smartx/IasConfigFile.hpp"
#include "audio/smartx/IasEventProvider.hpp"
#include "audio/smartx/IasSetupEvent.hpp"
#include "internal/audio/common/IasDataProbe.hpp"
#include "internal/audio/common/IasDataProbeHelper.hpp"
#include "model/IasPipeline.hpp"
#include "model/IasAudioSinkDevice.hpp"
#include "model/IasRoutingZoneWorkerThread.hpp"
#include "model/IasAudioPort.hpp"
#include "smartx/IasThreadNames.hpp"
#include "internal/audio/common/audiobuffer/IasAudioRingBufferMirror.hpp"
#include "internal/audio/common/audiobuffer/IasAudioRingBufferFactory.hpp"

namespace IasAudio {

static const std::string cClassName       = "IasRoutingZoneWorkerThread::";
static const std::string cRunnerClassName = "IasRunnerThread::";
#define LOG_PREFIX        cClassName       + __func__ + "(" + std::to_string(__LINE__) + "):"
#define LOG_RUNNER_PREFIX cRunnerClassName + __func__ + "(" + std::to_string(__LINE__) + "):"
#define LOG_ZONE "zone=" + mParams->name + ":"
#define LOG_RUNNER_NAME "runner thread, parent=" + mParentZoneName + ":PSM=" + std::to_string(mPeriodSizeMultiple) + ":"

IasRunnerThread::IasRunnerThread(uint32_t periodSizeMultiple, std::string parentZoneName)
  :mThreadShouldBeRunning(false)
  ,mLog(IasAudioLogging::registerDltContext("RNT", "Runner Thread"))
  ,mCondition()
  ,mMutexDerivedZones()
  ,mDerivedZoneParamsMap()
  ,mPeriodSizeMultiple(periodSizeMultiple)
  ,mPeriodCount(0)
  ,mThread(nullptr)
  ,mIsProcessing(false)
  ,mParentZoneName(parentZoneName)
{
  mThread = new IasThread(this, std::string("runner thread") + mParentZoneName);
  IAS_ASSERT(mThread != nullptr);
}

IasRunnerThread::~IasRunnerThread()
{
  stop();
  delete mThread;
}


IasAudioCommonResult IasRunnerThread::beforeRun()
{
  return eIasResultOk;
}

IasRunnerThread::IasResult IasRunnerThread::start()
{
  //Guaranteed by constructor.
  IAS_ASSERT(mThread != nullptr);

  mThreadShouldBeRunning = true;
  IasThreadResult startResult = mThread->start(true);
  if ((startResult != IasThreadResult::eIasThreadOk) && (startResult != IasThreadResult::eIasThreadAlreadyStarted))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_RUNNER_PREFIX, LOG_RUNNER_NAME, "Failed to start the thread.", startResult);
    return IasAudio::IasRunnerThread::IasResult::eIasFailed;
  }
  if (startResult == IasThreadResult::eIasThreadAlreadyStarted)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_RUNNER_PREFIX, LOG_RUNNER_NAME, "Runner thread: ", startResult);
  }
  return IasAudio::IasRunnerThread::IasResult::eIasOk;
}

IasRunnerThread::IasResult IasRunnerThread::stop()
{
  //Guaranteed by constructor.
  IAS_ASSERT(mThread != nullptr);

  IasThreadResult stopResult = mThread->stop();

  if ((stopResult != IasThreadResult::eIasThreadOk) && stopResult != IasThreadResult::eIasThreadNotRunning)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_RUNNER_PREFIX, LOG_RUNNER_NAME, "Failed to stop the thread.", stopResult);
    return IasAudio::IasRunnerThread::IasResult::eIasFailed;
  }
  return IasRunnerThread::IasResult::eIasOk;
}

IasAudioCommonResult IasRunnerThread::shutDown()
{
  //Guaranteed by constructor.
  IAS_ASSERT(mThread != nullptr);

  DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_RUNNER_PREFIX, LOG_RUNNER_NAME, "Shutting down thread");
  mThreadShouldBeRunning = false;
  wake();
  // returning success will cause IasThread to join threads
  return eIasResultOk;
}

IasAudioCommonResult IasRunnerThread::afterRun()
{
  return eIasResultOk;
}

IasRunnerThread::IasResult IasRunnerThread::addZone(IasDerivedZoneParamsPair derivedZone)
{
  IAS_ASSERT(derivedZone.first != nullptr); // already checked
  std::lock_guard<std::mutex> lk(this->mMutexDerivedZones);
  mDerivedZoneParamsMap.insert(derivedZone);
  DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_RUNNER_PREFIX, LOG_RUNNER_NAME, "Adding zone");
  return IasRunnerThread::IasResult::eIasOk;
}

void IasRunnerThread::deleteZone(IasRoutingZoneWorkerThreadPtr derivedZoneWorkerThread)
{
  if (derivedZoneWorkerThread != nullptr)
  {
    IasDerivedZoneParamsMap::iterator  mapIt;
    mapIt = mDerivedZoneParamsMap.find(derivedZoneWorkerThread);

    if (mapIt != mDerivedZoneParamsMap.end())
    {
      std::lock_guard<std::mutex> lk(mMutexDerivedZones);
      mDerivedZoneParamsMap.erase(mapIt);
    }
  } else {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_RUNNER_PREFIX, LOG_RUNNER_NAME, "Invalid worker thread == nullptr");
  }
}

bool IasRunnerThread::hasZone(IasRoutingZoneWorkerThreadPtr worker)
{
  IasDerivedZoneParamsMap::iterator  mapIt;
  mapIt = mDerivedZoneParamsMap.find(worker);

  if (mapIt != mDerivedZoneParamsMap.end())
  {
    return true;
  }
  return false;
}

bool IasRunnerThread::isEmpty()
{
  return mDerivedZoneParamsMap.empty();
}

uint32_t IasRunnerThread::addPeriod(uint32_t periodCount)
{
  // This mutex ensures derived zones don't get added/removed when we
  // iterate over map and update them.
  std::lock_guard<std::mutex> lk(mMutexDerivedZones);
  mPeriodCount += periodCount;
  for (IasDerivedZoneParamsMap::iterator mapIt = mDerivedZoneParamsMap.begin(); mapIt != mDerivedZoneParamsMap.end();
        mapIt++)
  {
    IasRoutingZoneWorkerThreadPtr derivedZoneTransferThread = mapIt->first;
    IasDerivedZoneParams* derivedZoneParams = &(mapIt->second);
    if (derivedZoneTransferThread->isActive())
    {
      derivedZoneParams->countPeriods += periodCount;
    }
  }
  return mPeriodCount;
}

bool IasRunnerThread::isAnyActive() const
{
  // This mutex ensures derived zones don't get added/removed when we
  // iterate over map and update them.
  std::lock_guard<std::mutex> lk(mMutexDerivedZones);
  for (IasDerivedZoneParamsMap::const_iterator mapIt = mDerivedZoneParamsMap.cbegin(); mapIt != mDerivedZoneParamsMap.cend();
        mapIt++)
  {
    IasRoutingZoneWorkerThreadPtr derivedZoneTransferThread = mapIt->first;
    if (derivedZoneTransferThread->isActive())
    {
      return true;
    }
  }
  return false;
}

IasAudioCommonResult IasRunnerThread::run()
{
  std::unique_lock<std::mutex> lk(mMutexDerivedZones);
  IasThreadNames::getInstance()->setThreadName(IasThreadNames::eIasRealTime, std::string("Runner thread for periodSizeMultiple ") + std::to_string(mPeriodSizeMultiple));
  IasConfigFile::configureThreadSchedulingParameters(mLog, eIasPriorityOneLess);
  DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_RUNNER_PREFIX, LOG_RUNNER_NAME, "Starting thread");
  while (mThreadShouldBeRunning)
  {
    while (mPeriodCount < mPeriodSizeMultiple)
    {
      // This can spuriously wake, so need to check if have enough period count
      // to process.
      mCondition.wait(lk);
      if (!mThreadShouldBeRunning)
      {
        return eIasResultOk;
      }
    }
    if (mPeriodCount >= mPeriodSizeMultiple)
    {
      mPeriodCount = 0;
    }
    mIsProcessing = true;
    // At this point there should be enough samples to handle each zone
    for (IasDerivedZoneParamsMap::iterator mapIt = mDerivedZoneParamsMap.begin(); mapIt != mDerivedZoneParamsMap.end();
        mapIt++)
    {
      IasRoutingZoneWorkerThreadPtr derivedZoneTransferThread = mapIt->first;
      IasDerivedZoneParams* derivedZoneParams = &(mapIt->second);
      if (derivedZoneTransferThread->isActive())
      {
        if (derivedZoneParams->countPeriods >= derivedZoneParams->periodSizeMultiple)
        {
          derivedZoneParams->countPeriods = 0;
        }
        std::string derivedZoneName = "(name=\"" + derivedZoneTransferThread->mParams->name + "\"):";

        DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_RUNNER_PREFIX, LOG_RUNNER_NAME,
                    "Executing IasRoutingZoneWorkerThread::transferPeriod method of derived zone", derivedZoneName);
        IasRoutingZoneWorkerThread::IasResult result;
        result = mapIt->first->transferPeriod();
        if (result != IasRoutingZoneWorkerThread::IasResult::eIasOk)
        {
          DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_RUNNER_PREFIX, LOG_RUNNER_NAME,
                      "Error during IasRoutingZoneWorkerThread::transferPeriod method of derived zone", derivedZoneName,
                      toString(result));
          mIsProcessing = false;
          return eIasResultFailed;
        }
      }
    }
    mIsProcessing = false;
  }
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_RUNNER_PREFIX, LOG_RUNNER_NAME, "End runner thread");
  return eIasResultOk;
}

void IasRunnerThread::wake()
{
  // It can share and verify mutex mMutexDerivedZone, if mutex is locked,
  // it means processing is still happening and may require logging
  mCondition.notify_one();
}

bool IasRunnerThread::isProcessing() const
{
  return mIsProcessing;
}

uint32_t IasRunnerThread::getPeriodSizeMultiple() const
{
  return mPeriodSizeMultiple;
}

IasRoutingZoneWorkerThread::IasRoutingZoneWorkerThread(IasRoutingZoneParamsPtr params)
  :mLog(IasAudioLogging::registerDltContext("RZN", "Routing Zone"))
  ,mParams(params)
  ,mSinkDevice(nullptr)
  ,mSinkDeviceRingBuffer(nullptr)
  ,mPeriodSize(0)
  ,mSinkDeviceDataFormat(eIasFormatUndef)
  ,mSinkDeviceNumChannels(0)
  ,mThread(nullptr)
  ,mThreadIsRunning(false)
  ,mSwitchMatrix(nullptr)
  ,mEventProvider(nullptr)
  ,mDataProbe(nullptr)
  ,mProbingActive(false)
  ,mIsDerivedZone(false)
  ,mDerivedZoneCallCount(0)
  ,mPipeline(nullptr)
  ,mCurrentState(eIasInActive)
  ,mDiagnosticsFileName("")
  ,mBasePipeline(nullptr)
  ,mLogCnt(0)
  ,mPeriodTime(0)
  ,mLogInterval(0)
  ,mTimeoutCnt(0)
  ,mLogOkCnt(0)
{
  IAS_ASSERT(params != nullptr)
  mEventProvider = IasEventProvider::getInstance();
  IAS_ASSERT(mEventProvider != nullptr);
}

IasRoutingZoneWorkerThread::~IasRoutingZoneWorkerThread()
{
  DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX, LOG_ZONE);
  stop();
  mDataProbe = nullptr;
  delete mThread;
  std::lock_guard<std::mutex> lk(mMutexConversionBuffers);

  IasAudioRingBufferFactory* ringBufferFactory = IasAudioRingBufferFactory::getInstance();

  for (auto &it : mConversionBufferParamsMap)
  {
    const IasRoutingZoneWorkerThread::IasConversionBufferParams tmpParams = it.second;
    ringBufferFactory->destroyRingBuffer(tmpParams.ringBuffer);
  }
  mConversionBufferParamsMap.clear();
  unlinkAudioSinkDevice();
  deletePipeline();
  deleteBasePipeline();
  mSwitchMatrix = nullptr;
}


IasRoutingZoneWorkerThread::IasResult IasRoutingZoneWorkerThread::init()
{
  IAS_ASSERT(mParams != nullptr);

  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_ZONE, "Initialization of Routing Zone Worker Thread.");

  std::lock_guard<std::mutex> lk(mMutexConversionBuffers);
  mConversionBufferParamsMap.clear();
  if (mThread == nullptr)
  {
    mThread = new IasThread(this, mParams->name);
    IAS_ASSERT(mThread != nullptr);
  }
  return eIasOk;
}


IasRoutingZoneWorkerThread::IasResult IasRoutingZoneWorkerThread::linkAudioSinkDevice(IasAudioSinkDevicePtr sinkDevice)
{
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_ZONE, "Inside linkAudioSinkDevice()");
  IAS_ASSERT(sinkDevice != nullptr); // already checked in IasSetupImpl::link(IasRoutingZonePtr, IasAudioSinkDevicePtr)

  // Verify that the period size of the sink device is valid.
  if (sinkDevice->getPeriodSize() == 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "Error, period size of sink device must not be 0");
    return eIasFailed;
  }

  // If a pipeline has been linked to the routing zone, verify that the period size and the sample rate
  // of the sink device match to the corresponding parameters of the pipeline.
  if (mPipeline != nullptr)
  {
    IasPipelineParamsPtr pipelineParameters = mPipeline->getParameters();
    IAS_ASSERT(pipelineParameters != nullptr); // already checked in constructor of IasPipeline
    if (sinkDevice->getPeriodSize() != pipelineParameters->periodSize)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE,
                  "Error, periodSizes do not match: sink device:", sinkDevice->getPeriodSize(),
                  "pipeline:", pipelineParameters->periodSize);
      return eIasFailed;
    }
    if (sinkDevice->getSampleRate() != pipelineParameters->samplerate)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE,
                  "Error, sample rates do not match: sink device:", sinkDevice->getSampleRate(),
                  "pipeline:", pipelineParameters->samplerate);
      return eIasFailed;
    }
  }

  mSinkDevice = sinkDevice;
  mPeriodSize = mSinkDevice->getPeriodSize();
  mSinkDevice->enableEventQueue(true);
  mPeriodTime = mPeriodSize * 1000 / sinkDevice->getSampleRate();
  if (mPeriodTime == 0)
  {
    mPeriodTime = 1; //to avoid floating point exception in next step
  }
  mLogInterval = 1000 / mPeriodTime;
  std::string diagnosticsFileNameTest  = "IasRoutingZoneWorkerThread_createDiagnostics";
  std::string diagnosticsFileNameTrunk = "/tmp/IasRoutingZoneWorkerThread_Diagnostics";

  // Check whether there is a file whose name matches with diagnosticsFileNameTest.
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_ZONE,
              "Checking whether file", diagnosticsFileNameTest, "exists.");

  std::ifstream testStream;
  testStream.open(diagnosticsFileNameTest);
  if (testStream.fail())
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_ZONE,
                "Routing zone diagnostics won't be created, since file", diagnosticsFileNameTest, "does not exist.");
  }
  else
  {
    testStream.close();

    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_ZONE, "Saving Diagnostics");
    // Open the file for saving the routing zone diagnostics.
    const IasAudioDeviceParamsPtr sinkDeviceParams = sinkDevice->getDeviceParams();
    IAS_ASSERT(sinkDeviceParams != nullptr);
    mDiagnosticsFileName = diagnosticsFileNameTrunk + "_" + sinkDeviceParams->name + ".txt";

    // Replace characters that are not supported by NTFS,
    // since we want to analyze the diagnostics file on a Windows PC.
    boost::algorithm::replace_all(mDiagnosticsFileName, ":", "_");

    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_ZONE,
                "Writing routing zone diagnostics to file:", mDiagnosticsFileName);

    mDiagnosticsStream.open(mDiagnosticsFileName);
    if (mDiagnosticsStream.fail())
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_WARN, LOG_PREFIX, LOG_ZONE,
                "Error while opening routing zone diagnostics file:", mDiagnosticsFileName);
    }
  }
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_ZONE,
              "Exiting linkAudioSinkDevice Successfully");

  return eIasOk;
}

void IasRoutingZoneWorkerThread::unlinkAudioSinkDevice()
{
  if (mSinkDevice != nullptr)
  {
    mSinkDevice->enableEventQueue(false);
    mSinkDevice.reset();
  }
}


IasRoutingZoneWorkerThread::IasResult IasRoutingZoneWorkerThread::addDerivedZoneWorkerThread(IasRoutingZoneWorkerThreadPtr derivedZoneWorkerThread)
{
  if (derivedZoneWorkerThread == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "Error, parameter derivedZoneWorkerThread == nullptr");
    return eIasInvalidParam;
  }

  IasAudioSinkDevicePtr derivedZoneSinkDevice = derivedZoneWorkerThread->getSinkDevice();
  if (derivedZoneSinkDevice == nullptr)
  {
    /**
     * @log link(IasRoutingZonePtr, IasAudioSinkDevicePtr) method of IasISetup interface was not called before.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "Error, derived zone does not have a sink device");
    return eIasFailed;
  }

  IAS_ASSERT(derivedZoneSinkDevice->getDeviceParams() != nullptr);

  if (mPeriodSize == 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "Error, period size of base zone must not be 0 (maybe due to missing linkage to sink)");
    return eIasFailed;
  }

  // Calculate the factor between the derived zone period size and the base zone period size.
  uint64_t derivedZonePeriodSize = static_cast<uint64_t>(derivedZoneSinkDevice->getPeriodSize());
  uint64_t derivedZoneSampleRate = static_cast<uint64_t>(derivedZoneSinkDevice->getSampleRate());
  uint64_t baseZonePeriodSize    = static_cast<uint64_t>(mPeriodSize);
  uint64_t baseZoneSampleRate    = static_cast<uint64_t>(mSinkDevice->getSampleRate());
  uint64_t periodSizeMultiple    = ((derivedZonePeriodSize * baseZoneSampleRate) /
                                       (derivedZoneSampleRate * baseZonePeriodSize));

  // Check that periodSizeMultiple is really an integer.
  if (derivedZonePeriodSize * baseZoneSampleRate != derivedZoneSampleRate * baseZonePeriodSize * periodSizeMultiple)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "Error, derived zone period time (",
                static_cast<float>(1000 * derivedZonePeriodSize) / static_cast<float>(derivedZoneSampleRate),
                "ms ) is not a multiple of the base zone period time (",
                static_cast<float>(1000 * baseZonePeriodSize) / static_cast<float>(baseZoneSampleRate),
                "ms )");
    return eIasFailed;
  }
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_ZONE,
              "Zone", derivedZoneWorkerThread->mParams->name,
              "becomes a derived zone now with periodSizeMultiple of ", periodSizeMultiple);

  // Prepare the scheduling parameters for the derived zone and insert it into the map.
  IasDerivedZoneParams  derivedZoneParams;
  derivedZoneParams.periodSize = static_cast<uint32_t>(derivedZonePeriodSize);
  derivedZoneParams.periodSizeMultiple = static_cast<uint32_t>(periodSizeMultiple);
  derivedZoneParams.countPeriods = 0;
  derivedZoneParams.runnerEnabled = false;
  IasConfigFile::IasOptionState runnerThreadState = IasConfigFile::getInstance()->getRunnerThreadState(derivedZoneWorkerThread->mParams->name);
  if ((runnerThreadState == IasConfigFile::eIasEnabled) && (periodSizeMultiple != 1))
  {
    derivedZoneParams.runnerEnabled = true;
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_ZONE,
                "Runner thread enabled for derived routing zone", derivedZoneWorkerThread->mParams->name);
  }
  IasDerivedZoneParamsPair tmp = std::make_pair(derivedZoneWorkerThread, derivedZoneParams);

  // Zones with periodSizeMultiple are handled in parent thread.
  if (derivedZoneParams.runnerEnabled == true)
  {
    bool found = false;
    for (IasRunnerThreadParamsMap::iterator mapIt = mRunnersParamsMap.begin(); mapIt != mRunnersParamsMap.end(); mapIt++)
    {
      if (mapIt->second.periodSizeMultiple == periodSizeMultiple)
      {
        mapIt->first->addZone(tmp);
        found = true;
        break;
      }
    }
    if (!found)
    {
      // There was no runner thread found for given periodSizeMultiple.
      // This will create one and connect it with the derived zone.
      IasRoutingZoneRunnerThreadPtr runnerThread = std::make_shared<IasRunnerThread>(derivedZoneParams.periodSizeMultiple, mParams->name);
      IasRunnerThreadParamsPair runnerPair = std::make_pair(runnerThread, derivedZoneParams);
      mRunnersParamsMap.insert(runnerPair);
      runnerThread->addZone(tmp);
    }
  }
  mDerivedZoneParamsMap.insert(tmp);


  // To avoid that the routing zones are driven by conflicting clocks, the
  // sink device of the derived zone has to operate in non-blocking mode.
  // This has to be done for ALSA handlers only, because SmartXClients
  // do not have a corresponding function.
  if (derivedZoneSinkDevice->isAlsaHandler())
  {
    IasAlsaHandlerPtr alsaHandler = nullptr;
    derivedZoneSinkDevice->getConcreteDevice(&alsaHandler);
    IAS_ASSERT(alsaHandler != nullptr); // already tested by IasAudioDevice::isAlsaHandler()

    IasAlsaHandler::IasResult ahResult = alsaHandler->setNonBlockMode(true);
    if (ahResult != IasAlsaHandler::eIasOk)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "Error in IasAlsaHandler::setNonBlockMode:", toString(ahResult));
      return eIasFailed;
    }
  }

  return eIasOk;
}


void IasRoutingZoneWorkerThread::deleteDerivedZoneWorkerThread(IasRoutingZoneWorkerThreadPtr derivedZoneWorkerThread)
{
  if (derivedZoneWorkerThread != nullptr)
  {
    // Remove worker thread from runner
    for (IasRunnerThreadParamsMap::iterator mapIt = mRunnersParamsMap.begin();
        mapIt != mRunnersParamsMap.end();
        mapIt++)
    {
      IasRoutingZoneRunnerThreadPtr runner = mapIt->first;
      if (runner->hasZone(derivedZoneWorkerThread))
      {
        std::lock_guard<std::mutex> lk(mMutexDerivedZones);
        // This could potentially just delete worker thread from all runner threads,
        // it will be a no-op in runner threads which do not contain it.
        runner->deleteZone(derivedZoneWorkerThread);

        // If it was last zone for this runner thread, remove it
        if (runner->isEmpty())
        {
          mRunnersParamsMap.erase(mapIt);
        }

        // No need to check other runner threads, zone should only be in one.
        break;
      }
    }
    IasDerivedZoneParamsMap::iterator mapIt;
    mapIt = mDerivedZoneParamsMap.find(derivedZoneWorkerThread);
    if (mapIt != mDerivedZoneParamsMap.end())
    {
      std::lock_guard<std::mutex> lk(mMutexDerivedZones);
      mDerivedZoneParamsMap.erase(mapIt);
    }
  } else {
    // nullptr worker thread cannot be present, it is checked for in addDerivedZoneWorkerThread
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "Invalid worker thread == nullptr");
  }
}


IasRoutingZoneWorkerThread::IasResult IasRoutingZoneWorkerThread::prepareStates()
{
  // Check for mSinkDevice. This function and the real-time thread function depends on it.
  if (mSinkDevice == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "Missing link to sink device (mSinkDevice == nullptr)");
    return eIasFailed;
  }

  IasAudioDeviceParamsPtr sinkDeviceParams = mSinkDevice->getDeviceParams();
  if (sinkDeviceParams == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "Error: sink device parameters have not been set");
    return eIasFailed;
  }
  mPeriodSize = sinkDeviceParams->periodSize;

  // Get the handle to the ring buffer of the sink device.
  // The ring buffer is provided either by the AlsaHandler or by the smartXClient.
  if (mSinkDevice->isAlsaHandler())
  {
    IasAlsaHandlerPtr alsaHandler;
    mSinkDevice->getConcreteDevice(&alsaHandler);
    IAS_ASSERT(alsaHandler != nullptr); // already tested by IasAudioDevice::isAlsaHandler()

    // Start the ALSA handler in playback direction.
    IasAlsaHandler::IasResult result = alsaHandler->start();
    if (result != IasAlsaHandler::eIasOk)
    {
      /**
       * @log A more detailed description of the error is provided by the method IasAlsaHandler::eIasInvalidParam.
       */
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "Error during IasAlsaHandler::start:", toString(result));
      return eIasFailed;
    }

    result = alsaHandler->getRingBuffer(&mSinkDeviceRingBuffer);
    if (result != IasAlsaHandler::eIasOk)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "Error during IasAlsaHandler::getRingBuffer:", toString(result));
      return eIasFailed;
    }
    if (mSinkDeviceRingBuffer == nullptr)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "Error during IasAlsaHandler::getRingBuffer: mSinkDeviceRingBuffer is null, result = ", toString(result));
      return eIasFailed;
    }
  }
  else if (mSinkDevice->isSmartXClient())
  {
    IasSmartXClientPtr smartXClient = nullptr;
    mSinkDevice->getConcreteDevice(&smartXClient);
    IAS_ASSERT(smartXClient != nullptr); // already tested by IasAudioDevice::isSmartXClient()

    IasSmartXClient::IasResult result = smartXClient->getRingBuffer(&mSinkDeviceRingBuffer);
    if (result != IasSmartXClient::eIasOk)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "Error during IasSmartXClient::getRingBuffer:", toString(result));
      return eIasFailed;
    }
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "Error: sink device", sinkDeviceParams->name, "is neither alsaHandler nor smartXClient!");
    return eIasFailed;
  }


  IAS_ASSERT(mSinkDeviceRingBuffer != nullptr); // already checked in IasAudioDevice::isAlsaHandler() / IasSmartXClient::getRingBuffer()
  IasAudioRingBufferResult rbResult = mSinkDeviceRingBuffer->getDataFormat(&mSinkDeviceDataFormat);
  if (rbResult != IasAudioRingBufferResult::eIasRingBuffOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "Error during IasAudioRingBuffer::getDataFormat:", toString(rbResult));
    return eIasFailed;
  }

  mSinkDeviceNumChannels = mSinkDeviceRingBuffer->getNumChannels();
  if(mSinkDeviceNumChannels == 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "Error during IasAudioRingBuffer::getNumChannels, ringbuffer returned 0 channels");
    return eIasFailed;
  }

  std::lock_guard<std::mutex> lk(mMutexConversionBuffers);
  if (mConversionBufferParamsMap.begin() == mConversionBufferParamsMap.end())
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "Error: routing zone does not include any conversion buffers");
    return eIasFailed;
  }

  // Loop over all conversion buffers, i.e., loop over all routingZoneInputPorts
  for (IasConversionBufferParamsMap::const_iterator mapIt = mConversionBufferParamsMap.begin();
       mapIt != mConversionBufferParamsMap.end(); mapIt++)
  {
    const IasAudioPortPtr&           routingZoneInputPort   = mapIt->first;
    const IasConversionBufferParams& conversionBufferParams = mapIt->second;
    const IasAudioRingBuffer* conversionBuffer     = conversionBufferParams.ringBuffer;
    const IasAudioPortPtr&    sinkDeviceInputPort  = conversionBufferParams.sinkDeviceInputPort;

    IasAudioRingBufferResult cbResult;
    IasAudioCommonDataFormat conversionBufferDataFormat;
    cbResult = conversionBuffer->getDataFormat(&conversionBufferDataFormat);
    if (cbResult != eIasRingBuffOk)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE,
                  "Error during IasAudioRingBuffer::getDataFormat method of the conversion buffer:", toString(cbResult));
      return eIasFailed;
    }

    IasAudioArea *conversionBufferAreas = nullptr;
    uint32_t conversionBufferNumChannels = 0;
    cbResult = conversionBuffer->getAreas(&conversionBufferAreas);
    if (cbResult != eIasRingBuffOk)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE,
                  "Error during IasAudioRingBuffer::getAreas method of the conversion buffer:", toString(cbResult));
      return eIasFailed;
    }
    conversionBufferNumChannels = conversionBuffer->getNumChannels();
    if (conversionBufferNumChannels == 0)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE,
                        "Error during IasAudioRingBuffer::getNumChannels method of the conversion buffer: buffer returned 0 channels");
    }

    // Check if this conversionBuffer (routing zone input port) is connected to a sink device input port
    // and verify that both ports have the same number of channels.
    if (sinkDeviceInputPort != nullptr)
    {
      IasAudioPortCopyInformation sinkDeviceInputPortCopyInfo;
      IasAudioPort::IasResult portResult = sinkDeviceInputPort->getCopyInformation(&sinkDeviceInputPortCopyInfo);
      if (portResult != IasAudioPort::eIasOk)
      {
        return eIasFailed;
      }
      uint32_t sinkDeviceInputPortNumChannels = sinkDeviceInputPortCopyInfo.numChannels;

      if (conversionBufferNumChannels != sinkDeviceInputPortNumChannels)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE,
                    "Error: number of channels do not match: routingZoneInputPort", routingZoneInputPort->getParameters()->name,
                    "has", conversionBufferNumChannels,
                    "channels, sinkDeviceInputPort", sinkDeviceInputPort->getParameters()->name,
                    "has", sinkDeviceInputPortNumChannels,
                    "channels");
      }
    }
    else
    {
      // Generate a warning (not an error) if the conversionBuffer is not connected.
      DLT_LOG_CXX(*mLog, DLT_LOG_WARN, LOG_PREFIX, LOG_ZONE,
                  "routingZoneInputPort", mapIt->first->getParameters()->name, "is not connected to any sink device input ports");
    }
  }

  return eIasOk;
}


IasRoutingZoneWorkerThread::IasResult IasRoutingZoneWorkerThread::addConversionBuffer(const IasAudioPortPtr audioPort, IasAudioRingBuffer* conversionBuffer)
{
  IAS_ASSERT(audioPort != nullptr);
  IAS_ASSERT(conversionBuffer != nullptr);

  // Verify that the mConversionBufferParamsMap does not already contain this conversion buffer.
  for (const IasConversionBufferParamsPair& it :mConversionBufferParamsMap)
  {
    const IasConversionBufferParams tmpParams = it.second;
    if (tmpParams.ringBuffer == conversionBuffer)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE,
                  "Error: cannot add conversion buffer; conversion buffer has already been added");
      return eIasFailed;
    }
  }

  if (mConversionBufferParamsMap.find(audioPort) != mConversionBufferParamsMap.end())
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE,
                "Error: cannot add conversion buffer; audio port has already been added");
    return eIasFailed;
  }

  IasConversionBufferParams tmpParams;
  tmpParams.ringBuffer = conversionBuffer;

  // Add the new conversion buffer to the mConversionBufferParamsMap
  IasConversionBufferParamsPair tmp = std::make_pair(audioPort, tmpParams);
  mConversionBufferParamsMap.insert(tmp);

  return eIasOk;

}


void IasRoutingZoneWorkerThread::deleteConversionBuffer(const IasAudioPortPtr audioPort)
{
  IAS_ASSERT(audioPort != nullptr);
  std::lock_guard<std::mutex> lk(mMutexConversionBuffers);
  mConversionBufferParamsMap.erase(audioPort);
}

IasAudioRingBuffer* IasRoutingZoneWorkerThread::getConversionBuffer(const IasAudioPortPtr audioPort)
{
  IAS_ASSERT(audioPort != nullptr);

  // Try to find audioPort in mConversionBufferParamsMap. Return nullptr, if it does not exist.
  IasConversionBufferParamsMap::const_iterator it = mConversionBufferParamsMap.find(audioPort);
  if (it == mConversionBufferParamsMap.end())
  {
    return nullptr;
  }

  // Extract the pointer to the ring buffer and return.
  IasConversionBufferParams tmpParams = it->second;
  return tmpParams.ringBuffer;
}


IasRoutingZoneWorkerThread::IasResult IasRoutingZoneWorkerThread::linkAudioPorts(IasAudioPortPtr zoneInputPort, IasAudioPortPtr sinkDeviceInputPort)
{
  IAS_ASSERT(zoneInputPort != nullptr);       // Already checked in IasSetupImpl
  IAS_ASSERT(sinkDeviceInputPort != nullptr); // Already checked in IasSetupImpl

  // Verify that the zoneInputPort has been added to the routing zone before.
  if (mConversionBufferParamsMap.find(zoneInputPort) == mConversionBufferParamsMap.end())
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE,
                "Error: cannot link zoneInputPort", zoneInputPort->getParameters()->name,
                "to sinkDeviceInputPort, because zoneInputPort has not been added to the routing zone before.");
    return eIasFailed;
  }

  // Verify that the sinkDeviceInputPort is not already linked with another zoneInputPort.
  for (const IasConversionBufferParamsPair& it :mConversionBufferParamsMap)
  {
    const IasConversionBufferParams tmpParams = it.second;
    if (tmpParams.sinkDeviceInputPort == sinkDeviceInputPort)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE,
                  "Error: cannot link zoneInputPort to sinkDeviceInputPort", sinkDeviceInputPort->getParameters()->name,
                  "because sinkDeviceInputPort is already linked with another zoneInputPort");
      return eIasFailed;
    }
  }

  // Verify that the sinkDeviceInputPort belongs to the sink device that has been linked to this routing zone.
  const IasAudioDevice::IasAudioPortSetPtr sinkDeviceAudioPortSet = mSinkDevice->getAudioPortSet();
  if (sinkDeviceAudioPortSet->find(sinkDeviceInputPort) == sinkDeviceAudioPortSet->end())
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE,
                "Error: cannot link zoneInputPort to sinkDeviceInputPort", sinkDeviceInputPort->getParameters()->name,
                "because sinkDeviceInputPort does not belong to the sink device that is connected to the routing zone");
    return eIasFailed;
  }

  // Add the sinkDeviceInputPort to the mConversionBufferParamsMap for the specified zoneInputPort.
  mConversionBufferParamsMap[zoneInputPort].sinkDeviceInputPort = sinkDeviceInputPort;

  return eIasOk;
}


void IasRoutingZoneWorkerThread::unlinkAudioPorts(IasAudioPortPtr zoneInputPort, IasAudioPortPtr)
{
  IAS_ASSERT(zoneInputPort != nullptr);       // Already checked in IasSetupImpl

  // Verify that the zoneInputPort has been added to the routing zone before.
  if (mConversionBufferParamsMap.find(zoneInputPort) == mConversionBufferParamsMap.end())
  {
    return;
  }

  mConversionBufferParamsMap[zoneInputPort].sinkDeviceInputPort = nullptr;
}


IasRoutingZoneWorkerThread::IasResult IasRoutingZoneWorkerThread::addPipeline(IasPipelinePtr pipeline)
{
  IAS_ASSERT(pipeline != nullptr); // already checked in IasSetupImpl::addPipeline

  // Check whether routing zone already owns a different pipeline.
  if ((mPipeline != nullptr) && (mPipeline != pipeline))
  {
    IasPipelineParamsPtr newPipelineParams      = pipeline->getParameters();
    IasPipelineParamsPtr existingPipelineParams = mPipeline->getParameters();
    IAS_ASSERT(newPipelineParams      != nullptr); // already checked in constructor of IasPipeline
    IAS_ASSERT(existingPipelineParams != nullptr); // already checked in constructor of IasPipeline

    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE,
                "Error: cannot add pipeline", newPipelineParams->name,
                "since the routing zone already owns pipeline", existingPipelineParams->name);
    return eIasFailed;
  }

  // If a sink device has been added to the routing zone, verify that the period size and the sample rate
  // of the pipeline match to the corresponding parameters of the sink device.
  if (mSinkDevice != nullptr)
  {
    IasPipelineParamsPtr pipelineParams = pipeline->getParameters();
    IAS_ASSERT(pipelineParams != nullptr); // already checked in constructor of IasPipeline
    if (mSinkDevice->getPeriodSize() != pipelineParams->periodSize)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE,
                  "Error, periodSizes do not match: sink device:", mSinkDevice->getPeriodSize(),
                  "pipeline:", pipelineParams->periodSize);
      return eIasFailed;
    }
    if (mSinkDevice->getSampleRate() != pipelineParams->samplerate)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE,
                  "Error, sample rates do not match: sink device:", mSinkDevice->getSampleRate(),
                  "pipeline:", pipelineParams->samplerate);
      return eIasFailed;
    }
  }

  mPipeline = pipeline;
  return eIasOk;
}

void IasRoutingZoneWorkerThread::getLinkedSinkPort(IasAudioPortPtr zoneInputPort, IasAudioPortPtr *sinkDeviceInputPort)
{
  IAS_ASSERT(zoneInputPort != nullptr);       // Already checked in IasSetupImpl

  if (mConversionBufferParamsMap[zoneInputPort].sinkDeviceInputPort != nullptr)
  {
    *sinkDeviceInputPort = mConversionBufferParamsMap[zoneInputPort].sinkDeviceInputPort;
  }
  else
  {
    *sinkDeviceInputPort = nullptr; //not linked to a sink device
  }
}

void IasRoutingZoneWorkerThread::deletePipeline()
{
  mMutexPipeline.lock(); // avoid that the pipeline is currently executed
  mPipeline.reset();
  mMutexPipeline.unlock();
}


IasRoutingZoneWorkerThread::IasResult IasRoutingZoneWorkerThread::addBasePipeline(IasPipelinePtr pipeline)
{
  IAS_ASSERT(pipeline != nullptr); // already checked in

  // Check whether routing zone already owns a different pipeline.
  if ((mBasePipeline != nullptr) && (mBasePipeline != pipeline))
  {
    IasPipelineParamsPtr newPipelineParams      = pipeline->getParameters();
    IasPipelineParamsPtr existingPipelineParams = mBasePipeline->getParameters();
    IAS_ASSERT(newPipelineParams      != nullptr); // already checked in constructor of IasPipeline
    IAS_ASSERT(existingPipelineParams != nullptr); // already checked in constructor of IasPipeline

    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE,
                "Error: cannot add base pipeline", newPipelineParams->name,
                "since the routing zone already owns base pipeline", existingPipelineParams->name);
    return eIasFailed;
  }

  // If a sink device has been added to the routing zone, verify that the period size and the sample rate
  // of the base pipeline match to the corresponding parameters of the sink device.
  if (mSinkDevice != nullptr)
  {
    IasPipelineParamsPtr pipelineParams = pipeline->getParameters();
    IAS_ASSERT(pipelineParams != nullptr); // already checked in constructor of IasPipeline
    if (mSinkDevice->getPeriodSize() != pipelineParams->periodSize)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE,
                  "Error, periodSizes do not match: sink device:", mSinkDevice->getPeriodSize(),
                  "base pipeline:", pipelineParams->periodSize);
      return eIasFailed;
    }
    if (mSinkDevice->getSampleRate() != pipelineParams->samplerate)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE,
                  "Error, sample rates do not match: sink device:", mSinkDevice->getSampleRate(),
                  "base pipeline:", pipelineParams->samplerate);
      return eIasFailed;
    }
  }

  mBasePipeline = pipeline;
  return eIasOk;
}


void IasRoutingZoneWorkerThread::deleteBasePipeline()
{
  mMutexPipeline.lock(); // avoid that the pipeline is currently executed
  mBasePipeline.reset();
  mMutexPipeline.unlock();
}

/**
 * @brief Private function: transfer one period of PCM frames from the routing zone to the linked audio sink device.
 */
IasRoutingZoneWorkerThread::IasResult IasRoutingZoneWorkerThread::transferPeriod()
{
  IAS_ASSERT(mSinkDevice != nullptr);           // already checked in prepareStates()
  IAS_ASSERT(mSinkDeviceRingBuffer != nullptr); // already checked in prepareStates()
  IAS_ASSERT(mSinkDevice != nullptr);           // already checked in prepareStates()

  std::lock_guard<std::timed_mutex> transferLock(mMutexTransferInProgress);
  // Check here again if we are in active state and if we are not active exit immediately
  // This check is required after the mutex is locked here, because there is a potential race condition
  // with the check being done outside of this method
  if (mCurrentState != eIasActive)
  {
    return eIasOk;
  }

  // Check if we got an event from the linked sink device
  IasAudioDevice::IasEventType eventType = IasAudioDevice::IasEventType::eIasNoEvent;
  IasAudioDevice::IasEventType lastEvent = IasAudioDevice::IasEventType::eIasNoEvent;
  while ((eventType = mSinkDevice->getNextEventType()) != IasAudioDevice::IasEventType::eIasNoEvent)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Got event", toString(eventType), "from sink", mSinkDevice->getName());
    lastEvent = eventType;
    if (eventType == IasAudioDevice::IasEventType::eIasStop)
    {
      // In case of an stop event, we will reset all buffers and all states to initial state to restart synchronization
      if (isActive())
      {
        changeState(eIasInactivate, false);
        for (auto &entry : mConversionBufferParamsMap)
        {
          mSwitchMatrix->lockJob(entry.first);
        }
        mSinkDeviceRingBuffer->resetFromReader();
        clearConversionBuffers();
        changeState(eIasPrepare);
      }
    }
  }
  if (lastEvent == IasAudioDevice::IasEventType::eIasStop)
  {
    return eIasOk;
  }

  // Check the probing queue for any outstanding probing actions
  IasProbingQueueEntry probingQueueEntry;
  while(mProbingQueue.try_pop(probingQueueEntry))
  {
    IasDataProbe::IasResult probeRes = IasDataProbeHelper::processQueueEntry(probingQueueEntry, &mDataProbe, &mProbingActive, mSinkDevice->getNumPeriods()*mPeriodSize);
    if (probeRes != IasDataProbe::eIasOk)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "error during ", toString(probingQueueEntry.action)," :", toString(probeRes));
    }
  }

  // Setup a vector of boolean flags, which indicates for each channel of the sink device
  // whether this channel has received valid PCM samples from one of the routing zone input ports.
  const IasAudioDeviceParamsPtr sinkDeviceParams = mSinkDevice->getDeviceParams();
  IAS_ASSERT(sinkDeviceParams != nullptr); // already checked in constructor of IasAudioDevice.
  const uint32_t cNumChannelsSinkDevice = sinkDeviceParams->numChannels;
  bool isChannelServiced[cNumChannelsSinkDevice];
  for (uint32_t channel = 0; channel < cNumChannelsSinkDevice; channel++)
  {
    isChannelServiced[channel] = false;
  }

  IasAudioRingBufferResult result;

  // Call the updateAvailable method of the associated sink device
  // and identify the number of frames that are available.
  uint32_t sinkDeviceNumFramesAvailable = 0;
  result = mSinkDeviceRingBuffer->updateAvailable(IasRingBufferAccess::eIasRingBufferAccessWrite,
                                                  &sinkDeviceNumFramesAvailable);
  if (result == IasAudioRingBufferResult::eIasRingBuffTimeOut)
  {
    if (mTimeoutCnt > mLogInterval || mTimeoutCnt == 0)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "Timeout during IasAudioRingBuffer::updateAvailable. Trying to continue.");
      mTimeoutCnt = 0;
    }
    mTimeoutCnt++;
    sinkDeviceNumFramesAvailable = 0;
  }
  else if (result != IasAudioRingBufferResult::eIasRingBuffOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "Error during IasAudioRingBuffer::updateAvailable:", toString(result));
    if ((result == eIasRingBuffAlsaXrunError) || (result == eIasRingBuffAlsaSuspendError) || (result == eIasRingBuffAlsaError))
    {
      // Create event
      IasSetupEventPtr event = mEventProvider->createSetupEvent();
      event->setEventType(IasSetupEvent::eIasUnrecoverableSinkDeviceError);
      event->setSinkDevice(mSinkDevice);
      mEventProvider->send(event);
    }
    return eIasFailed;
  }
  else
  {
    mTimeoutCnt = 0;
    DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX, LOG_ZONE, "space available in sink device:",sinkDeviceNumFramesAvailable,"period size is:",mPeriodSize);
  }
  // If this is a base zone: trigger switchmatrix here to have all data ready in conversion buffers.
  if (!mIsDerivedZone)
  {
    mSwitchMatrix->trigger();
  }

  if (sinkDeviceNumFramesAvailable < mPeriodSize)
  {
    // The sink device ring buffer is full. This can happen in a derived zone with an ALSA capture plugin as sink device, when
    // the application is not able to read from the capture device for whatever reason (scheduling issue, tired of reading samples, etc.).
    // In this case we simply zero out the samples in the ring buffer but keep the buffer filled. This will lead to the situation, that
    // the samples cannot be copied to the sink device ring buffer and will be discarded. However, when the application sometime later
    // starts reading again from the ring buffer it will first get the zeroed out periods and afterwards the correct samples. If we would
    // not zero out the samples here, we would pass already outdated samples to the application, which is not intended.
    mSinkDeviceRingBuffer->zeroOut();
    if (mLogCnt > mLogInterval || mLogCnt == 0)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_ZONE, "Only", sinkDeviceNumFramesAvailable, "frames free space available, but", mPeriodSize, "required. Zeroed out sink device buffer.");
      mLogCnt = 0;
    }
    mLogCnt++;
    mLogOkCnt = 0;
  }
  else
  {
    mLogOkCnt++;
    if (mLogOkCnt > 10)
    {
      // Only if the buffer fill level is ok for at least 10 times, then we can reset the mLogCnt. This avoids flooding the log when the buffer fill level toggles
      // between ok and not ok too often.
      mLogCnt = 0;
      mLogOkCnt = 0;
    }
  }

  // Decide whether the sink device provides enough space so that we can write
  // a period of PCM frames into the ring buffer of the sink. If the sink does
  // not provide enough space, we will discard the samples from the conversion
  // buffers. This could happen, if the routing zone is a derived zone that
  // writes into an ALSA handler using the non-blocking mode.
  bool writeToSinkDevice = (sinkDeviceNumFramesAvailable >= mPeriodSize);

  uint32_t sinkDeviceOffset    = 0;
  uint32_t sinkDeviceNumFrames = mPeriodSize;
  IasAudio::IasAudioArea* sinkDeviceAreas = nullptr;

  if (writeToSinkDevice)
  {
    result = mSinkDeviceRingBuffer->beginAccess(eIasRingBufferAccessWrite, &sinkDeviceAreas,
                                                &sinkDeviceOffset, &sinkDeviceNumFrames);

    // Since we always write blocks of mPeriodSize PCM frames to the sink device, the number
    // of contiguous frames in the sink device must never be smaller than mPeriodSize,
    // as long as (sinkDeviceNumFramesAvailable >= mPeriodSize), which we have checked above.
    IAS_ASSERT(sinkDeviceNumFrames >= mPeriodSize);

    if (result != IasAudioRingBufferResult::eIasRingBuffOk)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "Error during IasAudioRingBuffer::beginAccess:", toString(result));
      if (result == eIasRingBuffAlsaError)
      {
        // Create event
        IasSetupEventPtr event = mEventProvider->createSetupEvent();
        event->setEventType(IasSetupEvent::eIasUnrecoverableSinkDeviceError);
        event->setSinkDevice(mSinkDevice);
        mEventProvider->send(event);
      }
      return eIasFailed;
    }
  }

  // Loop over all conversion buffers, i.e., loop over all routingZoneInputPorts
  std::lock_guard<std::mutex> lk(mMutexConversionBuffers);
  for (IasConversionBufferParamsMap::iterator mapIt = mConversionBufferParamsMap.begin();
       mapIt != mConversionBufferParamsMap.end(); mapIt++)
  {
    const IasAudioPortPtr&     routingZoneInputPort   = mapIt->first;
    IasConversionBufferParams& conversionBufferParams = mapIt->second;
    IasAudioRingBuffer* conversionBuffer    = conversionBufferParams.ringBuffer;
    IasStreamingState&  streamingState      = conversionBufferParams.streamingState;
    IasAudioPortPtr&    sinkDeviceInputPort = conversionBufferParams.sinkDeviceInputPort;

    uint32_t sinkDeviceInputPortNumChannels = 0;
    uint32_t sinkDeviceInputPortIndex       = 0;

    // If the routing zone input port is connected to the sink device input port, setup copy information.
    bool isLinkedToSinkDeviceInputPort = (sinkDeviceInputPort != nullptr);
    if (isLinkedToSinkDeviceInputPort)
    {
      IasAudioPortCopyInformation sinkDeviceInputPortCopyInfo;
      IasAudioPort::IasResult portResult = sinkDeviceInputPort->getCopyInformation(&sinkDeviceInputPortCopyInfo);
      IAS_ASSERT(portResult == IasAudioPort::eIasOk);
      (void)portResult;
      sinkDeviceInputPortNumChannels = sinkDeviceInputPortCopyInfo.numChannels;
      sinkDeviceInputPortIndex       = sinkDeviceInputPortCopyInfo.index;
    }

    IasAudioCommonDataFormat conversionBufferDataFormat;
    result = conversionBuffer->getDataFormat(&conversionBufferDataFormat);
    IAS_ASSERT(result == eIasRingBuffOk); // already checked in prepareStates()

    IasAudioArea *conversionBufferAreas = nullptr;
    uint32_t conversionBufferNumChannels = 0;
    result = conversionBuffer->getAreas(&conversionBufferAreas);
    IAS_ASSERT(result == eIasRingBuffOk); // already checked in prepareStates()
    conversionBufferNumChannels = conversionBuffer->getNumChannels();
    IAS_ASSERT(conversionBufferNumChannels != 0); // already checked in prepareStates()

    // Transfer the PCM frames from the conversion buffer to the sink device.
    // We might need several chunks, if conversion buffer provides the PCM frames
    // not contiguously. This is implemented by the while loop. This could happen
    // if the routing zone is a derived zone whose periodSize is larger than the
    // period size of the switchmatrix.
    uint32_t numFramesTransferred = 0;
    while (numFramesTransferred < sinkDeviceNumFrames)
    {
      uint32_t numFramesToTransfer = sinkDeviceNumFrames - numFramesTransferred;
      uint32_t conversionBufferNumSamplesAvail = 0;
      result = conversionBuffer->updateAvailable(eIasRingBufferAccessRead, &conversionBufferNumSamplesAvail);
      if (result != eIasRingBuffOk)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE,
                    "Error during IasAudioRingBuffer::updateAvailable method of the conversion buffer:", toString(result));
        return eIasFailed;
      }

      // State machine for the streaming states: bufferEmpty, bufferPartlyFromEmpty, bufferFull, partlyFromFull.
      IasStreamingState streamingStatePrevious = streamingState;
      if (conversionBufferNumSamplesAvail == 0)
      {
        streamingState = eIasStreamingStateBufferEmpty;
      }
      else if (conversionBufferNumSamplesAvail >= numFramesToTransfer)
      {
        streamingState = eIasStreamingStateBufferFull;
      }
      else if (streamingState == eIasStreamingStateBufferEmpty)
      {
        streamingState = eIasStreamingStateBufferPartlyFromEmpty;
      }
      else if (streamingState == eIasStreamingStateBufferFull)
      {
        streamingState = eIasStreamingStateBufferPartlyFromFull;
      }

      // Print out the new status of the state machine, if it has changed.
      if (streamingState != streamingStatePrevious)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_ZONE, "Changing to state", toString(streamingState), "conversionBufferNumSamplesAvail=", conversionBufferNumSamplesAvail, "numFramesToTransfer=", numFramesToTransfer);
      }

      // Ask the conversion buffer for the number of contiguous frames available.
      uint32_t conversionBufferOffset = 0;
      uint32_t conversionBufferNumFrames = numFramesToTransfer;
      result = conversionBuffer->beginAccess(eIasRingBufferAccessRead, &conversionBufferAreas,
                                             &conversionBufferOffset, &conversionBufferNumFrames);
      IAS_ASSERT(numFramesToTransfer >= conversionBufferNumFrames);

      if (result != eIasRingBuffOk)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE,
                    "Error during IasAudioRingBuffer::beginAccess method of the conversion buffer:", toString(result));
        return eIasFailed;
      }

      // Determine how many frames we will read from the conversion buffer
      // and how many frames we will write into the sink device.
      uint32_t numFramesToRead;
      uint32_t numFramesToWrite;
      if ((streamingState == eIasStreamingStateBufferEmpty) ||
          (streamingState == eIasStreamingStateBufferPartlyFromEmpty))
      {
        // Don't read from the conversion buffer. Instead, write zeros to the sink device.
        numFramesToRead  = 0;
        numFramesToWrite = numFramesToTransfer;
      }
      else
      {
        // Copy from the conversion buffer to the sink device.
        numFramesToRead  = conversionBufferNumFrames;
        numFramesToWrite = conversionBufferNumFrames;
      }

      // If the routing zone owns a pipeline, provide the PCM data from the current port to the pipeline.
      // The pipeline decides internally whether it needs the PCM frames from this routingZoneInputPort.
      // The actual pipeline processing will be done later (on a basis of a complete period).
      mMutexPipeline.lock(); // avoid that the pipeline is erased
      if (mPipeline != nullptr)
      {
        uint32_t numFramesRemaining;
        IasPipeline::IasResult pipelineResult = mPipeline->provideInputData(routingZoneInputPort,
                                                                            conversionBufferOffset,
                                                                            numFramesToRead,
                                                                            numFramesToWrite,
                                                                            &numFramesRemaining);
        (void)numFramesRemaining;
        IAS_ASSERT(pipelineResult == IasPipeline::eIasOk);
        (void)pipelineResult;
      }
      mMutexPipeline.unlock();

      // If there is a direct link for this audio port, copy the data directly.
      if (writeToSinkDevice && isLinkedToSinkDeviceInputPort)
      {
        // Copy from conversion buffer into input buffer of ALSA handler.
        copyAudioAreaBuffers(sinkDeviceAreas, mSinkDeviceDataFormat, sinkDeviceOffset + numFramesTransferred,
                             sinkDeviceInputPortNumChannels, sinkDeviceInputPortIndex,
                             numFramesToWrite,
                             conversionBufferAreas, conversionBufferDataFormat, conversionBufferOffset,
                             conversionBufferNumChannels, 0,
                             numFramesToRead);
        // Mark the affected channels as already serviced.
        for (uint32_t channel = 0; channel < sinkDeviceInputPortNumChannels; channel++)
        {
          IAS_ASSERT(channel+sinkDeviceInputPortIndex < cNumChannelsSinkDevice);
          isChannelServiced[channel+sinkDeviceInputPortIndex] = true;
        }
      }

      result = conversionBuffer->endAccess(eIasRingBufferAccessRead, conversionBufferOffset, numFramesToRead);
      if (result != eIasRingBuffOk)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE,
                    "Error during IasAudioRingBuffer::endAccess method of the conversion buffer:", toString(result));
        return eIasFailed;
      }
      numFramesTransferred += numFramesToWrite;
    }
    // Now we have transferred sinkDeviceNumFrames PCM frames from one conversion buffer.
  }
  // Now we have serviced all conversion buffers.

  if (writeToSinkDevice)
  {
    // Iterate over all channels of the sink device and write zeros to all channels
    // that have not been serviced by the direct link before.
    for (uint32_t channel = 0; channel < cNumChannelsSinkDevice; channel++)
    {
      if (!(isChannelServiced[channel]))
      {
        zeroAudioAreaBuffers(sinkDeviceAreas, mSinkDeviceDataFormat, sinkDeviceOffset, 1, channel, sinkDeviceNumFrames);
      }
    }

    // If the routing zone owns a pipeline, execute the pipeline and write the processed PCM frames to the sink device.
    // The pipeline decides internally which audio ports (or which audio channels) need to be processed.
    mMutexPipeline.lock(); // avoid that the pipeline is erased
    if (mBasePipeline != nullptr)
    {
      // If there is a base pipeline, we simply retrieve all channels already processed for that sink
      mBasePipeline->retrieveOutputData(mSinkDevice, sinkDeviceAreas, mSinkDeviceDataFormat, sinkDeviceNumFrames, sinkDeviceOffset);
    }
    if (mPipeline != nullptr)
    {
      mPipeline->process();
      mPipeline->retrieveOutputData(mSinkDevice, sinkDeviceAreas, mSinkDeviceDataFormat, sinkDeviceNumFrames, sinkDeviceOffset);
    }
    mMutexPipeline.unlock();

    // Execute the data probing.
    if (mDataProbe)
    {
      IasDataProbe::IasResult probeRes =  mDataProbe->process(sinkDeviceAreas, sinkDeviceOffset, sinkDeviceNumFrames);
      if(probeRes != IasDataProbe::eIasOk)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "delete probe");
        mProbingActive.store(false);
        mDataProbe = nullptr;
      }
    }

    // Call the endAccess method of the linked sink device.
    DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX, LOG_ZONE, "Calling IasAudioRingBuffer::endAccess with sinkDeviceNumFrames=", sinkDeviceNumFrames);
    result = mSinkDeviceRingBuffer->endAccess(eIasRingBufferAccessWrite, sinkDeviceOffset, sinkDeviceNumFrames);
    if (result != IasAudioRingBufferResult::eIasRingBuffOk)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "Error during IasAudioRingBuffer::endAccess:", toString(result));
      if (result == eIasRingBuffAlsaError)
      {
        // Create event
        IasSetupEventPtr event = mEventProvider->createSetupEvent();
        event->setEventType(IasSetupEvent::eIasUnrecoverableSinkDeviceError);
        event->setSinkDevice(mSinkDevice);
        mEventProvider->send(event);
      }
      return eIasFailed;
    }

    if (mDiagnosticsStream.is_open())
    {
      // Get timestamp information and write routing zone diagnostics into the diagnostics stream.
      // If the routing zone writes into a realBuffer, the timestamp refers to the moment of the write access.
      IasAudioTimestamp  audioTimestampSinkDeviceBuffer;
      result = mSinkDeviceRingBuffer->getTimestamp(eIasRingBufferAccessWrite, &audioTimestampSinkDeviceBuffer);
      IAS_ASSERT(result == IasAudioRingBufferResult::eIasRingBuffOk);

      mDiagnosticsStream << audioTimestampSinkDeviceBuffer.timestamp  << ", "
                         << audioTimestampSinkDeviceBuffer.numTransmittedFrames << std::endl;
    }
  }

  return eIasOk;
}

void IasRoutingZoneWorkerThread::clearConversionBuffers()
{
  for (auto &entry : mConversionBufferParamsMap)
  {
    auto &convParams = entry.second;
    convParams.ringBuffer->resetFromReader();
    convParams.streamingState = eIasStreamingStateBufferEmpty;
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_ZONE, "Cleared conversion buffer of port", entry.first->getParameters()->name);
  }
}

IasRoutingZoneWorkerThread::IasResult IasRoutingZoneWorkerThread::start()
{
  if (mThread == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "Error due to non-initialized component");
    return eIasNotInitialized;
  }

  // Check that mSwitchMatrix has been initialized, since the run() method depends on it.
  if (mSwitchMatrix == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE,
                "Error: Switch Matrix Worker Thread has not been set (mSwitchMatrixWorkerThread == nullptr)");
    return eIasFailed;
  }

  // Set the streaming state of all conversion buffers to empty.
  std::lock_guard<std::mutex> lk(mMutexConversionBuffers);
  for (IasConversionBufferParamsMap::iterator mapIt = mConversionBufferParamsMap.begin();
       mapIt != mConversionBufferParamsMap.end(); mapIt++)
  {
    IasConversionBufferParams& conversionBufferParams = mapIt->second;
    conversionBufferParams.streamingState = eIasStreamingStateBufferEmpty;
  }

  mThreadIsRunning = true;
  mThread->start(true);
  for (auto &thread : mRunnersParamsMap)
  {
   DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_ZONE, "Starting runner thread for derived zone with countPeriods = ", thread.second.countPeriods);
   IasRunnerThread::IasResult res = thread.first->start();
   if (res != IasRunnerThread::IasResult::eIasOk)
   {
     DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "Failed to start runner thread, result = ", toString(res));
     return eIasFailed;
   }
  }
  return eIasOk;
}


IasRoutingZoneWorkerThread::IasResult IasRoutingZoneWorkerThread::stop()
{
  if (mThread == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "Error due to non-initialized component (mThread == nullptr)");
    return eIasNotInitialized;
  }

  for (auto &thread : mRunnersParamsMap)
  {
   DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_ZONE, "Stopping runner thread for derived zone with countPeriods = ", thread.second.countPeriods);
   if (thread.first->isAnyActive())
   {
       DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_ZONE, "cannot stop runner thread since at least one derived RZ active");
       continue;
   }
   IasRunnerThread::IasResult res = thread.first->stop();
   if (res != IasRunnerThread::IasResult::eIasOk)
   {
     DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "Failed to stop runner thread, result = ", toString(res));
     return eIasFailed;
   }
  }
  mThread->stop();
  // A derived RZ may have it processing handled by a runner thread, prevent transfering period
  // before stopping the ALSA handler.
  changeState(eIasInactivate);

  if ((mSinkDevice != nullptr) && (mSinkDevice->isAlsaHandler()))
  {
    IasAlsaHandlerPtr alsaHandler;
    mSinkDevice->getConcreteDevice(&alsaHandler);
    IAS_ASSERT(alsaHandler != nullptr); // already tested by IasAudioDevice::isAlsaHandler()

    // Stop the ALSA handler
    alsaHandler->stop();
  }

  return eIasOk;
}


IasAudioCommonResult IasRoutingZoneWorkerThread::beforeRun()
{
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_ZONE);
  return eIasResultOk;
}

void IasRoutingZoneWorkerThread::changeState(IasAction action, bool lock /* = true */)
{
  if (mCurrentState == eIasInActive)
  {
    if (action == eIasPrepare)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_ZONE, "Change state from inactive to active pending");
      if (mIsDerivedZone == true)
      {
        prefillSinkBuffer();
      }
      mCurrentState = eIasActivePending;
    }
  }
  else if (mCurrentState == eIasActivePending)
  {
    if (action == eIasActivate)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_ZONE, "Change state from active pending to active");
      mCurrentState = eIasActive;
    }
    else if (action == eIasInactivate)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_ZONE, "Change state from active pending to inactive");
      mCurrentState = eIasInActive;
    }
  }
  else if (mCurrentState == eIasActive)
  {
    if (action == eIasInactivate)
    {
      // We have to check if a transfer is currently in progress. This is simply done by using
      // the mMutexTransferinProgress, which is locked while the transfer is active.
      // We use a timed mutex here in order not to block here forever.
      // However, if this method is called from inside the method where the mutex was locked, this will lead to a deadlock.
      // This is avoided by passing in the parameter lock = false.
      bool lockStatus = false;
      if (lock == true)
      {
        lockStatus = mMutexTransferInProgress.try_lock_for(std::chrono::milliseconds(100));
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_ZONE, "Change state from active to inactive, lockStatus=", lockStatus);
      }
      else
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_ZONE, "Change state from active to inactive");
      }
      mCurrentState = eIasInActive;
      if (lockStatus == true)
      {
        // Unlock is only allowed after we successfully locked the mutex a few lines above.
        mMutexTransferInProgress.unlock();
      }
    }
  }
}

void IasRoutingZoneWorkerThread::activatePendingWorker()
{
  for (auto &entry : mDerivedZoneParamsMap)
  {
    IasRoutingZoneWorkerThreadPtr derivedZoneWorkerThread = entry.first;
    if (derivedZoneWorkerThread->isActivePending())
    {
      // Trigger the derived zone now by setting the countPeriods counter to the period size multiple
      // This ensures a synchronized start up.
      entry.second.countPeriods = entry.second.periodSizeMultiple;

      if (derivedZoneWorkerThread->isSinkServiced())
      {
        derivedZoneWorkerThread->changeState(IasRoutingZoneWorkerThread::eIasActivate);
      }
    }
  }
}


IasAudioCommonResult IasRoutingZoneWorkerThread::run()
{
  IAS_ASSERT(mSwitchMatrix != nullptr); // already checked in start() method.

  IasThreadNames::getInstance()->setThreadName(IasThreadNames::eIasRealTime, "Routing Zone worker thread for Routing Zone " + mParams->name);
  IasConfigFile::configureThreadSchedulingParameters(mLog);

  while (mThreadIsRunning == true)
  {
    if(mDerivedZoneCallCount == 0)
    {
      //unlock all switch matrix jobs
      mSwitchMatrix->unlockJobs();
      // synchronized activation of the pending worker threads
      activatePendingWorker();
    }
    // Execute the transferPeriod method of the base zone; this transfers all
    // PCM frames from the conversion buffer to the sink device.
    IasRoutingZoneWorkerThread::IasResult result = transferPeriod();
    if (result != eIasOk)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE,
                  "Error during IasRoutingZoneWorkerThread::transferPeriod method of base zone:", toString(result));
      return eIasResultFailed;
    }
    mDerivedZoneCallCount = 0;
    // Loop over all derived routing zones.

    // Process all derived zones, but these with periodSizeMultiple == 1, as those should
    // not have their own thread, but rather be processed in this one.
    for (IasRunnerThreadParamsMap::iterator mapIt = mRunnersParamsMap.begin();
         mapIt != mRunnersParamsMap.end(); mapIt++)
    {
      IasRoutingZoneRunnerThreadPtr derivedZoneRunnerThread = mapIt->first;
      if (derivedZoneRunnerThread->isAnyActive()) {
        uint32_t periodCount = derivedZoneRunnerThread->addPeriod(1);
        if (periodCount >= mapIt->second.periodSizeMultiple) {
          if (derivedZoneRunnerThread->isProcessing()) {
            DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "Runner thread for periodSizeMultiple:", derivedZoneRunnerThread->getPeriodSizeMultiple(), " is still not done processing when new periods arrived");
          }
          derivedZoneRunnerThread->wake();
          // mDerivedZoneCallCount only cares about whether there was a zone called or not
          mDerivedZoneCallCount++;
        }
      }
    }
    // Process all zones, for which we have no runner thread enabled, in this thread
    for (IasDerivedZoneParamsMap::iterator zoneMapIt = mDerivedZoneParamsMap.begin();
        zoneMapIt != mDerivedZoneParamsMap.end(); zoneMapIt++)
    {
      IasDerivedZoneParams* derivedZoneParams = &(zoneMapIt->second);
      if (zoneMapIt->first->isActive() && zoneMapIt->second.runnerEnabled == false)
      {
        derivedZoneParams->countPeriods++;
        if (derivedZoneParams->countPeriods >= derivedZoneParams->periodSizeMultiple)
        {
          derivedZoneParams->countPeriods = 0;
          zoneMapIt->first->transferPeriod();
        }
      }
    }
  }
  return eIasResultOk;
}


IasAudioCommonResult IasRoutingZoneWorkerThread::shutDown()
{
  mThreadIsRunning = false;
  return eIasResultOk;
}


IasAudioCommonResult IasRoutingZoneWorkerThread::afterRun()
{
  return eIasResultOk;
}

IasRoutingZoneWorkerThread::IasResult IasRoutingZoneWorkerThread::startProbing(const std::string &namePrefix,
                                                                               bool inject,
                                                                               uint32_t numSeconds,
                                                                               uint32_t startIndex,
                                                                               uint32_t numChannels)
{

  IasProbingQueueEntry entry;
  entry.params.name = namePrefix;
  entry.params.duration = numSeconds;
  entry.params.isInject = inject;
  entry.params.numChannels = numChannels;
  entry.params.startIndex = startIndex;
  entry.params.sampleRate = mSinkDevice->getSampleRate();
  entry.params.dataFormat = mSinkDeviceDataFormat;
  entry.action = eIasProbingStart;

  bool isProbingActive = mProbingActive.load(std::memory_order_relaxed);
  if(isProbingActive)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Probing already active");
    return eIasFailed;
  }
  mProbingQueue.push(entry);
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Name=", namePrefix, "duration=", numSeconds, "isInject=", inject,
              "numChannels=", numChannels, "startIndex=", startIndex, "sampleRate=", entry.params.sampleRate, "dataFormat=", toString(entry.params.dataFormat));
  return eIasOk;
}

void IasRoutingZoneWorkerThread::stopProbing()
{
  IasProbingQueueEntry entry;
  entry.action = eIasProbingStop;
  mProbingQueue.push(entry);
}

bool IasRoutingZoneWorkerThread::isSinkServiced() const
{
  IAS_ASSERT(mSinkDeviceRingBuffer != nullptr);
  IAS_ASSERT(mSinkDevice != nullptr);
  uint32_t avail = 0;
  IasAudioRingBufferResult rbres = mSinkDeviceRingBuffer->updateAvailable(eIasRingBufferAccessWrite, &avail);
  IAS_ASSERT(rbres == eIasRingBuffOk);
  (void)rbres;
  uint32_t periodSize = mSinkDevice->getPeriodSize();
  uint32_t bufferSize = mSinkDevice->getNumPeriods() * periodSize;
  uint32_t targetValue = 0;
  if (mSinkDevice->isAlsaHandler())
  {
    if (mSinkDevice->getDeviceParams()->clockType == IasClockType::eIasClockReceivedAsync)
    {
      // In case we have an ASRC activated in the sink, we need to start filling the ASRC buffer right away.
      // Setting the targetValue to 0 directly sets the sink to serviced and activates the processing.
      targetValue = 0;
    }
    else
    {
      // In this case we prefilled the buffer with bufferSize - periodSize zeros. If one period was consumed from
      // the buffer, then the sink device is serviced.
      targetValue = 2*periodSize;
    }
  }
  else
  {
    // In this case we prefilled the buffer with one periodSize zeros. If there is less than one period of samples in
    // the buffer or respectively more space than bufferSize - periodSize + 1, then the sink device is serviced.
    // The + 1 is required because we check for >= and not only >.
    targetValue = bufferSize - periodSize + 1;
  }

  if (avail >= targetValue)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_ZONE, "Sink device", mSinkDevice->getName(), "was serviced now. avail =", avail, "periodSize=", periodSize);
    return true;
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX, LOG_ZONE, "Sink device", mSinkDevice->getName(), "was not serviced yet. avail =", avail, "periodSize=", periodSize);
    return false;
  }
}

void IasRoutingZoneWorkerThread::prefillSinkBuffer()
{
  if (mSinkDeviceRingBuffer != nullptr)
  {
    IAS_ASSERT(mSinkDevice != nullptr);
    uint32_t periodSize = mSinkDevice->getPeriodSize();
    uint32_t bufferSize = mSinkDevice->getNumPeriods() * periodSize;
    IasAudioArea *areas = nullptr;
    uint32_t offset = 0;
    uint32_t framesToBeFilled = 0;
    if (mSinkDevice->isAlsaHandler())
    {
      // In case of an ALSA handler prefill the sink device ALSA buffer with one period size less than the buffer size.
      // The additional empty period size space is used to compensate for scheduling jitter
      framesToBeFilled = bufferSize - periodSize;
    }
    else
    {
      // In case of a SmartXbar plugin we are providing the data for an ALSA capture device. Therefor it is engough to fill only with one
      // period size. This prevents the application to read more than one period in a loop at full speed, like e.g. arecord would do.
      framesToBeFilled = periodSize;
    }
    uint32_t frames = framesToBeFilled;
    IasAudioRingBufferResult rbres = mSinkDeviceRingBuffer->beginAccess(eIasRingBufferAccessWrite, &areas, &offset, &frames);
    if (rbres == eIasRingBuffOk)
    {
      IAS_ASSERT(areas != nullptr);
      if (frames != framesToBeFilled)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "Only", frames, "samples can be filled with zero instead of", framesToBeFilled);
      }
      zeroAudioAreaBuffers(areas, mSinkDeviceDataFormat, offset, mSinkDeviceNumChannels, 0, frames);

      rbres = mSinkDeviceRingBuffer->endAccess(eIasRingBufferAccessWrite, offset, frames);
      if (rbres != eIasRingBuffOk)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "Error while trying to end access for sink ringbuffer");
      }
      else
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_ZONE, "Prefilled sink device ringbuffer with", frames, "zeros");
        const IasAudioRingBufferMirror *mirror = mSinkDeviceRingBuffer->getMirror();
        if (mirror != nullptr)
        {
          mirror->startDevice();
        }
      }
    }
    else
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "Error while trying to access sink ringbuffer");
    }
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_ZONE, "Sink device ringbuffer not initialized yet");
  }
}

bool IasRoutingZoneWorkerThread::hasPipeline(IasPipelinePtr pipeline) const
{
  return (pipeline == mPipeline);
}


/*
 * Function to get a IasRoutingZoneWorkerThread::IasResult as string.
 */
#define STRING_RETURN_CASE(name) case name: return std::string(#name); break
#define DEFAULT_STRING(name) default: return std::string(name)
std::string toString(const IasRoutingZoneWorkerThread::IasResult& type)
{
  switch(type)
  {
    STRING_RETURN_CASE(IasRoutingZoneWorkerThread::eIasOk);
    STRING_RETURN_CASE(IasRoutingZoneWorkerThread::eIasInvalidParam);
    STRING_RETURN_CASE(IasRoutingZoneWorkerThread::eIasInitFailed);
    STRING_RETURN_CASE(IasRoutingZoneWorkerThread::eIasNotInitialized);
    STRING_RETURN_CASE(IasRoutingZoneWorkerThread::eIasFailed);
    DEFAULT_STRING("Invalid IasRoutingZoneWorkerThread::IasResult => " + std::to_string(type));
  }
}


std::string toString(const IasRoutingZoneWorkerThread::IasStreamingState& streamingState)
{
  switch(streamingState)
  {
    STRING_RETURN_CASE(IasRoutingZoneWorkerThread::eIasStreamingStateBufferEmpty);
    STRING_RETURN_CASE(IasRoutingZoneWorkerThread::eIasStreamingStateBufferPartlyFromEmpty);
    STRING_RETURN_CASE(IasRoutingZoneWorkerThread::eIasStreamingStateBufferFull);
    STRING_RETURN_CASE(IasRoutingZoneWorkerThread::eIasStreamingStateBufferPartlyFromFull);
    DEFAULT_STRING("Invalid IasRoutingZoneWorkerThread::IasStreamingState => " + std::to_string(streamingState));
  }
}


std::string toString(const IasRunnerThread::IasResult& type)
{
  switch(type)
  {
    STRING_RETURN_CASE(IasRunnerThread::eIasOk);
    STRING_RETURN_CASE(IasRunnerThread::eIasInvalidParam);
    STRING_RETURN_CASE(IasRunnerThread::eIasInitFailed);
    STRING_RETURN_CASE(IasRunnerThread::eIasNotInitialized);
    STRING_RETURN_CASE(IasRunnerThread::eIasFailed);
    DEFAULT_STRING("Invalid IasRunnerThread::IasResult => " + std::to_string(type));
  }
}

} //namespace IasAudio
