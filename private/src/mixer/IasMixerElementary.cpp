/*
  * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 *  @file   IasMixerElementary.cpp
 *  @date   August 2016
 *  @brief  The elementary mixer mixes several input streams into one output stream.
 *
 *  For each output zone, we need an own elementary mixer.
 */

#include <cstring>
#include <cmath>
#include "mixer/IasMixerElementary.hpp"
#include "audio/mixerx/IasMixerCmd.hpp"
#include "rtprocessingfwx/IasAudioChannelBundle.hpp"
#include "rtprocessingfwx/IasBundleAssignment.hpp"
#include "audio/smartx/rtprocessingfwx/IasModuleEvent.hpp"
#include "audio/smartx/IasEventProvider.hpp"


#ifdef __linux__
#define MS_VC  0
#else
#define MS_VC  1
#endif

#if !(MS_VC)
extern "C" {
#include <malloc.h>
}
#endif

#ifdef DEBUG
#define DO_DEBUG_PRINTING 0
#else
#define DO_DEBUG_PRINTING 0
#endif

#if DO_DEBUG_PRINTING
#include <cstdio>
#endif

/* Define SSE to enable optimization */
#define SSE 1


#if SSE
#include <xmmintrin.h>
#include <emmintrin.h>
#endif


namespace IasAudio {

static const std::string cClassName = "IasMixerElementary::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"

/**
 *  @brief  Private function: createChannelMapping()
 *
 *  Create the channelMappingVector for the specified audio stream.
 *  The channelMappingVector describes for each channel of the audio
 *  stream in which bundle it is located and which channel (within
 *  the bundle) refers to it.
 *
 *  @return                                     Error code.
 *  @retval  eIasAudioProcInitializationFailed  Incorrect configuration.
 *  @retval  eIasAudioProcOK                    Success.
 *
 *  @param[out] channelMappingVector  Will be created by this function.
 *  @param[in]  audioStream           Audio stream, for which the channelMappingVector shall be created.
 *  @param[in]  bundlesList           List denoting all (input or output) bundles that are processed by the mixer.
 *  @param[in,out] logContext         The log context for sending dlt error messages.
 */
static IasAudioProcessingResult createChannelMapping(IasMixerElementaryChannelMappingVector&        channelMappingVector,
                                                     IasAudioStream*                                audioStream,
                                                     std::list<IasAudioChannelBundle*> const& bundlesList,
                                                     DltContext*                                    logContext)
{
  channelMappingVector.clear();

  uint32_t cntChannelsThisStream = 0;

  const IasBundleAssignmentVector& bundleAssignmentVector = audioStream->asBundledStream()->getBundleAssignments();

  // Loop over all bundles that contain channels belonging to this stream.
  for(const auto& bundleAssignment : bundleAssignmentVector)
  {
    auto* bundle = bundleAssignment.getBundle();

    uint32_t bundleIndex = 0;
    std::list<IasAudioChannelBundle*>::const_iterator bundlesListIt;

    for (bundlesListIt = bundlesList.begin(); bundlesListIt != bundlesList.end(); bundlesListIt++)
    {
      IasAudioChannelBundle *tmpBundle = *bundlesListIt;
      if (tmpBundle == bundle)
      {
        break;
      }
      bundleIndex++;
    }

    if (bundlesListIt == bundlesList.end())
    {
      DLT_LOG_CXX(*logContext, DLT_LOG_INFO, LOG_PREFIX,
                  "createChannelMapping: Did not find bundle with the list of input bundles");
      return eIasAudioProcInitializationFailed;
    }
    // Now bundleIndex denotes at which position the current bundle is listed within bundlesList

    // Extend the channelMappingVector for the channels that belong to the current bundle.
    const uint32_t numChannelsThisBundle = bundleAssignment.getNumberChannels();
    for (uint32_t cntChannelsThisBundle = 0; cntChannelsThisBundle < bundleAssignment.getNumberChannels();
        cntChannelsThisBundle++)
    {
      IasMixerElementaryChannelMapping channelMapping;
      channelMapping.bundleIndex = bundleIndex;
      channelMapping.channelIndex = bundleAssignment.getIndex() + cntChannelsThisBundle;
      channelMappingVector.push_back(std::move(channelMapping));
    }
    cntChannelsThisStream += numChannelsThisBundle;

  }

  // Verify that the number of channels, which we have collected from the
  // individual bundles, complies with the number of channels that has been
  // declared for this stream.
  if (cntChannelsThisStream != (audioStream)->getNumberChannels())
  {
    DLT_LOG_CXX(*logContext, DLT_LOG_ERROR, LOG_PREFIX,
                    "createChannelMapping: Number of bundles/channels associated with",
                    "this stream does not comply with the number of channels declared for this stream:",
                    cntChannelsThisStream, (audioStream)->getNumberChannels());
    return eIasAudioProcInitializationFailed;
  }

  return eIasAudioProcOK;
}


IasMixerElementary::IasMixerElementary(const IasIGenericAudioCompConfig* config,
                                       const IasStreamPair &streamPair,
                                       uint32_t          frameLength,
                                       uint32_t          sampleFreq)
  :mStreamPair{streamPair}
  ,mNumberInputChannels{}
  ,mNumberOutputChannels{}
  ,mNumberInputBundles{}
  ,mNumberOutputBundles{}
  ,mFrameLength{frameLength}
  ,mSampleFreq{sampleFreq}
  ,mGainTileMatrix{nullptr}
  ,mStreamParamsMap{}
  ,mRampActiveStreams{}
  ,mMatrixUpdateFunc{nullptr}
  ,mOutputChannelMappingVector{}
  ,mInputBundlesList{}
  ,mOutputBundlesList{}
  ,multiChannelInputPresent{false}
  ,mBalanceQueue{}
  ,mGainOffsetQueue{}
  ,mConfig{config}
  ,mTypeName{""}
  ,mInstanceName{""}
  ,mLogContext(IasAudioLogging::getDltContext("_MIX"))
{
}


IasMixerElementary::~IasMixerElementary()
{
  for(auto& streamParam : mStreamParamsMap)
  {
    // Delete all stream parameters for this input stream
    this->cleanupStreamParams(&streamParam.second);
  }

  for (uint32_t cntOutputBundles = 0; cntOutputBundles < mNumberOutputBundles; cntOutputBundles++)
  {
#if MS_VC
    _aligned_free(mGainTileMatrix[cntOutputBundles]);
#else
    std::free(mGainTileMatrix[cntOutputBundles]);
#endif
    mGainTileMatrix[cntOutputBundles] = nullptr;
  }

  std::free(mGainTileMatrix);
  mGainTileMatrix = nullptr;
  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX, "~IasMixerElementary: Deleted");
}


IasAudioProcessingResult IasMixerElementary::init()
{
  const IasBundleAssignmentVector &outputBundleAssignmentVector = (mStreamPair.first->asBundledStream())->getBundleAssignments();
  IasBundleAssignmentVector::const_iterator bundleIterator;
  IasStreamPointerList::const_iterator inStreamIt;

  // Get the configuration properties.
  const IasProperties &properties = mConfig->getProperties();

  // Retrieve typeName and instanceName from the configuration properties.
  IasProperties::IasResult result;
  result = properties.get<std::string>("typeName", &mTypeName);
  if (result != IasProperties::eIasOk)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX,
                "Error: cannot find key 'typeName' in module configuration properties");
    return eIasAudioProcInitializationFailed;
  }

  result = properties.get<std::string>("instanceName",  &mInstanceName);
  if (result != IasProperties::eIasOk)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX,
                "Error: cannot find key 'instanceName' in module configuration properties");
    return eIasAudioProcInitializationFailed;
  }

  // Create the sorted+unique list of output bundles
  // Loop over all bundles that belong to the output stream.
  for(auto& bundleAssignment : outputBundleAssignmentVector)
  {
    mOutputBundlesList.push_back(bundleAssignment.getBundle());
  }

  mOutputBundlesList.sort();
  mOutputBundlesList.unique();
  mNumberOutputBundles = static_cast<uint32_t>(mOutputBundlesList.size());

#if DO_DEBUG_PRINTING

  printf("Output streams\n");
  printf("--------------------------------------------------\n");

  for (bundleIterator=outputBundleAssignmentVector.begin(); bundleIterator<outputBundleAssignmentVector.end(); ++bundleIterator)
  {
    IasBundleAssignment *bundleAssignment = *bundleIterator;
    IasAudioChannelBundle *bundle = bundleAssignment->getBundle();

    printf("channel index = %d\n",   bundleAssignment->getIndex());
    printf("num channels  = %d\n",   bundleAssignment->getNumberChannels());
    printf("pointer       = %p\n",   bundle->getAudioDataPointer());
    printf("--------------------------------------------------\n");
  }


  for (inStreamIt = (mStreamPair.second).begin(); inStreamIt != (mStreamPair.second).end(); ++inStreamIt)
  {
    printf("Input streams\n");
    printf("--------------------------------------------------\n");

    const IasBundleAssignmentVector &inputBundleAssignmentVector = (*inStreamIt)->asBundledStream()->getBundleAssignments();
    IasBundleAssignmentVector::const_iterator bundleIterator;

    for (bundleIterator = inputBundleAssignmentVector.begin(); bundleIterator < inputBundleAssignmentVector.end(); ++bundleIterator)
    {
      IasBundleAssignment *bundleAssignment = *bundleIterator;
      IasAudioChannelBundle *bundle = bundleAssignment->getBundle();

      printf("channel index = %d\n",   bundleAssignment->getIndex());
      printf("num channels  = %d\n",   bundleAssignment->getNumberChannels());
      printf("pointer       = %p\n",   bundle->getAudioDataPointer());
      printf("--------------------------------------------------\n");
    }
  }
#endif

  std::list<IasAudioChannelBundle*>::iterator inputBundlesListIt;  // Iterator for the list of input bundles
  std::list<IasAudioChannelBundle*>::iterator outputBundlesListIt; // Iterator for the list of output bundles

  // Create the sorted+unique list of input bundles
  // Loop over all input streams
  for (inStreamIt = (mStreamPair.second).begin(); inStreamIt != (mStreamPair.second).end(); ++inStreamIt)
  {
    const IasBundleAssignmentVector &inputBundleAssignmentVector = (*inStreamIt)->asBundledStream()->getBundleAssignments();
    IasBundleAssignmentVector::const_iterator bundleIterator;

    // Loop over all bundles that belong to this stream.
    for(auto& bundleAssignment : inputBundleAssignmentVector)
    {
      mInputBundlesList.push_back(bundleAssignment.getBundle());
    }
  }
  mInputBundlesList.sort();
  mInputBundlesList.unique();
  mNumberInputBundles = static_cast<uint32_t>(mInputBundlesList.size());

#if DO_DEBUG_PRINTING
  // Print out the list of input bundles
  for (inputBundlesListIt = mInputBundlesList.begin(); inputBundlesListIt != mInputBundlesList.end(); inputBundlesListIt++)
  {
    IasAudioChannelBundle *bundle = *inputBundlesListIt;
    printf("<<< pointer = %p\n", bundle->getAudioDataPointer());
  }
  printf("--------------------------------------------------\n");
#endif

#if DO_DEBUG_PRINTING
  // Print out the list of output bundles
  for (outputBundlesListIt = mOutputBundlesList.begin(); outputBundlesListIt != mOutputBundlesList.end(); outputBundlesListIt++)
  {
    IasAudioChannelBundle *bundle = *outputBundlesListIt;
    printf(">>> pointer = %p\n", bundle->getAudioDataPointer());
  }
  printf("--------------------------------------------------\n");
#endif

  // Create the matrix with the gain tiles: mGainTileMatrix[mNumberOutputBundles][mNumberInputBundles]
  // Each gain tile consists of 4x4 gain values.
  mGainTileMatrix = (IasMixerElementaryGainTile**)std::calloc(mNumberOutputBundles, sizeof(IasMixerElementaryGainTile*));
  if (mGainTileMatrix == nullptr)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "init: not enough memory!");
    return eIasAudioProcNotEnoughMemory;
  }
  for (uint32_t cntOutputBundles=0; cntOutputBundles < mNumberOutputBundles; cntOutputBundles++)
  {
#if MS_VC
    mGainTileMatrix[cntOutputBundles] = (IasMixerElementaryGainTile*)_aligned_malloc(mNumberInputBundles*sizeof(IasMixerElementaryGainTile), 16);
#else
    mGainTileMatrix[cntOutputBundles] = (IasMixerElementaryGainTile*)memalign(16, mNumberInputBundles*sizeof(IasMixerElementaryGainTile));
#endif
    if (mGainTileMatrix[cntOutputBundles] == nullptr)
    {
      DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "init: not enough memory!");
      return eIasAudioProcNotEnoughMemory;
    }

    std::memset(mGainTileMatrix[cntOutputBundles], 0, mNumberInputBundles*sizeof(IasMixerElementaryGainTile));
  }
  // Now we are ready to use our new  hyper-matrix in a way like mGainTileMatrix[0][0][1][2] = 0.25f;


  mNumberOutputChannels = (mStreamPair.first)->getNumberChannels();
  if (mNumberOutputChannels > 6)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "init: too many output channels!",
                    "Not more than six channels are supported. Current number is", mNumberOutputChannels);
    return eIasAudioProcInvalidParam;
  }
  if (mNumberOutputChannels != 2 && mNumberOutputChannels != 4 && mNumberOutputChannels != 6)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "init: incorrect number of output channels!",
                    "Must be 2, 4, or 6. Current number is", mNumberOutputChannels);
    return eIasAudioProcInvalidParam;
  }

  if(mNumberOutputChannels == 2)
  {
    mMatrixUpdateFunc = &updateMatrix_2Channels;
  }
  else if (mNumberOutputChannels == 4)
  {
    mMatrixUpdateFunc = &updateMatrix_4Channels;
  }
  else
  {
    mMatrixUpdateFunc = &updateMatrix_6Channels;
  }

  // Create the channelMappingVector for all input streams.
  // Loop over all input streams.
  for (inStreamIt = (mStreamPair.second).begin(); inStreamIt != (mStreamPair.second).end(); ++inStreamIt)
  {
    uint32_t tempNumInputChannels = (*inStreamIt)->getNumberChannels();
    if(tempNumInputChannels == 6)
    {
      multiChannelInputPresent = true; //this flag is used to turn off balance & fader for multichannel input
    }
    mNumberInputChannels += tempNumInputChannels;

    IasMixerElementaryStreamParams streamParams;
    streamParams.nChannels = (*inStreamIt)->getNumberChannels();
    streamParams.matrixGainVector.resize(mNumberOutputChannels);

    // Create the channelMappingVector for this input stream
    if (createChannelMapping(streamParams.channelMappingVector, *inStreamIt, mInputBundlesList, mLogContext))
    {
      DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "init: error while calling createChannelMapping()");
      return eIasAudioProcInitializationFailed;
    }

#if DO_DEBUG_PRINTING
    for (uint32_t cntChannelsThisStream = 0; cntChannelsThisStream < streamParams.nChannels; cntChannelsThisStream++)
    {
      printf(" Input Bundle Index = %d, Channel Index = %d\n",
             streamParams.channelMappingVector[cntChannelsThisStream].bundleIndex,
             streamParams.channelMappingVector[cntChannelsThisStream].channelIndex);
    }
    printf("--------------------------------------------------\n");
#endif

    IasAudioProcessingResult status = initStreamParams(&streamParams);
    if (status != eIasAudioProcOK)
    {
      DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "init: out of memory error while calling IasMixerElementary::initStreamParams()");
      return status;
    }
    IasMixerElementaryStreamParamsPair streamParamsPair((*inStreamIt)->getId(), streamParams);
    mStreamParamsMap.insert(streamParamsPair);
  }


  // Create the channelMappingVector for the output stream
  if (createChannelMapping(mOutputChannelMappingVector, (mStreamPair.first), mOutputBundlesList, mLogContext))
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "init: error while calling createChannelMapping()");
    return eIasAudioProcInitializationFailed;
  }

#if DO_DEBUG_PRINTING
  const uint32_t numChannelsThisStream = (mStreamPair.first)->getNumberChannels();
  for (uint32_t cntChannelsThisStream = 0; cntChannelsThisStream < numChannelsThisStream; cntChannelsThisStream++)
  {
    printf("Output Bundle Index = %d, Channel Index = %d\n",
           mOutputChannelMappingVector[cntChannelsThisStream].bundleIndex,
           mOutputChannelMappingVector[cntChannelsThisStream].channelIndex);
  }
  printf("--------------------------------------------------\n");
#endif

  return eIasAudioProcOK;
}


IasAudioProcessingResult IasMixerElementary::setupModeMain()
{
  // For each input stream: set up the vector matrixGainVector[mNumberOutputChannels],
  // which contains the pointers to the affected gain values within matrixGainVector.
  for(auto& streamParamsMapIt : mStreamParamsMap)
  {
    IasMixerElementaryStreamParams* streamParams = &(streamParamsMapIt.second);
    const uint32_t nChannels = streamParams->nChannels;
    uint32_t tempRowIdx = 0;

    for (uint32_t j = 0; j < mNumberOutputChannels; j++)
    {
      uint32_t outputBundle  = mOutputChannelMappingVector[j].bundleIndex;
      uint32_t outputChannel = mOutputChannelMappingVector[j].channelIndex;
      uint32_t inputBundle   = streamParams->channelMappingVector[tempRowIdx].bundleIndex;
      uint32_t inputChannel  = streamParams->channelMappingVector[tempRowIdx].channelIndex;

#if DO_DEBUG_PRINTING
      printf("outputBundle = %d, outputChannel = %d, inputBundle =%d, inputChannel = %d\n",
             outputBundle, outputChannel, inputBundle, inputChannel);
#endif

      switch (nChannels)
      {
        case 1:
          // mono input: since we have only one input channel, all gain values
          // of this stream affect the same row of mGainTileMatrix.
          //we mix a mono input stream ONLY on the first two channels of the output (mono -> outputChan0 , mono-> outputChan1)
          streamParams->matrixGainVector[j] = &(mGainTileMatrix[outputBundle][inputBundle][outputChannel][inputChannel]);
          if(multiChannelInputPresent)
          {
            if(j < (uint32_t)eIasMixerLFE)
            {
              *(streamParams->matrixGainVector[j]) =  1.0f;
            }
            else
            {
              *(streamParams->matrixGainVector[j]) = 0.0f;
            }
          }
          else
          {
            *(streamParams->matrixGainVector[j]) = 1.0f;
          }
        break;
        case 2:
          // stereo input: since we have two input channels, the gain values
          // of this stream affect two rows of mGainTileMatrix.
          //we mix a stereo input stream ONLY on the first two channels of the output (left -> outputChan0 , right-> outputChan1)
          streamParams->matrixGainVector[j] = &(mGainTileMatrix[outputBundle][inputBundle][outputChannel][inputChannel]);
          tempRowIdx++;
          tempRowIdx %=2;
          if(multiChannelInputPresent)
          {
            if(j < (uint32_t)eIasMixerLFE)
            {
              *(streamParams->matrixGainVector[j]) = 1.0f;
            }
            else if(j >=(uint32_t)eIasMixerLFE && j<(uint32_t)eIasMixerRearLeft)
            {
              *(streamParams->matrixGainVector[j]) = 0.0f;
            }
            else
            {
              *(streamParams->matrixGainVector[j]) = 0.707f;
            }
          }
          else
          {
            *(streamParams->matrixGainVector[j]) = 1.0f;
          }
          break;
        case 6:
          // input with six channels
          streamParams->matrixGainVector[j] = &(mGainTileMatrix[outputBundle][inputBundle][outputChannel][inputChannel]);
          tempRowIdx++;
          tempRowIdx %=6;
          *(streamParams->matrixGainVector[j]) = 1.0f;
          break;
        default:
          DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX,
                          "setupModeMain: input stream provides incorrect number of channels:", nChannels,
                          "must be 1, 2, or 6.");
          return eIasAudioProcInvalidParam;
      }
    }
  }

#if DO_DEBUG_PRINTING
  printMatrix();
#endif
  return eIasAudioProcOK;
}


void IasMixerElementary::updateGainTileMatrix(uint32_t sampleIdx)
{
  if (mRampActiveStreams.size() == 0)
  {
    return;
  }

  mRampActiveStreams.sort();
  mRampActiveStreams.unique();

  IasEMixerListIterator listIt;
  IasMixerElementaryStreamParamsMap::iterator streamParamsMapIt;

  for (listIt = mRampActiveStreams.begin(); listIt != mRampActiveStreams.end(); listIt++)
  {
    streamParamsMapIt = mStreamParamsMap.find(*listIt);
    if (streamParamsMapIt == mStreamParamsMap.end())
    {
      DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX,
                  "run: error, iterator not found! Size of active list:", mRampActiveStreams.size());
      return;
    }
    IasMixerElementaryStreamParams* params = &(streamParamsMapIt->second);
    mMatrixUpdateFunc(params, sampleIdx, multiChannelInputPresent);
  }
}


void IasMixerElementary::updateRampActiveStreams()
{
  if (mRampActiveStreams.size() == 0)
  {
    return;
  }

  mRampActiveStreams.sort();
  mRampActiveStreams.unique();

  IasMixerElementaryStreamParamsMap::iterator streamParamsMapIt;

  for (auto listIt = mRampActiveStreams.begin(); listIt != mRampActiveStreams.end();)
  {
    streamParamsMapIt = mStreamParamsMap.find(*listIt);
    if (streamParamsMapIt == mStreamParamsMap.end())
    {
      DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX,
                  "run: error, iterator not found! Size of active list:", mRampActiveStreams.size());
      return;
    }
    IasMixerElementaryStreamParams* params = &(streamParamsMapIt->second);

    if (params->balanceParams.active &&
        params->balanceParams.balanceRight[mFrameLength-1] == params->balanceParams.balanceRightTarget &&
        params->balanceParams.balanceLeft[mFrameLength-1] == params->balanceParams.balanceLeftTarget)
    {
      params->balanceParams.active = false;
 
      balanceFinishedEvent(streamParamsMapIt->first,
                           params->balanceParams.balanceLeftTarget,
                           params->balanceParams.balanceRightTarget);
    }

    if (params->fadeParams.active &&
        params->fadeParams.faderFront[mFrameLength-1] == params->fadeParams.faderFrontTarget &&
        params->fadeParams.faderRear[mFrameLength-1] == params->fadeParams.faderRearTarget)
    {
      params->fadeParams.active = false;

      faderFinishedEvent(streamParamsMapIt->first,
                         params->fadeParams.faderFrontTarget,
                         params->fadeParams.faderRearTarget);
    }

    if (params->gainOffsetParams.active &&
        params->gainOffsetParams.gainOffset[mFrameLength-1] == params->gainOffsetParams.gainOffsetTarget)
    {
      params->gainOffsetParams.active = false;

      inputGainOffsetFinishedEvent(streamParamsMapIt->first,
                                   params->gainOffsetParams.gainOffsetTarget);
    }

    if (params->balanceParams.active == false &&
        params->fadeParams.active == false &&
        params->gainOffsetParams.active == false)
    {
      if(params->nChannels == 6)
      {
        DLT_LOG_CXX(*mLogContext, DLT_LOG_VERBOSE, LOG_PREFIX, "balance left =", params->balanceParams.balanceLeft[mFrameLength-1]);
        DLT_LOG_CXX(*mLogContext, DLT_LOG_VERBOSE, LOG_PREFIX, "balance right =",params->balanceParams.balanceRight[mFrameLength-1]);
      }
      listIt = mRampActiveStreams.erase(listIt);
    }
    else
    {
      ++listIt;
    }
  }
}


void IasMixerElementary::run()
{
  IasEMixerListIterator listIt;
  IasMixerElementaryStreamParamsMap::iterator streamParamsMapIt;

  checkQueues();

  if (mRampActiveStreams.size() != 0)
  {
    mRampActiveStreams.sort();
    mRampActiveStreams.unique();

    for (listIt = mRampActiveStreams.begin(); listIt != mRampActiveStreams.end(); listIt++)
    {
      streamParamsMapIt = mStreamParamsMap.find(*listIt);
      if (streamParamsMapIt == mStreamParamsMap.end())
      {
        DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX,
                    "run: error, iterator not found! Size of active list:", mRampActiveStreams.size());
        return;
      }

      IasMixerElementaryStreamParams* params = &(streamParamsMapIt->second);
      if (params->balanceParams.active)
      {
        params->balanceParams.rampBalanceLeft->getRampValues(params->balanceParams.balanceLeft);
        params->balanceParams.rampBalanceRight->getRampValues(params->balanceParams.balanceRight);
      }
      if (params->fadeParams.active)
      {
        params->fadeParams.rampFaderFront->getRampValues(params->fadeParams.faderFront);
        params->fadeParams.rampFaderRear->getRampValues(params->fadeParams.faderRear);
      }
      if (params->gainOffsetParams.active)
      {
        params->gainOffsetParams.rampGainOffset->getRampValues(params->gainOffsetParams.gainOffset);
      }
    }
  }

  // Now we apply the core processing: calculate the output bundles
  // by multiplying the input bundles with the gain tiles.
  std::list<IasAudioChannelBundle*>::iterator outputBundlesListIt;
  std::list<IasAudioChannelBundle*>::iterator inputBundlesListIt;
  outputBundlesListIt = mOutputBundlesList.begin();

  for (uint32_t cntOutputBundle = 0; cntOutputBundle < mNumberOutputBundles; cntOutputBundle++)
  {
    IAS_ASSERT(outputBundlesListIt != mOutputBundlesList.end());
    IasAudioChannelBundle *outputBundle = *outputBundlesListIt;
    float *outputData = outputBundle->getAudioDataPointer();

    inputBundlesListIt = mInputBundlesList.begin();
    for (uint32_t cntInputBundle = 0; cntInputBundle < mNumberInputBundles; cntInputBundle++)
    {
      IAS_ASSERT(inputBundlesListIt != mInputBundlesList.end());
      IasAudioChannelBundle *inputBundle = *inputBundlesListIt;
      float *inputData = inputBundle->getAudioDataPointer();

      for (uint32_t sampleIdx = 0; sampleIdx < mFrameLength; sampleIdx++)
      {
        updateGainTileMatrix(sampleIdx);
#if SSE
        __m128 s0 = _mm_mul_ps(*(__m128 *)(&inputData[4*sampleIdx]), *(__m128 *)(mGainTileMatrix[cntOutputBundle][cntInputBundle][0]));
        __m128 s1 = _mm_mul_ps(*(__m128 *)(&inputData[4*sampleIdx]), *(__m128 *)(mGainTileMatrix[cntOutputBundle][cntInputBundle][1]));
        __m128 s2 = _mm_mul_ps(*(__m128 *)(&inputData[4*sampleIdx]), *(__m128 *)(mGainTileMatrix[cntOutputBundle][cntInputBundle][2]));
        __m128 s3 = _mm_mul_ps(*(__m128 *)(&inputData[4*sampleIdx]), *(__m128 *)(mGainTileMatrix[cntOutputBundle][cntInputBundle][3]));

        __m128 t0 =_mm_add_ps (_mm_unpacklo_ps(s0, s1), _mm_unpackhi_ps(s0, s1));
        __m128 t1 =_mm_add_ps (_mm_unpacklo_ps(s2, s3), _mm_unpackhi_ps(s2, s3));
        __m128 s = _mm_add_ps (_mm_movelh_ps(t0, t1), _mm_movehl_ps(t1, t0));

        __m128 sum = _mm_add_ps(*(__m128 *)(&outputData[4*sampleIdx]), s);
        _mm_store_ps(&outputData[4*sampleIdx], sum);
#else
        for (uint32_t outputChan = 0; outputChan < cIasNumChannelsPerBundle; outputChan++)
        {
          float sum = outputData[4*sampleIdx+outputChan];
          for (uint32_t inputChan = 0; inputChan < cIasNumChannelsPerBundle; inputChan++)
          {
            sum += inputData[4*sampleIdx+inputChan] * mGainTileMatrix[cntOutputBundle][cntInputBundle][outputChan][inputChan];
          }
          outputData[4*sampleIdx+outputChan] = sum;
        }
#endif
      }
      inputBundlesListIt++;
    }
    outputBundlesListIt++;
  }
  
  updateRampActiveStreams();
}


IasAudioProcessingResult IasMixerElementary::setBalance(int32_t   streamId,
                                                        float balanceLeft,
                                                        float balanceRight)
{
  const auto mapIt = mStreamParamsMap.find(streamId);
  if (mapIt == mStreamParamsMap.end())
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "setBalance: invalid streamId: streamId=", streamId);
    return eIasAudioProcInvalidParam;
  }

  IasMixerBalanceQueueEntry queueEntry;
  queueEntry.streamId = streamId;
  queueEntry.left = balanceLeft;
  queueEntry.right = balanceRight;
  mBalanceQueue.push(queueEntry);

  return eIasAudioProcOK;
}


IasAudioProcessingResult IasMixerElementary::setFader(int32_t   streamId,
                                                      float faderFront,
                                                      float faderRear)
{
  const auto mapIt = mStreamParamsMap.find(streamId);
  if (mapIt == mStreamParamsMap.end())
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "setFader: invalid streamId: streamId=", streamId);
    return eIasAudioProcInvalidParam;
  }

  IasMixerFaderQueueEntry queueEntry;
  queueEntry.streamId = streamId;
  queueEntry.front = faderFront;
  queueEntry.rear = faderRear;
  mFaderQueue.push(queueEntry);

  return eIasAudioProcOK;
}


IasAudioProcessingResult IasMixerElementary::setInputGainOffset(int32_t streamId, float gainOffset)
{
  const auto mapIt = mStreamParamsMap.find(streamId);
  if (mapIt == mStreamParamsMap.end())
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "setInputGainOffset: invalid streamId: streamId=", streamId);
    return eIasAudioProcInvalidParam;
  }

  IasMixerGainOffsetQueueEntry queueEntry;
  queueEntry.gainOffset = gainOffset;
  queueEntry.streamId = streamId;
  mGainOffsetQueue.push(queueEntry);

  return eIasAudioProcOK;
}


IasAudioProcessingResult IasMixerElementary::initStreamParams(IasMixerElementaryStreamParams* params)
{
  // Preinit pointers to zero in case we need to call cleanupStreamParams which deletes the pointers
  params->balanceParams.balanceLeft = nullptr;
  params->balanceParams.rampBalanceLeft = nullptr;
  params->balanceParams.balanceRight = nullptr;
  params->balanceParams.rampBalanceRight = nullptr;
  params->fadeParams.faderFront = nullptr;
  params->fadeParams.rampFaderFront = nullptr;
  params->fadeParams.faderRear = nullptr;
  params->fadeParams.rampFaderRear = nullptr;
  params->gainOffsetParams.gainOffset = nullptr;
  params->gainOffsetParams.rampGainOffset = nullptr;

  params->balanceParams.active = false;
  params->balanceParams.balanceLeftTarget = 1.0f;
  params->balanceParams.balanceRightTarget = 1.0f;
  params->balanceParams.rampBalanceLeft = new IasRamp(mSampleFreq, mFrameLength);
  if (params->balanceParams.rampBalanceLeft == nullptr)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "initStreamParams: not enough memory!");
    cleanupStreamParams(params);
    return eIasAudioProcNotEnoughMemory;
  }
  params->balanceParams.rampBalanceRight = new IasRamp(mSampleFreq, mFrameLength);
  if (params->balanceParams.rampBalanceRight == nullptr)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "initStreamParams: not enough memory!");
    cleanupStreamParams(params);
    return eIasAudioProcNotEnoughMemory;
  }
#if MS_VC
  params->balanceParams.balanceLeft = (float*)_aligned_malloc(mFrameLength * sizeof(float), 16);
  params->balanceParams.balanceRight = (float*)_aligned_malloc(mFrameLength * sizeof(float), 16);
#else
  params->balanceParams.balanceLeft = (float*)memalign(16, mFrameLength * sizeof(float));
  params->balanceParams.balanceRight = (float*)memalign(16, mFrameLength * sizeof(float));
#endif
  if (params->balanceParams.balanceLeft == nullptr || params->balanceParams.balanceRight == nullptr)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "initStreamParams: not enough memory!");
    cleanupStreamParams(params);
    return eIasAudioProcNotEnoughMemory;
  }
  for (uint32_t i = 0; i < mFrameLength; i++)
  {
    params->balanceParams.balanceLeft[i] = 1.0f;
    params->balanceParams.balanceRight[i] = 1.0f;
  }

  params->fadeParams.active = false;
  params->fadeParams.faderFrontTarget = 1.0f;
  params->fadeParams.faderRearTarget = 1.0f;
  params->fadeParams.rampFaderFront = new IasRamp(mSampleFreq, mFrameLength);
  if (params->fadeParams.rampFaderFront == nullptr)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "initStreamParams: not enough memory!");
    cleanupStreamParams(params);
    return eIasAudioProcNotEnoughMemory;
  }
  params->fadeParams.rampFaderRear = new IasRamp(mSampleFreq, mFrameLength);
  if (params->fadeParams.rampFaderRear == nullptr)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "initStreamParams: not enough memory!");
    cleanupStreamParams(params);
    return eIasAudioProcNotEnoughMemory;
  }
#if MS_VC
  params->fadeParams.faderFront = (float*)_aligned_malloc(mFrameLength * sizeof(float), 16);
  params->fadeParams.faderRear = (float*)_aligned_malloc(mFrameLength * sizeof(float), 16);
#else
  params->fadeParams.faderFront = (float*)memalign(16, mFrameLength * sizeof(float));
  params->fadeParams.faderRear = (float*)memalign(16, mFrameLength * sizeof(float));
#endif
  if (params->fadeParams.faderFront == nullptr || params->fadeParams.faderRear == nullptr)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "initStreamParams: not enough memory!");
    cleanupStreamParams(params);
    return eIasAudioProcNotEnoughMemory;
  }
  for (uint32_t i = 0; i < mFrameLength; i++)
  {
    params->fadeParams.faderFront[i] = 1.0f;
    params->fadeParams.faderRear[i] = 1.0f;
  }

  params->gainOffsetParams.active = false;
  params->gainOffsetParams.gainOffsetTarget = 1.0f;
  params->gainOffsetParams.rampGainOffset = new IasRamp(mSampleFreq, mFrameLength);
  if (params->gainOffsetParams.rampGainOffset == nullptr)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "initStreamParams: not enough memory!");
    cleanupStreamParams(params);
    return eIasAudioProcNotEnoughMemory;
  }
#if MS_VC
  params->gainOffsetParams.gainOffset = (float*)_aligned_malloc(mFrameLength * sizeof(float), 16);
#else
  params->gainOffsetParams.gainOffset = (float*)memalign(16, mFrameLength * sizeof(float));
#endif
  if (params->gainOffsetParams.gainOffset == nullptr)
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, LOG_PREFIX, "initStreamParams: not enough memory!");
    cleanupStreamParams(params);
    return eIasAudioProcNotEnoughMemory;
  }
  for (uint32_t i = 0; i< mFrameLength; i++)
  {
    params->gainOffsetParams.gainOffset[i] = 1.0f;
  }

  return eIasAudioProcOK;
}


void IasMixerElementary::updateMatrix_2Channels(IasMixerElementaryStreamParams* params, uint32_t sampleIdx, bool activeMultiChannelInput)
{
  (void)activeMultiChannelInput;
  *(params->matrixGainVector[0]) = params->balanceParams.balanceLeft[sampleIdx]*
    params->gainOffsetParams.gainOffset[sampleIdx];
  *(params->matrixGainVector[1]) = params->balanceParams.balanceRight[sampleIdx]*
    params->gainOffsetParams.gainOffset[sampleIdx];
}

void IasMixerElementary::updateMatrix_4Channels(IasMixerElementaryStreamParams* params, uint32_t sampleIdx, bool activeMultiChannelInput)
{
  (void)activeMultiChannelInput;
  *(params->matrixGainVector[0]) = params->balanceParams.balanceLeft[sampleIdx]*
    params->fadeParams.faderFront[sampleIdx]*
    params->gainOffsetParams.gainOffset[sampleIdx];
  *(params->matrixGainVector[1]) = params->balanceParams.balanceRight[sampleIdx]*
    params->fadeParams.faderFront[sampleIdx]*
    params->gainOffsetParams.gainOffset[sampleIdx];
  *(params->matrixGainVector[2]) = params->balanceParams.balanceLeft[sampleIdx]*
    params->fadeParams.faderRear[sampleIdx]*
    params->gainOffsetParams.gainOffset[sampleIdx];
  *(params->matrixGainVector[3]) = params->balanceParams.balanceRight[sampleIdx]*
    params->fadeParams.faderRear[sampleIdx]*
    params->gainOffsetParams.gainOffset[sampleIdx];
}

void IasMixerElementary::cleanupStreamParams(IasMixerElementaryStreamParams* params)
{
  delete params->balanceParams.rampBalanceLeft;
  delete params->balanceParams.rampBalanceRight;
  delete params->fadeParams.rampFaderFront;
  delete params->fadeParams.rampFaderRear;
  delete params->gainOffsetParams.rampGainOffset;
  params->balanceParams.rampBalanceLeft   = nullptr;
  params->balanceParams.rampBalanceRight  = nullptr;
  params->fadeParams.rampFaderFront       = nullptr;
  params->fadeParams.rampFaderRear        = nullptr;
  params->gainOffsetParams.rampGainOffset = nullptr;
#if MS_VC
  _aligned_free(params->balanceParams.balanceLeft);
  _aligned_free(params->balanceParams.balanceRight);
  _aligned_free(params->fadeParams.faderFront);
  _aligned_free(params->fadeParams.faderRear);
  _aligned_free(params->gainOffsetParams.gainOffset);
#else
  std::free(params->balanceParams.balanceLeft);
  std::free(params->balanceParams.balanceRight);
  std::free(params->fadeParams.faderFront);
  std::free(params->fadeParams.faderRear);
  std::free(params->gainOffsetParams.gainOffset);
#endif
  params->balanceParams.balanceLeft   = nullptr;
  params->balanceParams.balanceRight  = nullptr;
  params->fadeParams.faderFront       = nullptr;
  params->fadeParams.faderRear        = nullptr;
  params->gainOffsetParams.gainOffset = nullptr;
}


void IasMixerElementary::updateMatrix_6Channels(IasMixerElementaryStreamParams* params, uint32_t sampleIdx, bool activeMultiChannelInput)
{
  static float centerAttenuation = 1.0f;
  if(!activeMultiChannelInput)
  {
    *(params->matrixGainVector[0]) = params->balanceParams.balanceLeft[sampleIdx] *
        params->fadeParams.faderFront[sampleIdx] *
        params->gainOffsetParams.gainOffset[sampleIdx];
    *(params->matrixGainVector[1]) = params->balanceParams.balanceRight[sampleIdx] *
        params->fadeParams.faderFront[sampleIdx] *
        params->gainOffsetParams.gainOffset[sampleIdx];
    *(params->matrixGainVector[2]) = params->balanceParams.balanceLeft[sampleIdx] *
        params->gainOffsetParams.gainOffset[sampleIdx];
    *(params->matrixGainVector[3]) = params->balanceParams.balanceRight[sampleIdx] *
        params->gainOffsetParams.gainOffset[sampleIdx];
    *(params->matrixGainVector[4]) = params->balanceParams.balanceLeft[sampleIdx] *
        params->fadeParams.faderRear[sampleIdx] *
        params->gainOffsetParams.gainOffset[sampleIdx];
    *(params->matrixGainVector[5]) = params->balanceParams.balanceRight[sampleIdx] *
        params->fadeParams.faderRear[sampleIdx] *
        params->gainOffsetParams.gainOffset[sampleIdx];
  }
  else
  {
    //when we run in multichannel mode, we assume the following channel layout:
    // FL, FR, C, LFE, RL, RR
    // front left and front right are normally affected by balance and fader
    // center is normally affected by fader, but for balance we calculate the attentuation depending
    // on the balance values for left an right like this : att = 1 - (abs(r-l)/2)
    // LFE is not affected by any balance or balance
    // rear left and rear right are normally affected by balance and fader
    *(params->matrixGainVector[eIasMixerFrontLeft])  = params->fadeParams.faderFront[sampleIdx] *
                                                        params->balanceParams.balanceLeft[sampleIdx] *
                                                        params->gainOffsetParams.gainOffset[sampleIdx];
    *(params->matrixGainVector[eIasMixerFrontRight]) = params->fadeParams.faderFront[sampleIdx] *
                                                        params->balanceParams.balanceRight[sampleIdx] *
                                                        params->gainOffsetParams.gainOffset[sampleIdx];

    if(params->nChannels == 1)
    {
      *(params->matrixGainVector[eIasMixerCenter]) = 0.0f;
      *(params->matrixGainVector[eIasMixerLFE]) = 0.0f;
    }
    else if(params->nChannels == 2)
    {
      *(params->matrixGainVector[eIasMixerCenter]) = 0.0f;
      *(params->matrixGainVector[eIasMixerLFE]) = 0.0f;
      *(params->matrixGainVector[eIasMixerRearLeft])  = params->fadeParams.faderRear[sampleIdx] *
                                                        params->balanceParams.balanceLeft[sampleIdx] *
                                                        params->gainOffsetParams.gainOffset[sampleIdx] * 0.707f;
      *(params->matrixGainVector[eIasMixerRearRight]) = params->fadeParams.faderRear[sampleIdx] *
                                                        params->balanceParams.balanceRight[sampleIdx] *
                                                        params->gainOffsetParams.gainOffset[sampleIdx] * 0.707f;
    }
    else
    {
      const float oldCenter = *(params->matrixGainVector[eIasMixerCenter]);
      if(params->balanceParams.active == false && params->fadeParams.active == false)
      {
        fprintf(stderr,"center value is %f\n",oldCenter);
        fprintf(stderr,"fader front value is %f\n",params->fadeParams.faderFront[sampleIdx]);
      }
      if(params->balanceParams.balanceLeft[sampleIdx] == params->balanceParams.balanceRight[sampleIdx])
      {
        centerAttenuation = params->balanceParams.balanceLeft[sampleIdx];
      }
      else
      {
        centerAttenuation = 1.0f - static_cast<float>(std::abs(params->balanceParams.balanceRight[sampleIdx]-params->balanceParams.balanceLeft[sampleIdx])*0.5f);
      }
      *(params->matrixGainVector[eIasMixerCenter])    = params->fadeParams.faderFront[sampleIdx] *
                                                        centerAttenuation *
                                                        params->gainOffsetParams.gainOffset[sampleIdx];
      *(params->matrixGainVector[eIasMixerLFE])       = params->gainOffsetParams.gainOffset[sampleIdx];

      *(params->matrixGainVector[eIasMixerRearLeft])  = params->fadeParams.faderRear[sampleIdx] *
                                                        params->balanceParams.balanceLeft[sampleIdx] *
                                                        params->gainOffsetParams.gainOffset[sampleIdx];
      *(params->matrixGainVector[eIasMixerRearRight]) = params->fadeParams.faderRear[sampleIdx] *
                                                        params->balanceParams.balanceRight[sampleIdx] *
                                                        params->gainOffsetParams.gainOffset[sampleIdx];

    }
  }
}

void IasMixerElementary::checkQueues()
{
  IasMixerBalanceQueueEntry balanceQueueEntry;
  balanceQueueEntry.streamId = 0;
  balanceQueueEntry.left = 0.0f;
  balanceQueueEntry.right= 0.0f;
  while(mBalanceQueue.try_pop(balanceQueueEntry))
  {
    updateBalance(balanceQueueEntry.streamId, balanceQueueEntry.left, balanceQueueEntry.right);
  }

  IasMixerFaderQueueEntry faderQueueEntry;
  faderQueueEntry.streamId = 0;
  faderQueueEntry.front = 0.0f;
  faderQueueEntry.rear= 0.0f;
  while(mFaderQueue.try_pop(faderQueueEntry))
  {
    updateFader(faderQueueEntry.streamId, faderQueueEntry.front, faderQueueEntry.rear);
  }

  IasMixerGainOffsetQueueEntry gainOffsetQueueEntry;
  gainOffsetQueueEntry.streamId = 0;
  gainOffsetQueueEntry.gainOffset = 0.0f;
  while(mGainOffsetQueue.try_pop(gainOffsetQueueEntry))
  {
    updateGainOffset(gainOffsetQueueEntry.streamId, gainOffsetQueueEntry.gainOffset);
  }
}

void IasMixerElementary::updateBalance(int32_t streamId, float left, float right)
{
  auto mapIt = mStreamParamsMap.find(streamId);
  IAS_ASSERT(mapIt != mStreamParamsMap.end());

  auto* params = &(mapIt->second);
  params->balanceParams.balanceLeftTarget = left;
  params->balanceParams.balanceRightTarget = right;
  params->balanceParams.rampBalanceLeft->setTimedRamp(params->balanceParams.balanceLeft[mFrameLength-1],
                                                      left,
                                                      100,
                                                      eIasRampShapeLinear);
  params->balanceParams.rampBalanceRight->setTimedRamp(params->balanceParams.balanceRight[mFrameLength-1],
                                                       right,
                                                       100,
                                                       eIasRampShapeLinear);
  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX,
                  "setBalance: setting new balance, streamId=", streamId,
                  "balanceLeft=",  left,
                  "balanceRight=", right);
  params->balanceParams.active = true;
  mRampActiveStreams.push_back(streamId);
}

void IasMixerElementary::updateFader(int32_t streamId, float front, float rear)
{
  auto mapIt = mStreamParamsMap.find(streamId);
  IAS_ASSERT(mapIt != mStreamParamsMap.end());

  auto* params = &(mapIt->second);
  params->fadeParams.faderFrontTarget = front;
  params->fadeParams.faderRearTarget = rear;
  params->fadeParams.rampFaderFront->setTimedRamp(params->fadeParams.faderFront[mFrameLength-1],
                                                  front,
                                                  100,
                                                  eIasRampShapeLinear);
  params->fadeParams.rampFaderRear->setTimedRamp(params->fadeParams.faderRear[mFrameLength-1],
                                                 rear,
                                                 100,
                                                 eIasRampShapeLinear);
  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX
                  "setFader: setting new fader, streamId=", streamId,
                  "faderFront=", front,
                  "faderRear=",  rear);
  params->fadeParams.active = true;
  mRampActiveStreams.push_back(streamId);
}

void IasMixerElementary::updateGainOffset(int32_t streamId, float gainOffset)
{
  auto mapIt = mStreamParamsMap.find(streamId);
  IAS_ASSERT(mapIt != mStreamParamsMap.end());

  auto* params = &(mapIt->second);
  params->gainOffsetParams.gainOffsetTarget = gainOffset;
  params->gainOffsetParams.rampGainOffset->setTimedRamp(params->gainOffsetParams.gainOffset[mFrameLength-1],
                                                        gainOffset,
                                                        100,
                                                        eIasRampShapeLinear);
  DLT_LOG_CXX(*mLogContext, DLT_LOG_INFO, LOG_PREFIX
                  "setInputGainOffset: setting new inputGainOffset, streamId=", streamId,
                  "gainOffset=", gainOffset);
  params->gainOffsetParams.active = true;
  mRampActiveStreams.push_back(streamId);
}


void IasMixerElementary::sendEvent(int32_t streamId, IasProperties& properties)
{
  std::string pinName;
  mConfig->getPinName(streamId, pinName);
  properties.set("pin", pinName);
  properties.set<std::string>("typeName",     mTypeName);
  properties.set<std::string>("instanceName", mInstanceName);
  IasModuleEventPtr event = IasEventProvider::getInstance()->createModuleEvent();
  event->setProperties(properties);
  IasEventProvider::getInstance()->send(event);
  IasEventProvider::getInstance()->destroyModuleEvent(event);
}


void IasMixerElementary::balanceFinishedEvent(int32_t streamId, float balanceLeft, float balanceRight)
{
  int32_t balanceReturn;

  // the lowest value to be really calculated should be -143.9 dB -> 6.382634861905487e-08
  // -144 dB should never be calculated, it should represent 0.0 ( the real value would be 6.3095734448e-08 )
  // for comparison against zero we use a comparison to be lower than 6.3095734448e-08
  const float low = static_cast<float>(6.309573444801932494e-08);

  if (balanceLeft < low) //we can expect zero
  {
    balanceReturn = 1440;
  }
  else if (balanceRight < low) //we can expect zero
  {
    balanceReturn = -1440;
  }
  else if(balanceLeft < 1.0f)
  {
    balanceReturn = (int32_t)(-200.0f*log10(balanceLeft) +0.5f );
  }
  else if (balanceRight < 1.0f)
  {
    balanceReturn = (int32_t)(200.0f*log10(balanceRight) -0.5f );
  }
  else // both value are 1.0f, so we return 0 dB
  {
    balanceReturn = 0;
  }

  IasProperties properties;
  properties.set<int32_t>("eventType", IasMixer::IasMixerEventTypes::eIasBalanceFinished);
  properties.set<int32_t>("balance", balanceReturn);
  sendEvent(streamId, properties);
}

void IasMixerElementary::faderFinishedEvent(int32_t streamId, float faderFront, float faderRear)
{
  int32_t faderReturn;

  // the lowest value to be really calculated should be -143.9 dB -> 6.382634861905487e-08
  // -144 dB should never be calculated, it should represent 0.0 ( the real value would be 6.3095734448e-08 )
  // for comparison against zero we use a comparison to be lower than 6.3095734448e-08
  const float low = static_cast<float>(6.309573444801932494e-08);

  if (faderFront < low ) //we can expect zero
  {
    faderReturn = -1440;
  }
  else if (faderRear < low) //we can expect zero
  {
    faderReturn = 1440;
  }
  else if (faderFront < 1.0f)
  {
    faderReturn = (int32_t)(200.0f*log10(faderFront) -0.5f );
  }
  else if (faderRear < 1.0f)
  {
    faderReturn = (int32_t)(-200.0f*log10(faderRear) +0.5f );
  }
  else // both value are 1.0f, so we return 0 dB
  {
    faderReturn = 0;
  }

  IasProperties properties;
  properties.set<int32_t>("eventType", IasMixer::IasMixerEventTypes::eIasFaderFinished);
  properties.set<int32_t>("fader", faderReturn);
  sendEvent(streamId, properties);
}

void IasMixerElementary::inputGainOffsetFinishedEvent(int32_t streamId, float gainOffset)
{

  float factor = (gainOffset >= 1 ? 0.5f : -0.5f);
  int32_t gain_lin = (int32_t)(200.0f * log10(gainOffset) +factor);
  IasProperties properties;
  properties.set<int32_t>("eventType", IasMixer::IasMixerEventTypes::eIasInputGainOffsetFinished);
  properties.set<int32_t>("gainOffset", gain_lin);
  sendEvent(streamId, properties);
}

} // namespace IasAudio
