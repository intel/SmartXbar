/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file   IasTestFrameworkRoutingZone.cpp
 * @date   2017
 * @brief  The Routing Zone of the Test Framework.
 */

#include "testfwx/IasTestFrameworkRoutingZone.hpp"
#include "testfwx/IasTestFrameworkWaveFile.hpp"
#include "model/IasAudioPort.hpp"
#include "model/IasPipeline.hpp"

#include "avbaudiomodules/audio/common/IasAudioCommonTypes.hpp"
#include "avbaudiomodules/internal/audio/common/IasAudioLogging.hpp"
#include "avbaudiomodules/internal/audio/common/audiobuffer/IasAudioRingBuffer.hpp"
#include "avbaudiomodules/internal/audio/common/audiobuffer/IasAudioRingBufferFactory.hpp"


namespace IasAudio {

static const std::string cClassName = "IasTestFrameworkRoutingZone::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"


IasTestFrameworkRoutingZone::IasTestFrameworkRoutingZone(IasTestFrameworkRoutingZoneParamsPtr params)
  :mLog(IasAudioLogging::registerDltContext("TRZN", "Test Framework Routing Zone"))
  ,mParams(params)
  ,mPipeline(nullptr)
  ,mIsRunning(false)
{
  IAS_ASSERT(mParams != nullptr);
}


IasTestFrameworkRoutingZone::~IasTestFrameworkRoutingZone()
{
  DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX);

  stop();

  // Destroy all input buffers
  IasAudioRingBufferFactory* ringBufferFactory = IasAudioRingBufferFactory::getInstance();

  for (const IasRingBufferParamsPair& it :mInputBufferParamsMap)
  {
    const IasRingBufferParams tmpParams = it.second;
    ringBufferFactory->destroyRingBuffer(tmpParams.ringBuffer);
  }

  // Destroy all output buffers
  for (const IasRingBufferParamsPair& it :mOutputBufferParamsMap)
  {
    const IasRingBufferParams tmpParams = it.second;
    ringBufferFactory->destroyRingBuffer(tmpParams.ringBuffer);
  }

  mInputBufferParamsMap.clear();
  mOutputBufferParamsMap.clear();
  deletePipeline();
}


IasTestFrameworkRoutingZone::IasResult IasTestFrameworkRoutingZone::init()
{
  IAS_ASSERT(mParams != nullptr);

  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Initialization of test framework routing zone.");

  mInputBufferParamsMap.clear();
  mOutputBufferParamsMap.clear();

  return eIasOk;
}


IasTestFrameworkRoutingZone::IasResult IasTestFrameworkRoutingZone::createInputBuffer(const IasAudioPortPtr audioPort,
                                                                                            IasAudioCommonDataFormat dataFormat,
                                                                                            IasTestFrameworkWaveFilePtr waveFile,
                                                                                            IasAudioRingBuffer**     inputBuffer)
{
  if (audioPort == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: audioPort is nullptr");
    return eIasInvalidParam;
  }
  if (waveFile == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: waveFile is nullptr");
    return eIasInvalidParam;
  }
  if (inputBuffer == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: inputBuffer is nullptr");
    return eIasInvalidParam;
  }

  // Verify that we do not already have a input buffer for this audio port.
  if (mInputBufferParamsMap.find(audioPort) != mInputBufferParamsMap.end())
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Routing zone already includes an input buffer for audio port", audioPort->getParameters()->name);
    return eIasInvalidParam;
  }

  IasResult res = createRingBuffer(audioPort, dataFormat, inputBuffer);
  if (res != eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Create input buffer failed");
    return eIasFailed;
  }

  IasRingBufferParams tmpParams;
  tmpParams.ringBuffer = *inputBuffer;
  tmpParams.waveFile = waveFile;

  // Add the new input buffer to the mInputBufferParamsMap
  IasRingBufferParamsPair tmp = std::make_pair(audioPort, tmpParams);
  mInputBufferParamsMap.insert(tmp);

  return eIasOk;
}


IasTestFrameworkRoutingZone::IasResult IasTestFrameworkRoutingZone::destroyInputBuffer(const IasAudioPortPtr audioPort)
{
  if (audioPort == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: audioPort is nullptr");
    return eIasInvalidParam;
  }

  // Try to find audioPort in mInputBufferParamsMap.
  IasRingBufferParamsMap::const_iterator it = mInputBufferParamsMap.find(audioPort);
  if (it == mInputBufferParamsMap.end())
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: Routing zone does not include an input buffer for audio port",
                audioPort->getParameters()->name);
    return eIasInvalidParam;
  }

  // Extract the pointer to the ring buffer.
  IasAudioRingBuffer* inputBuffer = it->second.ringBuffer;
  IAS_ASSERT(inputBuffer != nullptr);

  IasAudioRingBufferFactory* ringBufferFactory = IasAudioRingBufferFactory::getInstance();
  ringBufferFactory->destroyRingBuffer(inputBuffer);

  mInputBufferParamsMap.erase(audioPort);

  return eIasOk;
}


IasTestFrameworkRoutingZone::IasResult IasTestFrameworkRoutingZone::createOutputBuffer(const IasAudioPortPtr audioPort,
                                                                                       IasAudioCommonDataFormat dataFormat,
                                                                                       IasTestFrameworkWaveFilePtr waveFile,
                                                                                       IasAudioRingBuffer**     outputBuffer)
{
  if (audioPort == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: audioPort is nullptr");
    return eIasInvalidParam;
  }
  if (waveFile == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: waveFile is nullptr");
    return eIasInvalidParam;
  }
  if (outputBuffer == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: outputBuffer is nullptr");
    return eIasInvalidParam;
  }

  // Verify that we do not already have a output buffer for this audio port.
  if (mOutputBufferParamsMap.find(audioPort) != mOutputBufferParamsMap.end())
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Routing zone already includes an output buffer for audio port", audioPort->getParameters()->name);
    return eIasInvalidParam;
  }

  IasResult res = createRingBuffer(audioPort, dataFormat, outputBuffer);
  if (res != eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Create output buffer failed");
    return eIasFailed;
  }

  IasRingBufferParams tmpParams;
  tmpParams.ringBuffer = *outputBuffer;
  tmpParams.waveFile = waveFile;

  // Add the new output buffer to the mOutputBufferParamsMap
  IasRingBufferParamsPair tmp = std::make_pair(audioPort, tmpParams);
  mOutputBufferParamsMap.insert(tmp);

  return eIasOk;
}


IasTestFrameworkRoutingZone::IasResult IasTestFrameworkRoutingZone::destroyOutputBuffer(const IasAudioPortPtr audioPort)
{
  if (audioPort == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: audioPort is nullptr");
    return eIasInvalidParam;
  }

  // Try to find audioPort in mOutputBufferParamsMap.
  IasRingBufferParamsMap::const_iterator it = mOutputBufferParamsMap.find(audioPort);
  if (it == mOutputBufferParamsMap.end())
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: Routing zone does not include an output buffer for audio port",
                audioPort->getParameters()->name);
    return eIasInvalidParam;
  }

  // Extract the pointer to the ring buffer.
  IasAudioRingBuffer* outputBuffer = it->second.ringBuffer;
  IAS_ASSERT(outputBuffer != nullptr);

  IasAudioRingBufferFactory* ringBufferFactory = IasAudioRingBufferFactory::getInstance();
  ringBufferFactory->destroyRingBuffer(outputBuffer);

  mOutputBufferParamsMap.erase(audioPort);

  return eIasOk;
}


IasTestFrameworkRoutingZone::IasResult IasTestFrameworkRoutingZone::createRingBuffer(const IasAudioPortPtr audioPort,
                                                                                     IasAudioCommonDataFormat dataFormat,
                                                                                     IasAudioRingBuffer**     ringBuffer)
{
  IAS_ASSERT(audioPort != nullptr);
  IAS_ASSERT(ringBuffer != nullptr);

  if (dataFormat != eIasFormatFloat32)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_WARN, LOG_PREFIX, "Using data format eIasFormatFloat32 as default");
    dataFormat = eIasFormatFloat32;
  }

  *ringBuffer = nullptr;
  IasAudioPortParamsPtr audioPortParams = audioPort->getParameters();

  uint32_t periodSize = mPipeline->getParameters()->periodSize;
  uint32_t numPeriods = 1; //default one period per iteration for test framework
  uint32_t numChannels = audioPortParams->numChannels;
  std::string ringBufferName = "RingBuffer_" + audioPortParams->name;

  // Now we create the ring buffer.
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX,
              "Creating ring buffer, periodSize =", periodSize,
              "numPeriods =", numPeriods,
              "numChannels =", numChannels,
              "dataFormat =",  toString(dataFormat));

  IasAudioRingBufferFactory* ringBufferFactory = IasAudioRingBufferFactory::getInstance();
  IasAudioCommonResult result = ringBufferFactory->createRingBuffer(ringBuffer,
                                                                    periodSize,
                                                                    numPeriods,
                                                                    numChannels,
                                                                    dataFormat,
                                                                    eIasRingBufferLocalReal,
                                                                    ringBufferName);
  if (result != eIasResultOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Error while creating ring buffer:", ringBufferName,
                ":", toString(result));
    return eIasFailed;
  }

  return eIasOk;
}


IasTestFrameworkRoutingZone::IasResult IasTestFrameworkRoutingZone::addPipeline(IasPipelinePtr pipeline)
{
  if (pipeline == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: pipeline is nullptr");
    return eIasInvalidParam;
  }

  // Check whether routing zone already owns a different pipeline.
  if ((mPipeline != nullptr) && (mPipeline != pipeline))
  {
    IasPipelineParamsPtr newPipelineParams      = pipeline->getParameters();
    IasPipelineParamsPtr existingPipelineParams = mPipeline->getParameters();
    IAS_ASSERT(newPipelineParams      != nullptr); // already checked in constructor of IasPipeline
    IAS_ASSERT(existingPipelineParams != nullptr); // already checked in constructor of IasPipeline

    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Error: cannot add pipeline", newPipelineParams->name,
                "since the test framework routing zone already owns pipeline", existingPipelineParams->name);
    return eIasFailed;
  }

  mPipeline = pipeline;

  return eIasOk;
}


void IasTestFrameworkRoutingZone::deletePipeline()
{
  if (mPipeline != nullptr)
  {
    mPipeline.reset();
  }
}


IasTestFrameworkRoutingZone::IasResult IasTestFrameworkRoutingZone::start()
{
  for (IasRingBufferParamsMap::iterator mapIt = mInputBufferParamsMap.begin();
       mapIt != mInputBufferParamsMap.end(); mapIt++)
  {
    IasRingBufferParams& inputBufferParams = mapIt->second;
    IasTestFrameworkWaveFilePtr inputWaveFile = inputBufferParams.waveFile;
    IasTestFrameworkWaveFile::IasResult res = inputWaveFile->openForReading(mPipeline->getParameters()->periodSize);
    if (res != IasTestFrameworkWaveFile::eIasOk)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error while opening ", inputWaveFile->getFileName(), 
        " for reading: ", toString(res));
      return eIasFailed;
    }
  }

  for (IasRingBufferParamsMap::iterator mapIt = mOutputBufferParamsMap.begin();
       mapIt != mOutputBufferParamsMap.end(); mapIt++)
  {
    IasRingBufferParams& outputBufferParams = mapIt->second;
    IasTestFrameworkWaveFilePtr outputWaveFile = outputBufferParams.waveFile;
    IasTestFrameworkWaveFile::IasResult res = outputWaveFile->openForWriting(mPipeline->getParameters()->periodSize);
    if (res != IasTestFrameworkWaveFile::eIasOk)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error while opening ", outputWaveFile->getFileName(), 
        " for writing: ", toString(res));
      return eIasFailed;
    }
  }

  mIsRunning = true;

  return eIasOk;;
}


IasTestFrameworkRoutingZone::IasResult IasTestFrameworkRoutingZone::stop()
{
  if (!mIsRunning)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Test framework routing zone has not been started yet");
  }

  //if previous start() method call did not finish successfully, some wave files 
  //might remain open, here we close these files.
  for (IasRingBufferParamsMap::iterator mapIt = mInputBufferParamsMap.begin();
       mapIt != mInputBufferParamsMap.end(); mapIt++)
  {
    IasRingBufferParams& inputBufferParams = mapIt->second;
    IasTestFrameworkWaveFilePtr inputWaveFile = inputBufferParams.waveFile;
    inputWaveFile->close();
  }

  for (IasRingBufferParamsMap::iterator mapIt = mOutputBufferParamsMap.begin();
       mapIt != mOutputBufferParamsMap.end(); mapIt++)
  {
    IasRingBufferParams& outputBufferParams = mapIt->second;
    IasTestFrameworkWaveFilePtr outputWaveFile = outputBufferParams.waveFile;
    outputWaveFile->close();
  }

  mIsRunning = false;

  return eIasOk;
}


IasTestFrameworkRoutingZone::IasResult IasTestFrameworkRoutingZone::processPeriods(uint32_t numPeriods)
{
  if (numPeriods == 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: numPeriods is 0");
    return eIasInvalidParam;
  }

  if (!mIsRunning)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Test framework routing zone has not been started yet");
    return eIasNotInitialized;
  }

  for (uint32_t i = 0; i < numPeriods; i++)
  {
    // Execute the transferPeriod method of the test framework routing zone;
    // this transfers all frames from the input buffer to the output.
    IasResult result = transferPeriod();

    if (result == eIasFinished)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "All periods have been processed.");
      return eIasFinished;
    }
    else if (result != eIasOk)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error during transferPeriod method:", toString(result));
      return eIasFailed;
    }
  }

  return eIasOk;
}


IasTestFrameworkRoutingZone::IasResult IasTestFrameworkRoutingZone::transferPeriod()
{
  IasResult result = eIasOk;
  IAS_ASSERT(mPipeline != nullptr);
  IAS_ASSERT(mIsRunning);


  // We always write blocks of PeriodSize PCM frames to the output file
  uint32_t numFramesToTransfer = mPipeline->getParameters()->periodSize;

  // Number of frames read from all input files.
  uint32_t numAllFramesRead = 0;

  // Loop over all input buffers, i.e., loop over all routingZoneInputPorts
  for (IasRingBufferParamsMap::iterator mapIt = mInputBufferParamsMap.begin();
       mapIt != mInputBufferParamsMap.end(); mapIt++)
  {
    const IasAudioPortPtr& routingZoneInputPort = mapIt->first;
    IasRingBufferParams&  inputBufferParams = mapIt->second;

    IasAudioRingBuffer* inputBuffer = inputBufferParams.ringBuffer;
    IasTestFrameworkWaveFilePtr inputWaveFile = inputBufferParams.waveFile;

    uint32_t numFramesRead = 0;
    result = readDataFromWaveFile(inputWaveFile, inputBuffer, numFramesToTransfer, numFramesRead);
    if (result != eIasOk)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error during readDataFromWaveFile:", toString(result));
      return eIasFailed;
    }
    numAllFramesRead += numFramesRead;

    uint32_t numFramesRemaining = 0;
    // provide data from the current port to the pipeline.
    IasPipeline::IasResult pipelineResult = mPipeline->provideInputData(routingZoneInputPort,
                                                                        0, //inputBufferOffset=0
                                                                        numFramesToTransfer,
                                                                        numFramesToTransfer,
                                                                        &numFramesRemaining);
    if (pipelineResult != IasPipeline::eIasOk)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Failed to provide input data to pipeline");
      return eIasFailed;
    }
  }// Now we have serviced all input buffers.

  // If number of frames read from all input files is 0 we assume that we reached end of file in each input file.
  // We can break the processing here and return eIasFinished.
  if (numAllFramesRead == 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "No more data in input wave files, processing finished.");
    return eIasFinished;
  }

  // Execute the pipeline and write the processed PCM frames to the output.
  // The pipeline decides internally which audio ports (or which audio channels) need to be processed.
  mPipeline->process();

  // Loop over all output buffers, i.e., loop over all routingZoneOutputPorts
  for (IasRingBufferParamsMap::iterator mapIt = mOutputBufferParamsMap.begin();
       mapIt != mOutputBufferParamsMap.end(); mapIt++)
  {
    IasRingBufferParams&  outputBufferParams = mapIt->second;

    IasAudioRingBuffer* outputBuffer = outputBufferParams.ringBuffer;
    IasTestFrameworkWaveFilePtr outputWaveFile = outputBufferParams.waveFile;
    IasAudioSinkDevicePtr sinkDevice = outputWaveFile->getDummySinkDevice();

    result = retrieveDataFromPipeline(sinkDevice, outputBuffer, numFramesToTransfer);
    if (result != eIasOk)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Failed to retrieve data from pipeline");
      return eIasFailed;
    }

    //write output data into a wave file
    uint32_t numFramesWritten = 0;
    result = writeDataIntoWaveFile(outputWaveFile, outputBuffer, numFramesToTransfer, numFramesWritten);
    if (result != eIasOk)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error during writeDataIntoWaveFile:", toString(result));
      return eIasFailed;
    }
  }

  return eIasOk;
}


IasTestFrameworkRoutingZone::IasResult IasTestFrameworkRoutingZone::retrieveDataFromPipeline(IasAudioSinkDevicePtr sinkDevice,
                                                                                             IasAudioRingBuffer* outputBuffer,
                                                                                             uint32_t numFrames)
{
  IasAudioArea* areas = nullptr;
  uint32_t offset = 0;
  uint32_t bufferNumFrames = 0;
  IasAudioRingBufferResult bufferRes;

  bufferRes = outputBuffer->beginAccess(eIasRingBufferAccessWrite, &areas, &offset, &bufferNumFrames);
  if (bufferRes != eIasRingBuffOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Error during IasAudioRingBuffer::beginAccess for writing:", toString(bufferRes));
    return eIasFailed;
  }

  //retrieve data from pipeline
  mPipeline->retrieveOutputData(sinkDevice, areas, eIasFormatFloat32, numFrames, offset);

  bufferRes = outputBuffer->endAccess(eIasRingBufferAccessWrite, offset, bufferNumFrames);
  if (bufferRes != eIasRingBuffOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Error during IasAudioRingBuffer::endAccess for writing:", toString(bufferRes));
    return eIasFailed;
  }

  return eIasOk;
}


IasTestFrameworkRoutingZone::IasResult IasTestFrameworkRoutingZone::readDataFromWaveFile(IasTestFrameworkWaveFilePtr waveFile,
                                                                                         IasAudioRingBuffer* inputBuffer,
                                                                                         uint32_t numFrames,
                                                                                         uint32_t& numFramesRead)
{
   IasAudioArea *areas = nullptr;
   uint32_t offset = 0;
   uint32_t bufferNumFrames = 0;
   IasAudioRingBufferResult bufferRes;

   bufferRes = inputBuffer->beginAccess(eIasRingBufferAccessWrite, &areas, &offset, &bufferNumFrames);
   if (bufferRes != eIasRingBuffOk)
   {
     DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                 "Error during IasAudioRingBuffer::beginAccess for writing", toString(bufferRes));
     return eIasFailed;
   }

   // Provide PCM frames from wave file to the input buffer
   IasTestFrameworkWaveFile::IasResult waveFileResult = waveFile->readFrames(areas, offset, numFrames, numFramesRead);
   if (waveFileResult != IasTestFrameworkWaveFile::eIasOk)
   {
     DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                  "Error during IasTestFrameworkWaveFile::readData:", toString(waveFileResult));
   }

   bufferRes = inputBuffer->endAccess(eIasRingBufferAccessWrite, offset, bufferNumFrames);
   if (bufferRes != eIasRingBuffOk)
   {
     DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                 "Error during IasAudioRingBuffer::endAccess for writing", toString(bufferRes));
   }

   if (waveFileResult != IasTestFrameworkWaveFile::eIasOk || bufferRes != eIasRingBuffOk)
   {
     return eIasFailed;
   }

   return eIasOk;
}


IasTestFrameworkRoutingZone::IasResult IasTestFrameworkRoutingZone::writeDataIntoWaveFile(IasTestFrameworkWaveFilePtr waveFile,
                                                                                          IasAudioRingBuffer* outputBuffer,
                                                                                          uint32_t numFrames,
                                                                                          uint32_t& numFramesWritten)
{
  IasAudioArea *areas = nullptr;
  uint32_t offset = 0;
  uint32_t bufferNumFrames = 0;
  IasAudioRingBufferResult bufferRes;

  bufferRes = outputBuffer->beginAccess(eIasRingBufferAccessRead, &areas, &offset, &bufferNumFrames);
  if (bufferRes != eIasRingBuffOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Error during IasAudioRingBuffer::beginAccess for reading", toString(bufferRes));
    return eIasFailed;
  }

  IasTestFrameworkWaveFile::IasResult waveFileResult = waveFile->writeFrames(areas, offset, numFrames, numFramesWritten);
  if (waveFileResult != IasTestFrameworkWaveFile::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Error during IasTestFrameworkWaveFile::writeData:", toString(waveFileResult));
  }

  bufferRes = outputBuffer->endAccess(eIasRingBufferAccessRead, offset, bufferNumFrames);
  if (bufferRes != eIasRingBuffOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX,
                "Error during IasAudioRingBuffer::endAccess for reading", toString(bufferRes));
  }

  if (waveFileResult != IasTestFrameworkWaveFile::eIasOk || bufferRes != eIasRingBuffOk)
  {
    return eIasFailed;
  }

  return eIasOk;
}


/*
 * Function to get a IasRoutingZone::IasResult as string.
 */
#define STRING_RETURN_CASE(name) case name: return std::string(#name); break
#define DEFAULT_STRING(name) default: return std::string(name)
std::string toString(const IasTestFrameworkRoutingZone::IasResult& type)
{
  switch(type)
  {
    STRING_RETURN_CASE(IasTestFrameworkRoutingZone::eIasOk);
    STRING_RETURN_CASE(IasTestFrameworkRoutingZone::eIasFinished);
    STRING_RETURN_CASE(IasTestFrameworkRoutingZone::eIasInvalidParam);
    STRING_RETURN_CASE(IasTestFrameworkRoutingZone::eIasInitFailed);
    STRING_RETURN_CASE(IasTestFrameworkRoutingZone::eIasNotInitialized);
    STRING_RETURN_CASE(IasTestFrameworkRoutingZone::eIasFailed);
    DEFAULT_STRING("Invalid IasTestFrameworkRoutingZone::IasResult => " + std::to_string(type));
  }
}

} // namespace IasAudio
