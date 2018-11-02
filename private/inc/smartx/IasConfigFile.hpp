/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasConfigFile.hpp
 * @date   2015
 * @brief
 */

#ifndef IASCONFIGFILE_HPP
#define IASCONFIGFILE_HPP

#include <sched.h>
#include <boost/program_options.hpp>

#include "avbaudiomodules/audio/common/IasAudioCommonTypes.hpp"
#include "avbaudiomodules/internal/audio/common/IasAudioLogging.hpp"

namespace po = boost::program_options;

namespace IasAudio {

/**
 * @brief
 *
 */
class IAS_AUDIO_PUBLIC IasConfigFile
{
  public:
    /**
     * @brief The option state
     */
    enum IasOptionState
    {
      eIasDisabled,       //!< The option is disabled
      eIasEnabled         //!< The option is enabled
    };

    /**
     * @brief All relevant ALSA handler diagnostic params
     */
    struct IasAlsaHandlerDiagnosticParams
    {
      IasAlsaHandlerDiagnosticParams()
        :portName()           //!< Default: empty
        ,copyTo("log")        //!< Default: write to log
        ,errorThreshold(2)    //!< Default: Copy file or write to log after error was triggered twice
      {};
      std::string portName;               //!< Port name associated with this ALSA device to enable diagnostics on connect
      std::string copyTo;                 //!< Copy to destination path after diagnostic file is closed
      std::uint32_t errorThreshold;       //!< The error trigger threshold after which the file is either copied or logged to DLT
    };

    /**
     * @brief Return a pointer to the IasConfigFile singleton
     *
     * @return A pointer to the IasConfigFile singleton
     */
    static IasConfigFile* getInstance();

    /**
     * @brief Set the real-time scheduling parameters of the current thread to the configured parameters
     *
     * The current thread is identified by calling pthread_self() so this method has to be
     * called in the thread function before executing the actual thread work (endless while loop).
     *
     * @param[in] logCtx The DLT log context of the calling process
     * @param[in] priorityConfig The priority to use for the real-time thread being configured
     */
    static void configureThreadSchedulingParameters(DltContext *logCtx, IasRunnerThreadSchedulePriority priorityConfig = eIasPriorityNormal);

    /**
     * @brief Load the configuration from the configuration file
     *
     * This function is called once directly after start-up to read the
     * configuration of the SmartXbar from the configuration file that
     * has to be called smartx_config.txt. This file is typically located
     * in /etc. You can change the default directory by setting the environment
     * variable SMARTX_CFG_DIR.
     */
    void load();

    /**
     * @brief Get the current configured scheduling policy
     *
     * @return The currently configured sched policy
     * @retval SCHED_OTHER Default scheduler (cfs)
     * @retval SCHED_FIFO Real-time fifo scheduler
     * @retval SCHED_RR Real-time round-robin scheduler
     */
    int32_t getSchedPolicy() const { return mSchedPolicy; }

    /**
     * @brief Get the current configured scheduling priority
     *
     * @return The current configured scheduling priority
     */
    uint32_t getSchedPriority() const { return mSchedPriority; }

    /**
     * @brief Get the current configured cpu affinities for the real-time threads
     *
     * @return A vector with the current configured CPU affinities
     */
    const std::vector<uint32_t>& getCpuAffinities() const { return mCpuAffinities; }

    /**
     * @brief Get the current configured shm group name
     *
     * @return The current configured shm group name
     */
    const std::string& getShmGroupName() const;

    /**
     * @brief Set the default path
     *
     * This is useful for testing purposes only. The default default path is set to
     * "/etc/". The default path has to end with a forward slash "/".
     *
     * @param[in] defaultPath The new default path
     */
    void setDefaultPath(const std::string &defaultPath) { mDefaultPath = defaultPath; }

    /**
     * @brief Get value for a specific key
     *
     * This method is useful to query unregistered options
     *
     * @returns The value for an unregistered option. If the key does not exist the string is empty.
     */
    std::string getKey(const std::string& key) const;

    /**
     * @brief Get the configured runner thread state for the given routing zone worker thread
     *
     *  @param[in] routingZoneName The name of the routing zone for which the runner thread state shall be returned
     *
     *  @returns The runner thread state for the given routing zone.
     *  @retval eIasDisabled Runner thread shall be disabled for the given routing zone.
     *  @retval eIasEnabled Runner thread shall be enabled for the given routing zone.
     */
    IasOptionState getRunnerThreadState(const std::string& routingZoneName) const;

    /**
     * @brief Get the configured ALSA handler diagnostic parameters
     *
     * @param[in] deviceName The name of the ALSA device for which the diagnostic parameters shall be queried
     * @returns A pointer to the diagnostic parameters for the device, or the nullptr in case it isn't configured
     */
    const IasAlsaHandlerDiagnosticParams* getAlsaHandlerDiagParams(const std::string& deviceName) const;

    /**
     * @brief Get the log period time in ms
     *
     * @return The log period time in ms
     */
    std::uint32_t getLogPeriodTime() const;

    /**
     * @brief Get the number of entries per msg
     *
     * @return The number of entries per msg
     */
    std::uint32_t getNumEntriesPerMsg() const;

  private:
    /**
     * @brief Map of string pair for key, values that are not registered
     */
    using IasUnregisteredOptions = std::map<std::string, std::string>;

    /**
     * @brief Map to store the runner thread state for each routing zone identified by the routing zone name
     */
    using IasRunnerThreadStateMap = std::map<std::string, IasOptionState>;

    /**
     * @brief Map to store the ALSA handler diagnostic params for each ALSA handler
     *
     * The key is equal to the key in the config file including the device name
     */
    using IasAlsaHandlerDiagnosticParamsMap = std::map<std::string, IasAlsaHandlerDiagnosticParams>;

    /**
     * @brief Constructor.
     */
    IasConfigFile();
    /**
     * @brief Destructor.
     *
     * Class is not intended to be subclassed.
     */
    ~IasConfigFile();

    /**
     * @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasConfigFile(IasConfigFile const &other);
    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasConfigFile& operator=(IasConfigFile const &other);

    void addLogLevel(po::variable_value value, DltLogLevelType logLevel);
    void setSchedPolicy(po::variable_value value);
    void setSchedPriority(po::variable_value value);
    void addCpuAffinity(po::variable_value value);
    void setShmGroupName(po::variable_value value);
    void addRunnerThreadState(const std::string& optionKey, const std::string& optionValue);
    void addAlsaHandlerDiagParam(const std::string& optionKey, const std::string& optionValue);

    DltContext               *mLog;                 //!< The DLT log context
    int32_t                mSchedPolicy;         //!< The scheduling policy
    uint32_t               mSchedPriority;       //!< The scheduling priority
    std::vector<uint32_t>  mCpuAffinities;       //!< The CPU affinities
    std::string               mDefaultPath;         //!< The default path of the config file
    IasUnregisteredOptions    mUnregistered;        //!< A vector with all unregistered options
    std::string               mShmGroupName;        //!< The group name for all shm files
    IasRunnerThreadStateMap   mRunnerThreadStates;  //!< Map containing the runner thread states for a given routing zone
    IasAlsaHandlerDiagnosticParamsMap mAlsaHandlerDiagnosticParams;  //!< Map of all ALSA handler diagnostic params
    std::uint32_t             mNumEntriesPerMsg;    //!< Number of entries per msg for the ALSA handler diagnostics
    std::uint32_t             mLogPeriodTime;       //!< Log period time for the ALSA handler diagnostics in ms
};

} //namespace IasAudio

#endif // IASCONFIGFILE_HPP
