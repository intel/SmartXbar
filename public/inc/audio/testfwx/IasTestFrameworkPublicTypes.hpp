/*
 * * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasTestFrameworkPublicTypes.hpp
 * @date   2017
 * @brief  Public types for test framework
 */

#ifndef IASTESTFRAMEWORKPUBLICTYPES_HPP_
#define IASTESTFRAMEWORKPUBLICTYPES_HPP_

#include <memory>


/**
 * @brief Namespace for all audio related definitions and declarations
 */
namespace IasAudio {

struct IasTestFrameworkWaveFileParams
{
  IasTestFrameworkWaveFileParams()
    :fileName("")
  {}

  IasTestFrameworkWaveFileParams(std::string p_fileName)
    :fileName(p_fileName)
  {}

  std::string fileName;     //!< File name
};

/**
 * @brief Shared ptr type for IasTestFrameworkWaveFileParams
 */
using IasTestFrameworkWaveFileParamsPtr = std::shared_ptr<IasTestFrameworkWaveFileParams>;

} // Namespace IasAudio


#endif // IASTESTFRAMEWORKPUBLICTYPES_HPP_
