/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file    IasGenericAudioCompCore.hpp
 * @date    2012
 * @brief   The definition of the IasGenericAudioCompCore base class.
 */

#ifndef IASGENERICAUDIOCOMPCORE_HPP_
#define IASGENERICAUDIOCOMPCORE_HPP_

#include "avbaudiomodules/audio/common/IasAudioCommonTypes.hpp"
#include "avbaudiomodules/internal/audio/common/IasAudioLogging.hpp"
#include "audio/smartx/rtprocessingfwx/IasProcessingTypes.hpp"

// disable conversion warnings for tbb
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#include "tbb/tbb.h"
// turn the warnings back on
#pragma GCC diagnostic pop

namespace IasAudio {

/**
 * @brief Audio component processing state.
 *
 * Is used to enable or disable process method of an audio component.
 */
enum IasAudioCompState
{
  eIasAudioCompEnabled,   //!< Component enabled
  eIasAudioCompDisabled   //!< Component disabled
};

class IasGenericAudioCompCore;
class IasGenericAudioCompConfig;
class IasIGenericAudioCompConfig;
struct IasProbingQueueEntry;

/**
 * @brief A map containing IasAudioAreas corresponding to stream Ids
 */
using IasGenericProbeAreas = std::map<int32_t,IasAudioArea*>;

/**
 * @brief A map to contain the data probes corresponding to stream Ids
 */
using IasGenericCoreProbeMap = std::map<int32_t,IasDataProbePtr>;

/**
 * @brief A vector of IasGenericAudioCompCore to manage multiple audio component cores.
 */
using IasGenericAudioCompCoreVector = std::vector<IasGenericAudioCompCore*>;

/**
 * @class IasGenericAudioCompCore
 *
 * This is the definition of the audio processing core base class. Every audio processing component
 * has to define a class deriving from this base class and implementing all pure virtual methods. For
 * a more detailed description of the construction of an audio processing component please see the
 * definition of the IasGenericAudioCompoCore base class.
 */
class IAS_AUDIO_PUBLIC IasGenericAudioCompCore
{
  public:
    /**
     * @brief Constructor.
     */
    IasGenericAudioCompCore(const IasIGenericAudioCompConfig *config, const std::string &componentName);

    /**
     * @brief Destructor, virtual by default.
     */
    virtual ~IasGenericAudioCompCore();

    /**
     * @brief Processes the audio data by applying the algorithm of the audio component.
     *
     * This method implements common features, which applies to all audio components, such
     * as en-/disable processing and calls the processes method of the child class.
     *
     * @returns  The result indicating success (eIasAudioCompOK) or failure.
     */
    IasAudioProcessingResult process();


    /**
     * @brief Resets the internal audio component states, pure virtual.
     *
     * Must be implemented by each audio component. This method resets the
     * internal states of the audio component.
     *
     * @returns  The result indicating success (eIasAudioCompOK) or failure.
     */
    virtual IasAudioProcessingResult reset() = 0;

    /**
     * @brief Initializes the internal audio component states, pure virtual.
     *
     * Must be implemented by each audio component. This method can be
     * used to create additional members that require memory allocation which shall
     * be avoided in the constructor.
     *
     * @returns  The result indicating success (eIasAudioCompOK) or failure.
     */
    virtual IasAudioProcessingResult init() = 0;

    /**
     * @brief Initialize the IasGenericCompCore component
     *
     * This method does some basic initialization and then calls the
     * method IasGenericAudioCompCore::init() implemented by the
     * audio module component
     *
     * @param[in] env The audio chain environment parameters
     */
    IasAudioProcessingResult init(IasAudioChainEnvironmentPtr env);

    /**
     * @brief Enables processing.
     *
     * Enables the process method of the child.
     * This mechanism is implemented by the parent class, the child doesn't
     * care about the implementation.
     */
    void enableProcessing();

    /**
     * @brief switch processing off.
     *
     * Disables the process method of the child.
     * This mechanism is implemented by the parent class, the child doesn't
     * care about the implementation.
     */
    void disableProcessing();

    /**
     * @brief Gets the current processing state.
     *
     * Returns the current processing state of an audio component.
     * This mechanism is implemented by the parent class, the child doesn't
     * care about the implementation.
     *
     * @returns    eIasAudioCompEnabled or eIasAudioCompDisabled.
     */
    inline IasAudioCompState getProcssingState() const { return mProcessingState; }

    /**
     * @brief Sets the chain index of the audio component.
     *
     * The chain index is the index of the audio component in the processing chain starting by zero.
     *
     * @param[in] componentIndex The index of the audio component in the processing chain.
     *
     * Sets the chain index of an audio component.
     */
    inline void setComponentIndex(int32_t componentIndex)  { mComponentIndex = componentIndex; }

    /**
     * @brief Gets the chain index of the audio component.
     *
     * Returns the chain index of an audio component.
     *
     * @returns    Int32  0..n .
     */
    inline int32_t getComponentIndex() const { return mComponentIndex; }

    /**
     * @brief Gets the component name of the audio component.
     *
     * Returns the unique name set during implementation of an audio component.
     *
     * @returns    String e.g. "Limiter".
     */
    inline std::string getComponentName() const { return mComponentName; }

    /**
     * @brief Start probing for a certain stream id
     *
     * @param[in] fileNamePrefix   A prefix that will be added to the wave file name
     * @param[in] bInject          Define if injecting or recording is used
     * @param[in] numSeconds       The duration of probing in seconds
     * @param[in] streamId         The Id of the stream that will be probed
     * @param[in] input            Defines if probing is done before processing of module
     * @param[in] output           Defines if probing is done after processing of module
     *
     * @returns Error code, unequal to zero in case of error
     */
    IasAudioProcessingResult startProbe(const std::string& fileNamePrefix,
                                        bool bInject,
                                        uint32_t numSeconds,
                                        uint32_t streamId,
                                        bool input,
                                        bool output);
    
    /**
     * @brief Stop probing for a certain stream id
     *
     * @param[in] streamId  The Id of the stream
     * @param[in] input     Defines if probing is be done before processing of module
     * @param[in] output    Defines if probing is done after processing of module
     * 
     * @returns Error code, unequal to zero in case of error
     */
    IasAudioProcessingResult stopProbe(int32_t streamId,
                                       bool input,
                                       bool output);

  protected:
    /**
     * @brief Processes the audio data by applying the algorithm of the audio component, pure virtual.
     *
     * Must be implemented by each audio component. This is the actual processing
     * method which is called by the process method of the base class.
     *
     * @returns  The result indicating success (eIasAudioCompOK) or failure.
     */
    virtual IasAudioProcessingResult processChild() = 0;

    uint32_t    mFrameLength;             //!< Frame length of one audio frame.
    uint32_t    mSampleRate;              //!< Sample rate in Hz of the audio processing chain - e.g. 48000Hz.
    int32_t     mComponentIndex;          //!< Component Index in audio chain. Set by Framework
    DltContext    *mLogContext;              //!< The log context
    const IasIGenericAudioCompConfig *mConfig;     //!< Pointer to the configuration of the audio module

  private:
    /**
     * @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasGenericAudioCompCore(IasGenericAudioCompCore const &other);

    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasGenericAudioCompCore& operator=(IasGenericAudioCompCore const &other);

    /**
     * @brief Setup all infos needed for probing
     *
     * @param[in] stream Pointer to the audio stream
     * @param[in] area Pointer to the data used by probing
     */
    void setupProbingInfo(IasAudioStream* stream, IasAudioArea* area);

    // Member variables
    IasAudioCompState                            mProcessingState;         //!< Used to enable or disable the processing.
    std::string                                  mComponentName;           //!< Unique Name of the specific Component set by the developer during implementation
    IasGenericCoreProbeMap                       mInputDataProbes;         //!< map containing all probes that probe before processing
    IasGenericCoreProbeMap                       mOutputDataProbes;        //!< map containing all probes that probe after processing
    IasGenericCoreProbeMap                       mActiveInputDataProbes;   //!< Map containing active input probes associated to streamIds
    IasGenericCoreProbeMap                       mActiveOutputDataProbes;  //!< Map containing active output probes associated to streamIds
    IasGenericProbeAreas                         mInputDataProbesAreas;    //!< Map containing IasAudioAreas for InputProbes for streamIds
    IasGenericProbeAreas                         mOutputDataProbesAreas;   //!< Map containing IasAudioAreas for OutputProbes for streamIds
    tbb::concurrent_queue<IasProbingQueueEntry>  mProbingQueue;            //!< The queue for the probing
};

} //namespace IasAudio

#endif /* IASGENERICAUDIOCOMPCORE_HPP_ */
