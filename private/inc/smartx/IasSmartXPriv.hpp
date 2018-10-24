/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasSmartXPriv.hpp
 * @date   2015
 * @brief
 */

#ifndef IASSMARTXPRIV_HPP_
#define IASSMARTXPRIV_HPP_


#include "audio/common/IasAudioCommonTypes.hpp"
#include "internal/audio/common/IasAudioLogging.hpp"
#include "audio/smartx/IasSmartX.hpp"
#include "IasAudioTypedefs.hpp"

namespace IasAudio {

class IasConfiguration;
class IasISetup;

/**
 * @brief The SmartXBar private implementation class
 *
 */
class IasSmartXPriv
{
  public:
    enum IasResult
    {
      eIasOk                = IasSmartX::eIasOk,                //!< Operation successful
      eIasFailed            = IasSmartX::eIasFailed,            //!< Operation failed
      eIasOutOfMemory       = IasSmartX::eIasOutOfMemory,       //!< Out of memory during memory allocation
      eIasNullPointer       = IasSmartX::eIasNullPointer,       //!< One of the parameters was a nullptr
      eIasTimeout           = IasSmartX::eIasTimeout,           //!< Timeout occured
      eIasNoEventAvailable  = IasSmartX::eIasNoEventAvailable,  //!< No event available in the event queue
    };

    /**
     * @brief Constructor.
     */
    IasSmartXPriv();

    /**
     * @brief Destructor.
     *
     * Class is not intended to be subclassed.
     */
    ~IasSmartXPriv();

    IasResult init();

    /**
     * @brief Start data processing thread(s) of the SmartXbar
     *
     * @return The status of the start call
     * @retval IasSmartX::eIasOk SmartXbar data processing successfully started
     * @retval IasSmartX::eIasFailed SmartXbar data processing couldn't be started
     */
    IasResult start();

    /**
     * @brief Stop data processing thread(s) of the SmartXbar
     *
     * @return The status of the stop call
     * @retval IasSmartX::eIasOk SmartXbar data processing successfully stopped
     * @retval IasSmartX::eIasFailed SmartXbar data processing couldn't be stopped
     */
    IasResult stop();

    /**
     * @brief Get a pointer to the IasISetup interface
     *
     * The setup interface is used to setup the static and dynamic configuration of the
     * audio user space environment. See IasISetup for more details.
     * @return A pointer to the IasISetup interface
     */
    IasISetup* setup();

    /**
     * @brief Get a pointer to the IasIRouting interface
     *
     * The routing interface is used to establish and remove connections between source output
     * ports and sink or routing zone input ports. See IasIRouting for more details.
     * @return A pointer to the IasIRouting interface
     */
    IasIRouting* routing();

    /**
     * @brief Get a pointer to the IasIProcessing interface
     *
     * The processing interface is used to control the audio processing modules. See IasIProcessing for
     * for more details.
     * @return A pointer to the IasIProcessing interface
     */
    IasIProcessing* processing();

    /**
     * @brief Get a pointer to the IasIDebug interface
     *
     * The debug interface is used for different debug purposes.
     * See IasIDebug for more details.
     * @return A pointer to the IasIDebug interface
     */
    IasIDebug* debug();

    /**
     * @brief Wait for an asynchronous event from the SmartXbar
     *
     * This is a blocking call that waits for an event to occur. After an event occured the method IasSmartXPriv::getNextEvent has to
     * be called to receive the next event from the event queue.
     * @param[in] timeout The timeout value in msec after which the call will return in any case. If 0 is given call will block forever
     * @return The result of the method call
     * @retval IasSmartXPriv::eIasOk Event occured. Receive it via IasSmartXPriv::getNextEvent
     * @retval IasSmartXPriv::eIasTimeout Timeout occured while waiting for event
     */
    IasResult waitForEvent(uint32_t timeout) const;

    /**
     * @brief Get next event from the event queue
     *
     * This can either be used to poll the event queue or it can be used after a call to the method
     * IasSmartXPriv::waitForEvent in a separate thread to be unblocked only when a new event is available. A call of this
     * method removes the pending event from the event queue
     * @param[in,out] event A pointer where a pointer to the event will be stored
     * @return The result of the method call
     * @retval IasSmartXPriv::eIasOk Event stored in location where the parameter points to
     * @retval IasSmartXPriv::eIasNoEventAvailable No event in event queue available
     * @retval IasSmartXPriv::eIasNullPointer Parameter event is a nullptr
     */
    IasResult getNextEvent(IasEventPtr *event);

  private:
    /**
     * @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasSmartXPriv(IasSmartXPriv const &other);

    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasSmartXPriv& operator=(IasSmartXPriv const &other);

    // Member variables
    DltContext           *mLog;
    IasConfigurationPtr   mConfig;
    IasISetup            *mSetup;
    IasIRouting          *mRouting;
    IasIProcessing       *mProcessing;
    IasIDebug            *mDebug;
    IasCmdDispatcherPtr   mCmdDispatcher;
};

/**
 * @brief Function to get a IasSmartXPriv::IasResult as string.
 *
 * @return Enum Member
 */
std::string toString(const IasSmartXPriv::IasResult& type);


} //namespace IasAudio

#endif /* IASSMARTXPRIV_HPP_ */
