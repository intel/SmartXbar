/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file
 */

#ifndef IASBUFFERTASK_HPP_
#define IASBUFFERTASK_HPP_

#include "audio/common/IasAudioCommonTypes.hpp"
#include "internal/audio/common/IasAudioLogging.hpp"
#include "smartx/IasAudioTypedefs.hpp"
#include "tbb/concurrent_queue.h"
#include <set>
#include <atomic>

/*!
 * @brief namespace IasAudio
 */
namespace IasAudio
{

class IasSwitchMatrixJob;
class IasAudioLogging;
class IasAudioRingBuffer;
class IasEventProvider;
class IasDataProbe;
struct IasProbingQueueEntry;

using IasSwitchMatrixJobPtr = std::shared_ptr<IasSwitchMatrixJob>;

/*!
 * @brief Documentation for class IasBufferTask
 */
class IAS_AUDIO_PUBLIC IasBufferTask
{
    /**
     * @brief State of the source, tells if data is available or not
     */
    enum IasSourceState
    {
      eIasSourceUnderrun=0, //!< underun state, the source did not deliver data
      eIasSourcePlaying,    //!< playing state, the source delivered data again after underrun
    };

    /**
     * @brief Buffer task action for a job
     */
    enum IasJobAction
    {
      eIasAddJob,   //!< adding a job
      eIasDeleteJob, //!< deleting a job
      eIasDeleteAllSourceJobs //!< deleting all jobs related to a source
    };

    using IasJobActionQueueEntry = std::pair<IasSwitchMatrixJobPtr, IasJobAction>;
    using IasJobActionMap        = std::map<IasSwitchMatrixJobPtr, IasJobAction>;
    using IasConnectionPair      = std::pair<IasAudioPortPtr, IasSwitchMatrixJobPtr>;
    using IasConnectionMap       = std::map<IasAudioPortPtr, IasSwitchMatrixJobPtr>;

  public:

    enum IasResult
    {
      eIasOk,               //!< Ok, Operation successful
      eIasFailed,           //!< Invalid parameter, e.g., out of range or NULL pointer
      eIasNoJobs,
      eIasObjectNotFound, //!< connection is already established
      eIasSinkInUse,        //!< connection not possible, sink already connected
      eIasConnectionAlreadyExists,
      eIasNotConnected      //!< no connection established
    };

    /*!
     *  @brief Constructor.
     */
    IasBufferTask(IasAudioPortPtr src,
                  uint32_t readSize,
                  uint32_t destSize,
                  uint32_t sampleRate,
                  bool isDummy);

    /*!
     *  @brief Destructor, virtual by default.
     */
    virtual ~IasBufferTask();

    IasResult doJobs();

    IasResult addJob(IasAudioPortPtr source, IasAudioPortPtr sink);

    IasResult triggerDeleteJob(IasAudioPortPtr source, IasAudioPortPtr sink,
                        IasBufferTask::IasJobAction deleteReason = eIasDeleteJob);
    std::set<IasSwitchMatrixJobPtr>::iterator deleteJob(std::set<IasSwitchMatrixJobPtr>::iterator jobIt, IasJobAction jobAction);

    IasResult deleteAllJobs(IasAudioPortPtr source);

    bool isDummy(){return mIsDummy;};

    void makeDummy(){mIsDummy = true;};

    void makeReal(){mIsDummy = false;};

    IasResult doDummy();

    IasSwitchMatrixJobPtr findJob(IasAudioPortPtr port);

    IasResult startProbing(const std::string &prefix,
                           uint32_t numSeconds,
                           bool inject,
                           uint32_t numChannels,
                           uint32_t startIndex,
                           uint32_t sampleRate,
                           IasAudioCommonDataFormat dataFormat);

    void stopProbing();

    /**
     * @brief Unlock all jobs to provide data
     */
    void unlockJobs();

    /**
     * @brief Lock all jobs so that no data is copied
     */
    void lockJobs();

    /**
     * @brief Lock job for the given sinkPort
     *
     * @param[in] sinkPort The sink port of a connection job which shall be locked
     */
    void lockJob(IasAudioPortPtr sinkPort);

    /**
     * @brief Check if the buffer task is active
     *
     * Active means there are jobs to execute in the job list.
     *
     * @returns True if the buffer task is active or false if not.
     */
    bool isActive() const;

  private:

    /*!
     *  @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasBufferTask(IasBufferTask const &other);

    /*!
     *  @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasBufferTask& operator=(IasBufferTask const &other);

    IasResult checkConnection(IasAudioPortPtr source, IasAudioPortPtr sink);

    DltContext*                                      mLog;
    IasAudioPortPtr                                  mSrcPort;
    IasAudioRingBuffer*                              mOrigin;
    std::set<IasSwitchMatrixJobPtr>                  mJobs;
    IasJobActionMap                                  mJobActions;
    std::set<IasAudioPortPtr>                        mConnectedSinks;
    tbb::concurrent_queue<IasJobActionQueueEntry>    mJobActionQueue;
    tbb::concurrent_queue<IasProbingQueueEntry>      mProbingQueue;
    IasConnectionMap                                 mConnections;
    uint32_t                                         mSourcePeriodSize;
    uint32_t                                         mDestSize;
    uint32_t                                         mSampleRate;
    IasEventProvider*                                mEventProvider;
    bool                                             mIsDummy;
    IasDataProbePtr                                  mDataProbe;
    std::atomic<bool>                                mProbingActive;
    IasSourceState                                   mSourceState;
};

} // namespace IasAudio

#endif // IASBUFFERTASK_HPP_
