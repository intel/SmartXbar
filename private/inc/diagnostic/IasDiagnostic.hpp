/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/*
 * File:   IasDiagnostic.hpp
 */

#ifndef IASDIAGNOSTIC_HPP
#define IASDIAGNOSTIC_HPP

#include "diagnostic/IasDiagnosticStream.hpp"
#include "avbaudiomodules/internal/audio/common/IasAudioLogging.hpp"
#include <cstdint>
#include <cmath>
#include <list>
#include <memory>
#include <string>
#include <mutex>

namespace IasAudio {

class IasDiagnosticStream;
using IasDiagnosticStreamPtr = std::shared_ptr<IasDiagnosticStream>;

/**
 * @brief List item of registered diagnostic stream.
 */
struct IasDiagnosticListItem
{
  const std::string deviceName;     //!< Name of the ALSA device
  const std::string portName;       //!< Name of the associated port
  IasDiagnosticStreamPtr stream;    //!< Pointer to the registered diagnostic stream
};

using IasDiagnosticStreamList = std::list<IasDiagnosticListItem>;

/**
 * This class handles access to the diagnostic features for debugging the ALSA handler/ASRC timing behavior
 *
 * During start-up an IasAlsaHandlerWorkerThread instance will check the SmartXbar
 * configuration file (default /etc/smartx_config.txt) to find out if the diagnosis
 * shall be activated for this instance. It then uses the IasDiagnostic singleton
 * to register a new diagnostic stream with its relevant configuration parameters.
 * One relevant configuration parameter is a port name, which can
 * be used to start the recording of the diagnostic data just by supplying this port name.
 * The port name is either a source device output port in case of a source device ALSA handler or a
 * routing zone input port in case of a sink device ALSA handler.
 * This means currently there is 1:1 mapping from a single port to an ALSA device,
 * with its ALSA handler, for which the diagnostic data shall be recorded.
 * In the IasRoutingImpl::connect method the recording can then be started by calling the
 * IasDiagnostic::startStream method and passing the port name as parameter.
 * The IasDiagnostic::stopStream method has to be called accordingly in the
 * IasRoutingImpl::disconnect method.
 * In the IasAlsaHandlerWorkerThread::run method the IasDiagnosticStream::writeAlsaHandlerData method has
 * to be called. If the diagnostic stream is not started, then this call will return immediately. If it
 * has been started, then the data will be written to a file located in the /tmp folder.
 * The data is written into the file until either
 *     - the IasDiagnostic::stop method is called
 *     - or if the connection is kept for longer than one hour. In this case the recording is automatically stopped.
 * Recording the data for a maximum time of one hour will lead to a maximum file size of 29MB at a period time
 * of 5,33ms:
 *     44 Bytes/5,33ms = 8251 Bytes/s * 60 * 60 = 29 703 600 Bytes/h
 * After the file is closed it is either removed from the /tmp folder, in case no error occurred, or it is
 * copied to the location specified in the config file, if an error occurred (fall-back to start-up phase).
 * This can be used e.g. to copy it into a folder which is monitored by the dlt-daemon to upload it into
 * the DLT log. It can then later be extracted from the DLT log by enabling the filetransfer plugin in the
 * DLT viewer.
 * There is also a mode to always copy the file after closing to keep it for later analysis.
 */
class IAS_AUDIO_PUBLIC IasDiagnostic
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
   * @brief Constructor
   */
  IasDiagnostic();

  /**
   * @brief Destructor
   */
  virtual ~IasDiagnostic();

  /**
   * @brief Static method to get the singleton instance of IasDiagnostic
   * @return The singleton
   */
  static IasDiagnostic* getInstance();

  /**
   * @brief Set the common configuration parameters
   *
   * @param[in] logPeriodTime The log period time in ms
   * @param[in] numEntriesPerMsg The number of entries (of size cBytesPerPeriod) per message
   */
  void setConfigParameters(std::uint32_t logPeriodTime, std::uint32_t numEntriesPerMsg);

  /**
   * @brief Register a new diagnostic stream
   *
   * The deviceName in the IasParams struct is the name of the device of type eIasClockReceivedAsync,
   * for which the diagnostic shall be activated.
   * The portName is the name of the port, which shall trigger the start of the recording on connect of this port. It will
   * also trigger a stop of the recording on disconnect.
   *
   * @param[in] params The configuration parameters for the diagnostic stream
   * @param[in,out] newStream Pointer to location for new diagnostic stream
   * @return Status of the method
   */
  IasResult registerStream(const struct IasDiagnosticStream::IasParams& params, IasDiagnosticStreamPtr* newStream);

  /**
   * @brief Start recording of the diagnostic data to a file
   *
   * The parameter *name* typically specifies the source device output port or the routing zone input port
   * associated with the diagnostic stream.
   * This is useful to start the recording after receiving the connect command.
   *
   * @param[in] name Name of the port for which recording shall be started
   * @return Status of the method
   */
  IasResult startStream(const std::string& name);

  /**
   * @brief Stop recording of the diagnostic data to a file
   *
   * The parameter *name* typically specifies the source device output port or the routing zone input port
   * associated with the diagnostic stream.
   * This is useful to stop the recording after receiving the disconnect command.
   *
   * @param[in] name Name of the port for which recording shall be stopped
   * @return Status of the method
   */
  IasResult stopStream(const std::string& name);

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
  IasDiagnostic(const IasDiagnostic& orig) = delete;

  /**
   * @brief Assignment operator
   *
   * Deleted to prevent misuse.
   */
  IasDiagnostic& operator=(IasDiagnostic const &other) = delete;

  DltContext* mLog;                   //!< DLT log context
  IasDiagnosticStreamList mStreams;   //!< List of registered diagnostic streams
  IasDiagnosticLogWriter mLogWriter;  //!< Instance of the diagnostic log writer
  std::mutex mMutex;                  //!< Mutex to lock access to the internal members from different threads
  bool mIsInitialized;                //!< Flag to check whether everything is initialized or not
};

} /* namespace IasAudio */


#endif /* IASDIAGNOSTIC_HPP */

