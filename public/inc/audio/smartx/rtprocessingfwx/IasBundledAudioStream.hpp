/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasBundledAudioStream.hpp
 * @date   2012
 * @brief  The definition of the IasBundledAudioStream class.
 */

#ifndef IASBUNDLEDAUDIOSTREAM_HPP_
#define IASBUNDLEDAUDIOSTREAM_HPP_


#include "audio/smartx/rtprocessingfwx/IasProcessingTypes.hpp"
#include "audio/smartx/rtprocessingfwx/IasBaseAudioStream.hpp"

namespace IasAudio {

class IasBundleAssignment;
class IasBundledAudioStream;
class IasBundleSequencer;
class IasSimpleAudioStream;

/**
 * @brief A vector of IasBundledAudioStream to manage multiple audio streams.
 */
typedef std::vector<IasBundledAudioStream*> IasBundledAudioStreamVector;

/**
 * @class IasBundledAudioStream
 *
 * This class defines an audio stream. An audio stream is a combination of several
 * mono audio channels. Audio streams are used to route the data through the audio chain. A vector
 * of audio streams is provided to each audio processing component to process the data belonging to
 * an audio stream. Typically the data processing happens in-place, but there's also the possibility to have
 * modules that read from input streams and write to output streams or intermediate streams like e.g. a mixer.
 * The IasAudioStream class is also responsible to allocate the required memory for the channels audio data
 * which is delegated to a bundle sequencer (see IasBundleSequencer). A method is provided to write data
 * into the stream and another method is provided to read the data from the stream.
 */
class IAS_AUDIO_PUBLIC IasBundledAudioStream : public IasBaseAudioStream
{
  public:
    /**
     * @brief Constructor.
     *
     * @param[in] name The name of the audio stream.
     * @param[in] id The ID of the audio stream.
     * @param[in] numberChannels The number of channels of the audio stream.
     * @param[in] type The type of the audio stream.
     * @param[in] sidAvailable If this stream supports the additional SID value.
     */
    IasBundledAudioStream(const std::string &name,
                          int32_t id,
                          uint32_t numberChannels,
                          IasAudioStreamTypes type,
                          bool sidAvailable);
    /**
     * @brief Destructor, virtual by default.
     */
    virtual ~IasBundledAudioStream();

    /**
     * @brief Initializes the audio stream.
     *
     * In the init method all enhanced initialization is done like e.g. memory allocation.
     *
     * @param[in] bundleSequencer A pointer to the bundle sequencer to be used for allocating memory in form of an audio bundle.
     * @param[in] env The audio chain environment
     *
     * @returns The result indicating success (eIasAudioProcOK) or failure.
     */
    IasAudioProcessingResult init(IasBundleSequencer *bundleSequencer, IasAudioChainEnvironmentPtr env);

    /**
     * @brief Getter method for the bundle assignments of the audio stream.
     *
     * @returns The bundle assignments of the audio stream.
     */
    inline const IasBundleAssignmentVector& getBundleAssignments() const  { return mBundleAssignments; }

    /**
     * @brief Getter method for the audio frame of the audio stream.
     *
     * The audio frame is a vector with pointers to the start of each single audio channel
     * inside a bundle. E.g. if a stream has two channels than the audio frame of this stream contains two pointers.
     * The pointers returned are simply added at the end of the audio frame given to this method as parameter, so you
     * can collect one single audio frame consisting of pointers belonging to several different streams.
     *
     * @param[out] audioFrame An audio frame that is extended by the channel pointers belonging to this stream.
     */
    void getAudioFrame(IasAudioFrame *audioFrame);

    /**
     * @brief Write (copy) data into the stream, i.e. into the bundle(s) of the stream.
     *
     * The audio frame given as parameter contains pointers to channels with non-interleaved audio samples.
     * They are interleaved into the audio channel bundle(s) of the stream in the order defined by the audio
     * frame.
     *
     * @param[in] audioFrame A vector with pointers to the audio channels to be copied into the streams memory buffer.
     *
     * @returns The result indicating success (eIasAudioProcOK) or failure.
     */
    IasAudioProcessingResult writeFromNonInterleaved(const IasAudioFrame &audioFrame);

    /**
     * @brief Write (copy) data into the stream, i.e. into the bundle(s) of the stream.
     *
     * The audio frame given as parameter contains pointers to channels with interleaved audio samples.
     * They are interleaved into the audio channel bundle(s) of the stream in the order defined by the audio
     * frame.
     *
     * @param[in] audioFrame A vector with pointers to the audio channels to be copied into the streams memory buffer.
     *
     * @returns The result indicating success (eIasAudioProcOK) or failure.
     */
    IasAudioProcessingResult writeFromInterleaved(const IasAudioFrame &audioFrame);

    /**
     * @brief Read (copy) data from the stream to the pointers given by the audio frame.
     *
     * The audio frame given as parameter has to provide pointers to memory buffers that can hold
     * frame length * sample size per channel.
     *
     * @param[in,out] audioFrame Pointers to data buffers were the audio samples shall be copied to.
     *
     * @returns The result indicating success (eIasAudioProcOK) or failure.
     */
    IasAudioProcessingResult read(const IasAudioFrame &audioFrame) const;

    /**
     * @brief Read stream data and return it interleaved.
     *
     * Reads the data and writes it interleaved to the provided memory area.
     *
     * @param[out] audioFrame Pointer to the memory were the interleaved data should be written.
     *
     * @returns The result indicating success (eIasAudioProcOK) or failure.
     */
    IasAudioProcessingResult read(float *audioFrame) const;

    /**
     * @brief Provide non-interleaved representation of stream.
     *
     * This method will convert the current representation of the stream into the
     * requested one or does nothing if the current representation matches the
     * requested representation. The given pointer has to point to an existing valid
     * IasSimpleAudioStream instance which will be filled with the samples of
     * the current representation.
     *
     * @param[in,out] nonInterleaved Pointer to non-interleaved representation
     */
    virtual void asNonInterleavedStream(IasSimpleAudioStream *nonInterleaved);

    /**
     * @brief Provide interleaved representation of stream.
     *
     * This method will convert the current representation of the stream into the
     * requested one or does nothing if the current representation matches the
     * requested representation. The given pointer has to point to an existing valid
     * IasSimpleAudioStream instance which will be filled with the samples of
     * the current representation.
     *
     * @param[in,out] interleaved Pointer to interleaved representation
     */
    virtual void asInterleavedStream(IasSimpleAudioStream *interleaved);

    /**
     * @brief Provide bundled representation of stream.
     *
     * This method will convert the current representation of the stream into the
     * requested one or does nothing if the current representation matches the
     * requested representation. The given pointer has to point to an existing valid
     * IasSimpleAudioStream instance which will be filled with the samples of
     * the current representation.
     *
     * @param[in,out] bundled Pointer to bundled representation
     */
    virtual void asBundledStream(IasBundledAudioStream *bundled);

    /**
     * @brief Copy the samples to the location referenced by outAudioFrame.
     *
     * This method copies the audio samples of the stream to the location referenced
     * by outAudioFrame. outAudioFrame has to contain valid pointers to non-interleaved
     * audio buffers, one buffer for each channel.
     *
     * @param[in,out] outAudioFrame Audio frame containing pointers to the destination of the audio samples.
     */
    virtual void copyToOutputAudioChannels(const IasAudioFrame &outAudioFrame) const;

    /**
     * @brief Get access to the internal channel buffers of the audio stream.
     *
     * @param[out] audioFrame  Frame of pointers that shall be set to the channel buffers (one pointer for each channel).
     *                         When calling this function, the audioFrame must already have the correct size
     *                         (according to the number of channels).
     * @param[out] stride      Stride between two samples, expressed in samples.
     *                         For streams with bundled layout, the stride is always 4.
     */
    virtual IasAudioProcessingResult getAudioDataPointers(IasAudioFrame &audioFrame, uint32_t *stride) const;

  private:
    /**
     * @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasBundledAudioStream(IasBundledAudioStream const &other);
    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasBundledAudioStream& operator=(IasBundledAudioStream const &other);

    // Member variables
    IasBundleAssignmentVector       mBundleAssignments;   //!< A vector containing the bundle assignments of the audio stream.
    IasAudioFrame                   mAudioFrame;          //!< The audio frame containing a pointer to the start of every single audio channel inside a bundle.
    IasAudioChainEnvironmentPtr     mEnv;                 //!< The audio chain environment
};

} //namespace IasAudio

#endif /* IASBUNDLEDAUDIOSTREAM_HPP_ */
