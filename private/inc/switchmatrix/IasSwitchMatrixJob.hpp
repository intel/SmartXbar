/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasSwitchMatrixJob.hpp
 * @date   2015
 * @brief
 */

#ifndef IASSWITCHMATRIXJOB_HPP_
#define IASSWITCHMATRIXJOB_HPP_

#include "audio/common/IasAudioCommonTypes.hpp"
#include "internal/audio/common/IasAudioLogging.hpp"
#include "model/IasAudioPort.hpp"
#include <atomic>

namespace IasAudio {

class IasAudioPort;
class IasAudioLogging;
class IasDataProbe;
class IasSrcWrapperBase;

struct IasProbingQueueEntry;

class IAS_AUDIO_PUBLIC IasSwitchMatrixJob
{

  enum IasSwitchMatrixJobConversions
  {
    eIasFloat32Float32= 0x11,
    eIasInt16Float32  = 0x12,
    eIasInt32Float32  = 0x13,
    eIasFloat32Int16  = 0x21,
    eIasInt16Int16    = 0x22,
    eIasInt32Int16    = 0x23,
    eIasFloat32Int32  = 0x31,
    eIasInt16Int32    = 0x32,
    eIasInt32Int32    = 0x33
   };

  enum IasJobTask
  {
    eIasJobSimpleCopy = 0,       //!< simple data format conversion and copy
    eIasJobSampleRateConversion  //!< sample rate conversion + copy converted samples
  };

  public:

    enum IasResult
    {
      eIasOk,               //!< Ok, Operation successful
      eIasFailed,           //!< Invalid parameter, e.g., out of range or NULL pointer
      eIasLocked            //!< job is not yet allowed to be executed
    };

    /**
     * @brief Constructor.
     */
    IasSwitchMatrixJob(const IasAudioPortPtr src, const IasAudioPortPtr sink);

    /**
     * @brief Destructor, virtual by default.
     */
    virtual ~IasSwitchMatrixJob();

    /**
     * @brief The init function for the switch matrix job.
     *
     * @param copySize the number of samples to be copied
     * @param baseSampleRate the sample rate of the base routing zone
     *
     * @returns IasSwitchMatrixJob::IasResult::eIasOk on success, otherwise error code
     */
    IasResult init(uint32_t copySize,uint32_t baseSampleRate);

    /**
     * @brief Execute the job
     *
     * @param[in] srcOffset the offset from where to read from the source buffer
     * @param[in] framesToRead The number of frames to be read from the source buffer
     * @param[out] framesStillToConsume The number of frames that still needs to be consumed
     * @param[out] framesConsumed The number of frames that have been consumed during the execute operation
     *
     * @returns IasSwitchMatrixJob::IasResult::eIasOk on success, otherwise error code
     */
    IasResult execute(uint32_t srcOffset, uint32_t framesToRead, uint32_t *framesStillToConsume, uint32_t* framesConsumed);

    /**
     * @brief returns the sink port
     *
     * @returns the pointer to the sink port of type const IasAudioPortPtr
     */
    inline const IasAudioPortPtr getSinkPort(){return mSink;};

   /**
     * @brief returns the source port
     *
     * @returns the pointer to the source port of type const IasAudioPortPtr
     */
    inline const IasAudioPortPtr getSourcePort(){return mSrc;};

    /**
     * @brief updates the audio area pointer in the source information struct
     *
     * @param[in] srcAreas pointer to the source areas
     *
     * @returns error value
     * @retval eIasOk everything went fine
     * @retval eIasFailed null pointer was passed to the function
     */
    IasResult updateSrcAreas(IasAudioArea* srcAreas);

    /**
     * @brief Returns the Id of the sink port
     *
     * @returns The id of the sink port as integer
     */
    inline int32_t getSinkPortId(){return mSink->getParameters()->id;};

    /**
     * @brief Returns the Id of the source port
     *
     * @returns The id of the source port as integer
     */
    inline int32_t getSourcePortId(){return mSrc->getParameters()->id;};


    /**
     * @brief Starts a probing operation
     *
     * @param[in] fileNamePrefix the prefix of the wav file name, will be completed by probe object
     * @param[in] bInject flag to indicate if probing is an inject or recording
     * @param[in] numSeconds the duration of the probing in seconds
     * @param[in] dataFormat the sample format of the data
     * @param[in] sampleRate the sample rate of the data
     * @param[in] numChannels the number of channels to be probed
     * @param[in] startIndex the index of the first channel to be probed
     *
     * @returns error coede
     * @retval eIasOk everything wnet fine
     * @retval eIasFails start probing failed
     */
    IasResult startProbe(const std::string& fileNamePrefix,
                         bool bInject,
                         uint32_t numSeconds,
                         IasAudioCommonDataFormat dataFormat,
                         uint32_t sampleRate,
                         uint32_t numChannels,
                         uint32_t startIndex);

    /**
     * @brief Stops a running probe
     */
    void stopProbe();

    void unlock();

    void lock(){mLocked = true;}

  private:
    /**
     * @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasSwitchMatrixJob(IasSwitchMatrixJob const &other);

    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasSwitchMatrixJob& operator=(IasSwitchMatrixJob const &other);

    /**
     * @brief Copies a certain number of frames
     *
     * @param[in] srcOffset The offset of the source buffer, where the frames will be taken from
     * @param[in] framesToRead the number of frame to be read from the source
     * @param[in] framesConsumed The number of frames consumed during this call
     * @param[in] framesStillToConsume The number of frames still to be consumed
     *
     * @returns error code
     * @retval eIasOk everything went fine
     * @retval eIasFailed copy went wrong, no space left in sink
     */
    IasResult copy(uint32_t srcOffset, uint32_t framesToRead, uint32_t *framesConsumed,uint32_t *framesStillToConsume);

    /**
     * @brief Does sample rate conversion for a certain number of frames
     *
     * @param[in] srcOffset The offset of the source buffer, where the frames will be taken from
     * @param[in] framesToRead the number of frame to be read from the source
     * @param[in] framesConsumed The number of frames consumed during this call
     * @param[in] framesStillToConsume The number of frames still to be consumed
     *
     * @returns error code
     * @retval eIasOk everything went fine
     * @retval eIasFailed copy went wrong, no space left in sink
     */
    IasResult sampleRateConvert(uint32_t srcOffset, uint32_t framesToRead, uint32_t *framesConsumed,uint32_t *framesStillToConsume);

    DltContext*                                 mLog;                     //!< The log object
    const IasAudioPortPtr                       mSrc;                     //!< The source port
    const IasAudioPortPtr                       mSink;                    //!< The sink port
    IasAudioPortCopyInformation                 mSrcCopyInfos;            //!< Struct containing some source information
    IasAudioPortCopyInformation                 mSinkCopyInfos;           //!< Struct containing some sink information
    uint32_t                                    mDestSize;                //!< The number of frames to be copied during one execute, can vary if sample rate conv s needed
    uint32_t                                    mSourceSize;              //!< The source period size
    float                                       mBasePeriodTime;          //!< The period time of the base routingzone
    IasDataProbePtr                             mDataProbe;               //!< The probe object, used for debugging
    IasSrcWrapperBase*                          mSrcWrapper;              //!< The sample rate converter object, only created when needed
    float                                       mRatio;                   //!< The frequency ratio of sink and source ( sinkfreq/sourcefreq)
    uint32_t                                    mNumFramesStillToProcess; //!< Current number of frames to still be processed within this execute operation
    IasJobTask                                  mJobTask;                 //!< The task the job has to do
    tbb::concurrent_queue<IasProbingQueueEntry> mProbingQueue;            //!< The queue for the probing
    std::atomic<bool>                           mProbingActive;           //!< flag to indicate if probing is active
    IasSwitchMatrixJobConversions               mSampleFormatConv;        //!< marks which format convertion should be applied
    bool                                        mLocked;                  //!< flag to indicate if the jobs are currently locked
    float                                       mSizeFactor;              //!< factor needed for copy size calculations
    uint32_t                                    mLogCnt;                  //!< Log counter to control the amount of job locked messages
    uint32_t                                    mLogInterval;             //!< The log interval to provide important logs that should not "spam" everything
};

} //namespace IasAudio

#endif /* IASSWITCHMATRIXJOB_HPP_ */
