/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/*
 * IasEqualizerCarTest.hpp
 *
 *  Created on: November 2016
 */

#ifndef IASEQUALIZERCARTEST_HPP_
#define IASEQUALIZERCARTEST_HPP_

#include <utility>

#include <gtest/gtest.h>

#include "audio/smartx/rtprocessingfwx/IasIModuleId.hpp"
#include "audio/equalizerx/IasEqualizerCmd.hpp"
#include "smartx/IasAudioTypedefs.hpp"

namespace IasAudio
{

class IasAudioChain;
class IasIGenericAudioCompConfig;
class IasGenericAudioComp;
class IasAudioStream;
class IasPluginEngine;

struct SetCarEqualizerNumFiltersParams
{
  int32_t cmd = static_cast<int32_t>(IasEqualizer::IasEqualizerCmdIds::eIasCarModeSetEqualizerNumFilters);
  int32_t channelIdx;
  int32_t numFilters;

  SetCarEqualizerNumFiltersParams(
  int32_t _channelIdx,
  int32_t _numFilters) :
    channelIdx{_channelIdx},
    numFilters{_numFilters}
  {}
};


struct GetCarEqualizerNumFiltersParams
{
  int32_t cmd = static_cast<int32_t>(IasEqualizer::IasEqualizerCmdIds::eIasCarModeGetEqualizerNumFilters);
  int32_t channelIdx;

  GetCarEqualizerNumFiltersParams(
  int32_t _channelIdx) :
    channelIdx{_channelIdx}
  {}
};

struct SetCarEqualizerFilterParamsParams
{
  int32_t cmd = static_cast<int32_t>(IasEqualizer::IasEqualizerCmdIds::eIasCarModeSetEqualizerFilterParams);
  int32_t channelIdx;
  int32_t filterId;
  int32_t freq;
  int32_t gain;
  int32_t quality;
  int32_t type;
  int32_t order;

  SetCarEqualizerFilterParamsParams(
  int32_t _channelIdx,
  int32_t _filterId,
  int32_t _freq,
  int32_t _gain,
  int32_t _quality,
  int32_t _type,
  int32_t _order) :
   channelIdx{_channelIdx},
   filterId{_filterId},
   freq{_freq},
   gain{_gain},
   quality{_quality},
   type{_type},
   order{_order}
  {}
};


struct AudioFilterParamsData
{
  int32_t         freq;         //!< cut-off frequency or mid frequency of the filter
  int32_t         gain;         //!< gain [dB/10], required only for peak and shelving filters
  int32_t         quality;      //!< quality, required only for band-pass and peak filters
  int32_t         type;         //!< filter type; the type IasAudioFilterTypes is defined in IasProcessingTypes.hpp
  int32_t         order;        //!< filter order
  int32_t         section;      //!< section that shall be implemented, required only for higher-order filters (order>2)
};


struct GetCarEqualizerFilterParamsParams
{
  int32_t cmd = static_cast<int32_t>(IasEqualizer::IasEqualizerCmdIds::eIasCarModeGetEqualizerFilterParams);
  int32_t channelIdx;
  int32_t filterId;

  GetCarEqualizerFilterParamsParams(
  int32_t _channelIdx,
  int32_t _filterId) :
   channelIdx{_channelIdx},
   filterId{_filterId}
  {}
};


class IasEqualizerCarGTest : public ::testing::Test
{

protected:

  void SetUp() override;
  void TearDown() override;

  IasIModuleId::IasResult setCarEqualizerNumFilters(const char* pin,
      const SetCarEqualizerNumFiltersParams params, IasIModuleId* cmd);

  std::pair<IasIModuleId::IasResult, int32_t> getCarEqualizerNumFilters(const char* pin,
      GetCarEqualizerNumFiltersParams params, IasIModuleId* cmd);

  IasIModuleId::IasResult setCarEqualizerFilterParams(const char* pin,
      const SetCarEqualizerFilterParamsParams params, IasIModuleId* cmd);

  std::pair<IasIModuleId::IasResult, AudioFilterParamsData> getCarEqualizerFilterParams(const char* pin,
      GetCarEqualizerFilterParamsParams params, IasIModuleId* cmd);

  IasIModuleId::IasResult setFiltersSingleStream(const char* pin,
      const SetCarEqualizerFilterParamsParams params, IasIModuleId* cmd);



public:

  IasAudioChain               *mAudioChain    = nullptr;
  IasIGenericAudioCompConfig  *mEqualizerConfig = nullptr;
  IasGenericAudioComp         *mEqualizer     = nullptr;
  IasCmdDispatcherPtr          mCmdDispatcher = nullptr;
  IasPluginEngine             *mPluginEngine  = nullptr;

  IasAudioStream              *mInStream1     = nullptr;
  IasAudioStream              *mInStream2     = nullptr;
  IasAudioStream              *mOutStream1    = nullptr;
  IasAudioStream              *mOutStream2    = nullptr;
};

} // namespace IasAudio

#endif /* IASEQUALIZERCARTEST_HPP_ */
