/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasBundleAssignment.hpp
 * @date   2012
 * @brief  The definition of the IasBundleAssignment class.
 */

#ifndef IASBUNDLEASSIGNMENT_HPP_
#define IASBUNDLEASSIGNMENT_HPP_

#include <rtprocessingfwx/IasAudioChannelBundle.hpp>



namespace IasAudio {

/**
 * @class IasBundleAssignment
 *
 * One or more bundle assignments are used by an audio stream to assign the channels of the stream
 * to channels inside bundle(s). Therefore the bundle assignment comprises a pointer to the bundle,
 * the index of the first channel in this bundle of the stream and the number of channels. A stream
 * can use one or more bundle assignments depending on the stream has more or less than 4 channels.
 * That's why a stream has a vector of bundle assignments.
 */
class IAS_AUDIO_PUBLIC IasBundleAssignment
{
  public:
    /**
     * @brief Constructor.
     *
     * @param[in] bundle A pointer to the bundle.
     * @param[in] channelIndex The index of the first channle in the bundle
     * @param[in] numberChannels The number of channels in the bundle.
     */
    IasBundleAssignment(IasAudioChannelBundle *bundle, uint32_t channelIndex, uint32_t numberChannels);

    /**
     * @brief Destructor, virtual by default.
     */
    virtual ~IasBundleAssignment();

    /**
     * @brief Getter method for the bundle pointer.
     *
     * @returns A pointer to the bundle.
     */
    inline IasAudioChannelBundle *getBundle() const { return mBundle; }

    /**
     * @brief Getter method for the channel index.
     *
     * @returns The channel index of the first channel in this bundle.
     */
    inline uint32_t getIndex() const { return mChannelIndex; }

    /**
     * @brief Getter method for the number of channels.
     *
     * @returns The number of channels belonging to the stream and contained in this bundle.
     */
    inline uint32_t getNumberChannels() const { return mNumberChannels; }

  private:
    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasBundleAssignment& operator=(IasBundleAssignment const &other);

    // Member variables
    IasAudioChannelBundle        *mBundle;                //!< A pointer to the audio bundle.
    uint32_t                   mChannelIndex;          //!< The channel index to the first channel of the stream inside the bundle.
    uint32_t                   mNumberChannels;        //!< The number of channels belonging to the stream and contained in the bundle.
};

} //namespace IasAudio

#endif /* IASBUNDLEASSIGNMENT_HPP_ */
