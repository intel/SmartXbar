/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasAudioBuffer.hpp
 * @date   2013
 * @brief
 */

#ifndef IASAUDIOBUFFER_HPP_
#define IASAUDIOBUFFER_HPP_


#include "audio/smartx/rtprocessingfwx/IasProcessingTypes.hpp"

namespace IasAudio {

class IasAudioBufferPool;

class IasAudioBuffer
{
  public:
    /**
     * @brief Constructor.
     */
    IasAudioBuffer(uint32_t bufferSize);

    /**
     * @brief Destructor, virtual by default.
     */
    virtual ~IasAudioBuffer();

    /**
     * @brief Initialize the buffer
     *
     * Allocates the memory for each channel of the audio buffer
     *
     * @returns eIasAudioProcOK in case of success or any other code in case of an error
     */
    IasAudioProcessingResult init();

    /**
     * @brief Get the pointer to the audio buffer
     *
     * @returns A pointer to the audio buffer
     */
    float* getData() const;

    /**
     * @brief Retrieve pointer to the buffer pool the buffer belongs to.
     *
     * @returns Pointer to home pool
     */
    inline IasAudioBufferPool* getHomePool() const;

    /**
     * @brief Sets the buffer pool the buffer belongs to. Can only be called once.
     *
     * @param[in] homePool Pointer to home pool
     */
    inline void setHomePool(IasAudioBufferPool* const homePool);

  private:
    /**
     * @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasAudioBuffer(IasAudioBuffer const &other);

    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasAudioBuffer& operator=(IasAudioBuffer const &other);

    void cleanup();

    // Member variables
    uint32_t             mBufferSize;        //!< Size of the buffers in Float32
    IasAudioBufferPool*     mHome;              //!< Pointer to the home pool
    float*           mData;              //!< Pointer to the audio buffer data
};


inline float* IasAudioBuffer::getData() const
{
  return mData;
}

inline IasAudioBufferPool* IasAudioBuffer::getHomePool() const
{
  return mHome;
}

inline void IasAudioBuffer::setHomePool(IasAudioBufferPool* const homePool)
{
  if ((NULL != homePool) && (NULL == mHome))
  {
    mHome = homePool;
  }
}


} //namespace IasAudio

#endif /* IASAUDIOBUFFER_HPP_ */
