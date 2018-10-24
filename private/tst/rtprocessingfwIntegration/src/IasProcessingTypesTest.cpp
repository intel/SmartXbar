/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasProcessingTypesTest.cpp
 * @date   July 20, 2016
 * @brief  Contains test cases for covering some toString functions.
 */

#include "audio/smartx/rtprocessingfwx/IasProcessingTypes.hpp"
#include "IasRtProcessingFwTest.hpp"

namespace IasAudio {

TEST_F(IasRtProcessingFwTest, filterTypeToStringTest)
{
  // Test of the toString() function for IasAudioFilterTypes
  std::cout << "Possible values for IasAudioFilterTypes are:" << std::endl;
  std::cout << "  " << toString(eIasFilterTypeFlat)           << std::endl;
  std::cout << "  " << toString(eIasFilterTypePeak)           << std::endl;
  std::cout << "  " << toString(eIasFilterTypeLowpass)        << std::endl;
  std::cout << "  " << toString(eIasFilterTypeHighpass)       << std::endl;
  std::cout << "  " << toString(eIasFilterTypeLowShelving)    << std::endl;
  std::cout << "  " << toString(eIasFilterTypeHighShelving)   << std::endl;
  std::cout << "  " << toString(eIasFilterTypeBandpass)       << std::endl;
}


} // namespace IasAudio
