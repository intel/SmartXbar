/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasRoutingZoneWorkerThread.hpp
 * @date   2015
 * @brief
 */

#ifndef IASROUTINGZONEWORKERTHREAD_HPP
#define IASROUTINGZONEWORKERTHREAD_HPP

#include <fstream>

#include "avbaudiomodules/internal/audio/common/helper/IasIRunnable.hpp"
#include "IasAudioTypedefs.hpp"
#include <mutex>
#include <atomic>
#include <condition_variable>

namespace IasAudio {

// Forward declarations of classes
class IasEventProvider;
class IasDataProbe;
struct IasProbingQueueEntry;
class IasRunnerThread;
class IasThread;

/**
 * @brief Struct for comprising the parameters for scheduling the worker threads of the derived zones.
 */
struct IasDerivedZoneParams
{
  uint32_t periodSize;         //!< Period size of the derived zone.
  uint32_t periodSizeMultiple; //!< Factor between the period size of the derived zone and the period size of the base zone.
  uint32_t countPeriods;       //!< Counter for calling the derived zones at the reduced rate.
  bool runnerEnabled;          //!< True when runner threads are enabled or false otherwise.
};

/**
 * @brief
 */
class IAS_AUDIO_PUBLIC IasRoutingZoneWorkerThread : public IasIRunnable
{
    friend class IasRunnerThread;
  public:

    /**
     * @brief  Result type of the class IasRoutingZoneWorkerThread.
     */
    enum IasResult
    {
      eIasOk,               //!< Ok, Operation successful
      eIasInvalidParam,     //!< Invalid parameter, e.g., out of range or NULL pointer
      eIasInitFailed,       //!< Initialization of the component failed
      eIasNotInitialized,   //!< Component has not been initialized appropriately
      eIasFailed,           //!< other error
    };

    /**
     * @brief State of the worker thread
     */
    enum IasState
    {
      eIasInActive,         //!< Worker thread is inactive, not scheduled
      eIasActivePending,    //!< Worker thread shall be made active, but is waiting for synchronized activation
      eIasActive,           //!< Worker thread is active, scheduled
    };

    /**
     * @brief Action to change the state
     */
    enum IasAction
    {
      eIasPrepare,          //!< Prepare worker thread for activation
      eIasActivate,         //!< Activate the worker thread
      eIasInactivate,       //!< Inactivate the worker thread
    };

    /**
     * @brief Pair of a derived zone worker thread handle and the associated scheduling parameters.
     */
    using IasDerivedZoneParamsPair = std::pair<IasRoutingZoneWorkerThreadPtr, IasDerivedZoneParams>;

    /**
     * @brief Map for the pairs of worker thread handles and the scheduling parameters of all derived zones.
     */
    using IasDerivedZoneParamsMap = std::map<IasRoutingZoneWorkerThreadPtr, IasDerivedZoneParams>;

    /**
     * @brief Pair of a derived zone worker thread handle and the associated scheduling parameters.
     */
    using IasRunnerThreadParamsPair = std::pair<IasRoutingZoneRunnerThreadPtr, IasDerivedZoneParams>;

    /**
     * @brief Map for the pairs of worker thread handles and the scheduling parameters of all derived zones.
     */
    using IasRunnerThreadParamsMap = std::map<IasRoutingZoneRunnerThreadPtr, IasDerivedZoneParams>;

    /**
     * @brief  State type for the state machine controlling the streaming from the conversion buffer to the sink device.
     *
     * Depending on the current fill level of the conversion buffer, the transferPeriod method applies
     * this state machine to decide how PCM frames are streamed from the conversion buffer to the sink
     * device. The following figure shows the corresponding state diagram.
     *
     * @image html RoutingZone-StreamingStates.png "State machine for controlling the streaming behavior from the conversion buffer to the sink device."
     *
     * Based on this state machine, the transferPeriod does not start the streaming until at least one
     * period of PCM frames is provided by the conversion buffer. On the other hand, if the conversion
     * buffer runs empty (due to a disconnect of the switch matrix), the residual PCM frames in the
     * conversion buffer are streamed to the sink device, before zeros are appended.
     */
    enum IasStreamingState
    {
      eIasStreamingStateBufferEmpty = 0,       //!< Conversion buffer is empty
      eIasStreamingStateBufferPartlyFromEmpty, //!< Conversion buffer is partly filled, was empty before
      eIasStreamingStateBufferFull,            //!< Conversion buffer is full
      eIasStreamingStateBufferPartlyFromFull   //!< Conversion buffer is partly filled, was full before
    };

    /**
     * @brief Struct for collecting all parameters for each conversion buffer that is serviced by the routing zone.
     */
    struct IasConversionBufferParams
    {
      //! The default constructor
      IasConversionBufferParams()
        :ringBuffer(nullptr)
        ,streamingState(eIasStreamingStateBufferEmpty)
        ,sinkDeviceInputPort(nullptr)
      {}

      IasAudioRingBuffer* ringBuffer;          //!< ring buffer handle
      IasStreamingState   streamingState;      //!< streaming state for this ring buffer
      IasAudioPortPtr     sinkDeviceInputPort; //!< input port of the sink device where the PCM frames shall be transfered to
    };

    /**
     * @brief Type definition for pairs of audio ports and their associated conversion buffers.
     */
    using IasConversionBufferParamsPair = std::pair<IasAudioPortPtr, IasConversionBufferParams>;

    /**
     * @brief Type definition for maps with all audio ports and their associated conversion buffers.
     */
    using IasConversionBufferParamsMap = std::map<IasAudioPortPtr,  IasConversionBufferParams>;

    /**
     * @brief Constructor.
     */
    IasRoutingZoneWorkerThread(IasRoutingZoneParamsPtr params);

    /**
     * @brief Destructor.
     *
     * Class is not intended to be subclassed.
     */
    ~IasRoutingZoneWorkerThread();

    IasResult init();

    IasResult linkAudioSinkDevice(IasAudioSinkDevicePtr sinkDevice);
    void unlinkAudioSinkDevice();

    /**
     * @brief Add the worker thread object of a derived zone to the current zone (which must be a base zone).
     *
     * Although derived zones do not have active worker threads, they still have an
     * worker thread object, which includes all the methods that are required for
     * transfering PCM frames from the conversion buffer to the sink device.
     *
     * This function adds the worker thread object of a derived zone to the current (base)
     * zone, so that the worker thread of the base zone can call the transferPeriod method
     * of all derived zones.
     *
     * @param[in] derivedZoneWorkerThread  Handle to the worker thread object of the derived zone
     *                                     that shall be added to the base zone.
     */
    IasResult addDerivedZoneWorkerThread(IasRoutingZoneWorkerThreadPtr derivedZoneWorkerThread);

    /**
     * @brief Delete the worker thread object of a derived zone from the current (base) zone.
     *
     * @param[in] derivedZoneWorkerThread  Handle to the worker thread object of the derived zone
     *                                     that shall be deleted from the base zone.
     */
    void deleteDerivedZoneWorkerThread(IasRoutingZoneWorkerThreadPtr derivedZoneWorkerThread);

    /**
     * @brief Set the pointer to the associated Switch Matrix.
     *
     * The Routing Zone Worker Thread needs this pointer, because it has to trigger
     * the Switch Matrix after each period of transferred PCM frames.
     *
     * @param[in] worker Pointer to the Switch Matrix.
     */
    void setSwitchMatrix(IasSwitchMatrixPtr switchMatrix) { mSwitchMatrix = switchMatrix; }

    /**
     * @brief Clear the association with the Switch Matrix.
     */
    void clearSwitchMatrix() { mSwitchMatrix = nullptr; }

    /**
     * @brief Prepare all states that are needed for the worker thread.
     *
     * This function asks the linked audio sink device for parameters like
     * mPeriodSize, mDataFormat, mAudioAreas, mNumChannels, mSinkDeviceParams.
     * Furthermore, several member variables are checked, so that the transferPeriod()
     * method can apply a simplified error handling based on IAS_ASSERT().
     * This function has to be called before the start() method is used.
     */
    IasResult prepareStates();

    /**
     * @brief Add a new conversion buffer with its associated audio port to the routing zone worker thread.
     *
     * The handles to the audio port and to the conversion buffer must not be nullptr. Furthermore,
     * the audio port and the conversion buffer must not have been added to the worker thread before.
     *
     * @param[in] audioPort         Audio port that is associated with the conversion buffer.
     * @param[in] conversionBuffer  Conversion buffer that shall be added.
     *
     * @return                      Result of the operation.
     * @retval    eIasOk            If the conversion buffer and its audio port have been added.
     * @retval    eIasFailed        In case of an error, i.e., if the conversion buffer or the audio port have already been added before.
     */
    IasResult addConversionBuffer(const IasAudioPortPtr audioPort, IasAudioRingBuffer* conversionBuffer);

    /**
     * @brief Delete the conversion buffer that is associated with the specified audio port from the routing zone worker thread.
     *
     * @param[in] audioPort  Audio port that is associated with the conversion buffer that shall be deleted.
     */
    void deleteConversionBuffer(const IasAudioPortPtr audioPort);

    /**
     * @brief Get access to the map of all audioPorts/conversionBuffers.
     */
    inline const IasConversionBufferParamsMap& getConversionBufferParamsMap() const { return mConversionBufferParamsMap; }

    /**
     * @brief Get the conversion buffer that is associated with the specified audioPort
     *
     * @param[in] audioPort  Audio port whose conversion buffer shall be returned.
     * @return               Handle to the conversion buffer, or nullptr if the audio port is not known.
     */
    IasAudioRingBuffer* getConversionBuffer(const IasAudioPortPtr audioPort);

    /**
     * @brief Link a zone input port to a sink device input port.
     *
     * This function controls how the routing zone worker thread transfers the PCM frames from the zone input ports
     * to the sink device input ports.
     *
     * @param[in] zoneInputPort The routing zone input port that shall be linked to the audio sink device input port
     * @param[in] sinkDeviceInputPort The audio sink device input port that shall be linked to the routing zone input port
     * @return The result of the call
     */
    IasResult linkAudioPorts(IasAudioPortPtr zoneInputPort, IasAudioPortPtr sinkDeviceInputPort);

    /*
     * @brief Unlink a zone input port from a sink device input port.
     *
     * @param[in] zoneInputPort The routing zone input port that shall be unlinked from the audio sink device input port
     * @param[in] sinkDeviceInputPort The audio sink device input port that shall be unlinked from the routing zone input port
     */
    void unlinkAudioPorts(IasAudioPortPtr zoneInputPort, IasAudioPortPtr sinkDeviceInputPort);

    /**
     * @brief Add pipeline to the routing zone worker thread.
     *
     * There is only one pipeline per routing zone allowed. Adding a second pipeline will fail.
     *
     * @param[in] pipeline Pipeline that shall be added to routing zone.
     *
     * @return The result of the method call
     */
    IasResult addPipeline(IasPipelinePtr pipeline);

    /**
     * @brief Delete pipeline from the routing zone worker thread.
     */
    void      deletePipeline();

    /**
     * @brief Get sink device port pointer connected to the routing zone input port otherwise returns null pointer
     *
     * @param[in] zoneInputPort Routing zone input port pointer
     * @param[out] sinkDeviceInputPort Sink device input port pointer
     *
     * @return The result of the method call
     */
    void getLinkedSinkPort(IasAudioPortPtr zoneInputPort, IasAudioPortPtr *sinkDeviceInputPort);

    /**
     * @brief Add pipeline of the base routing zone to this derived routing zone.
     *
     * There is only one base pipeline per derived routing zone allowed. Adding a second pipeline will fail.
     * This routing zone has to be a derived routing zone.
     *
     * @param[in] pipeline Pipeline of the base routing zone that shall be added to routing zone.
     *
     * @return The result of the method call
     */
    IasResult addBasePipeline(IasPipelinePtr pipeline);

    /**
     * @brief Delete base pipeline from the routing zone.
     */
    void      deleteBasePipeline();

    /**
     * @brief start a probing
     *
     * @param[in] namePrefix Prefix of the filename, without directory
     * @param[in] inject Flag to indicate if this is a inject or a record
     * @param[in] numSeconds The duration of the probe in seconds
     * @param[in] startIndex start channel index for the probed data in buffer
     * @param[in] numChannels the number of channels to be probed
     *
     * @returns error success of failed
     */
    IasResult startProbing(const std::string &namePrefix,
                           bool inject,
                           uint32_t numSeconds,
                           uint32_t startIndex,
                           uint32_t numChannels);

    /**
     * @brief stops a probing
     *
     */
    void stopProbing();

    IasResult start();
    IasResult stop();

    IasAudioCommonResult beforeRun();
    IasAudioCommonResult run();
    IasAudioCommonResult shutDown();
    IasAudioCommonResult afterRun();

    /**
     * @brief tells the worker thread that it is the thread of a derived zone
     */
    void setDerived(bool isDerived) { mIsDerivedZone = isDerived; }

    /**
     * @brief Verify whether the routing zone worker thread is active.
     *
     * Active means not necessarily that the thread pointed to by mThread is running, but that
     * the worker thread execution method is scheduled to be run. This can be either in the thread pointed to
     * by mThread in case of a base routing zone, or it can be that the execution method is called in the context
     * of another routing zone, when this is only a derived routing zone.
     *
     * @returns Boolean flag whether the routing zone worker thread is active.
     */
    bool isActive() const { return (mCurrentState == eIasActive);}

    /**
     * @brief Verify whether the routing zone worker thread is pending to be active.
     *
     * Active means not necessarily that the thread pointed to by mThread is running, but that
     * the worker thread execution method is scheduled to be run. This can be either in the thread pointed to
     * by mThread in case of a base routing zone, or it can be that the execution method is called in the context
     * of another routing zone, when this is only a derived routing zone.
     *
     * @returns Boolean flag whether the routing zone worker thread is pending to be active.
     */
    bool isActivePending() const { return (mCurrentState == eIasActivePending);}

    /**
     * @brief Change the state
     *
     * @param[in] action The action to do
     * @param[in] lock Used to disable locking of the mutex. Default is mutex will be locked.
     */
    void changeState(IasAction action, bool lock = true);

    /**
     * @brief Check if the given pipeline was added to this routing zone
     *
     * @param[in] pipeline The pipeline that shall be checked for existence
     *
     * @returns Boolean flag whether the given pipeline was added or not
     */
    bool hasPipeline(IasPipelinePtr pipeline) const;

    /**
     * @brief Get the pipeline
     *
     * @returns The pointer to the pipeline
     */
    IasPipelinePtr getPipeline() const { return mPipeline; }

  private:
    /**
     * @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasRoutingZoneWorkerThread(IasRoutingZoneWorkerThread const &other);
    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasRoutingZoneWorkerThread& operator=(IasRoutingZoneWorkerThread const &other);

    /**
     * @brief Transfer one period of PCM frames from the routing zone to the linked audio sink device.
     */
    IasResult transferPeriod();

    /**
     * @brief Get the handle of the sink device that is linked to this routing zone.
     */
    IasAudioSinkDevicePtr getSinkDevice() const { return mSinkDevice; };

    /**
     * @brief Activate pending worker threads
     */
    void activatePendingWorker();

    /**
     * @brief Prefill the sink ringbuffer with zeros
     *
     * This will fill the sink ringbuffer with (bufferSize - periodSize) zeros, to have
     * a defined startup fill level for the sink devices linked to a derived zone.
     * The base zone worker thread then waits until the device has removed one period from this
     * buffer before activating the processing for this derived zone. After the derived zone
     * is activated the free space is then filled with the first valid samples if a source
     * port is already connected, or again with zeros if nothing is connected. This keeps the buffer
     * always at a fill level of (bufferSize - periodSize).
     */
    void prefillSinkBuffer();

    /**
     * @brief Check if a sink device is serviced
     *
     * The sink device ringbuffer is filled with (bufferSize - periodSize) or periodSize zeros. If during
     * a check it is detected that the current buffer fill level is suddenly <= (bufferSize - 2*periodSize) or < periodSize,
     * then the sink device was serviced and there is space to fill one or more periods.
     *
     * @return True if the sink was serviced, or false if the sink device wasn't serviced yet.
     */
    bool isSinkServiced() const;

    /**
     * @brief Clear the conversion buffers
     *
     * The conversion buffers are reset to their initial state with buffer fill level = 0
     */
    void clearConversionBuffers();


    DltContext                                 *mLog;                       //!< The DLT log context
    IasRoutingZoneParamsPtr                     mParams;                    //!< The params of the routing zone
    IasAudioSinkDevicePtr                       mSinkDevice;                //!< Handle of the sink device
    IasAudioRingBuffer*                         mSinkDeviceRingBuffer;      //!< Handle to the ring buffer of the sink device
    IasRunnerThreadParamsMap                    mRunnersParamsMap;          //!< Runner thread handles and scheduling params of all derived zones
    IasDerivedZoneParamsMap                     mDerivedZoneParamsMap;      //!< Runner thread handles and scheduling params of all derived zones
    uint32_t                                    mPeriodSize;                //!< Period size
    IasAudioCommonDataFormat                    mSinkDeviceDataFormat;      //!< Audio data format of the sink device
    uint32_t                                    mSinkDeviceNumChannels;     //!< Number of channels of the sink device
    IasThread                                  *mThread;                    //!< The thread object of the worker thread
    bool                                        mThreadIsRunning;           //!< true, thread is running; false, thread shall be ended
    IasSwitchMatrixPtr                          mSwitchMatrix;              //!< Handle of the switch matrix
    IasConversionBufferParamsMap                mConversionBufferParamsMap; //!< Contains parameters of the conversion buffer associated to an audio port
    IasEventProvider*                           mEventProvider;             //!< Handle to the event provider
    IasDataProbePtr                             mDataProbe;                 //!< Data probing object. Only exists when data probing is activated
    tbb::concurrent_queue<IasProbingQueueEntry> mProbingQueue;              //!< Queue to pass probing actions from non-real-time to real-time thread
    std::atomic<bool>                           mProbingActive;             //!< Flag to signal whether probing is active
    bool                                        mIsDerivedZone;             //!< True, if this zone is a derived zone
    std::mutex                                  mMutexDerivedZones;         //!< Mutex to protect read vs. erase accesses to mDerivedZoneParamsMap
    std::mutex                                  mMutexConversionBuffers;    //!< Mutex to protect read vs. erase accesses to mConversionBufferParamsMap
    std::mutex                                  mMutexPipeline;             //!< Mutex to protect processing vs. erase accesses to mPipeline object
    std::timed_mutex                            mMutexTransferInProgress;   //!< Mutex to wait for transfer ended when stopping a derived routing zone
    std::uint32_t                               mDerivedZoneCallCount;      //!< Counter to check if all derivedZoneCounters are zero
    IasPipelinePtr                              mPipeline;                  //!< Handle to the pipeline associated with this routing zone
    IasState                                    mCurrentState;              //!< States for state machine of worker thread
    std::string                                 mDiagnosticsFileName;       //!< File name for saving routing zone diagnostics
    std::ofstream                               mDiagnosticsStream;         //!< Output stream for saving routing zone diagnostics
    IasPipelinePtr                              mBasePipeline;              //!< A pointer to the base pipeline
    uint32_t                                    mLogCnt;                    //!< Log counter to control the amount of sink buffer full messages
    uint32_t                                    mPeriodTime;                //!< The periodTime rounded to UInt
    uint32_t                                    mLogInterval;               //!< The log intervall to provide important logs that should not "spam" everything
    uint32_t                                    mTimeoutCnt;                //!< The timeout counter used to control the amount of logs. This also uses the mLogInterval for throtteling.
    uint32_t                                    mLogOkCnt;                  //!< This count is used to reset the mLogCnt, whenever the buffer fill level is ok for a certain number of times.
};


/**
 * @brief Function to get a IasRoutingZoneWorkerThread::IasResult as string.
 *
 * @return String carrying the result message.
 */
std::string toString(const IasRoutingZoneWorkerThread::IasResult& type);


/**
 * @brief Function to get a IasStreamingState as string.
 *
 * @return String carrying the result message.
 */
std::string toString(const IasRoutingZoneWorkerThread::IasStreamingState&  streamingState);


class IasRunnerThread final : public IasIRunnable
{
  public:

    /**
     * @brief  Result type of the class IasRoutingZoneWorkerThread.
     */
    enum IasResult
    {
      eIasOk,               //!< Ok, Operation successful
      eIasInvalidParam,     //!< Invalid parameter, e.g., out of range or NULL pointer
      eIasInitFailed,       //!< Initialization of the component failed
      eIasNotInitialized,   //!< Component has not been initialized appropriately
      eIasFailed,           //!< other error
    };

    /**
     * @brief Pair of a derived zone worker thread handle and the associated scheduling parameters.
     */
    using IasDerivedZoneParamsPair = std::pair<IasRoutingZoneWorkerThreadPtr, IasDerivedZoneParams>;

    /**
     * @brief Map for the pairs of worker thread handles and the scheduling parameters of all derived zones.
     */
    using IasDerivedZoneParamsMap = std::map<IasRoutingZoneWorkerThreadPtr, IasDerivedZoneParams>;

    IasRunnerThread(uint32_t mPeriodSizeMultiple, std::string parentZoneName);
    virtual ~IasRunnerThread();
    IasResult addZone(IasDerivedZoneParamsPair derivedZone);
    void deleteZone(IasRoutingZoneWorkerThreadPtr worker);
    bool hasZone(IasRoutingZoneWorkerThreadPtr worker);
    bool isEmpty();
    void wake();

    IasResult start();
    IasResult stop();

    IasAudioCommonResult beforeRun();
    IasAudioCommonResult run();
    IasAudioCommonResult shutDown();
    IasAudioCommonResult afterRun();
    uint32_t addPeriod(uint32_t periodCount);
    bool isAnyActive() const;

    bool isProcessing() const;
    uint32_t getPeriodSizeMultiple() const;

  private:
    /**
     * @brief Copy constructor, not required.
     */
    IasRunnerThread(IasRunnerThread const &other) = delete;
    /**
     * @brief Assignment operator, not required.
     */
    IasRunnerThread& operator=(IasRunnerThread const &other) = delete;

    bool                           mThreadShouldBeRunning;   //!< Flag to signal if the thread shall be running (true) or ended (false)
    DltContext                    *mLog;                     //!< The DLT log context
    std::condition_variable        mCondition;               //!< The condition variable to signal if the runner thread shall be running
    mutable std::mutex             mMutexDerivedZones;       //!< Mutex to protect read vs. erase accesses to mDerivedZoneParamsMap
    IasDerivedZoneParamsMap        mDerivedZoneParamsMap;    //!< Worker thread handles and scheduling params of all derived zones
    uint32_t                       mPeriodSizeMultiple;      //!< The period size multiple for which this runner thread was created
    uint32_t                       mPeriodCount;             //!< Variable counting the periods. The runner thread shall run when the period count reaches the period count multiple
    IasThread                     *mThread;                  //!< The thread object of the runner thread
    std::atomic_bool               mIsProcessing;            //!< Flag to indicate whether the runner thread is currently active and processing
    std::string                    mParentZoneName;          //!< Name of the parent routing zone for logging purposes
};

/**
 * @brief Function to get a IasRoutingZoneWorkerThread::IasResult as string.
 *
 * @return String carrying the result message.
 */
std::string toString(const IasRunnerThread::IasResult& type);

} //namespace IasAudio

#endif // IASROUTINGZONEWORKERTHREAD_HPP
