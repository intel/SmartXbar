/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file   IasTestFramework.hpp
 * @date   2017
 * @brief  The test framework of the SmartXbar for verifying custom processing modules.
 */

#ifndef AUDIO_DAEMON2_PUBLIC_INC_AUDIO_SMARTX_TESTFWX_IASTESTFRAMEWORK_HPP_
#define AUDIO_DAEMON2_PUBLIC_INC_AUDIO_SMARTX_TESTFWX_IASTESTFRAMEWORK_HPP_

#include <atomic>

#include "audio/common/IasAudioCommonTypes.hpp"

namespace IasAudio {

class IasTestFrameworkSetup;
class IasTestFrameworkPriv;
class IasIProcessing;
class IasIDebug;
class IasEvent;

/*!
 * @brief Documentation for class IasTestFramework
 */
class IAS_AUDIO_PUBLIC IasTestFramework
{
  public:
    /**
     * @brief The result type for the IasTestFramework methods
     */
    enum IasResult
    {
      eIasOk,               //!< Operation successful
      eIasFinished,          //!< Processing finished successfully
      eIasFailed,           //!< Operation failed
      eIasOutOfMemory,      //!< Out of memory during memory allocation
      eIasNullPointer,      //!< One of the parameters was a nullptr
      eIasTimeout,          //!< Timeout occured
      eIasNoEventAvailable  //!< No event available in the event queue
    };

    /**
     * @brief Create an instance of the IasTestFramework.
     *
     * It is only possible to create one instance of test framework.
     *
     * @param[in] pipelineParams The configuration parameters for the pipeline
     *
     * @return A pointer to the IasTestFramework instance or nullptr in case of an
     * error (e.g., if the application tries to create more than one single instance or if the IasTestFramework cannot be initialized).
     */
    static IasTestFramework* create(const IasPipelineParams &pipelineParams);

    /**
     * @brief Destroy the previously created IasTestFramework instance.
     *
     * @param[in] instance The pointer to the IasTestFramework instance to destroy
     */
    static void destroy(IasTestFramework *instance);

    /**
     * @brief Start data processing.
     *
     * After the topology of the pipeline and of the test framework has been defined, i.e., after
     *
     * @li all modules have been created and initialized,
     * @li all audio pins have been created and linked with each other, and
     * @li all pipeline input and output pins have been linked with their wave files,
     *
     * this method has to be called, before the pipeline and the test framework can be processed.
     * Equivalent to the IasISetup::initPipelineAudioChain method, this function analyzes
     * the signal dependencies and identifies the scheduling order of the processing modules.
     * Furthermore, the function identifies how many audio streams have to be created and how the
     * audio streams are mapped to the audio pins. Finally, the audio chain is set up, i.e., all
     * processing modules and and audio streams are added to the audio chain. In addition,
     * the previously added wave files are opened for reading and writing.
     *
     * After this method has been called, the test framework is ready for processing.
     * This means that now the method IasTestFramework::process can be called to process
     * one or more periods of PCM frames.
     *
     * @return The status of the start call
     * @retval eIasOk data processing successfully started
     * @retval eIasFailed data processing couldn't be started
     */
    IasResult start();

    /**
     * @brief Stop data processing.
     *
     * This method stops currently created test framework routing zone and closes
     * previously open wave files.
     *
     * @return The status of the stop call
     * @retval eIasOk Data processing successfully stopped
     * @retval eIasFailed Data processing couldn't be stopped
     */
    IasResult stop();

    /**
     * @brief Process number of periods of wave file.
     *
     * This method executes processing of given number of periods of wave files.
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
     * @brief Wait for an asynchronous event from test framework
     *
     * This is a blocking call that waits for an event to occur. After an event occured the method getNextEvent has to
     * be called to receive the next event from the event queue.
     * @param[in] timeout The timeout value in msec after which the call will return in any case. If 0 is given call will block forever
     * @return The result of the method call
     * @retval eIasOk Event occured. Receive it via getNextEvent
     * @retval eIasTimeout Timeout occured while waiting for event
     */
    IasResult waitForEvent(uint32_t timeout) const;

    /**
     * @brief Get next event from the event queue
     *
     * This can either be used to poll the event queue or it can be used after a call to the method
     * waitForEvent in a separate thread to be unblocked only when a new event is available. A call of this
     * method removes the pending event from the event queue
     * @param[in,out] event A pointer where a pointer to the event will be stored
     * @return The result of the method call
     * @retval eIasOk Event stored in location where the parameter points to
     * @retval eIasNoEventAvailable No event in event queue available
     * @retval eIasNullPointer Parameter event is a nullptr
     */
    IasResult getNextEvent(IasEventPtr *event);

  private:
    /*!
     *  @brief Constructor. Private to force usage of factory method.
     */
    IasTestFramework();

    /**
       * @brief Destructor.
       *
       * Class is not intended to be subclassed. Private to force usage of factory method.
       */
    ~IasTestFramework();

    /**
     * @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasTestFramework(IasTestFramework const &other);

    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasTestFramework& operator=(IasTestFramework const &other);

    /**
     * @brief Initialize test framework.
     *
     * @param[in] pipelineParams The configuration parameters for the pipeline
     *
     * @return The status of the init call
     * @retval eIasOk Initialization sucessful
     * @retval eIasOutOfMemory Out of memory during Initialization
     */
    IasResult init(const IasPipelineParams &pipelineParams);

    // Member variables
    static std::atomic_int          mNumberInstances;   //!< Count the maximum allowed instances of the test framework

    /**
     * Private implementation class
     */
    IasTestFrameworkPriv            *mPriv;             //!< Pointer to private implementation details
};


/**
 * @brief Function to get a IasTestFramework::IasResult as string.
 *
 * @return String carrying the result message.
 */
std::string IAS_AUDIO_PUBLIC toString(const IasTestFramework::IasResult& type);


} // namespace IasAudio

#endif // AUDIO_DAEMON2_PUBLIC_INC_AUDIO_SMARTX_TESTFWX_IASTESTFRAMEWORK_HPP_
