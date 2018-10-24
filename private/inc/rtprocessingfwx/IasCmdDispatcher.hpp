/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasCmdDispatcher.hpp
 * @date   2016
 * @brief
 */

#ifndef IASCMDDISPATCHER_HPP_
#define IASCMDDISPATCHER_HPP_

#include <dlt/dlt.h>
#include "audio/smartx/rtprocessingfwx/IasICmdRegistration.hpp"

namespace IasAudio {

class IAS_AUDIO_PUBLIC IasCmdDispatcher : public IasICmdRegistration
{
  public:
    IasCmdDispatcher();
    virtual ~IasCmdDispatcher();

    virtual IasICmdRegistration::IasResult registerModuleIdInterface(int32_t, IasIModuleId *)
    {
      return IasICmdRegistration::eIasFailed;
    }
    virtual IasICmdRegistration::IasResult propertiesEventCallback(int32_t, IasProperties &)
    {
      return IasICmdRegistration::eIasFailed;
    }
    virtual IasICmdRegistration::IasResult registerModuleIdInterface(const std::string &instanceName, IasIModuleId *interface);
    virtual void unregisterModuleIdInterface(const std::string &instanceName);

    IasICmdRegistration::IasResult dispatchCmd(const std::string &instanceName, const IasProperties &cmdProperties, IasProperties &returnProperties);

  private:
    using IasCmdMap = std::map<std::string, IasIModuleId*>;

    IasCmdMap     mCmdMap;    //!< The map storing the interface for a given instanceName
    DltContext   *mLog;       //!< The DLT context
};

} /* namespace IasAudio */

#endif /* IASCMDDISPATCHER_HPP_ */
