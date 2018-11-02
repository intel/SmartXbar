/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasAudioChannelBundle.hpp
 * @date   2012
 * @brief  The definition of the IasAudioChannelBundle class.
 */

#ifndef IASAUDIOCHANNELBUNDLE_HPP_
#define IASAUDIOCHANNELBUNDLE_HPP_

#include "audio/smartx/rtprocessingfwx/IasProcessingTypes.hpp"
#include "avbaudiomodules/internal/audio/common/IasAudioLogging.hpp"

namespace IasAudio {

static const uint32_t cIasNumChannelsPerBundle = 4;    //!< The maximum number of channels in one bundle.

/**
 * @class IasAudioChannelBundle
 *
 * An audio channel bundle provides the data buffer for four interleaved audio data channels
 * with 32-bit floating point format. This data layout provides the best optimization possibilities for SIMD
 * operations. The class provides mechanisms to reserve channels, interleave data into the bundle, deinterleave
 * data out of the bundle and get a pointer to directly access the audio data.
 */
class IAS_AUDIO_PUBLIC IasAudioChannelBundle
{
  public:
    /**
     * @brief Constructor.
     */
    IasAudioChannelBundle(uint32_t FrameLength);

    /**
     * @brief Destructor, virtual by default.
     */
    virtual ~IasAudioChannelBundle();

    /**
     * @brief Initializes an audio bundle.
     *
     * This method is used to allocate memory for the audio data
     * carried by the bundle. This function must be called before a bundle
     * is used.
     *
     * @returns The result indicating success (eIasAudioCompOK) or failure (eIasAudioCompInitializationFailed)
     */
    IasAudioProcessingResult init();

    /**
     * @brief Resets an audio bundle.
     *
     * This method is used to reset the internal states of an audio bundle.
     */
    void reset();

    /**
     * @brief Clear the complete audio buffer of this bundle.
     *
     * This method clears ALL samples of all four channels inside the bundle to zero.
     */
    void clear();

    /**
     * @brief Return a pointer to the audio data memory.
     *
     * @returns Returns the pointer to mAudioData if the audio channel bundle is initialized or NULL if audio channel bundle is not initialized.
     */
    inline float* getAudioDataPointer() const { return mAudioData; }

    /**
     * @brief Gets the number of free audio within the bundle.
     *
     * @returns Returns the free audio channels within the bundle.
     */
    inline uint32_t getNumFreeChannels() const { return mNumFreeChannels; }

    /**
     * @brief Gets the number of used audio within the bundle.
     *
     * @returns Returns the used audio channels within the bundle.
     */
    inline uint32_t getNumUsedChannels() const { return cIasNumChannelsPerBundle-mNumFreeChannels; }

    /**
     * @brief Reserves a number of channels in this bundle.
     *
     * The number of channels that will be reserved are all sequentially ordered
     *
     * @param[in] numberChannels The number of channels to reserve.
     * @param[out] startIndex The index of the first reserved channel inside the bundle.
     *
     * @returns The result indication success (eIasAudioProcOK) or an error when no space is left (eIasAudioProcNoSpaceLeft).
     */
    IasAudioProcessingResult reserveChannels(uint32_t numberChannels, uint32_t* startIndex);

    /**
     * @brief Interleaves one channel into the bundle ('Bundling').
     *
     * The originating data is in non-interleaved order and will be interleaved into the bundle.
     *
     * @param[in] offset The offset in the bundle were the channel shall be placed.
     * @param[in] channel A pointer to the samples in non-interleaved order to be copied into the bundle.
     */
    void writeOneChannelFromNonInterleaved(uint32_t offset, const float *channel);

    /**
     * @brief Interleaves two channels into the bundle ('Bundling').
     *
     * The originating data is in non-interleaved order and will be interleaved into the bundle.
     * The offset defines the location of the first channel and all other channels will be added adjacent to the first.
     *
     * @param[in] offset The offset in the bundle were the first channel shall be placed.
     * @param[in] channel0 A pointer to the samples of the first channel in non-interleaved order to be copied into the bundle.
     * @param[in] channel1 A pointer to the samples of the second channel in non-interleaved order to be copied into the bundle.
     */
    void writeTwoChannelsFromNonInterleaved(uint32_t offset, const float *channel0, const float *channel1);

    /**
     * @brief Interleaves three channels into the bundle ('Bundling').
     *
     * The originating data is in non-interleaved order and will be interleaved into the bundle.
     * The offset defines the location of the first channel and all other channels will be added adjacent to the first.
     *
     * @param[in] offset The offset in the bundle were the first channel shall be placed.
     * @param[in] channel0 A pointer to the samples of the first channel in non-interleaved order to be copied into the bundle.
     * @param[in] channel1 A pointer to the samples of the second channel in non-interleaved order to be copied into the bundle.
     * @param[in] channel2 A pointer to the samples of the third channel in non-interleaved order to be copied into the bundle.
     */
    void writeThreeChannelsFromNonInterleaved(uint32_t offset, const float *channel0, const float *channel1, const float *channel2);

    /**
     * @brief Interleaves four channels into the bundle ('Bundling').
     *
     * The originating data is in non-interleaved order and will be interleaved into the bundle.
     *
     * @param[in] channel0 A pointer to the samples of the first channel in non-interleaved order to be copied into the bundle.
     * @param[in] channel1 A pointer to the samples of the second channel in non-interleaved order to be copied into the bundle.
     * @param[in] channel2 A pointer to the samples of the third channel in non-interleaved order to be copied into the bundle.
     * @param[in] channel3 A pointer to the samples of the fourth channel in non-interleaved order to be copied into the bundle.
     */
    void writeFourChannelsFromNonInterleaved(const float *channel0, const float *channel1, const float *channel2, const float *channel3);

    /**
     * @brief Interleaves two channels into the bundle ('Bundling').
     *
     * The originating data is in interleaved order and will be interleaved into the bundle.
     * The offset defines the location of the first channel and all other channels will be added adjacent to the first.
     *
     * @param[in] offset The offset in the bundle were the first channel shall be placed.
     * @param[in] srcStride The stride of the source channels
     * @param[in] channel A pointer to the samples of the first channel in interleaved order to be copied into the bundle.
     */
    void writeTwoChannelsFromInterleaved(uint32_t offset, uint32_t srcStride, const float *channel);

    /**
     * @brief Interleaves three channels into the bundle ('Bundling').
     *
     * The originating data is in interleaved order and will be interleaved into the bundle.
     * The offset defines the location of the first channel and all other channels will be added adjacent to the first.
     *
     * @param[in] offset The offset in the bundle were the first channel shall be placed.
     * @param[in] srcStride The stride of the source channels
     * @param[in] channel A pointer to the samples of the first channel in interleaved order to be copied into the bundle.
     */
    void writeThreeChannelsFromInterleaved(uint32_t offset, uint32_t srcStride, const float *channel);

    /**
     * @brief Interleaves four channels into the bundle ('Bundling').
     *
     * The originating data is in interleaved order and will be interleaved into the bundle.
     *
     * @param[in] srcStride The stride of the source channels
     * @param[in] channel A pointer to the samples of the first channel in interleaved order to be copied into the bundle.
     */
    void writeFourChannelsFromInterleaved(uint32_t srcStride, const float *channel);

    /**
     * @brief Deinterleaves one channel out of the bundle ('Unbundling').
     *
     * The data in the bundle is in interleaved order and will be deinterleaved into the pointer given as parameter.
     *
     * @param[in] offset The offset in the bundle were the channel shall be read from.
     * @param[in] channel A pointer where the samples shall be deinterleaved to.
     */
    void readOneChannel(uint32_t offset, float *channel) const;

    /**
     * @brief Deinterleaves two channels out of the bundle ('Unbundling').
     *
     * The data in the bundle is in interleaved order and will be deinterleaved into the pointers given as parameters.
     * The offset defines the location of the first channel and all other channels will be read adjacent from the first.
     *
     * @param[in] offset The offset in the bundle were the first channel shall be read from.
     * @param[in] channel0 A pointer where the samples of the first channel shall be deinterleaved to.
     * @param[in] channel1 A pointer where the samples of the second channel shall be deinterleaved to.
     */
    void readTwoChannels(uint32_t offset, float *channel0, float *channel1) const;

    /**
     * @brief Deinterleaves three channels out of the bundle ('Unbundling').
     *
     * The data in the bundle is in interleaved order and will be deinterleaved into the pointers given as parameters.
     * The offset defines the location of the first channel and all other channels will be read adjacent from the first.
     *
     * @param[in] offset The offset in the bundle were the first channel shall be read from.
     * @param[in] channel0 A pointer where the samples of the first channel shall be deinterleaved to.
     * @param[in] channel1 A pointer where the samples of the second channel shall be deinterleaved to.
     * @param[in] channel2 A pointer where the samples of the third channel shall be deinterleaved to.
     */
    void readThreeChannels(uint32_t offset, float *channel0, float *channel1, float *channel2) const;

    /**
     * @brief Deinterleaves four channels out of the bundle ('Unbundling').
     *
     * The data in the bundle is in interleaved order and will be deinterleaved into the pointers given as parameters.
     *
     * @param[in] channel0 A pointer where the samples of the first channel shall be deinterleaved to.
     * @param[in] channel1 A pointer where the samples of the second channel shall be deinterleaved to.
     * @param[in] channel2 A pointer where the samples of the third channel shall be deinterleaved to.
     * @param[in] channel3 A pointer where the samples of the fourth channel shall be deinterleaved to.
     */
    void readFourChannels(float *channel0, float *channel1, float *channel2, float *channel3) const;

  private:
    /**
     * @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasAudioChannelBundle(IasAudioChannelBundle const &other); //lint !e1704

    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasAudioChannelBundle& operator=(IasAudioChannelBundle const &other); //lint !e1704

    // Member variables
    uint32_t                 mNumFreeChannels;   //!< Number of free audio channels within a bundle. Must be updated by the audio component.
    uint32_t                 mFrameLength;       //!< frame length of one audio frame.
    float               *mAudioData;         //!< Pointer to the begin of the audio data memory
    float               *mAudioDataEnd;      //!< Pointer to the end of the audio data memory
    DltContext              *mLogContext;        //!< The log context
};

} //namespace IasAudio

#endif /* IASAUDIOCHANNELBUNDLE_HPP_ */
