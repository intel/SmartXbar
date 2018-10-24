/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file
 */


#include "switchmatrix/IasBufferTask.hpp"
#include "internal/audio/common/audiobuffer/IasAudioRingBuffer.hpp"
#include "internal/audio/common/IasDataProbe.hpp"
#include "internal/audio/common/IasDataProbeHelper.hpp"
#include "switchmatrix/IasSwitchMatrixJob.hpp"
#include "internal/audio/common/IasAudioLogging.hpp"
#include "audio/smartx/IasEventProvider.hpp"
#include "audio/smartx/IasConnectionEvent.hpp"
#include "model/IasAudioPortOwner.hpp"
#include "alsahandler/IasAlsaHandler.hpp"

namespace IasAudio {

  static const std::string cClassName = "IasBufferTask::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"


IasBufferTask::IasBufferTask(IasAudioPortPtr src,
                             uint32_t readSize,
                             uint32_t destSize,
                             uint32_t sampleRate,
                             bool isDummy)
  :mLog(IasAudioLogging::registerDltContext("BFT", "Buffer Task"))
  ,mSrcPort(src)
  ,mOrigin(nullptr)
  ,mSourcePeriodSize(readSize)
  ,mDestSize(destSize)
  ,mSampleRate(sampleRate)
  ,mEventProvider(nullptr)
  ,mIsDummy(isDummy)
  ,mDataProbe(nullptr)
  ,mProbingActive(false)
  ,mSourceState(eIasSourceUnderrun)
{
  IAS_ASSERT(readSize != 0);
  IAS_ASSERT(src != nullptr);
  mEventProvider = IasEventProvider::getInstance();
  IAS_ASSERT(mEventProvider != nullptr);
  src->getRingBuffer(&mOrigin);
  IAS_ASSERT(mOrigin != nullptr);
  IasAudioPortOwnerPtr sourceDevice = nullptr;
  IasAudioPort::IasResult portRes = src->getOwner(&sourceDevice);
  if (portRes == IasAudioPort::eIasOk)
  {
    IasAlsaHandlerPtr alsaHandler = nullptr;
    IasAudioCommonResult res = sourceDevice->getConcreteDevice(&alsaHandler);
    if ((res == IasAudioCommonResult::eIasResultOk) && alsaHandler != nullptr)
    {
      // We only need to reset the ASRC buffer here to clear the old invalid content from a previous connect.
      // If the source device is an ALSA handler, then there are two possibilities:
      // 1. The ALSA handler is asynchronous, then there is an ASRC buffer and mOrigin is pointing to that.
      //    In that case we are resetting the buffer to clear the previous content as desired.
      // 2. The ALSA handler is synchronous, then there is no ASRC buffer, but mOrigin is pointing to a
      //    mirror buffer instead. In this case the two methods below do nothing.
      // If we are resetting the buffer here in this if branch, we also ensured not to reset the real ring-buffer
      // belonging to an alsa-smartx-plugin. If we would do that, this would lead to a serious problem:
      // It wouldn't be possible anymore to fill the buffer of the alsa-smartx-plugin from an application,
      // before doing the connect. The connect would obviously delete the content, which the application
      // put intentionally into the buffer and due to the fact, that we are not able to signal this to the
      // to the application, we would end up in a dead-lock.
      mOrigin->zeroOut();
      mOrigin->resetFromWriter();

      alsaHandler->reset();
    }
  }
}

IasBufferTask::~IasBufferTask()
{
  DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX);
  mJobActionQueue.clear();
  mJobs.clear();
  if(mDataProbe != nullptr)
  {
    mDataProbe = nullptr;
  }
}

IasBufferTask::IasResult IasBufferTask::doDummy()
{
  uint32_t srcSamples = 0;
  uint32_t srcOffset = 0;

  IasAudioArea* areas = nullptr;

  IasAudioRingBufferResult rbres = mOrigin->updateAvailable(eIasRingBufferAccessRead, &srcSamples);
  IAS_ASSERT( rbres == eIasRingBuffOk);

  rbres = mOrigin->beginAccess(eIasRingBufferAccessRead, &areas, &srcOffset, &srcSamples);
  IAS_ASSERT( rbres == eIasRingBuffOk);

  rbres = mOrigin->endAccess(eIasRingBufferAccessRead, srcOffset, srcSamples);
  IAS_ASSERT( rbres == eIasRingBuffOk);

  (void)rbres;
  return eIasOk;
}

std::set<IasSwitchMatrixJobPtr>::iterator IasBufferTask::deleteJob(std::set<IasSwitchMatrixJobPtr>::iterator jobIt, IasJobAction jobAction)
{
  DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX, "Delete job for", mOrigin->getName());
  int32_t sourcePort = (*jobIt)->getSourcePortId();
  int32_t sinkPort = (*jobIt)->getSinkPortId();
  jobIt = mJobs.erase(jobIt);
  IasConnectionEventPtr event = mEventProvider->createConnectionEvent();
  if(jobAction == eIasDeleteAllSourceJobs)
  {
    event->setEventType(IasConnectionEvent::eIasSourceDeleted);
  }
  else
  {
    event->setEventType(IasConnectionEvent::eIasConnectionRemoved);
  }
  event->setSourceId(sourcePort);
  event->setSinkId(sinkPort);
  mEventProvider->send(event);
  return (jobIt);
}

IasBufferTask::IasResult IasBufferTask::doJobs()
{
  IasResult res = eIasOk;
  IasJobActionQueueEntry entry;
  entry.first = nullptr;
  entry.second = eIasAddJob;

  while(mJobActionQueue.try_pop(entry))
  {
    if (entry.second == eIasAddJob)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX, "Adding new job for", mOrigin->getName());
      mJobs.insert(entry.first);
      IasConnectionEventPtr event = mEventProvider->createConnectionEvent();
      event->setEventType(IasConnectionEvent::eIasConnectionEstablished);
      event->setSourceId(entry.first->getSourcePortId());
      event->setSinkId(entry.first->getSinkPortId());
      mEventProvider->send(event);
    }
    else if (entry.second == eIasDeleteJob)
    {
      auto jobIt = mJobs.find(entry.first);
      IAS_ASSERT(jobIt != mJobs.end()); //it should not be possible to come here and search for a job that does not exist
      deleteJob(jobIt, entry.second);
    }
    else
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Delete all jobs for source", mOrigin->getName());
      auto jobIt = mJobs.begin();
      while (jobIt != mJobs.end())
      {
        if(mSrcPort == (*jobIt)->getSourcePort())
        {
          DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Job found for source", mOrigin->getName(),
              "and sink", (*jobIt)->getSinkPort()->getParameters()->name);
          jobIt = deleteJob(jobIt, eIasDeleteAllSourceJobs);
        }
        else
        {
          ++jobIt;
        }
      }
    }
  }

  IasProbingQueueEntry probingQueueEntry;
  while(mProbingQueue.try_pop(probingQueueEntry))
  {
    IasDataProbe::IasResult probeRes = IasDataProbeHelper::processQueueEntry(probingQueueEntry, &mDataProbe, &mProbingActive, mSourcePeriodSize);
    if (probeRes != IasDataProbe::eIasOk)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "error during ", toString(probingQueueEntry.action),"for", mOrigin->getName(), ":", toString(probeRes));
    }
  }

  if(mJobs.begin() == mJobs.end())
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "no more jobs to execute for", mOrigin->getName());
    return eIasNoJobs;
  }
  uint32_t srcSamples = 0;
  uint32_t srcOffset = 0;
  IasAudioArea* areas = nullptr;
  uint32_t framesStillToConsume = 0;
  uint32_t framesToRead = mSourcePeriodSize;
  bool lockAfterLoop = false;

  do
  {
    uint32_t framesConsumed = 0;
    IasAudioRingBufferResult rbres = mOrigin->updateAvailable(eIasRingBufferAccessRead, &srcSamples);
    if(rbres != eIasRingBuffOk)
    {
     DLT_LOG_CXX(*mLog, DLT_LOG_WARN, LOG_PREFIX,"Error can not access ring buffer", mOrigin->getName(), "for read: ", toString(rbres).c_str());
    }
    DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX,"wants to have at least",mSourcePeriodSize,"samples");
    if (srcSamples == 0 && mSourceState != eIasSourceUnderrun)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_WARN, LOG_PREFIX, mOrigin->getName(), "underrun , but still", framesStillToConsume,"sample to process");
      mSourceState = eIasSourceUnderrun;
      if(framesStillToConsume == 0)
      {
        lockJobs();
      }
      else
      {
        lockAfterLoop = true; //there are still frames to consume, so first finish this before locking the jobs
      }
    }
    if (srcSamples >= mSourcePeriodSize && mSourceState == eIasSourceUnderrun)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_WARN, LOG_PREFIX, mOrigin->getName(), "playing !!");
      mSourceState = eIasSourcePlaying;
    }

    framesToRead = mSourcePeriodSize;
    if(srcSamples < mSourcePeriodSize)
    {
      framesToRead = srcSamples;
    }
    rbres = mOrigin->beginAccess(eIasRingBufferAccessRead, &areas, &srcOffset, &framesToRead);
    if(rbres != eIasRingBuffOk)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,"Error begin accessing ring buffer", mOrigin->getName(), "for read: ", toString(rbres).c_str());
    }
    IAS_ASSERT(areas != nullptr);
    if(framesStillToConsume != 0 )
    {
      if (framesToRead >= framesStillToConsume)
      {
        framesToRead = framesStillToConsume;
      }
    }
    if(mDataProbe)
    {
      IasDataProbe::IasResult probeRes = mDataProbe->process(areas,srcOffset,framesToRead);
      if(probeRes != IasDataProbe::eIasOk)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "delete probe");
        mProbingActive.store(false);
        mDataProbe = nullptr;
      }
    }
    for( auto &entry : mJobs)
    {
      IasSwitchMatrixJob::IasResult smjres = entry->updateSrcAreas(areas);
      IAS_ASSERT(smjres == IasSwitchMatrixJob::eIasOk);
      (void)smjres;
      uint32_t prevFramesConsumed = framesConsumed;
      if(entry->execute(srcOffset,framesToRead, &framesStillToConsume, &framesConsumed))
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error executing job for", mOrigin->getName(),"with src", entry->getSourcePort()->getParameters()->id, "and sink", entry->getSourcePort()->getParameters()->id);
      }
      framesConsumed = std::max(framesConsumed, prevFramesConsumed);
    }
    if(mDataProbe)
    {
      mDataProbe->updateFilePosition(framesConsumed-framesToRead);
    }
    rbres = mOrigin->endAccess(eIasRingBufferAccessRead, srcOffset, framesConsumed);
    if(rbres != eIasRingBuffOk)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,"Error end accessing", mOrigin->getName(),"for read: ",  toString(rbres).c_str());
    }
  }while(framesStillToConsume != 0);
  if (lockAfterLoop == true)
  {
    lockJobs();
  }
  return res;
}


IasBufferTask::IasResult IasBufferTask::checkConnection(IasAudioPortPtr src, IasAudioPortPtr sink)
{
  IasResult res = eIasObjectNotFound;

  IasConnectionMap::iterator connectionIt;

  connectionIt = mConnections.find(sink);
  if(connectionIt != mConnections.end())
  {
    res = eIasSinkInUse;
    if(connectionIt->second->getSourcePort() == src )
    {
      res = eIasConnectionAlreadyExists;
    }
  }
  return res;
}

IasBufferTask::IasResult IasBufferTask::addJob(IasAudioPortPtr source, IasAudioPortPtr sink)
{
  if ((source == nullptr) || (sink == nullptr))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, ": null pointer paramter passed for", mOrigin->getName());
    return eIasFailed;
  }

  IasAudioPortCopyInformation sourcePortCopyInfo;
  IasAudioPortCopyInformation sinkPortCopyInfo;
  source->getCopyInformation(&sourcePortCopyInfo);
  sink->getCopyInformation(&sinkPortCopyInfo);

  IasResult res = checkConnection(source,sink);
  if(res == eIasObjectNotFound)
  {
    IasSwitchMatrixJobPtr job = std::make_shared<IasSwitchMatrixJob>(source,sink);
    if(job->init(mDestSize,mSampleRate))
    {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error initialising new job for", mOrigin->getName());
        job = nullptr;
        return eIasFailed;
    }
    mConnections.insert(IasConnectionPair(sink,job));

    IasJobActionQueueEntry entry(job,eIasAddJob);
    mJobActionQueue.push(entry);
    return eIasOk;
  }
  else if(res == eIasSinkInUse)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Adding job failed, sink is in use for", mOrigin->getName());
    return eIasFailed;
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Adding job failed, connection already exists for", mOrigin->getName());
    return eIasFailed;
  }
}

IasBufferTask::IasResult IasBufferTask::triggerDeleteJob(IasAudioPortPtr source, IasAudioPortPtr sink,
    IasBufferTask::IasJobAction deleteReason)
{
  if (deleteReason == eIasDeleteJob)
  {
    if(checkConnection(source, sink) == eIasConnectionAlreadyExists)
    {
      IasConnectionMap::iterator connectionIt = mConnections.find(sink);
      IAS_ASSERT(connectionIt != mConnections.end());
      IasSwitchMatrixJobPtr job = connectionIt->second;
      mConnections.erase(sink);
      IasJobActionQueueEntry entry(job, deleteReason);
      mJobActionQueue.push(entry);
      return eIasOk;
    }
    else
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Delete job failed, does not exist for", mOrigin->getName());
      return eIasFailed;
    }
  }
  else if (deleteReason == eIasDeleteAllSourceJobs)
  {
    auto connectionIt = mConnections.begin();
    while (connectionIt != mConnections.end())
    {
      if (connectionIt->second->getSourcePort() == source)
      {
        connectionIt = mConnections.erase(connectionIt);
      }
      else
      {
        ++connectionIt;
      }
    }
    IasJobActionQueueEntry entry(nullptr, deleteReason);
    mJobActionQueue.push(entry);
    return eIasOk;
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Delete job failed, invalid reason(", static_cast<std::uint32_t>(deleteReason),")for", mOrigin->getName());
    return eIasFailed;
  }
}

IasBufferTask::IasResult IasBufferTask::deleteAllJobs(IasAudioPortPtr source)
{
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Delete all jobs for", mOrigin->getName());
  return triggerDeleteJob(source, nullptr, eIasDeleteAllSourceJobs);
}

IasSwitchMatrixJobPtr IasBufferTask::findJob(IasAudioPortPtr port)
{
  IasSwitchMatrixJobPtr tmp = nullptr;
  for( auto &it :mJobs)
  {
    if(port == it->getSourcePort())
    {
      tmp = it;
      break;
    }
    if(port ==  it->getSinkPort())
    {
      tmp = it;
      break;
    }
  }
  return tmp;
}

IasBufferTask::IasResult IasBufferTask::startProbing(const std::string &prefix,
                                                     uint32_t numSeconds,
                                                     bool inject,
                                                     uint32_t numChannels,
                                                     uint32_t startIndex,
                                                     uint32_t sampleRate,
                                                     IasAudioCommonDataFormat dataFormat)

{

  IasProbingParams probingParams(prefix, numSeconds,inject,numChannels,startIndex,sampleRate,dataFormat);
  IasProbingQueueEntry entry;
  entry.params.name = prefix;
  entry.params.duration = numSeconds;
  entry.params.isInject = inject;
  entry.params.numChannels = numChannels;
  entry.params.startIndex = startIndex;
  entry.params.sampleRate = sampleRate;
  entry.params.dataFormat = dataFormat;
  entry.action = eIasProbingStart;

  bool isProbingActive = mProbingActive.load(std::memory_order_relaxed);
  if(isProbingActive)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Probing already active for", mOrigin->getName());
    return eIasFailed;
  }
  mProbingQueue.push(entry);
  return eIasOk;

}

void IasBufferTask::unlockJobs()
{
  if(mSourceState != eIasSourceUnderrun)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX, "Unlock all jobs for", mOrigin->getName());
    for( auto &job : mJobs)
    {
      job->unlock();
    }
  }
}

void IasBufferTask::lockJobs()
{
  DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX, "Lock all jobs for", mOrigin->getName());
  for( auto &job : mJobs)
  {
    job->lock();
  }
}

void IasBufferTask::lockJob(IasAudioPortPtr sinkPort)
{
  DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX, "Trying to lock job for sink", sinkPort->getParameters()->name);
  auto jobIt = mConnections.find(sinkPort);
  if (jobIt != mConnections.end())
  {
    jobIt->second->lock();
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Locked job between source", jobIt->second->getSourcePort()->getParameters()->name, "and sink", sinkPort->getParameters()->name);
  }
}

void IasBufferTask::stopProbing()
{

  IasProbingQueueEntry entry;
  entry.action = eIasProbingStop;
  mProbingQueue.push(entry);

}

bool IasBufferTask::isActive() const
{
  if (mConnections.empty() == true)
  {
    return false;
  }
  else
  {
    return true;
  }
}


} // namespace IasAudio
