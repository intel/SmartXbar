/*
 * * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasTestFrameworkTypes.hpp
 * @date   2017
 * @brief  Private type definitions for the test framework.
 */

#ifndef IASTESTFRAMEWORKTYPES_HPP_
#define IASTESTFRAMEWORKTYPES_HPP_

#include <memory>

#include "audio/testfwx/IasTestFrameworkPublicTypes.hpp"

/**
 * @brief Namespace for all audio related definitions and declarations
 */
namespace IasAudio {

struct IasTestFrameworkRoutingZoneParams
{
  IasTestFrameworkRoutingZoneParams()
    :name()
  {}
  IasTestFrameworkRoutingZoneParams(std::string p_name)
    :name(p_name)
  {}
  std::string name;       //!< The name of the test framework routing zone
};

/**
 * @brief Shared ptr type for IasTestFrameworkRoutingZoneParams
 */
using IasTestFrameworkRoutingZoneParamsPtr = std::shared_ptr<IasTestFrameworkRoutingZoneParams>;

class IasTestFrameworkRoutingZone;
/**
 * @brief Shared ptr type for IasTestFrameworkRoutingZone
 */
using IasTestFrameworkRoutingZonePtr = std::shared_ptr<IasTestFrameworkRoutingZone>;

class IasTestFrameworkConfiguration;
/**
 * @brief Shared ptr type for IasTestFrameworkConfiguration
 */
using IasTestFrameworkConfigurationPtr = std::shared_ptr<IasTestFrameworkConfiguration>;

class IasTestFrameworkWaveFile;
/**
 * @brief Shared ptr type for IasTestFrameworkWaveFile
 */
using IasTestFrameworkWaveFilePtr = std::shared_ptr<IasTestFrameworkWaveFile>;

} // Namespace IasAudio


#endif // IASTESTFRAMEWORKTYPES_HPP_
