/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasBaseAudioStream.hpp
 * @date   2013
 * @brief
 */

#ifndef IASBASEAUDIOSTREAM_HPP_
#define IASBASEAUDIOSTREAM_HPP_

#include "audio/smartx/rtprocessingfwx/IasProcessingTypes.hpp"


namespace IasAudio {

class IasBaseAudioStream;
class IasSimpleAudioStream;
class IasBundledAudioStream;
class IasUndefinedStream;
class IasBundleSequencer;


class IAS_AUDIO_PUBLIC IasBaseAudioStream
{
  public:
    /**
     * @brief The audio stream types.
     */
    enum IasAudioStreamTypes
    {
      eIasAudioStreamInput,
      eIasAudioStreamOutput,
      eIasAudioStreamIntermediate
    };

    /**
     * @brief Describes the layout of the samples in the buffer.
     */
    enum IasSampleLayout
    {
      eIasUndefined,              //!< Not defined yet
      eIasNonInterleaved,         //!< Non-interleaved sample layout, i.e. stride = 1
      eIasInterleaved,            //!< Interleaved sample layout, i.e. stride = number of channels
      eIasBundled,                //!< Interleaved in groups with four channels each buffer, i.e. stride = 4
    };

    /**
     * @brief Constructor.
     *
     * @param[in] name The name of the audio stream.
     * @param[in] id The ID of the audio stream.
     * @param[in] numberChannels The number of channels of the audio stream.
     * @param[in] type The type of the audio stream.
     * @param[in] sidAvailable If this stream supports the additional SID value.
     */
    IasBaseAudioStream(const std::string &name,
                   int32_t id,
                   uint32_t numberChannels,
                   IasAudioStreamTypes type,
                   bool sidAvailable);

    /**
     * @brief Destructor, virtual by default.
     */
    virtual ~IasBaseAudioStream();

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
    inline int32_t getId() const { return mId; }

    /**
     * @brief Getter method for the Sid of the audio stream.
     *
     * @returns The sid assigned to the audio stream.
     */
    inline uint32_t getSid() const { return mSid; }

    /**
     * @brief Setter method for the Sid of the audio stream.
     *
     * @param[in] newSid  The Sid that shall be assigned to the audio stream.
     */
    inline void setSid(uint32_t newSid) { mSid = newSid; }

    /**
     * @brief Getter method for the number of channels of the audio stream.
     *
     * @returns The number of channels of the audio stream.
     */
    inline uint32_t getNumberChannels() const { return mNumberChannels; }

    /**
     * @brief Getter method for the type of the stream.
     *
     * @returns The type of the stream.
     */
    inline IasAudioStreamTypes getType() const { return mType; }

    /**
     * @brief Getter method for the sample layout of the stream.
     *
     * @returns The sample layout of the stream, see #IasSampleLayout.
     */
    inline IasSampleLayout getSampleLayout() const { return mSampleLayout; }

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
    virtual void asNonInterleavedStream(IasSimpleAudioStream *nonInterleaved) = 0;

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
    virtual void asInterleavedStream(IasSimpleAudioStream *interleaved) = 0;

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
    virtual void asBundledStream(IasBundledAudioStream *bundled) = 0;

    /**
     * @brief Copy the samples to the location referenced by outAudioFrame.
     *
     * This method copies the audio samples of the stream to the location referenced
     * by outAudioFrame. outAudioFrame has to contain valid pointers to non-interleaved
     * audio buffers, one buffer for each channel.
     *
     * @param[in,out] outAudioFrame Audio frame containing pointers to the destination of the audio samples.
     */
    virtual void copyToOutputAudioChannels(const IasAudioFrame &outAudioFrame) const = 0;

    /**
     * @brief Get access to the internal channel buffers of the audio stream.
     *
     * @param[out] audioFrame  Frame of pointers that shall be set to the channel buffers (one pointer for each channel).
     *                         When calling this function, the audioFrame must already have the correct size
     *                         (according to the number of channels).
     * @param[out] stride      Stride between two samples, expressed in samples.
     */
    virtual IasAudioProcessingResult getAudioDataPointers(IasAudioFrame &audioFrame, uint32_t *stride) const = 0;

  protected:
    // Member variables
    std::string                     mName;                //!< The name of the audio stream.
    int32_t                      mId;                  //!< The ID of the audio stream.
    uint32_t                     mNumberChannels;      //!< The number of channels of the audio stream.
    IasAudioStreamTypes             mType;                //!< The type of the audio stream.
    bool                       mSidAvailable;        //!< Boolean if a stream supports sid handling.
    uint32_t                     mSid;                 //!< Member to store the Stream Information data of a multichannel stream.
    IasSampleLayout                 mSampleLayout;        //!< The sample layout of this stream.

  private:
    /**
     * @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasBaseAudioStream(IasBaseAudioStream const &other);

    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasBaseAudioStream& operator=(IasBaseAudioStream const &other);
};

} //namespace IasAudio

#endif /* IASBASEAUDIOSTREAM_HPP_ */
