/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/*
 * File:   IasDiagnosticLogWriter.hpp
 */

#ifndef IASDIAGNOSTICLOGWRITER_HPP
#define IASDIAGNOSTICLOGWRITER_HPP

#include <stdint.h>
#include <list>
#include <string>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <mutex>
#include "avbaudiomodules/audio/common/IasAudioCommonTypes.hpp"

namespace IasAudio {

class IAS_AUDIO_PUBLIC IasDiagnosticLogWriter
{
 public:
  /**
   * @brief Constructor
   *
   */
  IasDiagnosticLogWriter();

  /**
   * @brief Destructor
   */
  virtual ~IasDiagnosticLogWriter();

  /**
   * @brief Set the common configuration parameters
   *
   * @param[in] logPeriodTime The log period time in ms
   * @param[in] numEntriesPerMsg The number of entries (of size cBytesPerPeriod) per message
   */
  void setConfigParameters(std::uint32_t logPeriodTime, std::uint32_t numEntriesPerMsg);

  /**
   * @brief Add filename to list of files to be logged
   *
   * The file is added to a list, which a worker thread reads, opens the file,
   * writes its contents to DLT and erases the file afterwards.
   *
   * @param[in] fileName Full filename including path
   */
  void addFile(const std::string& fileName);

  /**
   * @brief Waits until the log writer thread is finished
   *
   * This is meant to be used in unit tests to wait until the thread has finished writing to DLT
   *
   * @return true if thread is finished or false otherwise
   */
  bool isThreadFinished();

 private:
  /**
   * Copy constructor
   *
   * Deleted to prevent misuse.
   */
  IasDiagnosticLogWriter(const IasDiagnosticLogWriter& orig) = delete;

  /**
   * @brief Assignment operator
   *
   * Deleted to prevent misuse.
   */
  IasDiagnosticLogWriter& operator=(IasDiagnosticLogWriter const &other) = delete;

  /**
   * @brief Worker thread method to log the file to DLT
   *
   * The method checks if the mFilesToLog contains an entry and if yes,
   * opens the file, logs its contents to DLT spread over a specific time
   * and removes the file afterwards. It then checks if their is another file to log
   * and if not, ends execution.
   */
  void logFile();

  DltContext* mLog;                               //!< Log context
  DltContext* mLogCtxWorker;                      //!< Log context for the log writer worker thread, which logs the tmp file contents
  char *mReadBuffer;                              //!< Pointer to buffer for reading fragments of the file before writing to DLT
  std::list<std::string> mFilesToLog;             //!< List of files to dump contents to log
  std::thread mLogWorker;                         //!< Worker thread for logging of content
  std::mutex mMutexFilesToLog;                    //!< Mutex to protect access to mFilesToLog list
  std::atomic<bool> mLogWorkerRunning;            //!< Flag to signal that log worker thread is running
  std::condition_variable mCondFinished;          //!< Condition variable to signal if thread is finished
  std::mutex mCondFinishedMutex;                  //!< Mutex for finished cond var
  std::uint32_t mFileIdx;                         //!< Index of the currently logged file. Will be increased after a file was successfully dumped to DLT
  std::uint32_t mLineIdx;                         //!< Index of the currently logged line of the file
  std::uint32_t mLogPeriodTime;                   //!< The log period time in ms
  std::uint32_t mNumEntriesPerMsg;                //!< The number of entries per msg
  std::uint16_t mSizeOfReadBuffer;                //!< The size of the read buffer in bytes
};


} /* namespace IasAudio */

#endif /* IASDIAGNOSTICLOGWRITER_HPP */

