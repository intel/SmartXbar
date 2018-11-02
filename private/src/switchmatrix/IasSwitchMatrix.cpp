/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasSwitchMatrix.cpp
 * @date   2015
 * @brief
 */

#include "switchmatrix/IasSwitchMatrix.hpp"
#include <iostream>

#include "switchmatrix/IasBufferTask.hpp"
#include "switchmatrix/IasSwitchMatrixJob.hpp"
#include "avbaudiomodules/internal/audio/common/audiobuffer/IasAudioRingBuffer.hpp"
#include "smartx/IasConfigFile.hpp"

namespace IasAudio {

static const std::string cClassName = "IasSwitchMatrix::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"

IasSwitchMatrix::IasSwitchMatrix()
  :mLog(IasAudioLogging::registerDltContext("SMW", "Switch Matrix Worker"))
  ,mName()
  ,mInitialized(false)
  ,mTaskMap()
  ,mCopySize(0)
  ,mSampleRate(0)
  ,mBufferTasks()
  ,mActionQueue()
  ,mMutex()
  ,mCondition()
{
}

IasSwitchMatrix::~IasSwitchMatrix()
{
  DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX, mName.c_str());

  IasBufferTaskMap::iterator it;
  for(it=mTaskMap.begin(); it!=mTaskMap.end();it++)
  {
    it->second = nullptr;
  }

  mTaskMap.clear();
  mBufferTasks.clear();
}

IasSwitchMatrix::IasResult IasSwitchMatrix::init(const std::string& name, uint32_t copySize, uint32_t sampleRate)
{
  if (mInitialized == false)
  {
    if ( (copySize != 0) && (sampleRate != 0))
    {
      mCopySize = copySize;
      mSampleRate = sampleRate;
      mName = name;
      mInitialized = true;
    }
    else
    {
      if (copySize == 0)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Copy size of 0 not supported");
      }
      if (sampleRate == 0)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Sample rate of 0 not supported");
      }
      return eIasFailed;
    }
    return eIasOk;
  }
  else
  {
    /**
     * @log This error occurs when you try to link one routing zone to two different audio sink devices.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Init function only allowed to be called one time");
    return eIasFailed;
  }


}

IasSwitchMatrix::IasResult IasSwitchMatrix::connect(IasAudioPortPtr src, IasAudioPortPtr sink)
{

  IasAudioRingBuffer* srcBuffer;
  IasBufferTaskPtr task = nullptr;
  if (mInitialized == false)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "worker thread not yet initialized");
    return eIasFailed;
  }
  if(src == NULL || sink == NULL)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "null pointer paramter passed");
    return eIasFailed;
  }

  if(src->getRingBuffer(&srcBuffer))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "could not get src ringbuffer from port");
    return eIasFailed;
  }
  IasSwitchMatrix::IasResult smwres = addBufferTask(src, &task, sink);
  if (smwres != IasSwitchMatrix::eIasOk)
  {
    // Detailed printout is already done inside addBufferTask
    return eIasFailed;
  }
  if(task->isDummy() == true)
  {
    task->makeReal(); // this is needed if a dummy connection is changed to a real connection
  }
  if (task->addJob(src, sink) != IasBufferTask::eIasOk)
  {
    smwres = eIasFailed;
  }
  mTaskMap[srcBuffer] = task;
  if(smwres == IasSwitchMatrix::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "leaving ok");
    return eIasOk;
  }
  else
  {
    return eIasFailed;
  }
}

IasSwitchMatrix::IasResult IasSwitchMatrix::disconnect(IasAudioPortPtr src, IasAudioPortPtr sink)
{
  if (mInitialized == false)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, ": worker thread not yet initialized");
    return eIasFailed;
  }
  if(src == nullptr || sink == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, ": null pointer passed as parameter");
    return eIasFailed;
  }
  IasAudioRingBuffer* srcBuffer;
  IasBufferTaskPtr task;

  IasAudioPort::IasResult portres = src->getRingBuffer(&srcBuffer);
  if(portres != IasAudioPort::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, ": could not get src ringbuffer from port");
    return eIasFailed;
  }
  if(findBufferTask(srcBuffer, &task))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, ": No buffer task found, so no connection present");
    return eIasFailed;
  }

  if (task->triggerDeleteJob(src, sink) == IasBufferTask::eIasOk)
  {
    if (task->isActive() == false)
    {
      // There are no active jobs anymore, so the buffer task can be deleted
      removeBufferTask(srcBuffer);
      mActionQueue.push(IasActionQueueEntry(task, eIasDeleteBufferTask));
    }
    return eIasOk;
  }
  else
  {
    return eIasFailed;
  }
}

IasSwitchMatrix::IasResult IasSwitchMatrix::removeConnections(IasAudioPortPtr src)
{
  if(src == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, ": null pointer passed as parameter");
    return eIasFailed;
  }

  if(src->getParameters()->direction != eIasPortDirectionOutput)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, ": No connections present on sink device");
    return eIasOk;
  }

  IasAudioRingBuffer* srcBuffer;
  IasBufferTaskPtr task;

  if(src->getRingBuffer(&srcBuffer) != IasAudioPort::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, ": could not get src ringbuffer from port");
    return eIasFailed;
  }
  if(findBufferTask(srcBuffer, &task) != eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, ": No buffer task found, so no connection present");
    return eIasFailed;
  }

  if (task->deleteAllJobs(src) == IasBufferTask::eIasOk)
  {
    if (task->isActive() == false)
    {
      std::unique_lock<std::mutex> lk(mMutex);
      // There are no active jobs anymore, so the buffer task can be deleted
      removeBufferTask(srcBuffer);
      mActionQueue.push(IasActionQueueEntry(task, eIasDeleteBufferTask));
      bool event =  mCondition.wait_for(lk, std::chrono::milliseconds(150),
          [this, task] { return std::find(mBufferTasks.begin(), mBufferTasks.end(), task) == mBufferTasks.end();});
      if(event == true)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, ": Wait for task deletion completed successfully");
      }
      else
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_WARN, LOG_PREFIX, ": Time out on wait for task deletion");
      }
    }
    return eIasOk;
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_WARN, LOG_PREFIX, ": Delete task jobs failed");
    return eIasFailed;
  }
}

IasSwitchMatrix::IasResult IasSwitchMatrix::findBufferTask(IasAudioRingBuffer* src,IasBufferTaskPtr* task)
{
  IasBufferTaskMap::iterator mapIt;
  mapIt = mTaskMap.find(src);
  if(mapIt == mTaskMap.end())
  {
    *task = NULL;
    return eIasFailed;
  }
  else
  {
    *task = mapIt->second;
    return eIasOk;
  }
}

void IasSwitchMatrix::removeBufferTask(IasAudioRingBuffer* rb)
{
  auto mapIt = mTaskMap.find(rb);
  if (mapIt != mTaskMap.end())
  {
    mTaskMap.erase(mapIt);
  }
}


IasSwitchMatrix::IasResult IasSwitchMatrix::addBufferTask(IasAudioPortPtr src, IasBufferTaskPtr* newTask, IasAudioPortPtr sink)
{
  IasBufferTaskMap::iterator mapIt;

  IasAudioRingBuffer* srcBuf = nullptr;
  IAS_ASSERT(src != nullptr);
  src->getRingBuffer(&srcBuf);
  mapIt = mTaskMap.find(srcBuf);
  if(mapIt == mTaskMap.end())
  {
    // Calculate the periodSize for this connection. This might be different from the copySize
    // of the SwitchMatrixWorkerThread if this connection runs at a different sample rate.
    IasAudioPortCopyInformation sinkInfos;
    IasAudioPortCopyInformation sourceInfos;
    IasAudioPort::IasResult portres = sink->getCopyInformation(&sinkInfos);
    if (portres != IasAudioPort::IasResult::eIasOk)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error during getCopyInformation for", sink->getParameters()->name);
      return eIasFailed;
    }
    portres = src->getCopyInformation(&sourceInfos);
    if (portres != IasAudioPort::IasResult::eIasOk)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error during getCopyInformation for", src->getParameters()->name);
      return eIasFailed;
    }
    uint64_t periodSize = (static_cast<int64_t>(mCopySize) * static_cast<int64_t>(sinkInfos.sampleRate)) / static_cast<int64_t>(mSampleRate);
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, ": Sample rates are:", mSampleRate,
                "Hz (base),", sinkInfos.sampleRate, "Hz (derived)");
    // Verify that the periodSize for this connection is integer.
    if (periodSize * mSampleRate != static_cast<uint64_t>(mCopySize) * sinkInfos.sampleRate)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, ": Error, periodSize of derived connection is not integer. Sample rates are:", mSampleRate,
                  "Hz (base),", sinkInfos.sampleRate, "Hz (derived)");
      return eIasFailed;
    }
    *newTask = std::make_shared<IasBufferTask>(src, sourceInfos.periodSize,mCopySize, mSampleRate, false);
    mActionQueue.push(IasActionQueueEntry(*newTask, eIasAddBufferTask));
    IAS_ASSERT(*newTask != nullptr);
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "new buffer task will copy ",static_cast<uint32_t>(mCopySize), "from the source");
  }
  else
  {
    *newTask = mapIt->second;
  }
  return eIasOk;
}

IasSwitchMatrix::IasResult IasSwitchMatrix::setCopySize(uint32_t copySize)
{
  if (mInitialized == false)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, ": worker thread not yet initialized");
    return eIasFailed;
  }
  if(copySize != 0)
  {
    mCopySize = copySize;
    return eIasOk;
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, ": copy Size of 0 is not allowed");
    return eIasFailed;
  }
}


IasSwitchMatrix::IasResult IasSwitchMatrix::trigger()
{
  IasActionQueueEntry entry;
  while(mActionQueue.try_pop(entry))
  {
    if (entry.second == eIasAddBufferTask)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX, "Adding new buffer task");
      mBufferTasks.push_back(entry.first);
    }
    else
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX, "Delete buffer task");
      // Before deleting the buffer task, we have to trigger it one more time to send all possible
      // disconnect events first.
      // If we don't do that, the buffer task will be deleted before the disconnect of a job is actually done
      // respectively signalled.
      IasBufferTaskPtr task = entry.first;
      if (task->isDummy() == false)
      {
        task->doJobs();
      }
      std::lock_guard<std::mutex> lk(mMutex);
      mBufferTasks.remove(entry.first);
      mCondition.notify_one();
    }
  }

  for (auto &task : mBufferTasks)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX, "worker ",mName,": Doing buffer tasks");
    if(task->isDummy() == true)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX, ": Worker ",mName," doingDummy");
      task->doDummy();
    }
    else
    {
      task->doJobs();
    }
  }
  return eIasOk;
}

IasSwitchMatrix::IasResult IasSwitchMatrix::dummyConnect(IasAudioPortPtr src, IasAudioPortPtr sink)
{
  IAS_ASSERT(src != nullptr);
  IAS_ASSERT(sink != nullptr);
  IasAudioPortCopyInformation sinkInfos;
  IasAudioPortCopyInformation sourceInfos;
  IasAudioPort::IasResult apres = sink->getCopyInformation(&sinkInfos);
  IAS_ASSERT(apres == IasAudioPort::eIasOk);
  apres = src->getCopyInformation(&sourceInfos);
  IAS_ASSERT(apres == IasAudioPort::eIasOk);
  (void)apres;
  IasAudioRingBuffer* srcBuffer = nullptr;
  apres = src->getRingBuffer(&srcBuffer);
  IAS_ASSERT(apres == IasAudioPort::eIasOk);
  IasBufferTaskPtr task;
  if(findBufferTask(srcBuffer, &task) == IasSwitchMatrix::eIasFailed)
  {
    IAS_ASSERT(mSampleRate != 0);
    task = std::make_shared<IasBufferTask>(src,
                                           sourceInfos.periodSize,
                                           sourceInfos.periodSize,
                                           mSampleRate,
                                           true);
    IAS_ASSERT(task != nullptr);
    mActionQueue.push(IasActionQueueEntry(task, eIasAddBufferTask));
    mTaskMap[srcBuffer] = task;
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Dummy buffer task created and added to task map. New map size is", mTaskMap.size());
  }
  else
  {
    task->makeDummy();
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Changed existing task type to dummy task, size of task map is", mTaskMap.size());
  }

  return eIasOk;
}

IasSwitchMatrix::IasResult IasSwitchMatrix::dummyDisconnect(IasAudioPortPtr src)
{
  IasAudioRingBuffer* srcBuffer;
  IasBufferTaskPtr task;
  IasAudioPort::IasResult portres = src->getRingBuffer(&srcBuffer);
  if(portres != IasAudioPort::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, ": could not get src ringbuffer from port");
    return eIasFailed;
  }
  if(findBufferTask(srcBuffer, &task) == IasSwitchMatrix::eIasFailed)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, ": No buffer task found, so no connection present");
    return eIasFailed;
  }
  if (task->isActive() == false)
  {
    // There are no active jobs anymore, so the buffer task can be deleted
    mActionQueue.push(IasActionQueueEntry(task, eIasDeleteBufferTask));
  }

  mTaskMap.erase(srcBuffer);

  return eIasOk;
}

void IasSwitchMatrix::stopProbing(IasAudioPortPtr port)
{
  IasSwitchMatrixJobPtr job;
  IasAudioRingBuffer* ringbuf = nullptr;
  port->getRingBuffer(&ringbuf);
  IAS_ASSERT(ringbuf != nullptr);

  if(port->getParameters()->direction == eIasPortDirectionInput)
  {
    for(auto &task : mTaskMap)
    {
      job = task.second->findJob(port);
      if (job != nullptr)
      {
        job->stopProbe();
        break;
      }
    }
  }
  else
  {
    if(mTaskMap.find(ringbuf) == mTaskMap.end())
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, ": No buffer task found, so no connection present, no probing stopped");
    }
    else
    {
      IasBufferTaskPtr bufferTask = mTaskMap[ringbuf];
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, ": trigger buffer task to stop probing");
      bufferTask->stopProbing();
    }
  }
}

IasSwitchMatrix::IasResult IasSwitchMatrix::startProbing(IasAudioPortPtr port,
                                                         const std::string& fileNamePrefix,
                                                         bool bInject,
                                                         uint32_t numSeconds,
                                                         IasAudioRingBuffer* ringbuf,
                                                         uint32_t sampleRate)
{
  IasSwitchMatrixJobPtr job = nullptr;;
  IasSwitchMatrixJob::IasResult smjres;
  IasAudioCommonDataFormat dataFormat;
  ringbuf->getDataFormat(&dataFormat);
  uint32_t numChannels = port->getParameters()->numChannels;
  uint32_t startIndex = port->getParameters()->index;

  if (port->getParameters()->direction == eIasPortDirectionInput)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, ": Probe would be done for input port with name ",port->getParameters()->name.c_str());
    for(auto &task : mTaskMap)
    {
      job = task.second->findJob(port);
      if (job != nullptr)
      {
        smjres =job->startProbe(fileNamePrefix,
                                bInject,
                                numSeconds,
                                dataFormat,
                                sampleRate,
                                numChannels,
                                startIndex);
        if(smjres != IasSwitchMatrixJob::eIasOk)
        {
          return eIasFailed;
        }
        break;
      }
    }
    if(job == nullptr)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, ": No switch matrix job found, no probing started");
      return eIasFailed;
    }
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, ": Probe would be done for output port");
    IasBufferTaskPtr task = nullptr;
    if (mTaskMap.find(ringbuf) != mTaskMap.end())
    {
      task = mTaskMap[ringbuf];
    }
    else
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, ": No connection active, no probing started");
      return eIasFailed;
    }
    task->startProbing(fileNamePrefix,numSeconds, bInject, numChannels, startIndex, sampleRate, dataFormat);
  }
  return eIasOk;
}

void IasSwitchMatrix::unlockJobs()
{
  for ( auto &task : mBufferTasks)
  {
    task->unlockJobs();
  }
}

void IasSwitchMatrix::lockJob(IasAudioPortPtr sinkPort)
{
  for ( auto &task : mBufferTasks)
  {
    task->lockJob(sinkPort);
  }
}


} // namespace IasAudio
