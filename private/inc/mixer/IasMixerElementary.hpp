/*
  * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file   IasMixerElementary.hpp
 * @date   August 2016
 * @brief  The elementary mixer mixes several input streams into one output stream.
 */

#ifndef IASMIXERELEMENTARY_HPP_
#define IASMIXERELEMENTARY_HPP_

#include "avbaudiomodules/internal/audio/common/IasAudioLogging.hpp"
#include <type_traits>
#include "audio/smartx/rtprocessingfwx/IasProcessingTypes.hpp"
#include "audio/smartx/rtprocessingfwx/IasAudioStream.hpp"
#include "audio/smartx/rtprocessingfwx/IasBundledAudioStream.hpp"
#include "audio/smartx/rtprocessingfwx/IasGenericAudioCompCore.hpp"
#include "audio/smartx/rtprocessingfwx/IasProcessingTypes.hpp"
#include "rtprocessingfwx/IasGenericAudioCompConfig.hpp"
#include "rtprocessingfwx/IasAudioChannelBundle.hpp"
#include "helper/IasRamp.hpp"

// disable conversion warnings for tbb
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#include "tbb/tbb.h"
// turn the warnings back on
#pragma GCC diagnostic pop

namespace IasAudio {

class IasMixerElementary;

/**
 *  @brief Fader parameters of one input stream.
 */
struct IasMixerFadeParams
{
  float   *faderFront;          ///< Gains for the front channel [mFrameSize]
  float    faderFrontTarget;    ///< Target gain for the front channel
  IasRamp *rampFaderFront;      ///< Ramp generator for the front channel
  float   *faderRear;           ///< Gains for the rear channel [mFrameSize]
  float    faderRearTarget;     ///< Target gain for the rear channel
  IasRamp *rampFaderRear;       ///< Ramp generator for the rear channel
  bool    active;              ///< Active state
};


/**
 *  @brief Balance parameters of one input stream.
 */
struct IasMixerBalanceParams
{
  float   *balanceLeft;         ///< Gains for the left channel [mFrameSize]
  float    balanceLeftTarget;   ///< Target gain for the left channel
  IasRamp *rampBalanceLeft;     ///< Ramp generator for the left channel
  float   *balanceRight;        ///< Gains for the right channel [mFrameSize]
  float    balanceRightTarget;  ///< Target gain for the right channel
  IasRamp *rampBalanceRight;    ///< Ramp generator for the right channel
  bool     active;              ///< Active state
};


/**
 *  @brief GainOffset parameters of one input stream.
 */
struct IasMixerGainParams
{
  float   *gainOffset;          ///< Gain offsets for all channels of stream [mFrameSize]
  float    gainOffsetTarget;    ///< Target gain offset for all channels of stream
  IasRamp *rampGainOffset;      ///< Ramp generator for gain offset for all channels of stream
  bool     active;              ///< Active state
};


/**
 *  @brief This struct describes for one channel in which bundle it can
 *         be found and which channel (within the bundle) belongs to it.
 */
struct IasMixerElementaryChannelMapping
{
  uint32_t   bundleIndex;   ///< index of the bundle in which the channel can be found
  uint32_t   channelIndex;  ///< index of the channel (within the bundle)
};


/**
 *  @brief Type definition for a vector, which describes for each channel
 *         of one audio stream in which bundle it is located and which
 *         channel (within the bundle) refers to it.
 */
using IasMixerElementaryChannelMappingVector = std::vector<IasMixerElementaryChannelMapping>;

/**
 *  @brief Type definition for a struct, which comprises all parameters
 *         that are associated with one input stream.
 */
struct IasMixerElementaryStreamParams
{
  uint32_t                               nChannels;            ///< Number of channels that belong to this stream.
  IasMixerFadeParams                     fadeParams;           ///< Fader parameters.
  IasMixerBalanceParams                  balanceParams;        ///< Balance parameters.
  IasMixerGainParams                     gainOffsetParams;     ///< Gain offset parameters.
  std::vector<float*>                    matrixGainVector;     ///< Vector [mNumOutputChannels] with pointers to the affected gain values within mMatrixVector
  IasMixerElementaryChannelMappingVector channelMappingVector; ///< Describes for each input channel in which bundle it can be found
};

/**
 * @brief queue element for the balance queue
 */
struct IasMixerBalanceQueueEntry
{
  int32_t streamId;    ///< Id of the stream
  float   left;        ///< Gain for the left channel
  float   right;       ///< Gain for the right channel
};

/**
 * @brief queue element for the fader queue
 */
struct IasMixerFaderQueueEntry
{
  int32_t streamId;    ///< Id of the stream
  float   front;       ///< Gain for the front channel
  float   rear;        ///< Gain for the rear channel
};

/**
 * @brief queue element for the gain offset queue
 */
struct IasMixerGainOffsetQueueEntry
{
  int32_t streamId;    ///< Id of the stream
  float   gainOffset;  ///< Gain offset for all channels of this stream
};

/**
 * @brief Multichannel layout
 */
enum IasMixerMultichannelIndex
{
  eIasMixerFrontLeft = 0,   ///< FL index
  eIasMixerFrontRight,      ///< FR index
  eIasMixerLFE,             ///< LFE index
  eIasMixerCenter,          ///< C index
  eIasMixerRearLeft,        ///< RL index
  eIasMixerRearRight,       ///< RR index
  eIasMaxNumMultichannel    ///< last entry
};


/**
 *  @brief Type definition for a function pointer to one of the matrix update functions.
 */
using MatrixUpdateFuncPtr = std::add_pointer<void(IasMixerElementaryStreamParams*, uint32_t, bool)>::type;

/**
 *  @brief  Type definition for a gain tile.
 *
 *  A gain tile comprises 4x4 gain values. The first index within the matrix refers
 *  to the output channel, whereas the second index refers to the input channel.
 */
typedef float IasMixerElementaryGainTile[cIasNumChannelsPerBundle][cIasNumChannelsPerBundle];

using IasMixerElementaryPtr = std::shared_ptr<IasMixerElementary>;
using IasMixerElementaryVector = std::vector<IasMixerElementaryPtr>;
using IasMixerElementaryStreamMap = std::map<int32_t, uint32_t>;
using IasMixerElementaryStreamParamsMap = std::map<int32_t,IasMixerElementaryStreamParams>;
using IasMixerElementaryStreamParamsPair = std::pair<int32_t,IasMixerElementaryStreamParams>;
using IasMixerElementaryStreamParamsVector = std::vector<IasMixerElementaryStreamParams>;
using IasMixerElementaryIndexMap = std::map<uint32_t,std::vector<float>>;
using IasEMixerDataVector = std::vector<float>;
using IasEMixerList = std::list<int32_t>;
using IasEMixerListIterator = IasEMixerList::iterator;

class IAS_AUDIO_PUBLIC IasMixerElementary
{
  public:
    /**
     *  @brief Constructor.
     *
     *  @param[in] streamPair   Defines the output stream and all input streams.
     *  @param[in] framelength  Number of samples in each audio frame.
     *  @param[in] sampleFreq   Sample rate [Hz].
     */
     IasMixerElementary(const IasIGenericAudioCompConfig* config,
                        const IasStreamPair &streamPair,
                        uint32_t          frameLength,
                        uint32_t          sampleFreq);
    /**
     *  @brief Destructor, virtual by default.
     */
    virtual ~IasMixerElementary();

    /**
     *  @brief Initialization method.
     *
     *  @return Error code.
     *  @retval eIasAudioProcOK                    Success!
     *  @retval eIasAudioProcNotEnoughMemory       Memory allocation has failed.
     *  @retval eIasAudioProcInvalidParam          Invalid number of input/output channels.
     *  @retval eIasAudioProcInitializationFailed  Error during execution of createChannelMapping().
     */
    IasAudioProcessingResult init();

    /**
     *  @brief Finalize the initialization such that the elementary mixer operatates in "main" mode.
     *
     *  The "main" mode is used if the elementary mixer is used by the IasMixerCore module.
     *
     *  @return Error code.
     *  @retval eIasAudioProcOK            Success!
     *  @retval eIasAudioProcInvalidParam  Input stream with invalid number of channels (must be 1 or 2).
     */
    IasAudioProcessingResult setupModeMain();

    /**
     *  @brief Execute the elementary mixer for one audio frame (the real-time processing function).
     */
    void run();

    /**
     *  @brief Announce a call back object, which shall be executed if a
     *         balance/fader/gainoffset ramp is finished.
     *
     *  @return    Error code.
     *  @retval    eIasAudioProcOK            Success!
     *  @retval    eIasAudioProcInvalidParam  Invalid function pointer.
     *  @param[in] callback                   Pointer to the callback object.
     */
    //IasAudioProcessingResult announceCallback(IasMixerCallback* callback);

    /**
     *  @brief Set the balance for the specified input stream.
     *
     *  @return    Error code.
     *  @retval    eIasAudioProcOK            Success!
     *  @retval    eIasAudioProcInvalidParam  Unknown streamId.
     *  @param[in] streamId                   Id of the input stream, whose balance shall be set.
     *  @param[in] balanceLeft                Gain for the left channel (linear, not in dB).
     *  @param[in] balanceRight               Gain for the right channel (linear, not in dB).
     */
    IasAudioProcessingResult setBalance(int32_t streamId, float balanceLeft, float balanceRight);

    /**
     *  @brief Set the fader for the specified input stream.
     *
     *  @return    Error code.
     *  @retval    eIasAudioProcOK            Success!
     *  @retval    eIasAudioProcInvalidParam  Unknown streamId.
     *  @param[in] streamId                   Id of the input stream, whose balance shall be set.
     *  @param[in] faderFront                 Gain for the front channel (linear, not in dB).
     *  @param[in] faderRear                  Gain for the rear channel (linear, not in dB).
     */
    IasAudioProcessingResult setFader(int32_t streamId, float faderFront, float faderRear);

    /**
     *  @brief Set the input gain offset for the specified input stream.
     *
     *  @return    Error code.
     *  @retval    eIasAudioProcOK            Success!
     *  @retval    eIasAudioProcInvalidParam  Unknown streamId.
     *  @param[in] streamId                   Id of the input stream, whose balance shall be set.
     *  @param[in] gainOffset                 Gain offset for all channels of this input stream (linear, not in dB).
     */
    IasAudioProcessingResult setInputGainOffset(int32_t streamId, float gainOffset);

  private:

    /**
     *  @brief Send event helper method
     *
     *  @param[in] streamId                 Id of the input stream
     *  @param[out] properties
     */
    void sendEvent(int32_t streamId, IasProperties& properties);

    /**
     *  @brief Event for the balance finished
     *
     *  @param[in] streamId                 Id of the input stream
     *  @param[in] balanceLeft              the balance value for the left hand side, as linear value
     *  @param[in] balanceRight             the balance value for the right hand side, as linear value
     */
    void balanceFinishedEvent(int32_t streamId, float balanceLeft, float balanceRight);

    /**
     *  @brief Event for the fader finished
     *
     * @param[in] streamId                  Id of the input stream
     * @param[in] faderFront                the fader value for the front, as linear value
     * @param[in] faderRear                 the fader value for the rear, as linear value
     */
    void faderFinishedEvent(int32_t streamId, float faderFront, float faderRear);

    /**
     *  @brief Event for the input gain offset finished
     *
     *  @param[in] streamId                 Id of the input stream
     *  @param[in] gain                     gain the new gain, as linear value
     */
    void inputGainOffsetFinishedEvent(int32_t streamId, float gain);

    /**
     *  @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasMixerElementary(IasMixerElementary const& other) = delete;

    /**
     *  @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasMixerElementary& operator=(IasMixerElementary const& other) = delete;

    /**
     *  @brief Move constructor, private unimplemented to prevent misuse.
     */
    IasMixerElementary(IasMixerElementary&& other) = delete;

    /**
     *  @brief Move assignment operator, private unimplemented to prevent misuse.
     */
    IasMixerElementary& operator=(IasMixerElementary&& other) = delete;

    /**
     *  @brief Initialize stream parameters, like balance/fader/gainoffset states.
     *
     *  @return Error code.
     *  @retval eIasAudioProcOK               Success!
     *  @retval eIasAudioProcNotEnoughMemory  Memory allocation has failed.
     *  @param[in,out] params                 Parameters of the input stream.
     */
    IasAudioProcessingResult initStreamParams(IasMixerElementaryStreamParams* params);

    /**
     *  @brief Cleanup (delete) all stream parameters, which have been created by initStreamParams().
     *
     *  @param[in,out] params               Parameters of the input stream.
     */
    void cleanupStreamParams(IasMixerElementaryStreamParams* params);

    /**
     *  @brief Update the GainTile matrix for one input stream, whose
     *         balance/fader/gainoffset states are descibed by @a params,
     *         for an elementary mixer with a 2-channel output stream.
     *
     *  @param[in,out] params               Parameters of the input stream.
     *  @param[in] sampleIdx                Sample index
     *  @param[in] activeMultiChannelInput  flag to indicate if a multichannel input is configured
     */
    static void updateMatrix_2Channels(IasMixerElementaryStreamParams* params, uint32_t sampleIdx, bool activeMultiChannelInput);

    /**
     *  @brief Update the GainTile matrix for one input stream, whose
     *         balance/fader/gainoffset states are descibed by @a params,
     *         for an elementary mixer with a 4-channel output stream.
     *
     *  @param[in,out] params               Parameters of the input stream.
     *  @param[in] sampleIdx                Sample index
     *  @param[in] activeMultiChannelInput  flag to indicate if a multichannel input is configured
     */
    static void updateMatrix_4Channels(IasMixerElementaryStreamParams* params, uint32_t sampleIdx, bool activeMultiChannelInput);

    /**
     *  @brief Update the GainTile matrix for one input stream, whose
     *         balance/fader/gainoffset states are descibed by @a params,
     *         for an elementary mixer with a 6-channel output stream.
     *
     *  @param[in,out] params               Parameters of the input stream.
     *  @param[in] sampleIdx                Sample index
     *  @param[in] activeMultiChannelInput  flag to indicate if a multichannel input is configured
     */
    static void updateMatrix_6Channels(IasMixerElementaryStreamParams* params, uint32_t sampleIdx, bool activeMultiChannelInput );

    /**
     *  @brief Update the GainTile matrix for all the active input streams for given sample index
     *
     *  @param[in] sampleIdx  Sample index
     */
    void updateGainTileMatrix(uint32_t sampleIdx);

    /**
     *  @brief Update ramp active streams list (remove streams which have finished ramping, send finished events and update
     *  gain tine matrix with last ramp values)
     */
    void updateRampActiveStreams();

    /**
     *  @brief check the cmd queues ( tbb cncurrent queues ) if there are new cmd to process
     */
    void checkQueues();

    /**
     *  @brief this function triggers the new balance ramp ad sets all necessary parameters. Gets called if there is a queue entry.
     *
     *  @param[in] streamId                 identifier of the stream
     *  @param[in] left                     the attenuation for the left side
     *  @param[in] right                    the attenuation for the right side
     */
    void updateBalance(int32_t streamId, float left, float right);

    /**
     *  @brief this function triggers the new fader ramp ad sets all necessary parameters. Gets called if there is a queue entry.
     *
     *  @param[in] streamId                 identifier of the stream
     *  @param[in] front                    the attenuation for the rear
     *  @param[in] rear                     the attenuation for the front
     */
    void updateFader(int32_t streamId, float front, float rear);

    /**
     *  @brief this function triggers the new gainoffset ramp ad sets all necessary parameters. Gets called if there is a queue entry.
     *
     *  @param[in] streamId                 identifier of the stream
     *  @param[in] gainOffset               the gain that shall be applied to the stream
     */
    void updateGainOffset(int32_t streamId, float gainOffset);

    // Member variables
    IasStreamPair                                        mStreamPair;           //!< Defines the output stream and all input streams.
    uint32_t                                             mNumberInputChannels;  //!< Number of all input channels (of all input streams).
    uint32_t                                             mNumberOutputChannels; //!< Number of all output channels.
    uint32_t                                             mNumberInputBundles;   //!< Number of all input bundles.
    uint32_t                                             mNumberOutputBundles;  //!< Number of all output bundles.
    uint32_t                                             mFrameLength;          //!< Number of samples per audio frame.
    uint32_t                                             mSampleFreq;           //!< Sample rate [Hz]
    IasMixerElementaryGainTile                         **mGainTileMatrix;       //!< Matrix with gain tiles [mNumberOutputBundles][mNumberInputBundles]
    IasMixerElementaryStreamParamsMap                    mStreamParamsMap;      //!< Map with stream parameters
    IasEMixerList                                        mRampActiveStreams;    //!< List with the IDs of all streams that are currently ramping
    MatrixUpdateFuncPtr                                  mMatrixUpdateFunc;     //!< Function pointer to the relevant matrix update function.
    IasMixerElementaryChannelMappingVector               mOutputChannelMappingVector; //!< describes for each output channel in which bundle it can be found
    std::list<IasAudioChannelBundle*>                    mInputBundlesList;     //!< List of all input bundles
    std::list<IasAudioChannelBundle*>                    mOutputBundlesList;    //!< List of all output bundles
    bool                                                 multiChannelInputPresent; //!< flag to indicate if mixer has a multichannel (5.1) input.
    tbb::concurrent_queue<IasMixerBalanceQueueEntry>     mBalanceQueue; //!< queue for balance commands
    tbb::concurrent_queue<IasMixerFaderQueueEntry>       mFaderQueue;   //!< queue for fader commands
    tbb::concurrent_queue<IasMixerGainOffsetQueueEntry>  mGainOffsetQueue; //!< queue for gain offset commands
    const IasIGenericAudioCompConfig                    *mConfig;              //!< mConfig needed for events
    std::string                                          mTypeName;            //!< Name of the module type
    std::string                                          mInstanceName;        //!< Name of the module instance
    DltContext                                          *mLogContext;          //!< The log context for the mixer

}; // class IasMixerElementary

}  //namespace IasAudio

#endif /* IASMIXERELEMENTARY_HPP_ */
