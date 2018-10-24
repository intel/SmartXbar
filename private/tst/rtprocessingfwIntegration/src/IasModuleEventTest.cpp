/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasModuleEventTest.cpp
 * @date   June 29, 2016
 * @brief  Contains some test cases for the IasModuleEvent class, which are not trivial to handle via an actual event handler.
 */

#include "audio/smartx/rtprocessingfwx/IasProcessingTypes.hpp"
#include "audio/smartx/rtprocessingfwx/IasModuleEvent.hpp"
#include "IasRtProcessingFwTest.hpp"

namespace IasAudio {

TEST_F(IasRtProcessingFwTest, ModuleEventTest)
{
  const IasModuleEvent* moduleEvent = new IasModuleEvent();

  const IasProperties& properties = moduleEvent->getProperties();
  (void)properties;

  delete moduleEvent;
}


}
