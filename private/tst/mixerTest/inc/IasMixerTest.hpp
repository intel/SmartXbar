/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/*
 * IasMixerTest.hpp
 *
 *  Created on: August 2016
 */

#ifndef IASMIXERTEST_HPP_
#define IASMIXERTEST_HPP_

#include <gtest/gtest.h>

namespace IasAudio
{

class IasAudioChain;
class IasIGenericAudioCompConfig;
class IasGenericAudioComp;
class IasAudioStream;
class IasCmdDispatcher;
class IasPluginEngine;

class IasMixerTest : public ::testing::Test
{

protected:
  virtual void SetUp();
  virtual void TearDown();

public:
  IasAudioChain               *mAudioChain    = nullptr;
  IasIGenericAudioCompConfig  *mMixerConfig   = nullptr;
  IasGenericAudioComp         *mMixer         = nullptr;
  IasCmdDispatcherPtr          mCmdDispatcher = nullptr;
  IasPluginEngine             *mPluginEngine  = nullptr;

  IasAudioStream              *mInStream0     = nullptr;
  IasAudioStream              *mInStream1     = nullptr;
  IasAudioStream              *mInStream6     = nullptr;
  IasAudioStream              *mOutStream4    = nullptr;
  IasAudioStream              *mOutStream8    = nullptr;
  IasAudioStream              *mOutStream3    = nullptr;
  IasAudioStream              *mOutStream2    = nullptr;
  IasAudioStream              *mOutStream6    = nullptr;
};

} // namespace IasAudio

#endif /* IASMIXERTEST_HPP_ */
