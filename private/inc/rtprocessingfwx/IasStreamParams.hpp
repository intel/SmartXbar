/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasStreamParams.hpp
 * @date   2012
 * @brief  The definition of the IasStreamParams class.
 */

#ifndef IASSTREAMPARAMS_HPP_
#define IASSTREAMPARAMS_HPP_

#include "audio/common/IasAudioCommonTypes.hpp"

namespace IasAudio {

/**
 * @class IasStreamParams
 *
 * This class provides an easy way to access all required parameters for a processing module when dealing
 * with streams and bundles.
 */
class IAS_AUDIO_PUBLIC IasStreamParams
{
  public:
    /**
     * @brief Constructor.
     *
     * @param[in] bundleIndex The bundle index into the list of bundles to be processed.
     * @param[in] channelIndex The channel index to the first channel inside this bundle.
     * @param[in] numberChannels The number of channels of the stream contained in this bundle.
     */
    IasStreamParams(uint32_t bundleIndex,
                    uint32_t channelIndex,
                    uint32_t numberChannels);

    /**
     * @brief Destructor, virtual by default.
     */
    virtual ~IasStreamParams();

    /**
     * @brief Getter method for the bundle index.
     *
     * The bundle index is the index into the list of bundles created by the method
     * addStreamToProcess of class IasGenericAudioCompConfig.
     *
     * @returns The bundle index.
     */
    inline uint32_t getBundleIndex() const { return mBundleIndex; }

    /**
     * @brief Getter method for the channel index.
     *
     * The channel index is the offset in the bundle where the first channel
     * of the corresponding stream can be found.
     *
     * @returns The channel index.
     */
    inline uint32_t getChannelIndex() const { return mChannelIndex; }

    /**
     * @brief Getter method for the number of channels.
     *
     * The number of channels in the referenced bundle belonging to the corresponding stream.
     *
     * @returns The number of channels in the referenced bundle.
     */
    inline uint32_t getNumberChannels() const { return mNumberChannels; }

  private:
    /**
     * @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasStreamParams(IasStreamParams const &other);

    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasStreamParams& operator=(IasStreamParams const &other);

    // Member variables
    uint32_t       mBundleIndex;               //!< The index of a bundle in the list of bundles to be processed.
    uint32_t       mChannelIndex;              //!< The index of the first channel inside the bundle belonging
                                                  //!< to the stream to be processed.
    uint32_t       mNumberChannels;            //!< The number of channels of the stream to be processed.
};

} //namespace IasAudio

#endif /* IASSTREAMPARAMS_HPP_ */
