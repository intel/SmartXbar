/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasAudioChain.hpp
 * @date   2012
 * @brief  The definition of the IasAudioChain class.
 */

#ifndef IASAUDIOCHAIN_HPP_
#define IASAUDIOCHAIN_HPP_

#include "internal/audio/common/IasAudioLogging.hpp"
#include "rtprocessingfwx/IasAudioChainEnvironment.hpp"
#include "rtprocessingfwx/IasBundleSequencer.hpp"


#include "audio/smartx/rtprocessingfwx/IasProcessingTypes.hpp"
#include "audio/smartx/rtprocessingfwx/IasAudioStream.hpp"
#include "audio/smartx/rtprocessingfwx/IasGenericAudioComp.hpp"
#include "audio/smartx/rtprocessingfwx/IasGenericAudioCompCore.hpp"

namespace IasAudio {

/**
 * @class IasAudioChain
 *
 * This class defines an audio processing chain. An audio processing chain consists of several
 * audio processing components, several input audio streams, several output audio streams and a common
 * set of parameters gathered in the IasAudioChainEnvironment class.
 */
class IAS_AUDIO_PUBLIC IasAudioChain
{
  public:
    /**
     * @brief The result type for the IasAudioChain methods
     */
    enum IasResult
    {
      eIasOk,                     //!< Operation successful
      eIasFailed                  //!< Operation failed
    };

    /**
     * @brief The initialization parameters
     */
    struct IasInitParams
    {
      //! @brief The default constructor
      IasInitParams()
        :periodSize(0)
        ,sampleRate(0)
      {}

      //! @brief Constructor for initializer list
      IasInitParams(uint32_t p_periodSize, uint32_t p_sampleRate)
        :periodSize(p_periodSize)
        ,sampleRate(p_sampleRate)
      {}

      uint32_t periodSize;   //!< The period size in frames
      uint32_t sampleRate;   //!< The sample rate in Hz
    };

    /**
     * @brief Constructor.
     */
    IasAudioChain();

    /**
     * @brief Destructor, virtual by default.
     */
    virtual ~IasAudioChain();

    /**
     * @brief Initialize the audio chain
     *
     * This method has to be called to initialize the audio chain before using it.
     *
     * @param[in] params The initialization params
     */
    IasResult init(const IasInitParams &params);

    /**
     * @brief Create an input audio stream.
     *
     * This method is used to create an input audio stream for the audio chain. An input audio stream is
     * typically mapped to an output zone via the mixer mapping which is defined during the configuration
     * by calling the method addStreamMapping of the IasGenericAudioCompConfig class.
     *
     * @param[in] name The name of the stream.
     * @param[in] id The ID of the stream which is used by the IasAudioRouting and IasAudioProcessing interfaces.
     * @param[in] numberChannels The number of channels of the stream.
     * @param[in] sidAvailable True if the stream contains a stream info data channel and false if not.
     *
     * @returns A pointer to the newly created stream.
     */
    IasAudioStream* createInputAudioStream(const std::string& name, int32_t id, int32_t numberChannels, bool sidAvailable);

    /**
     * @brief Create an output audio stream.
     *
     * This method is used to create an output audio stream for the audio chain. An output audio stream
     * typically defines an output zone like e.g. the main cabin.
     *
     * @param[in] name The name of the stream.
     * @param[in] id The ID of the stream.
     * @param[in] numberChannels The number of channels of the stream.
     * @param[in] sidAvailable True if the stream contains a stream info data channel and false if not.
     *
     * @returns A pointer to the newly created stream.
     */
    IasAudioStream* createOutputAudioStream(const std::string& name, int32_t id, int32_t numberChannels, bool sidAvailable);

    /**
     * @brief Create an intermediate input audio stream.
     *
     * This method is used to create an intermediate input audio stream for the audio chain. An intermediate audio stream
     * is used to connect two mixer-like components.
     *
     * @param[in] name The name of the stream.
     * @param[in] id The ID of the stream.
     * @param[in] numberChannels The number of channels of the stream.
     * @param[in] sidAvailable True if the stream contains a stream info data channel and false if not.
     *
     * @returns A pointer to the newly created stream.
     */
    IasAudioStream* createIntermediateInputAudioStream(const std::string& name, int32_t id, int32_t numberChannels, bool sidAvailable);

    /**
     * @brief Create an intermediate output audio stream.
     *
     * This method is used to create an intermediate output audio stream for the audio chain. An intermediate audio stream
     * is used to connect two mixer-like components.
     *
     * @param[in] name The name of the stream.
     * @param[in] id The ID of the stream.
     * @param[in] numberChannels The number of channels of the stream.
     * @param[in] sidAvailable True if the stream contains a stream info data channel and false if not.
     *
     * @returns A pointer to the newly created stream.
     */
    IasAudioStream* createIntermediateOutputAudioStream(const std::string& name, int32_t id, int32_t numberChannels, bool sidAvailable);

    /**
     * @brief Add an audio component to the processing chain.
     *
     * Add a new audio component to the processing chain. The order in which the audio components are added
     * defines the order in which they are called during execution of the processing chain.
     *
     * @param[in] newAudioComponent A pointer to an audio component like e.g. a mixer or an equalizer.
     */
    void addAudioComponent(IasGenericAudioComp *newAudioComponent);

    /**
     * @brief Get an vector containing all audio components added to the chain.
     *
     * @returns A vector with all audio components in the audio chain.
     */
    const IasGenericAudioCompVector& getAudioComponents() const { return mComps; }

    /**
     * @brief Execute the processing chain.
     *
     * This method iterates over all audio processing components and executes their processing function if
     * the module is enabled.
     */
    void process() const;

    /**
     * @brief Clear the audio buffers of all output stream bundles.
     *
     * This method clears all audio buffers allocated and used by output streams to zero.
     */
    void clearOutputBundleBuffers() const;

    /**
     * @brief Getter method for all configured input streams of the audio chain.
     *
     * @returns All configured input streams.
     */
    const IasAudioStreamVector& getInputStreams() const { return mInputAudioStreams; }

    /**
     * @brief Getter method for all configured output streams of the audio chain.
     *
     * @returns All configured output streams.
     */
    const IasAudioStreamVector& getOutputStreams() const { return mOutputAudioStreams; }

    /**
     * @brief Getter method for all configured intermediate input streams of the audio chain.
     *
     * @returns All configured intermediate streams.
     */
    const IasAudioStreamVector& getIntermediateInputStreams() const { return mIntermediateInputAudioStreams; }

    /**
     * @brief Getter method for all configured intermediate output streams of the audio chain.
     *
     * @returns All configured intermediate streams.
     */
    const IasAudioStreamVector& getIntermediateOutputStreams() const { return mIntermediateOutputAudioStreams; }

    /**
     * @brief Getter method for an output stream associated with the given zone ID
     *
     * @param[in] zoneId  The zone ID of the output audio stream
     *
     * @returns A pointer to the output audio stream associated with the given zone ID or NULL.
     */
    const IasAudioStream* getOutputStream(int32_t zoneId) const;

  private:
    /**
     * @brief Maps the zone ID to an output stream
     */
    using IasZoneIdOutputStreamMap = std::map<int32_t, IasAudioStream*>;

    /**
     * @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasAudioChain(IasAudioChain const &other);
    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasAudioChain& operator=(IasAudioChain const &other);

    // Member variables
    IasBundleSequencer              mInputBundleSequencer;              //!< The input bundle sequencer for all bundles in front of the mixer
    IasBundleSequencer              mOutputBundleSequencer;             //!< The output bundle sequencer for all bundles after the mixer
    IasBundleSequencer              mIntermediateInputBundleSequencer;  //!< The intermediate input bundle sequencer for all bundles between two mixer-like instances
    IasBundleSequencer              mIntermediateOutputBundleSequencer; //!< The intermediate output bundle sequencer for all bundles between two mixer-like instances
    IasAudioStreamVector            mInputAudioStreams;                 //!< A vector containing all input audio streams
    IasAudioStreamVector            mOutputAudioStreams;                //!< A vector containing all output audio streams
    IasAudioStreamVector            mIntermediateInputAudioStreams;     //!< A vector containing all intermediate input audio streams
    IasAudioStreamVector            mIntermediateOutputAudioStreams;    //!< A vector containing all intermediate output audio streams
    IasGenericAudioCompVector       mComps;                             //!< A vector containing all audio components
    IasGenericAudioCompCoreVector   mCompCores;                         //!< A vector containing a reference to the core for easier access
    IasZoneIdOutputStreamMap        mZoneIdOutputStreamMap;             //!< Map with Zone ID -> Output Stream
    DltContext                     *mLog;                               //!< The log context for rtprocessingfw
    IasAudioChainEnvironmentPtr     mEnv;                               //!< The audio chain environment parameters
};

/**
 * @brief Function to get a IasAudioChain::IasResult as string.
 *
 * @return Enum Member
 */
std::string toString(const IasAudioChain::IasResult& type);


} //namespace IasAudio

#endif /* IASAUDIOCHAIN_HPP_ */
