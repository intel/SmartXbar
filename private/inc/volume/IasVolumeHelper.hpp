/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file IasVolumeHelper.hpp
 * @date 2016
 * @brief
 */

#ifndef IASVOLUMEHELPER_HPP_
#define IASVOLUMEHELPER_HPP_

#include "avbaudiomodules/audio/common/IasAudioCommonTypes.hpp"

namespace IasAudio {

class IasProperties;
struct IasVolumeLoudnessTable;
struct IasAudioFilterConfigParams;
struct IasVolumeSDVTable;

namespace IasVolumeHelper {

IAS_AUDIO_PUBLIC bool getLoudnessTableFromProperties(const IasProperties &properties, uint32_t band, IasVolumeLoudnessTable &table);
IAS_AUDIO_PUBLIC void getDefaultLoudnessTable(IasVolumeLoudnessTable &table);
IAS_AUDIO_PUBLIC void setLoudnessTableInProperties(uint32_t band, const IasVolumeLoudnessTable &table, IasProperties &properties);
IAS_AUDIO_PUBLIC bool getLoudnessFilterParamsFromProperties(const IasProperties &properties, uint32_t band, IasAudioFilterConfigParams &params);
IAS_AUDIO_PUBLIC void getDefaultLoudnessFilterParams(uint32_t band, uint32_t numFilterBands, IasAudioFilterConfigParams &params);
IAS_AUDIO_PUBLIC void setLoudnessFilterParamsInProperties(uint32_t band, const IasAudioFilterConfigParams &params, IasProperties &properties);
IAS_AUDIO_PUBLIC bool getSDVTableFromProperties(const IasProperties &properties, IasVolumeSDVTable &sdvTable);
IAS_AUDIO_PUBLIC void getDefaultSDVTable(IasVolumeSDVTable &sdvTable);
IAS_AUDIO_PUBLIC void setSDVTableInProperties(const IasVolumeSDVTable &sdvTable, IasProperties &properties);
IAS_AUDIO_PUBLIC bool checkLoudnessFilterParams(IasAudioFilterConfigParams &params);
} // namespace IasVolumeHelper

} // namespace IasAudio

#endif /* IASVOLUMEHELPER_HPP_ */
