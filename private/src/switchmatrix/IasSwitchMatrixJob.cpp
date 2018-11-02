/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file
 */


#include "switchmatrix/IasSwitchMatrixJob.hpp"
#include "avbaudiomodules/internal/audio/common/audiobuffer/IasAudioRingBuffer.hpp"
#include "avbaudiomodules/internal/audio/common/IasAudioLogging.hpp"
#include "avbaudiomodules/internal/audio/common/IasDataProbe.hpp"
#include "avbaudiomodules/internal/audio/common/IasDataProbeHelper.hpp"
#include "avbaudiomodules/internal/audio/common/helper/IasCopyAudioAreaBuffers.hpp"
#include "avbaudiomodules/internal/audio/common/samplerateconverter/IasSrcWrapperBase.hpp"
#include "avbaudiomodules/internal/audio/common/samplerateconverter/IasSrcWrapper.hpp"
#include "model/IasAudioPortOwner.hpp"
#include "model/IasRoutingZone.hpp"

#include <algorithm>

namespace IasAudio {

  static const std::string cClassName = "IasSwitchMatrixJob::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"


IasSwitchMatrixJob::IasSwitchMatrixJob(const IasAudioPortPtr src, const IasAudioPortPtr sink)
  :mLog(IasAudioLogging::registerDltContext("SMJ", "Switch Matrix Job"))
  ,mSrc(src)
  ,mSink(sink)
  ,mDestSize(0)
  ,mSourceSize(0)
  ,mBasePeriodTime(0)
  ,mDataProbe(nullptr)
  ,mSrcWrapper(nullptr)
  ,mRatio(1.0f)
  ,mNumFramesStillToProcess(0)
  ,mJobTask(eIasJobSimpleCopy)
  ,mProbingActive(false)
  ,mSampleFormatConv(eIasInt16Int16)
  ,mLocked(true)
  ,mSizeFactor(0.0f)
  ,mLogCnt(0)
  ,mLogInterval(0)
{
  DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX);
}

IasSwitchMatrixJob::~IasSwitchMatrixJob()
{
  DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX);
  if (mDataProbe != nullptr)
  {
    mDataProbe = nullptr;
  }
  delete mSrcWrapper;
}

IasSwitchMatrixJob::IasResult IasSwitchMatrixJob::init(uint32_t copySize, uint32_t baseSampleRate)
{
  uint32_t srcNumChannels = mSrc->getParameters()->numChannels;
  uint32_t sinkNumChannels = mSink->getParameters()->numChannels;
  if( srcNumChannels != sinkNumChannels )
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "job init failed, mismatch of channels");
    return eIasFailed;
  }
  if(copySize == 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "job init failed, invalid copy size of buffer task");
    return eIasFailed;
  }
  if(baseSampleRate == 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "job init failed, invalid base sample rate");
    return eIasFailed;
  }
  //if the src ringbuffer is a mirror buffer, the areas in mSrcCopyInfos will ot be updated, also the struct member start
  // we have to wait for the updateSrcAreas call to have everything updated
  if(mSrc->getCopyInformation(&mSrcCopyInfos))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "job could not get copy information from source port");
    return eIasFailed;
  }
  //the sink ringbuffer is always a real ring buffer, so the areas will be up to date after this call
  if(mSink->getCopyInformation(&mSinkCopyInfos))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "job could not get copy information from sink port");
    return eIasFailed;
  }
  mBasePeriodTime = static_cast<float>(copySize) / static_cast<float>(baseSampleRate);
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "job is triggered by base period time", mBasePeriodTime*1000.0f,"ms");
  //check for periodTime of sink
  float sinkPeriodTime = static_cast<float>(mSinkCopyInfos.periodSize) / static_cast<float>(mSinkCopyInfos.sampleRate);
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "sink periodTime:",sinkPeriodTime);
  if (sinkPeriodTime < mBasePeriodTime)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Config mismatch, connection is intended for a sink with smaller periodTime than base zone!!");
    return eIasFailed;
  }

  std::uint32_t basePeriodTimeInt = static_cast<std::uint32_t>(mBasePeriodTime*1000.0f);
  mLogInterval =  (basePeriodTimeInt == 0) ? 1000 : 1000 / basePeriodTimeInt;

  mSampleFormatConv = (IasSwitchMatrixJobConversions)((mSinkCopyInfos.dataFormat << 4) | (mSrcCopyInfos.dataFormat ));

  if(mSrcCopyInfos.sampleRate != mSinkCopyInfos.sampleRate)
  {
    float sinkPeriodTime = static_cast<float>(mSinkCopyInfos.periodSize) / static_cast<float>(mSinkCopyInfos.sampleRate);
    mSourceSize = static_cast<uint32_t>(mBasePeriodTime * static_cast<float>(mSrcCopyInfos.sampleRate) );
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Job set up to copy",mSourceSize, "from source to provide the destination periodTime",sinkPeriodTime);
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "source rate:",mSrcCopyInfos.sampleRate);
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "sink rate:",mSinkCopyInfos.sampleRate);
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "sink periodSize:",mSinkCopyInfos.periodSize);

    IasSrcWrapperParams params;
    params.inputFormat = mSrcCopyInfos.dataFormat;
    params.outputFormat = mSinkCopyInfos.dataFormat;
    params.inputIndex = mSrcCopyInfos.index;
    params.outputIndex = mSinkCopyInfos.index;
    params.inputSampleRate = mSrcCopyInfos.sampleRate;
    params.outputSampleRate = mSinkCopyInfos.sampleRate;
    params.numChannels = mSrcCopyInfos.numChannels;

    switch(mSampleFormatConv)
    {
      case eIasFloat32Float32:
        mSrcWrapper = new IasSrcWrapper<float,float>();
        break;
      case eIasFloat32Int16:
        mSrcWrapper = new IasSrcWrapper<float,int16_t>();
        break;
      case eIasFloat32Int32:
        mSrcWrapper = new IasSrcWrapper<float,int32_t>();
        break;
      case eIasInt16Float32:
        mSrcWrapper = new IasSrcWrapper<int16_t,float>();
        break;
      case eIasInt16Int32:
        mSrcWrapper = new IasSrcWrapper<int16_t,int32_t>();
        break;
      case eIasInt16Int16:
        mSrcWrapper = new IasSrcWrapper<int16_t,int16_t>();
        break;
      case eIasInt32Float32:
        mSrcWrapper = new IasSrcWrapper<int32_t,float>();
        break;
      case eIasInt32Int16:
        mSrcWrapper = new IasSrcWrapper<int32_t,int16_t>();
        break;
      default:
        mSrcWrapper = new IasSrcWrapper<int32_t,int32_t>();
        break;
    }

    IAS_ASSERT(mSrcWrapper != nullptr);
    IasSrcWrapperResult res = mSrcWrapper->init(params, mSrcCopyInfos.areas, mSinkCopyInfos.areas);
    if(res != IasSrcWrapperResult::eIasOk)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "SRC could not be initialised, maybe not allowed conversion ratio");
      return eIasFailed;
    }

    mRatio = ((float)mSinkCopyInfos.sampleRate) /   ((float)mSrcCopyInfos.sampleRate);
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "job will do sync src");
    mJobTask = eIasJobSampleRateConversion;
  }
  else
  {
    mSourceSize = mSrcCopyInfos.periodSize;
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Job set up to copy",mSourceSize, "from source to provide the destination periodTime",sinkPeriodTime);
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "source rate:",mSrcCopyInfos.sampleRate);
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "sink rate:",mSinkCopyInfos.sampleRate);
    mJobTask = eIasJobSimpleCopy;
  }
  mDestSize = static_cast<uint32_t>(static_cast<float>(mSinkCopyInfos.sampleRate) *mBasePeriodTime);
  mNumFramesStillToProcess = mDestSize;
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "job created with destination copy size of",mDestSize);
  return eIasOk;
}

IasSwitchMatrixJob::IasResult IasSwitchMatrixJob::execute(uint32_t  srcOffset,
                                                          uint32_t  framesToRead,
                                                          uint32_t* framesStillToConsume,
                                                          uint32_t* framesConsumed)
{
  // function only gets called from IasBufferTask. There, the two variables framesStillToConsume and framesConsumed
  // are created, so the pointers can't be null. If this function gets called by a unit test, the asserts should take care
  // checking against null pointers. In the end, the caller is responsible to check the pointers.
  IAS_ASSERT(framesStillToConsume != nullptr);
  IAS_ASSERT(framesConsumed != nullptr);
  IasResult res = eIasOk;
  if(mLocked == true)
  {
    if (mLogCnt > mLogInterval || mLogCnt == 0)
    {
      mLogCnt = 0;
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX,"Job between", mSrc->getParameters()->name, "and", mSink->getParameters()->name, "is locked, unlock to execute");
    }
    mLogCnt++;

    *framesStillToConsume = 0;
    *framesConsumed = 0;
    return eIasOk;
  }
  mLogCnt = 0;

  IasProbingQueueEntry probingQueueEntry;
  while(mProbingQueue.try_pop(probingQueueEntry))
  {
    IasDataProbe::IasResult probeRes = IasDataProbeHelper::processQueueEntry(probingQueueEntry, &mDataProbe, &mProbingActive, mDestSize);
    if (probeRes != IasDataProbe::eIasOk)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "error during ", toString(probingQueueEntry.action)," :", toString(probeRes));
    }
  }

  if (mJobTask == eIasJobSimpleCopy)
  {
    res = copy(srcOffset, framesToRead, framesConsumed, framesStillToConsume);
  }
  else
  {
    res = sampleRateConvert(srcOffset, framesToRead, framesConsumed, framesStillToConsume);
  }
  return res;
}

IasSwitchMatrixJob::IasResult IasSwitchMatrixJob::copy(uint32_t srcOffset,
                                                       uint32_t framesToRead,
                                                       uint32_t *framesConsumed,
                                                       uint32_t *framesStillToConsume)
{

  uint32_t sinkOffset = 0;
  uint32_t sinkSamples = 0;
  IasAudioRingBuffer *sinkBuffer = nullptr;
  IasAudioPort::IasResult pres = mSink->getRingBuffer(&sinkBuffer);
  IAS_ASSERT(pres == IasAudioPort::eIasOk);
  (void)pres;
  IAS_ASSERT(sinkBuffer != nullptr);
  IasAudioRingBufferResult rbres = sinkBuffer->updateAvailable(eIasRingBufferAccessWrite, &sinkSamples);
  IAS_ASSERT(rbres == eIasRingBuffOk);
  rbres = sinkBuffer->beginAccess(eIasRingBufferAccessWrite, &(mSinkCopyInfos.areas), &sinkOffset, &sinkSamples);
  IAS_ASSERT(rbres == eIasRingBuffOk);
  (void)rbres;

  const IasAudioPortParamsPtr srcParams = mSrc->getParameters();
  const IasAudioPortParamsPtr sinkParams = mSink->getParameters();

  uint32_t numSamplesToCopy = std::min(mDestSize,framesToRead);
  numSamplesToCopy = std::min(numSamplesToCopy,sinkSamples); //to guarantee only to copy as much as there is space
  DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX, "copy", numSamplesToCopy, "samples from", srcParams->name, "to", sinkParams->name, "space available=", sinkSamples);
  copyAudioAreaBuffers(mSinkCopyInfos.areas,mSinkCopyInfos.dataFormat,sinkOffset,mSinkCopyInfos.numChannels,sinkParams->index,numSamplesToCopy,
                       mSrcCopyInfos.areas, mSrcCopyInfos.dataFormat,srcOffset,mSrcCopyInfos.numChannels,srcParams->index, numSamplesToCopy );

  if(mDataProbe != nullptr)
  {
    IasDataProbe::IasResult probeRes = mDataProbe->process(mSinkCopyInfos.areas, sinkOffset,numSamplesToCopy);
    if (probeRes != IasDataProbe::eIasOk)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "delete probe");
      mProbingActive.store(false);
      mDataProbe = nullptr;
    }
  }
  rbres = sinkBuffer->endAccess(eIasRingBufferAccessWrite, sinkOffset, numSamplesToCopy);
  IAS_ASSERT(rbres == eIasRingBuffOk);
  *framesConsumed = numSamplesToCopy;
  *framesStillToConsume = 0;

  return eIasOk;
}

IasSwitchMatrixJob::IasResult IasSwitchMatrixJob::sampleRateConvert(uint32_t srcOffset,
                                                                    uint32_t framesToRead,
                                                                    uint32_t *framesConsumed,
                                                                    uint32_t *framesStillToConsume)
{
  IasSrcFarrow::IasResult srcResult = IasSrcFarrow::eIasOk;
  uint32_t sinkOffset = 0;
  uint32_t sinkSamples = 0;
  IasAudioRingBuffer *sinkBuffer = nullptr;
  const IasAudioPortParamsPtr srcParams = mSrc->getParameters();
  const IasAudioPortParamsPtr sinkParams = mSink->getParameters();

  IasAudioPort::IasResult pres = mSink->getRingBuffer(&sinkBuffer);
  IAS_ASSERT(pres == IasAudioPort::eIasOk);
  (void)pres;
  IAS_ASSERT(sinkBuffer != nullptr);
  IasAudioRingBufferResult rbres = sinkBuffer->updateAvailable(eIasRingBufferAccessWrite, &sinkSamples);
  IAS_ASSERT(rbres == eIasRingBuffOk);

  if(framesToRead == 0 && mNumFramesStillToProcess != 0)
  {
    mNumFramesStillToProcess = 0;
    *framesStillToConsume = 0;
    *framesConsumed = 0;
    mSrcWrapper->reset();
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "SRC reset, due to no input data");
    return eIasOk;
  }
  uint32_t numSourceSamples = framesToRead;
  uint32_t numInputConsumed = 0;
  uint32_t numOutputGenerated = 0;

  rbres = sinkBuffer->beginAccess(eIasRingBufferAccessWrite, &(mSinkCopyInfos.areas), &sinkOffset, &sinkSamples);
  if(rbres != eIasRingBuffOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "result of begin ringbuffer access ",toString(rbres).c_str(), "for", sinkBuffer->getName());
  }
  IAS_ASSERT(rbres == eIasRingBuffOk);
  sinkSamples = std::min(sinkSamples,mNumFramesStillToProcess);
  IasSrcWrapperResult srcWrapRes = mSrcWrapper->process(&numOutputGenerated,
                                                        &numInputConsumed,
                                                        numSourceSamples,
                                                        sinkSamples,
                                                        srcOffset,
                                                        sinkOffset);
  if(srcWrapRes)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX " Error processing sample rate converter: result = ", toString(srcResult), "for", sinkBuffer->getName());
    return eIasFailed;
  }

  if(mDataProbe != nullptr)
  {
    IasDataProbe::IasResult probeRes = mDataProbe->process(mSinkCopyInfos.areas, sinkOffset,numOutputGenerated);
    if (probeRes != IasDataProbe::eIasOk)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "delete probe for", sinkBuffer->getName());
      mProbingActive.store(false);
      mDataProbe = nullptr;
    }
  }

  rbres = sinkBuffer->endAccess(eIasRingBufferAccessWrite, sinkOffset, numOutputGenerated);
  if(rbres != eIasRingBuffOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "result of end ringbuffer access ",toString(rbres).c_str(), "for", sinkBuffer->getName());
  }
  IAS_ASSERT(rbres == eIasRingBuffOk);
  DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX, "written",numOutputGenerated,"and consumed",numInputConsumed,"samples");
  mNumFramesStillToProcess -= numOutputGenerated;

  if(mNumFramesStillToProcess == 0)
  {
    mNumFramesStillToProcess = mDestSize;
    *framesStillToConsume = 0;
  }
  else
  {
    *framesStillToConsume = 1 +(uint32_t)( /*0.5f +*/ (float)mNumFramesStillToProcess /mRatio);
  }
  *framesConsumed = numInputConsumed;

  return eIasOk;
}

IasSwitchMatrixJob::IasResult IasSwitchMatrixJob::updateSrcAreas(IasAudioArea* srcAreas)
{
  if(srcAreas != NULL)
  {
    mSrcCopyInfos.areas = srcAreas;
    mSrcCopyInfos.start = srcAreas[0].start;
    return eIasOk;
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "passed NULL pointer to function, ");
    return eIasFailed;
  }
}

void IasSwitchMatrixJob::stopProbe()
{
  IasProbingQueueEntry entry;
  entry.action = eIasProbingStop;
  mProbingQueue.push(entry);
}

IasSwitchMatrixJob::IasResult IasSwitchMatrixJob::startProbe(const std::string& fileNamePrefix,
                                                             bool bInject,
                                                             uint32_t numSeconds,
                                                             IasAudioCommonDataFormat dataFormat,
                                                             uint32_t sampleRate,
                                                             uint32_t numChannels,
                                                             uint32_t startIndex)
{
  IasProbingQueueEntry entry;
  entry.params.name = fileNamePrefix;
  entry.params.duration = numSeconds;
  entry.params.isInject = bInject;
  entry.params.numChannels = numChannels;
  entry.params.startIndex = startIndex;
  entry.params.sampleRate = sampleRate;
  entry.params.dataFormat = dataFormat;
  entry.action = eIasProbingStart;

  bool isProbingActive = mProbingActive.load(std::memory_order_relaxed);
  if(isProbingActive)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Probing already active");
    return eIasFailed;
  }
  mProbingQueue.push(entry);
  return eIasOk;
}

void IasSwitchMatrixJob::unlock()
{
  if (mLocked == true)
  {
    IasAudioPortOwnerPtr owner = nullptr;
    mSink->getOwner(&owner);
    // Already checked during connect method when adding job to buffer task
    IAS_ASSERT(owner != nullptr);
    IasRoutingZonePtr rzn = owner->getRoutingZone();
    if (rzn != nullptr)
    {
      if (rzn->isActive())
      {
        mLocked = false;
        DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX,
                    "Job between source", mSrc->getParameters()->name, "and sink", mSink->getParameters()->name,
                    "successfully unlocked");
      }
      else
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX,
                    "Job between source", mSrc->getParameters()->name, "and sink", mSink->getParameters()->name,
                    "couldn't be unlocked: Routing zone", rzn->getName(), "is not active yet");
      }
    }
    else
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_WARN, LOG_PREFIX, "Sink port", mSink->getParameters()->name, "does not belong to a routing zone");
      mLocked = false;
    }
  }
}


} // namespace IasAudio

