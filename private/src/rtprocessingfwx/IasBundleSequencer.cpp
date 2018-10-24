/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasBundleSequencer.cpp
 * @date   2012
 * @brief  This is the implementation of the IasBundleSequencer class.
 */

#include "rtprocessingfwx/IasBundleAssignment.hpp"
#include "rtprocessingfwx/IasBundleSequencer.hpp"
#include "rtprocessingfwx/IasAudioChannelBundle.hpp"
#include "rtprocessingfwx/IasAudioChainEnvironment.hpp"


namespace IasAudio {

IasBundleSequencer::IasBundleSequencer()
  :mLogContext(IasAudioLogging::getDltContext("PFW"))
{
}

IasBundleSequencer::~IasBundleSequencer()
{
  while (!mMonoBundles.empty())
  {
    delete mMonoBundles.back();
    mMonoBundles.pop_back();
  }

  while (!mStereoBundles.empty())
  {
    delete mStereoBundles.back();
    mStereoBundles.pop_back();
  }

  while (!mMultiChannelBundles.empty())
  {
    delete mMultiChannelBundles.back();
    mMultiChannelBundles.pop_back();
  }
}

IasAudioProcessingResult IasBundleSequencer::init(IasAudioChainEnvironmentPtr env)
{
  if (env != nullptr)
  {
    mEnv = env;
    return eIasAudioProcOK;
  }
  else
  {
    return eIasAudioProcInvalidParam;
  }
}


void IasBundleSequencer::clearAllBundleBuffers() const
{
  IasBundleVector::const_iterator bundleIt;
  for (bundleIt = mMonoBundles.begin(); bundleIt != mMonoBundles.end(); ++bundleIt)
  {
    (*bundleIt)->clear();
  }
  for (bundleIt = mStereoBundles.begin(); bundleIt != mStereoBundles.end(); ++bundleIt)
  {
    (*bundleIt)->clear();
  }
  for (bundleIt = mMultiChannelBundles.begin(); bundleIt != mMultiChannelBundles.end(); ++bundleIt)
  {
    (*bundleIt)->clear();
  }
}

IasAudioProcessingResult IasBundleSequencer::getBundleAssignments(uint32_t numberChannels,
                                                                  IasBundleAssignmentVector* bundleAssignments)
{
  IasAudioProcessingResult result;
  if (mEnv->getFrameLength() != 0)
  {
    if (numberChannels == 1)
    {
      result = getSimpleBundleAssignments(numberChannels, &mMonoBundles, bundleAssignments);
    }
    else if (numberChannels == 2)
    {
      result = getSimpleBundleAssignments(numberChannels, &mStereoBundles, bundleAssignments);
    }
    else
    {
      result = getMultiChannelBundleAssignments(numberChannels, &mMultiChannelBundles, bundleAssignments);
    }
  }
  else
  {
    DLT_LOG_CXX(*mLogContext, DLT_LOG_ERROR, "IasBundleSequencer::getBundleAssignments: Frame length == 0");
    result = eIasAudioProcNotInitialization;
  }
  return result;
}

IasAudioProcessingResult IasBundleSequencer::getSimpleBundleAssignments(uint32_t numberChannels,
                                                                        IasBundleVector* bundleVector,
                                                                        IasBundleAssignmentVector* bundleAssignments)
{
  IasAudioProcessingResult result = eIasAudioProcOK;
  IasBundleVector::iterator itBundles = bundleVector->begin();

  do
  {
    IasAudioChannelBundle *bundle;
    if (itBundles < bundleVector->end())
    {
      bundle = *itBundles;
    }
    else
    {
      bundle = NULL;
    }
    uint32_t numberFreeChannels;
    uint32_t numberChannelsToReserve = numberChannels;
    if (bundle)
    {
      // There is already a bundle available that can be checked for free space
      // Increment the iterator for the next round
      if (itBundles < bundleVector->end())
      {
        itBundles++;
      }
      else
      {
        itBundles = bundleVector->begin();
      }
    }
    else
    {
      // No bundle available, so create a new one and add it to the list
      bundle = new IasAudioChannelBundle(mEnv->getFrameLength());
      IAS_ASSERT(bundle != nullptr);
      result = bundle->init();
      if (result != eIasAudioProcOK)
      {
        // Log already done inside bundle->init() method
        // Clean-up and return error
        delete bundle;
        return result;
      }
      bundleVector->push_back(bundle);
      itBundles = bundleVector->begin();
    }

    numberFreeChannels = bundle->getNumFreeChannels();

    if (numberChannelsToReserve <= numberFreeChannels)
    {
      // The channels can be reserved now
      uint32_t startIndex;
      result = bundle->reserveChannels(numberChannelsToReserve, &startIndex);
      if (result != eIasAudioProcOK)
      {
        break;
      }
      bundleAssignments->push_back(IasBundleAssignment(bundle, startIndex, numberChannelsToReserve));
      numberChannels -= numberChannelsToReserve;
    }
  }
  while (numberChannels);

  return result;
}

IasAudioProcessingResult IasBundleSequencer::getMultiChannelBundleAssignments(uint32_t numberChannels,
                                                                              IasBundleVector* bundleVector,
                                                                              IasBundleAssignmentVector* bundleAssignments)
{
  IasAudioProcessingResult result = eIasAudioProcOK;
  IasBundleVector::iterator itBundles = bundleVector->begin();

  do
  {
    IasAudioChannelBundle *bundle;
    if (itBundles < bundleVector->end())
    {
      bundle = *itBundles;
    }
    else
    {
      bundle = NULL;
    }
    uint32_t numberFreeChannels;
    uint32_t numberChannelsToReserve = numberChannels;
    if (bundle)
    {
      // There is already a bundle available that can be checked for free space
      // Increment the iterator for the next round
      if (itBundles < bundleVector->end())
      {
        itBundles++;
      }
      else
      {
        itBundles = bundleVector->begin();
      }
    }
    else
    {
      // No bundle available, so create a new one and add it to the list
      bundle = new IasAudioChannelBundle(mEnv->getFrameLength());
      IAS_ASSERT(bundle != nullptr);
      result = bundle->init();
      if (result != eIasAudioProcOK)
      {
        // Log already done inside bundle->init() method
        // Clean-up and return error
        delete bundle;
        return result;
      }
      bundleVector->push_back(bundle);
      itBundles = bundleVector->begin();
    }

    numberFreeChannels = bundle->getNumFreeChannels();
    if (numberFreeChannels)
    {
      if (numberChannelsToReserve > numberFreeChannels)
      {
        // Not all channels fit into current bundle. Check if and how we can split them up.
        if ((numberFreeChannels == 1) ||
            (numberChannelsToReserve == 4) ||
            (numberChannelsToReserve >= 8))
        {
          // Create a new bundle if there is only one free channel left. The free channel can then
          // eventually later be used for a mono channel
          // Also create a new bundle if we have 4 channels to reserve because e.g. we already reserved 2 channels
          // from a 6 channel stream.
          // Also create a new bundle when we have 8 or more channels to reserve to split this stream into as less
          // bundles as possible
          bundle = new IasAudioChannelBundle(mEnv->getFrameLength());
          IAS_ASSERT(bundle != nullptr);
          result = bundle->init();
          if (result != eIasAudioProcOK)
          {
            // Log already done inside bundle->init() method
            // Clean-up and return error
            delete bundle;
            return result;
          }
          bundleVector->push_back(bundle);
          numberFreeChannels = bundle->getNumFreeChannels();
        }

        if (numberChannelsToReserve > numberFreeChannels)
        {
          numberChannelsToReserve = numberFreeChannels;
        }
      }

      if (numberChannelsToReserve <= numberFreeChannels)
      {
        // The channels can be reserved now
        uint32_t startIndex;
        result = bundle->reserveChannels(numberChannelsToReserve, &startIndex);
        if (result != eIasAudioProcOK)
        {
          break;
        }
        bundleAssignments->push_back(IasBundleAssignment(bundle, startIndex, numberChannelsToReserve));
        numberChannels -= numberChannelsToReserve;
        itBundles = bundleVector->begin();
      }
    }
  }
  while (numberChannels);

  return result;
}

} // namespace IasAudio
