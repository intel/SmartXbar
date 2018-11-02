/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasAudioChainEnvironment.hpp
 * @date   2012
 * @brief  The definition of the IasAudioChainEnvironment class.
 */

#ifndef IASAUDIOCHAINENVIRONMENT_HPP_
#define IASAUDIOCHAINENVIRONMENT_HPP_

#include "avbaudiomodules/internal/audio/common/IasAudioLogging.hpp"
#include "audio/smartx/rtprocessingfwx/IasProcessingTypes.hpp"

namespace IasAudio {

/**
 * @class IasAudioChainEnvironment
 *
 * This class combines commonly used audio chain parameters that every component of the audio chain would need.
 * An instance of this environment class exists once per audio chain.
 */
class IAS_AUDIO_PUBLIC IasAudioChainEnvironment
{
  public:
    /**
     * @brief Constructor.
     */
    IasAudioChainEnvironment();

    /**
     * @brief Destructor, virtual by default.
     */
    virtual ~IasAudioChainEnvironment();

    /**
     * @brief Setter method for the frame length.
     *
     * The frame length is the number of samples per processing block of one channel.
     *
     * @param[in] frameLength The frame length used in the audio chain
     */
    void setFrameLength(uint32_t frameLength);

    /**
     * @brief Getter method for the frame length.
     *
     * @returns The frame length used in the audio chain.
     */
    uint32_t getFrameLength();

    /**
     * @brief Setter method for the sample rate
     *
     * @param[in] sampleRate The sample rate of the audio chain.
     */
    void setSampleRate(uint32_t sampleRate);

    /**
     * @brief Getter method for the sample rate
     *
     * @returns The sample rate of the audio chain.
     */
    uint32_t getSampleRate();

  private:
    /**
     * @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasAudioChainEnvironment(IasAudioChainEnvironment const &other);

    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasAudioChainEnvironment& operator=(IasAudioChainEnvironment const &other);

    // Member variables
    uint32_t         mFrameLength;       //!< The frame length of the blocks processed by the audio chain.
    uint32_t         mSamplerate;        //!< The sample rate of the audio chain.
    DltContext         *mLogContext;        //!< The log context
};

} //namespace IasAudio

#endif /* IASAUDIOCHAINENVIRONMENT_HPP_ */
