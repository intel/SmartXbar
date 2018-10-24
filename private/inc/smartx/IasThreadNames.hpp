/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasThreadNames.hpp
 * @date   2016
 * @brief
 */

#ifndef IASTHREADNAMES_HPP
#define IASTHREADNAMES_HPP

#include <mutex>
#include <fstream>

#include "audio/common/IasAudioCommonTypes.hpp"
#include "internal/audio/common/IasAudioLogging.hpp"

namespace IasAudio {

/**
 * @brief This class is responsible to give the threads of the SmartXbar an abbreviated name
 *
 * The naming scheme allows to distinguish real-time from non real-time threads. Due to the limitation of
 * 15 characters per thread name, more details about a thread can be found in the file /tmp/smartx_threads.txt.
 * All standard scheduled threads are named xbar_std_000, where the 000 at the end will be incremented for every
 * new thread. All real-time schedulted threads are named xbar_rt_000, where the 000 at the end will be
 * incremented for every new thread.
 */
class IAS_AUDIO_PUBLIC IasThreadNames
{
  public:
    /**
     * @brief The type of the current thread
     */
    enum IasThreadType
    {
      eIasStandard,   //!< Standard (non real-time) scheduled thread
      eIasRealTime,   //!< A real-time scheduled thread
    };

    /**
     * @brief Return a pointer to the IasThreadNames singleton
     *
     * @return A pointer to the IasThreadNames singleton
     */
    static IasThreadNames* getInstance();

    /**
     * @brief Set the thread name
     *
     * This method will set the name of the current thread (pthread_self()) according to the default naming scheme.
     * A more detailed description of the thread can be given via the parameter description. This description
     * can then be found in the file /tmp/smartx_threads.txt
     *
     * @param[in] type The type of the current thread
     * @param[in] description Detailed description to be put into file
     */
    void setThreadName(IasThreadType type, const std::string &description);

  private:
    /**
     * @brief Constructor.
     */
    IasThreadNames();
    /**
     * @brief Destructor.
     *
     * Class is not intended to be subclassed.
     */
    ~IasThreadNames();

    /**
     * @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasThreadNames(IasThreadNames const &other);
    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasThreadNames& operator=(IasThreadNames const &other);

    DltContext               *mLog;             //!< The DLT log context
    uint32_t               mRtIndex;         //!< Current thread index for real-time tasks
    uint32_t               mStdIndex;        //!< Current thread index for standard (non real-time) tasks
    std::mutex                mMutex;           //!< Mutex to guard access to the indices and the file
    std::ofstream             mFile;            //!< A handle to the output file
};

} //namespace IasAudio

#endif // IASTHREADNAMES_HPP
