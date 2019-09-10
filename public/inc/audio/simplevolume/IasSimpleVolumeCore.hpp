/*
 * Copyright (C) 2019 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file   IasSimpleVolumeCore.hpp
 * @date   2013
 * @brief
 */

#ifndef IASSIMPLEVOLUMECORE_HPP_
#define IASSIMPLEVOLUMECORE_HPP_

// disable conversion warnings for tbb
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#include "tbb/tbb.h"
// turn the warnings back on
#pragma GCC diagnostic pop

#include "audio/smartx/rtprocessingfwx/IasGenericAudioCompCore.hpp"

namespace IasAudio {

class IasIGenericAudioCompConfig;

class IAS_AUDIO_PUBLIC IasSimpleVolumeCore : public IasGenericAudioCompCore
{
  public:
    /**
     * @brief Constructor.
     *
     * @param config pointer to the component configuration.
     * @param componentName unique component name.
     */
    IasSimpleVolumeCore(const IasIGenericAudioCompConfig *config, std::string componentName);

    /**
     * @brief Destructor, virtual by default.
     */
    virtual ~IasSimpleVolumeCore();

    /**
     * @brief Resets the internal audio component states, pure virtual.
     *
     * Must be implemented by each audio component. This method resets the
     * internal states of the audio component.
     *
     * @returns  The result indicating success (eIasAudioCompOK) or failure.
     */
    virtual IasAudioProcessingResult reset();

    /**
     * @brief Initializes the internal audio component states, pure virtual.
     *
     * Must be implemented by each audio component. This method can be
     * used to create additional members that require memory allocation which shall
     * be avoided in the constructor.
     *
     * @returns  The result indicating success (eIasAudioCompOK) or failure.
     */
    virtual IasAudioProcessingResult init();

    /**
     * @brief Set the current volume value.
     *
     * @param[in] newVolume The new volume value
     */
    void setVolume(float newVolume);

  private:
    /**
     * @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasSimpleVolumeCore(IasSimpleVolumeCore const &other);

    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasSimpleVolumeCore& operator=(IasSimpleVolumeCore const &other);

    /**
     * @brief Processes the audio data by applying the algorithm of the audio component, pure virtual.
     *
     * Must be implemented by each audio component. This is the actual processing
     * method which is called by the process method of the base class.
     *
     * @returns  The result indicating success (eIasAudioCompOK) or failure.
     */
    virtual IasAudioProcessingResult processChild();

    // Member variables
    float                              mVolume;          //!< The current volume value.
    tbb::concurrent_queue<float>       mVolumeMsgQueue;  //!< Queue used to synchronize the Ctrl thread with processing thread.
};

} //namespace IasAudio

#endif /* IASSIMPLEVOLUMECORE_HPP_ */
