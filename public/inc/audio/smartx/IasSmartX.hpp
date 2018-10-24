/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasSmartX.hpp
 * @date   2015
 * @brief
 */

#ifndef IASSMARTX_HPP_
#define IASSMARTX_HPP_

#include <atomic>

#include "audio/common/IasAudioCommonTypes.hpp"

namespace IasAudio {

class IasIRouting;
class IasIProcessing;
class IasISetup;
class IasIDebug;
class IasEvent;
class IasSmartXPriv;

#define SMARTX_API_MAJOR       2
#define SMARTX_API_MINOR       2
#define SMARTX_API_PATCH       0

/**
 * @brief The main SmartXbar class
 *
 * This class has to be instantiated by calling the method IasSmartX::create() to instantiate the SmartXbar
 */
class IAS_AUDIO_PUBLIC IasSmartX
{
  public:
    /**
     * @brief The result type for the IasSmartX methods
     */
    enum IasResult
    {
      eIasOk,               //!< Operation successful
      eIasFailed,           //!< Operation failed
      eIasOutOfMemory,      //!< Out of memory during memory allocation
      eIasNullPointer,      //!< One of the parameters was a nullptr
      eIasTimeout,          //!< Timeout occured
      eIasNoEventAvailable, //!< No event available in the event queue
    };

    /**
     * @brief Get the major version number of the SmartX API implementation
     *
     * @return The major version number
     */
    static int32_t getMajor();
    /**
     * @brief Get the minor version number of the SmartX API implementation
     *
     * @return The minor version number
     */
    static int32_t getMinor();
    /**
     * @brief Get the patch version number of the SmartX API implementation
     *
     * @return The patch version number
     */
    static int32_t getPatch();
    /**
     * @brief Get the version number as human readable string
     *
     * @return The version number as human readable string
     */
    static std::string getVersion();
    /**
     * @brief Check if the implemented version number is at least the given one
     *
     * @param[in] major The major version number
     * @param[in] minor The minor version number
     * @param[in] patch The patch version number
     * @retval true The version number of the SmartX API implementation is at least the given one
     * @retval false The version number of the SmartX API implementation is below the given one
     */
    static bool isAtLeast(int32_t major, int32_t minor, int32_t patch);
    /**
     * @brief Check if the SmartX API implementation has a specific named feature
     *
     * @param[in] name The name of the feature to check for
     * @retval true The feature is available
     * @retval false The feature is not available
     */
    static bool hasFeature(const std::string &name);

    /**
     * @brief Create an instance of the SmartXbar.
     *
     * It is only possible to create one instance.
     *
     * @return A pointer to the SmartXbar instance or nullptr in case of an
     * error (e.g., if the application tries to create more than one single instance or if the SmartXbar cannot be initialized).
     */
    static IasSmartX* create();

    /**
     * @brief Destroy the previously created SmartXbar instance.
     *
     * @param[in] instance The pointer to the SmartXbar instance to destroy
     */
    static void destroy(IasSmartX *instance);

    /**
     * @brief Start data processing thread(s) of the SmartXbar.
     *
     * This method starts all currently created audio source devices and routing zones with
     * their linked audio sink devices. As there is no control over what is actually started,
     * and no response about what couldn't be started, it is recommended to use the dedicated methods
     * IasISetup::startAudioSourceDevice and IasISetup::startRoutingZone.
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
     * The debug interface is used for dfferent debug purposes.
     * See IasIDebug for more details.
     * @return A pointer to the IasIDebug interface
     */
    IasIDebug* debug();

    /**
     * @brief Wait for an asynchronous event from the SmartXbar
     *
     * This is a blocking call that waits for an event to occur. After an event occured the method IasSmartX::getNextEvent has to
     * be called to receive the next event from the event queue.
     * @param[in] timeout The timeout value in msec after which the call will return in any case. If 0 is given call will block forever
     * @return The result of the method call
     * @retval IasSmartX::eIasOk Event occured. Receive it via getNextEvent
     * @retval IasSmartX::eIasTimeout Timeout occured while waiting for event
     */
    IasResult waitForEvent(uint32_t timeout) const;

    /**
     * @brief Get next event from the event queue
     *
     * This can either be used to poll the event queue or it can be used after a call to the method
     * IasSmartX::waitForEvent in a separate thread to be unblocked only when a new event is available. A call of this
     * method removes the pending event from the event queue
     * @param[in,out] event A pointer where a pointer to the event will be stored
     * @return The result of the method call
     * @retval IasSmartX::eIasOk Event stored in location where the parameter points to
     * @retval IasSmartX::eIasNoEventAvailable No event in event queue available
     * @retval IasSmartX::eIasNullPointer Parameter event is a nullptr
     */
    IasResult getNextEvent(IasEventPtr *event);

  private:
    /**
     * @brief Constructor. Private to force usage of factory method.
     */
    IasSmartX();

    /**
     * @brief Destructor.
     *
     * Class is not intended to be subclassed. Private to force usage of factory method.
     */
    ~IasSmartX();

    /**
     * @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasSmartX(IasSmartX const &other);

    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasSmartX& operator=(IasSmartX const &other);

    /**
     * @brief Initialize the SmartXbar
     * @return The status of the init call
     * @retval IasSmartX::eIasOk Initialization sucessful
     * @retval IasSmartX::eIasOutOfMemory Out of memory during Initialization
     */
    IasResult init();

    // Member variables
    static std::atomic_int          mNumberInstances;   //!< Count the maximum allowed instances of the SmartX

    /**
     * Private implementation class
     */
    IasSmartXPriv              *mPriv;              //!< Pointer to private implementation details
};

/**
 * @brief Function to get a IasSmartX::IasResult as string.
 * @return Enum Member as string
 */
std::string toString(const IasSmartX::IasResult& type);

} //namespace IasAudio

#endif /* IASSMARTX_HPP_ */
