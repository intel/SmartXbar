/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file IasVolumeHelper.cpp
 * @date 2016
 * @brief
 */

#include <sstream>
#include <array>
#include "volume/IasVolumeHelper.hpp"
#include "volume/IasVolumeLoudnessCore.hpp"
#include "audio/common/IasAudioCommonTypes.hpp"
#include "audio/smartx/IasProperties.hpp"


namespace IasAudio {

namespace IasVolumeHelper {

static const std::array<int32_t, cIasAudioLoudnessTableDefaultLength> cDefaultGains = {{0,  10,    30,   90,  120,  180,  180,  180,  180,  180}};
static const std::array<int32_t, cIasAudioLoudnessTableDefaultLength> cDefaultVolumes = {{0, -100, -200, -300, -400, -500, -600, -700, -800, -900}};

bool checkLoudnessFilterParams(IasAudioFilterConfigParams &params)
{

  if( params.freq <  cMinLoudnessFilterFreq || params.freq > cMaxLoudnessFilterFreq)
  {
    return true;
  }
  if( params.order <  cMinLoudnessFilterOrder || params.order > cMaxLoudnessFilterOrder)
  {
    return true;
  }
  if( params.quality < cMinLoudnessFilterQual || params.quality > cMaxLoudnessFilterQual)
  {
    return true;
  }
  if( params.gain != 1.0f)
  {
    return true;
  }
  auto it = std::find(cLoudnessFilterAllowedTypes.begin(), cLoudnessFilterAllowedTypes.end(), params.type);
  if (it == cLoudnessFilterAllowedTypes.end())
  {
    return true;
  }
  return false;
}

void getDefaultLoudnessTable(IasVolumeLoudnessTable &table)
{
  // Otherwise provide a default table
  for(uint32_t i=0; i< cDefaultGains.size(); i++)
  {
    table.gains.push_back(cDefaultGains[i]);
    table.volumes.push_back(cDefaultVolumes[i]);
  }
}

bool getLoudnessTableFromProperties(const IasProperties &properties, uint32_t band, IasVolumeLoudnessTable &table)
{
  table.gains.clear();
  table.volumes.clear();
  IasInt32Vector ldGains;
  IasInt32Vector ldVolumes;
  std::string gainPrefix = "ld.gains.";
  std::string volumePrefix = "ld.volumes.";
  std::stringstream gain;
  std::stringstream volume;
  gain << gainPrefix << std::to_string(band);
  volume << volumePrefix << std::to_string(band);
  // We don't check the return values because it's enough to check the size of the vectors.
  properties.get(gain.str(), &ldGains);
  properties.get(volume.str(), &ldVolumes);
  if (ldGains.size() > 0 && ldGains.size() == ldVolumes.size())
  {
    // If we have loudness gains and loudness volumes then we can use them for initialization
    for (uint32_t i = 0; i < ldGains.size(); i++)
    {
      table.gains.push_back(ldGains[i]);
      table.volumes.push_back(ldVolumes[i]);
    }
    return true;
  }
  else
  {
    return false;
  }
}

void setLoudnessTableInProperties(uint32_t band, const IasVolumeLoudnessTable &table, IasProperties &properties)
{
  IasInt32Vector ldGains;
  IasInt32Vector ldVolumes;
  std::string gainPrefix = "ld.gains.";
  std::string volumePrefix = "ld.volumes.";
  std::stringstream gain;
  std::stringstream volume;
  gain << gainPrefix << std::to_string(band);
  volume << volumePrefix << std::to_string(band);
  IAS_ASSERT(table.gains.size() == table.volumes.size());
  for (uint32_t i = 0; i < table.gains.size(); i++)
  {
    ldGains.push_back(table.gains[i]);
    ldVolumes.push_back(table.volumes[i]);
  }
  properties.set(gain.str(), ldGains);
  properties.set(volume.str(), ldVolumes);
}

void getDefaultLoudnessFilterParams(uint32_t band, uint32_t numFilterBands, IasAudioFilterConfigParams &params)
{
  IasAudioFilterConfigParams filterParamsLow;
  filterParamsLow.freq     = 80;
  filterParamsLow.gain     = 1.0f;
  filterParamsLow.order    = 2;
  filterParamsLow.quality  = 2.0f;
  filterParamsLow.type     = eIasFilterTypePeak;

  IasAudioFilterConfigParams filterParamsMid;
  filterParamsMid.freq     = 2000;
  filterParamsMid.gain     = 1.0f;
  filterParamsMid.order    = 2;
  filterParamsMid.quality  = 2.0f;
  filterParamsMid.type     = eIasFilterTypePeak;

  IasAudioFilterConfigParams filterParamsHigh;
  filterParamsHigh.freq    = 8000;
  filterParamsHigh.gain    = 1.0f;
  filterParamsHigh.order   = 2;
  filterParamsHigh.quality = 2.0f;
  filterParamsHigh.type    = eIasFilterTypePeak;

  if (band == 0)
  {
    params = filterParamsLow;
  }
  else if (band == 1)
  {
    if (numFilterBands == 3)
    {
      params = filterParamsMid;
    }
    else
    {
      params = filterParamsHigh;
    }
  }
  else
  {
    params = filterParamsHigh;
  }
}

bool getLoudnessFilterParamsFromProperties(const IasProperties &properties, uint32_t band, IasAudioFilterConfigParams &params)
{
  IasInt32Vector freqOrderType;
  IasFloat32Vector gainQuality;
  std::string fotPrefix = "ld.freq_order_type.";
  std::string gqPrefix = "ld.gain_quality.";
  std::stringstream fot;
  std::stringstream gq;
  fot << fotPrefix << std::to_string(band);
  gq << gqPrefix << std::to_string(band);
  properties.get(fot.str(), &freqOrderType);
  properties.get(gq.str(), &gainQuality);
  if (freqOrderType.size() == 3 && gainQuality.size() == 2)
  {
    params.freq = freqOrderType[0];
    params.order = freqOrderType[1];
    params.type = static_cast<IasAudioFilterTypes>(freqOrderType[2]);
    params.gain = gainQuality[0];
    params.quality = gainQuality[1];
    return true;
  }
  else
  {
    return false;
  }
}

void setLoudnessFilterParamsInProperties(uint32_t band, const IasAudioFilterConfigParams &params, IasProperties &properties)
{
  IasInt32Vector freqOrderType;
  IasFloat32Vector gainQuality;
  std::string fotPrefix = "ld.freq_order_type.";
  std::string gqPrefix = "ld.gain_quality.";
  std::stringstream fot;
  std::stringstream gq;
  fot << fotPrefix << std::to_string(band);
  gq << gqPrefix << std::to_string(band);
  freqOrderType.resize(3);
  gainQuality.resize(2);
  IAS_ASSERT(freqOrderType.size() == 3 && gainQuality.size() == 2);
  freqOrderType[0] = params.freq;
  freqOrderType[1] = params.order;
  freqOrderType[2] = static_cast<int32_t>(params.type);
  gainQuality[0] = params.gain;
  gainQuality[1] = params.quality;
  properties.set(fot.str(), freqOrderType);
  properties.set(gq.str(), gainQuality);
}

void getDefaultSDVTable(IasVolumeSDVTable &sdvTable)
{
  // Set default table
  uint32_t tempSpeedStart = 30;
  float gainIncDiff = 0.5f;
  float gainDecDiff = 0.6f;

  for (uint32_t i = 0; i < cIasAudioMaxSDVTableLength; i++)
  {
    sdvTable.speed.push_back(tempSpeedStart);
    tempSpeedStart += 5;
    if (i != 0)
    {
      sdvTable.gain_inc.push_back((uint32_t)((float)sdvTable.speed[i] * gainIncDiff));
      sdvTable.gain_dec.push_back((uint32_t)((float)sdvTable.speed[i] * gainDecDiff));
    }
    else
    {
      sdvTable.gain_inc.push_back(0);
      sdvTable.gain_dec.push_back(0);
    }
  }
}

bool getSDVTableFromProperties(const IasProperties &properties, IasVolumeSDVTable &sdvTable)
{
  IasInt32Vector speed;
  IasInt32Vector gain_inc;
  IasInt32Vector gain_dec;
  properties.get("sdv.speed", &speed);
  properties.get("sdv.gain_inc", &gain_inc);
  properties.get("sdv.gain_dec", &gain_dec);

  if (speed.size() > 0 && speed.size() == gain_inc.size() && speed.size() == gain_dec.size())
  {
    for (uint32_t i = 0; i < speed.size(); i++)
    {
      sdvTable.speed.push_back(speed[i]);
      sdvTable.gain_inc.push_back(gain_inc[i]);
      sdvTable.gain_dec.push_back(gain_dec[i]);
    }
    return true;
  }
  else
  {
    return false;
  }
}

void setSDVTableInProperties(const IasVolumeSDVTable &sdvTable, IasProperties &properties)
{
  IasInt32Vector speed;
  IasInt32Vector gain_inc;
  IasInt32Vector gain_dec;

  IAS_ASSERT(   sdvTable.speed.size() > 0
             && sdvTable.speed.size() == sdvTable.gain_inc.size()
             && sdvTable.speed.size() == sdvTable.gain_dec.size());
  for (uint32_t i = 0; i < sdvTable.speed.size(); i++)
  {
    speed.push_back(sdvTable.speed[i]);
    gain_inc.push_back(sdvTable.gain_inc[i]);
    gain_dec.push_back(sdvTable.gain_dec[i]);
  }
  properties.set("sdv.speed", speed);
  properties.set("sdv.gain_inc", gain_inc);
  properties.set("sdv.gain_dec", gain_dec);
}


} // namespace IasVolumeHelper

} // namespace IasAudio
