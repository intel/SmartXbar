/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasAudioStream.hpp
 * @date   2013
 * @brief
 */

#ifndef IASAUDIOSTREAM_HPP_
#define IASAUDIOSTREAM_HPP_

#include "internal/audio/common/IasAudioLogging.hpp"
#include "audio/smartx/rtprocessingfwx/IasBaseAudioStream.hpp"


namespace IasAudio {

class IasBaseAudioStream;
class IasBundledAudioStream;
class IasSimpleAudioStream;
class IasAudioStream;
class IasAudioDevice;

/**
 * @brief A vector of IasAudioStream to manage multiple audio streams.
 */
using IasAudioStreamVector = std::vector<IasAudioStream*>;

/**
 * @brief A list of IasGenericAudioStream to manage multiple audio streams.
 */
using IasStreamPointerList = std::list<IasAudioStream*>;

/**
 * @brief Map one audio stream to multiple audio streams.
 *
 * This is used for the output stream to input streams mapping.
 */
using IasStreamMap = std::map<IasAudioStream*, IasStreamPointerList>;

/**
 * @brief A pair to combine one audio stream with multiple audio streams.
 */
using IasStreamPair = std::pair<IasAudioStream*, IasStreamPointerList>;


class IAS_AUDIO_PUBLIC IasAudioStream
{
  public:
    /**
     * @brief Constructor.
     */
    IasAudioStream(const std::string &name,
                   std::int32_t id,
                   std::uint32_t numberChannels,
                   IasBaseAudioStream::IasAudioStreamTypes type,
                   bool sidAvailable);

    /**
     * @brief Destructor, virtual by default.
     */
    virtual ~IasAudioStream();

    /**
     * @brief Initializes the audio stream.
     *
     * In the init method all enhanced initialization is done like e.g. memory allocation.
     *
     * @param[in] env The audio chain environment
     *
     * @returns The result indicating success (eIasAudioProcOK) or failure.
     */
    IasAudioProcessingResult init(IasAudioChainEnvironmentPtr env);

    /**
     * @brief Set the bundle sequencer to be used for bundled audio streams.
     *
     * The bundle sequencer is required to allocate memory when using bundled audio streams.
     *
     * @param[in] bundleSequencer A pointer to the bundle sequencer to be used for allocating memory in form of an audio bundle.
     */
    inline void setBundleSequencer(IasBundleSequencer *bundleSequencer) { mBundleSequencer = bundleSequencer; }

    /**
     * @brief Getter method for the name of the audio stream.
     *
     * @returns The name of the audio stream.
     */
    inline const std::string& getName() const { return mName; }

    /**
     * @brief Getter method for the ID of the audio stream.
     *
     * @returns The ID of the audio stream.
     */
    inline std::int32_t getId() const { return mId; }

    /**
     * @brief Getter method for the number of channels of the audio stream.
     *
     * @returns The number of channels of the audio stream.
     */
    inline std::uint32_t getNumberChannels() const { return mNumberChannels; }

    /**
     * @brief Getter method for the type of the stream.
     *
     * @returns The type of the stream.
     */
    inline IasBaseAudioStream::IasAudioStreamTypes getType() const { return mType; }

    /**
     * @brief Getter method for the sample layout of the stream.
     *
     * @returns The sample layout of the stream, see #IasBaseAudioStream::IasSampleLayout.
     */
    IasBaseAudioStream::IasSampleLayout getSampleLayout() const;

    /**
     * @brief Return pointer to input audio frame.
     *
     * This method returns a pointer to the input audio frame. The input audio frame
     * has to be filled with pointers to non-interleaved audio channels, one pointer
     * for each channel, where the initial audio samples shall be copied from into
     * the audio stream.
     *
     * @returns A pointer to the input audio frame.
     */
    inline IasAudioFrame* getInputAudioFrame() { return &mAudioFrameIn; }

    /**
     * @brief Return pointer to output audio frame.
     *
     * This method returns a pointer to the output audio frame. The output audio frame
     * has to be filled with pointers to non-interleaved audio channels, one pointer
     * for each channel, where the final audio samples shall be copied to from
     * the audio stream.
     *
     * @returns A pointer to the output audio frame.
     */
    inline IasAudioFrame* getOutputAudioFrame() { return &mAudioFrameOut; }

    /**
     * @brief Getter method for the Sid of the audio stream.
     *
     * @returns The sid assigned to the audio stream.
     */
    std::uint32_t getSid() const;

    /**
     * @brief Setter method for the Sid of the audio stream.
     *
     * @param[in] newSid  The Sid that shall be assigned to the audio stream.
     */
    void setSid(std::uint32_t newSid);

    /**
     * @brief Provide non-interleaved representation of stream.
     *
     * This method will return a pointer to the non-interleaved representation of the stream.
     * It also triggers the transformation from one stream representation to another in case
     * the previous representation was not the requested one.
     *
     * @returns A pointer to the requested stream representation.
     */
    IasSimpleAudioStream* asNonInterleavedStream();

    /**
     * @brief Provide interleaved representation of stream.
     *
     * This method will return a pointer to the interleaved representation of the stream.
     * It also triggers the transformation from one stream representation to another in case
     * the previous representation was not the requested one.
     *
     * @returns A pointer to the requested stream representation.
     */
    IasSimpleAudioStream* asInterleavedStream();

    /**
     * @brief Provide bundled representation of stream.
     *
     * This method will return a pointer to the bundled representation of the stream.
     * It also triggers the transformation from one stream representation to another in case
     * the previous representation was not the requested one.
     *
     * @returns A pointer to the requested stream representation.
     */
    IasBundledAudioStream* asBundledStream();

    /**
     * @brief Trigger the copying from the input audio buffers into the stream.
     *
     * This method has to be called on every input stream before the audio processing can be done.
     * It will trigger the copying of the audio samples from the non-interleaved input audio buffers
     * into the stream. The input audio buffers have to be set previously by a call to #getInputAudioFrame.
     */
    void copyFromInputAudioChannels();

    /**
     * @brief Copy the audio samples from inside this stream to the output audio buffers.
     *
     * This method copies the audio samples from inside the current representation of this
     * stream to the output audio buffers that have to be previously set by a call to
     * #getOutputAudioFrame.
     */
    void copyToOutputAudioChannels();

    /**
     * @brief Get a frame with pointers to the internal channel buffers of the audio stream.
     *
     * The internal buffers can be based on interleaved, non-interleaved, or bundled layout.
     * Therefore, in addition to the base pointers for each channel, this function returns
     * the stride between two samples.
     *
     * @param[out] audioFrame  Returns a pointer to the audio frame. The audio frame is a vector
     *                         of the channel base pointers (one pointer for each channel).
     * @param[out] stride      Distance between two samples of one channel, expressed in samples.
     */
    void getAudioDataPointers(IasAudioFrame **audioFrame, std::uint32_t *stride);

    /**
     * @brief Get a frame with pointers to the internal channel buffers of the audio stream.
     *
     * The internal buffers can be based on interleaved, non-interleaved, or bundled layout.
     *
     * @param[in,out] audioFrame  Reference to the audio frame. The audio frame is a vector
     *                            of the channel base pointers (one pointer for each channel).
     */
    void getAudioDataPointers(IasAudioFrame& audioFrame);

    /**
     * @brief set the connected device for the stream
     *
     * @param device the pointer to the device
     */
    void setConnectedDevice(IasAudioDevice *device);

    /**
     * @brief return the connected device
     *
     * @returns pointer to the connected device
     */
    inline IasAudioDevice* getConnectedDevice(){return mConnectedDevice;}

    /**
     * @brief remove the connected source
     *
     */
    void removeConnectedDevice();

  private:
    /**
     * @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasAudioStream(IasAudioStream const &other);

    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasAudioStream& operator=(IasAudioStream const &other);

    /**
     * @brief Get the non-interleaved representation of the audio stream.
     *
     * @returns A pointer to the non-interleaved representation of the stream.
     */
    IasSimpleAudioStream* getNonInterleavedStream();

    /**
     * @brief Get the interleaved representation of the audio stream.
     *
     * @returns A pointer to the interleaved representation of the stream.
     */
    IasSimpleAudioStream* getInterleavedStream();

    /**
     * @brief Get the bundled representation of the audio stream.
     *
     * @returns A pointer to the bundled representation of the stream.
     */
    IasBundledAudioStream* getBundledStream();

    /**
     * @brief Update the SID value of the current representation of the stream
     */
    void updateCurrentSid();

    // Member variables
    std::string                               mName;                    //!< The name of the audio stream.
    std::int32_t                              mId;                      //!< The ID of the audio stream.
    std::uint32_t                             mNumberChannels;          //!< The number of channels of the audio stream.
    IasBaseAudioStream::IasAudioStreamTypes   mType;                    //!< The type of the audio stream.
    bool                                      mSidAvailable;            //!< Boolean if a stream supports sid handling.
    IasAudioFrame                             mAudioFrameIn;            //!< The input audio frame to read the data from.
    IasAudioFrame                             mAudioFrameOut;           //!< The output audio frame to write the data to.
    IasAudioFrame                             mAudioFrameInternal;      //!< The audio frame used by getAudioDataPointers method: provides pointers to the internal buffers.
    bool                                      mCopyFromInput;           //!< Flag to indicate if a copy from the input audio channels has to happen or not.
    IasBundleSequencer                       *mBundleSequencer;         //!< The bundle sequencer to allocate memory for bundled audio streams.
    IasBundledAudioStream                    *mBundled;                 //!< A pointer to the bundled audio stream representation.
    IasSimpleAudioStream                     *mNonInterleaved;          //!< A pointer to the non-interleaved audio stream representation.
    IasSimpleAudioStream                     *mInterleaved;             //!< A pointer to the interleaved audio stream representation.
    IasBaseAudioStream                       *mCurrentRepresentation;   //!< A pointer to the current representation.
    IasAudioDevice                           *mConnectedDevice;         //!< A pointer to the connected device, NULL if no device is connected
    DltContext                               *mLogContext;              //!< The log context.
    IasAudioChainEnvironmentPtr               mEnv;                     //!< The audio chain environment
    std::uint32_t                             mLogCounterGADP1;         //!< The log counter for method getAudioDataPointers first variant
    std::uint32_t                             mLogCounterGADP2;         //!< The log counter for method getAudioDataPointers second variant
};

} //namespace IasAudio

#endif /* IASAUDIOSTREAM_HPP_ */
