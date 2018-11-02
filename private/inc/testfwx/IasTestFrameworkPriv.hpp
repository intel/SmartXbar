/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file   IasTestFrameworkPriv.hpp
 * @date   2017
 * @brief  The IasTestFramework private implementation class.
 */

#ifndef AUDIO_DAEMON2_PRIVATE_INC_TESTFWX_IASTESTFRAMEWORKPRIV_HPP_
#define AUDIO_DAEMON2_PRIVATE_INC_TESTFWX_IASTESTFRAMEWORKPRIV_HPP_


#include "avbaudiomodules/audio/common/IasAudioCommonTypes.hpp"
#include "avbaudiomodules/internal/audio/common/IasAudioLogging.hpp"
#include "testfwx/IasTestFrameworkTypes.hpp"
#include "audio/testfwx/IasTestFramework.hpp"
#include "smartx/IasAudioTypedefs.hpp"


namespace IasAudio {

class IasTestFrameworkSetup;

/**
 * @brief The IasTestFramework private implementation class
 */
class IasTestFrameworkPriv
{
  public:

    /**
     * @brief The result type for the IasTestFrameworkPriv methods
     */
    enum IasResult
    {
      eIasOk                = IasTestFramework::eIasOk,                //!< Operation successful
      eIasFinished          = IasTestFramework::eIasFinished,          //!< Processing finished successfully
      eIasFailed            = IasTestFramework::eIasFailed,            //!< Operation failed
      eIasOutOfMemory       = IasTestFramework::eIasOutOfMemory,       //!< Out of memory during memory allocation
      eIasNullPointer       = IasTestFramework::eIasNullPointer,       //!< One of the parameters was a nullptr
      eIasTimeout           = IasTestFramework::eIasTimeout,           //!< Timeout occured
      eIasNoEventAvailable  = IasTestFramework::eIasNoEventAvailable   //!< No event available in the event queue
    };

    /**
     *  @brief Constructor.
     */
    IasTestFrameworkPriv();

    /**
     *  @brief Destructor.
     */
    ~IasTestFrameworkPriv();

    /**
     *  @brief Initialization.
     *
     *  @param[in] pipelineParams The configuration parameters for the pipeline
     *
     */
    IasResult init(const IasPipelineParams &pipelineParams);

    /**
     * @brief Start data processing thread of the IasTestFramework
     *
     * @return The status of the start call
     * @retval IasTestFramework::eIasOk IasTestFramework data processing successfully started
     * @retval IasTestFramework::eIasFailed IasTestFramework data processing couldn't be started
     */
    IasResult start();

    /**
     * @brief Stop data processing thread of the IasTestFramework
     *
     * @return The status of the stop call
     * @retval IasTestFramework::eIasOk IasTestFramework data processing successfully stopped
     * @retval IasTestFramework::eIasFailed IasTestFramework data processing couldn't be stopped
     */
    IasResult stop();

    /**
     * @brief Process a number of periods of wave file
     *
     * This method executes processing of given number of periods of wave file.
     * First function call starts processing from the beginning of file. Next calls
     * take periods from the position in the file where previous call has finished.
     *
     * @param[in] numPeriods Number of periods to be processed
     * @return The status of the process call
     * @retval eIasOk Number of periods processed successfully
     * @retval eIasFailed Processing failed
     * @retval eIasEndOfFile End of file reached, assuming operation successful
     */
    IasResult process(uint32_t numPeriods);

    /**
     * @brief Get a pointer to the IasTestFrameworkSetup interface
     *
     * The setup interface is used to setup the static and dynamic configuration of the
     * audio user space environment. See IasTestFrameworkSetup for more details.
     * @return A pointer to the IasTestFrameworkSetup interface
     */
    IasTestFrameworkSetup* setup();

     /**
      * @brief Get a pointer to the IasIProcessing interface
      *
      * The processing interface is used to control the audio processing modules. See IasIProcessing
      * for more details.
      * @return A pointer to the IasIProcessing interface
      */
    IasIProcessing* processing();

    /**
     * @brief Get a pointer to the IasIDebug interface
     *
     * The debug interface is used to for probing. See IasIDebug
     * for more details.
     * @return A pointer to the IasIDebug interface
     */
    IasIDebug* debug();

    /**
     * @brief Wait for an asynchronous event from the IasTestFramework
     *
     * This is a blocking call that waits for an event to occur. After an event occured the method IasTestFramework::getNextEvent has to
     * be called to receive the next event from the event queue.
     * @param[in] timeout The timeout value in msec after which the call will return in any case. If 0 is given call will block forever
     * @return The result of the method call
     * @retval IasTestFramework::eIasOk Event occured. Receive it via IasTestFramework::getNextEvent
     * @retval IasTestFramework::eIasTimeout Timeout occured while waiting for event
     */
    IasResult waitForEvent(uint32_t timeout) const;

    /**
     * @brief Get next event from the event queue
     *
     * This can either be used to poll the event queue or it can be used after a call to the method
     * IasTestFramework::waitForEvent in a separate thread to be unblocked only when a new event is available. A call of this
     * method removes the pending event from the event queue
     * @param[in,out] event A pointer where a pointer to the event will be stored
     * @return The result of the method call
     * @retval IasTestFramework::eIasOk Event stored in location where the parameter points to
     * @retval IasTestFramework::eIasNoEventAvailable No event in event queue available
     * @retval IasTestFramework::eIasNullPointer Parameter event is a nullptr
     */
    IasResult getNextEvent(IasEventPtr *event);

  private:
    /*!
     * @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasTestFrameworkPriv(IasTestFrameworkPriv const &other);

    /*!
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasTestFrameworkPriv& operator=(IasTestFrameworkPriv const &other);

    /*!
     * @brief Create test framework routing zone

     * @return The result of the method call
     * @retval IasTestFramework::eIasOk Test framework routing zone created successfully
     * @retval IasTestFramework::eIasFailed Failed to create test framework routing zone
     */
    IasResult createTestFrameworkRoutingZone();

    /*!
     *  @brief Create pipeline
     *
     *  @param[in] pipelineParams The configuration parameters for the pipeline
     *
     * @return The result of the method call
     * @retval IasTestFramework::eIasOk Pipeline created successfully
     * @retval IasTestFramework::eIasFailed Failed to create pipeline
     */
    IasResult createPipeline(const IasPipelineParams &pipelineParams);

    // Member variables
    DltContext                          *mLog;
    IasTestFrameworkConfigurationPtr    mConfig;
    IasTestFrameworkSetup               *mSetup;
    IasIProcessing                      *mProcessing;
    IasIDebug                           *mDebug;
    IasCmdDispatcherPtr                 mCmdDispatcher;
};

} // namespace IasAudio

#endif // AUDIO_DAEMON2_PRIVATE_INC_TESTFWX_IASTESTFRAMEWORKPRIV_HPP_
