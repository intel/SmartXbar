/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * File:   IasDiagnosticStream.hpp
 */

#ifndef IASDIAGNOSTICSTREAM_HPP
#define IASDIAGNOSTICSTREAM_HPP

#include <cstdint>
#include <cmath>
#include <string>
#include <fstream>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>
#include "internal/audio/common/IasAudioLogging.hpp"
#include "diagnostic/IasDiagnosticLogWriter.hpp"

namespace IasAudio {

// Bytes saved per period = 44
static const std::uint32_t cBytesPerPeriod = static_cast<std::uint32_t>(4*sizeof(std::uint64_t)+2*sizeof(std::uint32_t)+1*sizeof(std::float_t));


class IAS_AUDIO_PUBLIC IasDiagnosticStream
{
 public:
  /**
   * @brief The result of the method calls
   */
  enum IasResult
  {
    eIasOk,       //!< Method successfully executed.
    eIasFailed,   //!< Method execution failed.
  };

  /**
   * @brief The desired copy trigger state, i.e. when to copy the recorded data to the destination.
   */
  enum IasCopyTriggerState
  {
    eIasOnError,        //!< Only copy on error. Currently it is hardcoded that the errorOccured method has to be called at least twice to trigger the copying.
    eIasAlways          //!< Always copy.
  };

  /**
   * Parameters used for diagnostic stream configuration
   */
  struct IasParams
  {
    std::string deviceName;           //!< Name of the ALSA device
    std::string portName;             //!< Name of the source device output port or routing zone input port
    std::string copyTo;               //!< The destination to copy the file to
    std::uint32_t errorThreshold;     //!< The error threshold after which to trigger copy or write to DLT
    std::uint32_t periodTime;         //!< Period time of the writeAlsaHandlerData call in µsec
  };

  /**
   * @brief Constructor
   *
   * @param[in] params Configuration parameters of the diagnostic stream
   * @param[in] logWriter Reference to the log writer instance
   */
  IasDiagnosticStream(const struct IasParams& params, IasDiagnosticLogWriter& logWriter);

  /**
   * @brief Destructor
   */
  virtual ~IasDiagnosticStream();

  /**
   * @brief Start recording of the diagnostic data to a file
   *
   * @return Status of the method
   */
  IasResult startStream();

  /**
   * @brief Stop recording of the diagnostic data to a file
   *
   * @return Status of the method
   */
  IasResult stopStream();

  /**
   * This method has to be called to write the data to a file in the /tmp folder
   *
   * The data is stored in binary format in the file and either removed after the file is closed, or it is copied to
   * a configured destination folder.
   *
   * @param[in] timestampDeviceBuffer Timestamp of the device buffer in µs
   * @param[in] numTransmittedFramesDevice Total number of transmitted frames of the device buffer
   * @param[in] timestampAsrcBuffer Timestamp of the ASRC buffer in µs
   * @param[in] numTransmittedFramesAsrc Total number of transmitted frames of the ASRC buffer
   * @param[in] asrcBufferNumFramesAvailable Current fill level of the ASRC buffer
   * @param[in] numTotalFrames ASRC buffer fill level to virtual frames (see IasAudio::IasAlsaHandlerWorkerThread for details)
   * @param[in] ratioAdaptive Current adaptive ratio
   */
  void writeAlsaHandlerData(std::uint64_t timestampDeviceBuffer,
      std::uint64_t numTransmittedFramesDevice,
      std::uint64_t timestampAsrcBuffer,
      std::uint64_t numTransmittedFramesAsrc,
      std::uint32_t asrcBufferNumFramesAvailable,
      std::uint32_t numTotalFrames,
      std::float_t ratioAdaptive);

  /**
   * @brief Has to be called whenever the error to be recorded is seen.
   */
  void errorOccurred();

  /**
   * @brief Check if the diagnostic stream is already started
   *
   * The method also waits for a maximum of 1s until all file operation, which is done in a separate worker thread,
   * is finished.
   *
   * @return True if the diagnostic stream is currently started, false otherwise.
   */
  bool isStarted();

  /**
   * @brief Check if the diagnostic stream is already stopped
   *
   * The method also waits for a maximum of 1s until all file operation, which is done in a separate worker thread,
   * is finished.
   *
   * @return True if the diagnostic stream is currently stopped, false otherwise.
   */
  bool isStopped();

 private:
  /**
   * @brief The state of the diagnostic stream state machine
   *
   * The state machine is used to handle the possibly lengthy file operations.
   */
  enum IasStreamState
  {
    eIasStreamStateIdle,          //!< The diagnostic stream is in idle state, i.e. not started yet
    eIasStreamStateStarted,       //!< The diagnostic stream is started and running
    eIasStreamStateOpening,       //!< The diagnostic stream is waiting for the file to be opened
    eIasStreamStateClosing,       //!< The diagnostic stream is waiting for the file to be closed
    eIasStreamStatePendingClose,  //!< The diagnostic stream received the request to stop before the file was successfully opened
    eIasStreamStatePendingOpen,   //!< The diagnostic stream received the request to start before the file was successfully closed
  };

  /**
   * @brief The trigger to change the state of the state machine
   */
  enum IasTriggerStateChange
  {
    eIasStart,                    //!< Trigger to start the diagnostic stream
    eIasStop,                     //!< Trigger to stop the diagnostic stream
    eIasOpeningFinished,          //!< Trigger to signal that file opening is finished
    eIasClosingFinished,          //!< Trigger to signal that file closing is finished
  };

  /**
   * @brief Copy constructor
   *
   * Deleted to prevent misuse.
   */
  IasDiagnosticStream(const IasDiagnosticStream& orig) = delete;

  /**
   * @brief Assignment operator
   *
   * Deleted to prevent misuse.
   */
  IasDiagnosticStream& operator=(IasDiagnosticStream const &other) = delete;

  /**
   * @brief Method that handles state changes of the state machine
   *
   * @param[in] trigger The trigger action
   * @return The new state after applying the trigger action
   */
  IasStreamState changeState(IasTriggerStateChange trigger);

  /**
   * @brief Worker thread method to open the file
   */
  void openFile();

  /**
   * @brief Worker thread method to close the file
   */
  void closeFile();

  /**
   * @brief Worker thread helper method to copy the file to the destination folder configured in the config file
   */
  void copyFile();

  /**
   * @brief Function to get a IasStreamState as string.
   *
   * @return String carrying the result message.
   */
  std::string toString(const IasStreamState&  streamState);

  /**
   * @brief Function to get a IasTriggerStateChange as string.
   *
   * @return String carrying the result message.
   */
  std::string toString(const IasTriggerStateChange&  trigger);

  DltContext* mLog;                               //!< Log context of ALSA handler for all non-worker thread methods
  DltContext* mLogThread;                         //!< Log context for worker thread methods. Required because log context is not thread-safe
  IasParams mParams;                              //!< Diagnostic stream configuration parameters
  std::uint32_t mPeriodCounter;                   //!< Counts the calls of the writeAlsaHandlerData method
  std::uint32_t mMaxCounter;                      //!< Maximum number which the mPeriodCounter may reach until the file will be automatically closed
  std::ofstream mTmpFile;                         //!< Handle to the tmp file created in the /tmp folder
  std::string mTmpFileName;                       //!< File name of the tmp file
  std::string mTmpFileNameFull;                   //!< File name of the tmp file including path
  IasStreamState mStreamState;                    //!< The current state machine state
  std::mutex mStreamStateMutex;                   //!< Mutex to lock the state machine state changes
  std::thread mFileOpen;                          //!< Handle of the worker thread for file open
  std::thread mFileClose;                         //!< Handle of the worker thread for file close
  std::condition_variable mCondWaitForStart;      //!< Condition variable to signal whether the diagnostic stream is started or stopped
  std::mutex mMutexWaitForStart;                  //!< Mutex for the condition variable
  bool mStreamStarted;                            //!< The variable used to check the condition
  std::atomic<std::uint32_t> mErrorCounter;       //!< This counts the calls of the errorOccurred method
  IasDiagnosticLogWriter &mLogWriter;             //!< Reference to the diagnostic log writer instance
  std::uint32_t mFileIdx;                         //!< Index of the file. Will be incremented after every file
};

} /* namespace IasAudio */

#endif /* IASDIAGNOSTICSTREAM_HPP */

