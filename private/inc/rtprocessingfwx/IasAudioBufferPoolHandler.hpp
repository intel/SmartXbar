/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasAudioBufferPoolHandler.hpp
 * @date   2013
 * @brief  The declaration of the IasAudioBufferPoolHandler class.
 */

#ifndef IASAUDIOBUFFERPOOLHANDLER_HPP_
#define IASAUDIOBUFFERPOOLHANDLER_HPP_

#include "avbaudiomodules/audio/common/IasAudioCommonTypes.hpp"

namespace IasAudio {

class IasAudioBufferPool;

class IAS_AUDIO_PUBLIC IasAudioBufferPoolHandler
{
  public:
    /**
     * @brief Constructor.
     */
    IasAudioBufferPoolHandler();

    /**
     * @brief Destructor, virtual by default.
     */
    virtual ~IasAudioBufferPoolHandler();

    static IasAudioBufferPoolHandler* getInstance();

    IasAudioBufferPool* getBufferPool(uint32_t ringBufferSize);

  private:
    /**
     * @brief Map the audio buffer size to a specific audio buffer pool
     */
    typedef std::map<uint32_t, IasAudioBufferPool*> IasSizePoolMap;

    /**
     * @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasAudioBufferPoolHandler(IasAudioBufferPoolHandler const &other);

    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasAudioBufferPoolHandler& operator=(IasAudioBufferPoolHandler const &other);

    // Member variables
    IasSizePoolMap        mSizePoolMap;     //!< Map containing all required audio buffer pools for different buffer sizes
};

} //namespace IasAudio

#endif /* IASAUDIOBUFFERPOOLHANDLER_HPP_ */
