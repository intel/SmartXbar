/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/*
 * IasEqualizerUserTest.hpp
 *
 *  Created on: January 2017
 */

#ifndef IASEQUALIZERUSERTEST_HPP_
#define IASEQUALIZERUSERTEST_HPP_

#include <utility>

#include <gtest/gtest.h>

#include "audio/smartx/rtprocessingfwx/IasIModuleId.hpp"
#include "audio/equalizerx/IasEqualizerCmd.hpp"
#include "smartx/IasAudioTypedefs.hpp"


namespace IasAudio
{

struct SetEqualizerParams
{
  int32_t filterId;
  int32_t freq;
  int32_t quality;
  int32_t type;
  int32_t order;

};

struct SetRampGradientSingleStreamParams
{
  int32_t cmd = static_cast<int32_t>(IasEqualizer::IasEqualizerCmdIds::eIasUserModeSetRampGradientSingleStream);
  int32_t gradient;

  SetRampGradientSingleStreamParams(
    int32_t _gradient) :
    gradient{_gradient}
  {}
};


class IasEqualizerUserGTest : public ::testing::Test
{

protected:

  void SetUp() override;
  void TearDown() override;

  void init(const int numFilterStagesMax);
  void init();
  void exit();

  IasIModuleId::IasResult setEqualizer(const char* pin,
    const int32_t filterId, const int32_t gain, IasIModuleId* cmd);

  IasIModuleId::IasResult setRampGradientSingleStream(const char* pin,
      const SetRampGradientSingleStreamParams params, IasIModuleId* cmd);

  IasIModuleId::IasResult setEqualizerParams(const char* pin,
    const SetEqualizerParams params, IasIModuleId* cmd);

  IasIModuleId::IasResult setConfigFilterParamsStream(const char* pin,
    const SetEqualizerParams params, IasIModuleId* cmd);

  std::pair<IasIModuleId::IasResult, SetEqualizerParams> getConfigFilterParamsStream(const char* pin,
    const int32_t filterId, IasIModuleId* cmd);

public:

  IasAudioChain               *mAudioChain    = nullptr;
  IasIGenericAudioCompConfig  *mEqualizerConfig = nullptr;
  IasGenericAudioComp         *mEqualizer     = nullptr;
  IasCmdDispatcherPtr          mCmdDispatcher = nullptr;
  IasPluginEngine             *mPluginEngine  = nullptr;

  IasAudioStream              *mInStream1     = nullptr;
  IasAudioStream              *mInStream2     = nullptr;
  IasAudioStream              *mInStream3     = nullptr;

};

} // namespace IasAudio


#endif /* IASEQUALIZERCARTEST_HPP_ */
