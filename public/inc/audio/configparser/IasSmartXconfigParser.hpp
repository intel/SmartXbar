/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasSmartXconfigParser.hpp
 * @date   2017
 * @brief
 */

#ifndef IASSMARTXCONFIGPARSERFILE_HPP
#define IASSMARTXCONFIGPARSERFILE_HPP

#include <stdio.h>
#include <stdlib.h>
#include "audio/smartx/IasSmartX.hpp"

namespace IasAudio {

/**
 * @brief Parse SmartXbar XML configuration file
 *
 * @param[in] smartx Pointer to the smartx
 * @param[in] xmlFileName XML file name
 * @return success or fail
 */
IAS_AUDIO_PUBLIC bool parseConfig(IasAudio::IasSmartX *smartx, const char * xmlFileName);

}

#endif // IASSMARTXCONFIGPARSERFILE_HPP
