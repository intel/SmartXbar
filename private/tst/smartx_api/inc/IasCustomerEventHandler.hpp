/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasCustomerEventHandler.hpp
 * @date   2015
 * @brief
 */

#ifndef IASCUSTOMEREVENTHANDLER_HPP
#define IASCUSTOMEREVENTHANDLER_HPP


#include "audio/common/IasAudioCommonTypes.hpp"
#include "audio/smartx/IasEventHandler.hpp"
#include "audio/smartx/IasConnectionEvent.hpp"

namespace IasAudio {

/**
 * @brief
 *
 */
class IAS_AUDIO_PUBLIC IasCustomerEventHandler : public IasEventHandler
{
public:
    /**
     * @brief Constructor.
     */
    IasCustomerEventHandler();
    /**
     * @brief Destructor.
     */
    virtual ~IasCustomerEventHandler();

    virtual void receivedConnectionEvent(IasConnectionEvent *event);

private:
    /**
     * @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasCustomerEventHandler(IasCustomerEventHandler const &other);
    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasCustomerEventHandler& operator=(IasCustomerEventHandler const &other);

    uint32_t   mCnt;
    IasConnectionEvent::IasEventType   mExpected[2];
};

} //namespace IasAudio

#endif // IASCUSTOMEREVENTHANDLER_HPP
