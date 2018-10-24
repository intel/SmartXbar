/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasSimpleAudioStream.hpp
 * @date   2013
 * @brief  The definition of the IasSimpleAudioStream class.
 */

#ifndef IASSIMPLEAUDIOSTREAM_HPP_
#define IASSIMPLEAUDIOSTREAM_HPP_

#include "audio/smartx/rtprocessingfwx/IasBaseAudioStream.hpp"
#include "audio/smartx/rtprocessingfwx/IasProcessingTypes.hpp"



namespace IasAudio {

class IasSimpleAudioStream;
class IasAudioBuffer;

/**
 * @class IasSimpleAudioStream
 *
 * This class defines an audio stream. An audio stream is a combination of several
 * mono audio channels. Audio streams are used to route the data through the audio chain. A vector
 * of audio streams is provided to each audio processing component to process the data belonging to
 * an audio stream.
 */
class IAS_AUDIO_PUBLIC IasSimpleAudioStream : public IasBaseAudioStream
{
  public:
    /**
     * @brief Constructor.
     */
    IasSimpleAudioStream();

    /**
     * @brief Destructor, virtual by default.
     */
    virtual ~IasSimpleAudioStream();

    /*
     * @brief Set the properties of this audio stream.
     *
     * @param[in] name The name of the audio stream.
     * @param[in] id The ID of the audio stream.
     * @param[in] numberChannels The number of channels of the audio stream.
     * @param[in] sampleLayout The layout of the samples in the buffer. See #IasSampleLayout.
     * @param[in] frameLength The number of frames of one buffer.
     */
    IasAudioProcessingResult setProperties(const std::string &name,
                                           int32_t id,
                                           uint32_t numberChannels,
                                           IasSampleLayout sampleLayout,
                                           uint32_t frameLength,
                                           IasAudioStreamTypes type,
                                           bool sidAvailable);

    /**
     * @brief Return all buffers back to pool
     */
    void cleanup();

    /**
     * @brief Get a reference to a vector with pointers to all single audio channels.
     *
     * @returns A vector with pointers to all single audio channels.
     */
    const IasAudioFrame& getAudioBuffers() const { return mAudioFrame; }

    /**
     * @brief Write (copy) data into the stream channel buffers.
     *
     * The audio frame given as parameter contains pointers to channels with non-interleaved audio samples.
     * @note: This method may only be called the first time each processing cycle when the data is copied initially
     * to the stream buffer.
     *
     * @param[in] audioFrame A vector with pointers to the audio channels to be copied into the streams memory buffer.
     *
     * @returns The result indicating success (eIasAudioProcOK) or failure.
     */
    IasAudioProcessingResult writeFromNonInterleaved(const IasAudioFrame &audioFrame);

    /**
     * @brief Write (copy) data into the stream channel buffers.
     *
     * The audio frame given as parameter contains pointers to channels with bundled audio samples.
     *
     * @param[in] audioFrame A vector with pointers to the audio channels to be copied into the streams memory buffer.
     *
     * @returns The result indicating success (eIasAudioProcOK) or failure.
     */
    IasAudioProcessingResult writeFromBundled(const IasAudioFrame &audioFrame);

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
     */
    virtual IasAudioProcessingResult getAudioDataPointers(IasAudioFrame &audioFrame, uint32_t *stride) const;


  protected:
    // Member variables
    IasAudioFrame                   mAudioFrame;          //!< A vector with pointers to every single audio channel.
    uint32_t                     mFrameLength;         //!< The frame length for one audio channel buffer in number of samples.
    IasAudioBuffer                 *mBuffer;              //!< A pointer to the used audio buffer from the buffer pool.

  private:
    /**
     * @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasSimpleAudioStream(IasSimpleAudioStream const &other);
    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasSimpleAudioStream& operator=(IasSimpleAudioStream const &other);

};

} //namespace IasAudio

#endif /* IASSIMPLEAUDIOSTREAM_HPP_ */
