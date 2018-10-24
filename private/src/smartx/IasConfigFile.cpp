/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasConfigFile.cpp
 * @date   2015
 * @brief
 */

#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <sched.h>
#include <exception>
#include <dlt/dlt_types.h>
#include "IasConfigFile.hpp"
#include "internal/audio/common/IasAudioLogging.hpp"

#ifndef SMARTX_CFG_DIR
#define SMARTX_CFG_DIR "/etc/"
#endif

using namespace std;

namespace boost {
namespace program_options {

using IasStringVector = std::vector<std::string>;

struct IasStringVectorType
{
  IasStringVector strings;
};

using IasUIntVector = std::vector<uint32_t>;

struct IasUIntVectorType
{
  IasUIntVector uintegers;
};

void validate(boost::any& v, std::vector<std::string> const & values, IasStringVectorType* target_type, int)
{
  (void)target_type;

  std::vector<std::string> strs;
  boost::split(strs, values[0], boost::is_any_of("\t "), token_compress_on);
  for (auto &entry : strs)
  {
    if (!entry.empty())
    {
      if (v.empty()) {
        v = boost::any(IasStringVectorType());
      }
      IasStringVectorType* tv = boost::any_cast< IasStringVectorType >(&v);
      IAS_ASSERT(tv != nullptr);
      tv->strings.push_back(entry);
    }
  }
}

void validate(boost::any& v, std::vector<std::string> const & values, IasUIntVectorType* target_type, int)
{
  (void)target_type;

  std::vector<std::string> strs;
  boost::split(strs, values[0], boost::is_any_of("\t "), token_compress_on);
  for (auto &entry : strs)
  {
    if (!entry.empty())
    {
      if (v.empty()) {
        v = boost::any(IasUIntVectorType());
      }
      IasUIntVectorType* tv = boost::any_cast< IasUIntVectorType >(&v);
      IAS_ASSERT(tv != nullptr);

      errno = 0;
      char *endPtr = nullptr;
      uint32_t cpuAffinity = static_cast<uint32_t>(strtol(entry.c_str(), &endPtr, 10));
      if ((errno == 0) && ((endPtr == nullptr) || (*endPtr == '\0')))
      {
        tv->uintegers.push_back(cpuAffinity);
      }
    }
  }
}


} // namespace program_options
} // namespace boost

namespace fs = boost::filesystem;

namespace IasAudio {

static const std::string cClassName = "IasConfigFile::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"
#define LOG_THREAD "thread=" + std::string(threadName) + ":"

static const uint32_t cConfigPathMaxLength = 100;
static const std::string cConfigEnvVar = "SMARTX_CFG_DIR";
static const std::string cConfigDefaultPath = SMARTX_CFG_DIR;
static const std::string cConfigFileName = "smartx_config.txt";
static const std::string cRunnerThreadPrefix = "routingzone.runner_threads";
static const std::string cAlsaHandlerDiagPrefix = "alsahandler.diagnostic";

IasConfigFile::IasConfigFile()
  :mLog(IasAudioLogging::registerDltContext("SMX", "SmartX Common"))
  ,mSchedPolicy(SCHED_FIFO)
  ,mSchedPriority(20)
  ,mCpuAffinities()
  ,mDefaultPath(cConfigDefaultPath)
  ,mUnregistered()
  ,mShmGroupName()
  ,mRunnerThreadStates()
  ,mAlsaHandlerDiagnosticParams()
  ,mNumEntriesPerMsg(18)
  ,mLogPeriodTime(500)
{
}

IasConfigFile::~IasConfigFile()
{
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX);
}

IasConfigFile* IasConfigFile::getInstance()
{
  static IasConfigFile theInstance;
  return &theInstance;
}

void IasConfigFile::addLogLevel(po::variable_value value, DltLogLevelType logLevel)
{
  if (!value.empty())
  {
    po::IasStringVectorType logLevelContexts = value.as<po::IasStringVectorType>();
    for (std::vector<std::string>::iterator stringIt=logLevelContexts.strings.begin(); stringIt!=logLevelContexts.strings.end(); ++stringIt)
    {
      IasAudioLogging::addDltContextItem(*stringIt, logLevel, DLT_TRACE_STATUS_ON);
    }
  }
}

void IasConfigFile::addCpuAffinity(po::variable_value value)
{
  if (!value.empty())
  {
    po::IasUIntVectorType cpuAffinities = value.as<po::IasUIntVectorType>();
    for (std::vector<uint32_t>::iterator uintIt=cpuAffinities.uintegers.begin(); uintIt!=cpuAffinities.uintegers.end(); ++uintIt)
    {
      mCpuAffinities.push_back(*uintIt);
    }
  }
}

static std::string policyToString(int32_t policy)
{
  if (policy == SCHED_FIFO)
  {
    return "SCHED_FIFO";
  }
  else if (policy == SCHED_RR)
  {
    return "SCHED_RR";
  }
  else
  {
    return "SCHED_OTHER";
  }
}

void IasConfigFile::setSchedPolicy(po::variable_value value)
{
  // value is always filled because we provided a default value
  IAS_ASSERT(!value.empty());
  std::string policy = value.as<std::string>();
  if (policy == "fifo")
  {
    mSchedPolicy = SCHED_FIFO;
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Scheduling policy", policyToString(mSchedPolicy), "set");
  }
  else if (policy == "rr")
  {
    mSchedPolicy = SCHED_RR;
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Scheduling policy", policyToString(mSchedPolicy), "set");
  }
  else if (policy == "cfs")
  {
    mSchedPolicy = SCHED_OTHER;
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Scheduling policy", policyToString(mSchedPolicy), "set");
  }
  else
  {
    mSchedPolicy = SCHED_FIFO;
    /**
     * @log The scheduling policy in the config file smartx.conf is invalid.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid scheduling policy set:", policy, ". Use default policy", policyToString(mSchedPolicy), "instead");
  }
}

void IasConfigFile::setSchedPriority(po::variable_value value)
{
  // value is always filled because we provided a default value
  IAS_ASSERT(!value.empty());
  uint32_t priority = value.as<uint32_t>();
  mSchedPriority = priority;
  if (mSchedPolicy == SCHED_OTHER && priority != 0)
  {
    /**
     * @log The cfs scheduler only allows a priority of 0. A value > 0 is set for the policy cfs in the config file.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Scheduling priority", priority, "is ignored for scheduling policy cfs");
  }
  else if (priority > 99)
  {
    mSchedPriority = 0;
    /**
     * @log The scheduling priority configured in the config file is out of range.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Priority has to be in the range 0 to 99. Configured scheduling priority", priority, "ignored. Set priority to 0.");
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Scheduling priority", priority, "set");
  }
}

void IasConfigFile::setShmGroupName(po::variable_value value)
{
  // value is always filled because we provided a default value
  IAS_ASSERT(!value.empty());
  std::string groupName = value.as<std::string>();
  mShmGroupName = groupName;
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Shm group name", mShmGroupName, "set");
}

const std::string& IasConfigFile::getShmGroupName() const
{
  return mShmGroupName;
}

void IasConfigFile::load()
{
  // Reset the vector in case load was called before
  mCpuAffinities.clear();
  mAlsaHandlerDiagnosticParams.clear();

  po::options_description descriptions;

  descriptions.add_options()
    ("logging.off", po::value<po::IasStringVectorType>()->multitoken(), "Log level off")
    ("logging.fatal", po::value<po::IasStringVectorType>()->multitoken(), "Log level fatal")
    ("logging.error", po::value<po::IasStringVectorType>()->multitoken(), "Log level error")
    ("logging.warning", po::value<po::IasStringVectorType>()->multitoken(), "Log level warning")
    ("logging.info", po::value<po::IasStringVectorType>()->multitoken(), "Log level info")
    ("logging.debug", po::value<po::IasStringVectorType>()->multitoken(), "Log level debug")
    ("logging.verbose", po::value<po::IasStringVectorType>()->multitoken(), "Log level verbose")

    ("scheduling.rt.policy", po::value<std::string>()->default_value("fifo"), "Policy for real-time audio threads")
    ("scheduling.rt.priority", po::value<uint32_t>()->default_value(20), "Priority for real-time audio threads")
    ("scheduling.rt.cpu_affinity", po::value<po::IasUIntVectorType>()->multitoken(), "CPU affinity for real-time audio threads")

    ("shm.group", po::value<std::string>()->default_value("ias_audio"), "Group name of the created shared memory files")

    ("routingzone.runner_threads", po::value<std::string>()->default_value("disabled"), "Runner thread configuration option")
    ;

  fs::path fullConfigPath;
  const char *envVar = getenv(cConfigEnvVar.c_str());
  size_t pathLength = 0;
  if (envVar != nullptr)
  {
    pathLength = strnlen(envVar, cConfigPathMaxLength);
  }
  if (pathLength != cConfigPathMaxLength && pathLength != 0)
  {
    fullConfigPath = envVar;
    fullConfigPath /= cConfigFileName.c_str();
  }
  else
  {
    // First check in default path
    fullConfigPath = mDefaultPath.c_str();
    fullConfigPath /= cConfigFileName.c_str();
    if (fs::exists(fullConfigPath) == false)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Config file", fullConfigPath.c_str(), "not found");
      // If it does not exist try current directory
      fullConfigPath = ".";
      fullConfigPath /= cConfigFileName.c_str();
    }
  }

  if (fs::exists(fullConfigPath))
  {
    po::variables_map varMap;
    auto parsedOptions = po::parse_config_file<char>(fullConfigPath.c_str(), descriptions, true);
    po::store(parsedOptions, varMap);
    po::notify(varMap);
    // Set the log levels
    addLogLevel(varMap["logging.off"], DLT_LOG_OFF);
    addLogLevel(varMap["logging.fatal"], DLT_LOG_FATAL);
    addLogLevel(varMap["logging.error"], DLT_LOG_ERROR);
    addLogLevel(varMap["logging.warning"], DLT_LOG_WARN);
    addLogLevel(varMap["logging.info"], DLT_LOG_INFO);
    addLogLevel(varMap["logging.debug"], DLT_LOG_DEBUG);
    addLogLevel(varMap["logging.verbose"], DLT_LOG_VERBOSE);
    // Set the sched params
    setSchedPolicy(varMap["scheduling.rt.policy"]);
    setSchedPriority(varMap["scheduling.rt.priority"]);
    addCpuAffinity(varMap["scheduling.rt.cpu_affinity"]);
    // Set the global runner_threads state
    // value is always filled because we provided a default value
    po::variable_value globalRunnerThreads = varMap[cRunnerThreadPrefix];
    IAS_ASSERT(!globalRunnerThreads.empty());
    std::string globalRunnerThreadsStateString = globalRunnerThreads.as<std::string>();
    addRunnerThreadState(cRunnerThreadPrefix, globalRunnerThreadsStateString);
    for (const auto &entry : parsedOptions.options)
    {
      if (entry.unregistered == true)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Unregistered option: key=", entry.string_key, "value=", entry.value[0]);
        mUnregistered[entry.string_key] = entry.value[0];
        // There are probably unregistered entries for specific routing zones, so we try to add them to the map
        addRunnerThreadState(entry.string_key, entry.value[0]);
        // There are probably unregistered entries for ALSA handler diagnostics, so we try to add them to the map
        addAlsaHandlerDiagParam(entry.string_key, entry.value[0]);
      }
    }
    // Set the group name
    setShmGroupName(varMap["shm.group"]);
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Config file", fullConfigPath.c_str(), "successfully loaded");
    return;
  }
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Config file", fullConfigPath.c_str(), "not found");
}

void IasConfigFile::configureThreadSchedulingParameters(DltContext *logCtx, IasRunnerThreadSchedulePriority priorityConfig)
{
  char threadName[16];
  int err = pthread_getname_np(pthread_self(), threadName, sizeof(threadName));
  if(err != 0) {
    DLT_LOG_CXX(*logCtx, DLT_LOG_ERROR, LOG_PREFIX, "pthread_getname_np error: ",err);
    IAS_ASSERT(err == 0);
    return;
  }
  struct sched_param params;
  IasConfigFile *cfg = getInstance();
  IAS_ASSERT(cfg != nullptr);
  uint32_t sched_priority = cfg->getSchedPriority();
  if (priorityConfig == eIasPriorityOneLess)
  {
    sched_priority -= 1;
  }
  params.sched_priority = sched_priority;
  int32_t policy = cfg->getSchedPolicy();
  int32_t result = pthread_setschedparam(pthread_self(), policy, &params);
  if(result == 0)
  {
    result = pthread_getschedparam(pthread_self(), &policy, &params);
    if(result == 0)
    {
      DLT_LOG_CXX(*logCtx, DLT_LOG_INFO, LOG_PREFIX, LOG_THREAD, "Scheduling parameters set. Policy=", policyToString(policy), "Priority=", params.sched_priority);
    }
  }
  else
  {
    /**
     * @log This error indicates a permission issue. The process doesn't seem to have the permission to change scheduling parameters.
     */
    DLT_LOG_CXX(*logCtx, DLT_LOG_ERROR, LOG_PREFIX, LOG_THREAD, "Scheduling parameters couldn't be set. Policy=", policyToString(policy), "Priority=", params.sched_priority);
  }
  if (cfg->getCpuAffinities().size() > 0)
  {
    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    for (auto &entry : cfg->getCpuAffinities())
    {
      // Set an upper limit to avoid errors in macro
      if (entry <= 16)
      {
        CPU_SET(entry, &cpu_set);
        DLT_LOG_CXX(*logCtx, DLT_LOG_INFO, LOG_PREFIX, LOG_THREAD, "Set CPU affinity to CPU", entry);
      }
    }
    result = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpu_set);
    if (result == 0)
    {
      DLT_LOG_CXX(*logCtx, DLT_LOG_INFO, LOG_PREFIX, LOG_THREAD, "CPU affinity successfully set.");
    }
    else
    {
      /**
       * @log The cpu affinity couldn't be set because of either a permission issue or of an invalid CPU core ID.
       */
      DLT_LOG_CXX(*logCtx, DLT_LOG_ERROR, LOG_PREFIX, LOG_THREAD, "CPU affinity failed:", strerror(result));
    }
  }
}

std::string IasConfigFile::getKey(const std::string& key) const
{
  if (mUnregistered.find(key) != mUnregistered.end())
  {
    return mUnregistered.at(key);
  }
  else
  {
    std::string value;
    return value;
  }
}

IasConfigFile::IasOptionState IasConfigFile::getRunnerThreadState(const std::string& routingZoneName) const
{
  std::string optionKey = cRunnerThreadPrefix + "." + routingZoneName;
  auto threadStateIt = mRunnerThreadStates.find(optionKey);
  if (threadStateIt != mRunnerThreadStates.end())
  {
    return threadStateIt->second;
  }
  else
  {
    IAS_ASSERT(mRunnerThreadStates.find(cRunnerThreadPrefix) != mRunnerThreadStates.end());
    return mRunnerThreadStates.at(cRunnerThreadPrefix);
  }
}

void IasConfigFile::addRunnerThreadState(const std::string& optionKey, const std::string& optionValue)
{
  if (optionKey.find(cRunnerThreadPrefix) == 0)
  {
    IasOptionState optionState = eIasDisabled;
    if (optionValue.compare("enabled") == 0)
    {
      optionState = eIasEnabled;
    }
    mRunnerThreadStates[optionKey] = optionState;
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Stored", optionKey, "=", optionValue);
  }
}

void IasConfigFile::addAlsaHandlerDiagParam(const std::string& optionKey, const std::string& optionValue)
{
  if (optionKey.find(cAlsaHandlerDiagPrefix) == 0)
  {
    std::string mapKey = optionKey.substr(0, optionKey.find_last_of("."));
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "mapKey=", mapKey);
    if (optionKey.find("port_name") != std::string::npos)
    {
      mAlsaHandlerDiagnosticParams[mapKey].portName = optionValue;
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully set", optionKey, "=", mAlsaHandlerDiagnosticParams[mapKey].portName);
    }
    else if (optionKey.find("copy_to") != std::string::npos)
    {
      mAlsaHandlerDiagnosticParams[mapKey].copyTo = optionValue;
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully set", optionKey, "=", mAlsaHandlerDiagnosticParams[mapKey].copyTo);
    }
    else if (optionKey.find("error_threshold") != std::string::npos)
    {
      try
      {
        mAlsaHandlerDiagnosticParams[mapKey].errorThreshold = static_cast<std::uint32_t>(std::stoul(optionValue));
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully set", optionKey, "=", mAlsaHandlerDiagnosticParams[mapKey].errorThreshold);
      }
      catch(std::exception&)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid value in config file for", optionKey, ". Keeping default value", mAlsaHandlerDiagnosticParams[mapKey].errorThreshold);
      }
    }
    else if (optionKey.find("log_period_time") != std::string::npos)
    {
      try
      {
        mLogPeriodTime = static_cast<std::uint32_t>(std::stoul(optionValue));
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully set", optionKey, "=", mLogPeriodTime);
      }
      catch(std::exception&)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid value in config file for", optionKey, ". Keeping default value", mLogPeriodTime);
      }
    }
    else if (optionKey.find("num_entries_per_msg") != std::string::npos)
    {
      try
      {
        mNumEntriesPerMsg = static_cast<std::uint32_t>(std::stoul(optionValue));
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Successfully set", optionKey, "=", mNumEntriesPerMsg);
      }
      catch(std::exception&)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid value in config file for", optionKey, ". Keeping default value", mNumEntriesPerMsg);
      }
    }
    else
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Unknown ALSA handler diagnostic key detected:", optionKey);
    }
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "No ALSA handler diagnostic parameter detected");
  }
}

const IasConfigFile::IasAlsaHandlerDiagnosticParams* IasConfigFile::getAlsaHandlerDiagParams(const std::string& deviceName) const
{
  std::string mapKey = cAlsaHandlerDiagPrefix + "." + deviceName;
  auto entryIter = mAlsaHandlerDiagnosticParams.find(mapKey);
  if(entryIter != mAlsaHandlerDiagnosticParams.end())
  {
    return &(entryIter->second);
  }
  else
  {
    return nullptr;
  }
}

std::uint32_t IasConfigFile::getLogPeriodTime() const
{
  return mLogPeriodTime;
}

std::uint32_t IasConfigFile::getNumEntriesPerMsg() const
{
  return mNumEntriesPerMsg;
}


} //namespace IasAudio
