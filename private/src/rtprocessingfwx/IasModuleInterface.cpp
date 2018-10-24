/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file
 */

#include "audio/smartx/rtprocessingfwx/IasModuleInterface.hpp"

namespace IasAudio
{

IasModuleInfo::IasModuleInfo(std::string const &name)
  :mModuleTypeName(name)
{

}

IasModuleInterface::IasModuleInterface()
{
}

IasModuleInterface::~IasModuleInterface()
{
}

} // namespace IasAudio
