/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasSwitchMatrix.hpp
 * @date   2015
 * @brief
 */

#ifndef IASSWITCHMATRIX_HPP_
#define IASSWITCHMATRIX_HPP_

#include <thread>
#include <mutex>
#include <condition_variable>
#include <boost/pool/singleton_pool.hpp>
#include "avbaudiomodules/audio/common/IasAudioCommonTypes.hpp"
#include "smartx/IasAudioTypedefs.hpp"
#include "avbaudiomodules/internal/audio/common/IasAudioLogging.hpp"



namespace IasAudio {

class IasSwitchMatrixJob;
class IasBufferTask;
class IasAudioLogging;
class IasAudioRingBuffer;

class IAS_AUDIO_PUBLIC IasSwitchMatrix
{

  public:

    enum IasResult
    {
      eIasOk,               //!< Ok, Operation successful
      eIasFailed,           //!< Operation failed, logs will give details
    };

    /**
     * @brief Constructor.
     */
    IasSwitchMatrix();

    /**
     * @brief Destructor, virtual by default.
     */
    virtual ~IasSwitchMatrix();

    IasResult init(const std::string &name, uint32_t copySize, uint32_t sampleRate);

    IasResult connect(IasAudioPortPtr src, IasAudioPortPtr sink);

    IasResult disconnect(IasAudioPortPtr src, IasAudioPortPtr sink);

    IasResult trigger();

    IasResult setCopySize(uint32_t copySize);

    IasResult dummyConnect(IasAudioPortPtr src, IasAudioPortPtr sink);

    IasResult dummyDisconnect(IasAudioPortPtr src);

    IasResult startProbing(IasAudioPortPtr port,
                           const std::string& fileNamePrefix,
                           bool bInject,
                           uint32_t numSeconds,
                           IasAudioRingBuffer* ringbuf,
                           uint32_t sampleRate);

    void stopProbing(IasAudioPortPtr port);

    /**
     * @brief unlock all switchmatrix jobs so that they get executed
     *
     */
    void unlockJobs();

    /**
     * @brief Lock job for the given sinkPort
     *
     * @param[in] sinkPort The sink port of a connection job which shall be locked
     */
    void lockJob(IasAudioPortPtr sinkPort);

    /**
     * @brief Remove all connections for src port
     *
     * @param[in] src The source port of a to remove connections from
     */
    IasSwitchMatrix::IasResult removeConnections(IasAudioPortPtr src);

  private:
    /**
     * @brief Action to be applied on buffer task
     *
     * The action is used to send a request from a normally scheduled thread to
     * a real-time scheduled thread
     */
    enum IasBufferTaskAction
    {
      eIasAddBufferTask,        //!< Add a buffer task to the currently scheduled task list
      eIasDeleteBufferTask      //!< Delete a buffer task from the currently scheduled task list
    };

    using IasBufferTaskPtr = std::shared_ptr<IasBufferTask>;
    using IasBufferJobPair = std::pair<IasAudioRingBuffer*, IasSwitchMatrixJob*>;
    using IasBufferTaskPair = std::pair<IasAudioRingBuffer*, IasBufferTaskPtr> ;
    using IasBufferTaskMap = std::map<IasAudioRingBuffer*, IasBufferTaskPtr>;
    using IasBufferTaskList = std::list<IasBufferTaskPtr>;
    using IasActionQueueEntry = std::pair<IasBufferTaskPtr, IasBufferTaskAction>;

    IasResult addBufferTask(IasAudioPortPtr src, IasBufferTaskPtr* task, IasAudioPortPtr sink);

    IasResult findBufferTask(IasAudioRingBuffer* srcBuffer,IasBufferTaskPtr* task);

    /**
     * @brief Remove the buffer task associated with the given source ringbuffer
     *
     * @param[in] rb Ringbuffer for which the buffer task shall be removed
     */
    void removeBufferTask(IasAudioRingBuffer* rb);

    /**
     * @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasSwitchMatrix(IasSwitchMatrix const &other);

    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasSwitchMatrix& operator=(IasSwitchMatrix const &other);

    DltContext                                 *mLog;
    std::string                                 mName;
    bool                                   mInitialized;
    IasBufferTaskMap                            mTaskMap;
    uint32_t                                 mCopySize;
    uint32_t                                 mSampleRate;
    IasBufferTaskList                           mBufferTasks;   //!< List of all buffer tasks being scheduled in rt thread
    tbb::concurrent_queue<IasActionQueueEntry>  mActionQueue;   //!< The buffer task action queue to submit requests
    std::mutex                                  mMutex;         //!< Mutex for condition variable
    std::condition_variable                     mCondition;     //!< Condition variable used to wait for deleted buffer task
};

} //namespace IasAudio

#endif /* IASSWITCHMATRIX_HPP_ */
