/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasAudioBufferPool.hpp
 * @date   2013
 * @brief  The declaration of the IasAudioBufferPool class
 */

#ifndef IASAUDIOBUFFERPOOL_HPP_
#define IASAUDIOBUFFERPOOL_HPP_

#include "IasAudioBuffer.hpp"


namespace IasAudio {

class IAS_AUDIO_PUBLIC IasAudioBufferPool
{
  public:
    /**
     * @brief Constructor.
     *
     * @param[in] bufferSize Buffer size in number of float32
     */
    IasAudioBufferPool(uint32_t bufferSize);

    /**
     * @brief Destructor, virtual by default.
     */
    virtual ~IasAudioBufferPool();

    /**
     * @brief Get a buffer from the pool
     *
     * @returns A pointer to a buffer from the buffer pool
     */
    IasAudioBuffer* getBuffer();

    /**
     * @brief Return the buffer to the pool
     *
     * @param[in] buffer Pointer to the buffer to be returned
     *
     * @returns eIasAudioProcOK in case of success or any other code in case of an error
     */
    inline static IasAudioProcessingResult returnBuffer(IasAudioBuffer* buffer);


  private:
    typedef std::list<IasAudioBuffer*> IasRingBufferList;

    /**
     * @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasAudioBufferPool(IasAudioBufferPool const &other);

    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasAudioBufferPool& operator=(IasAudioBufferPool const &other);

    /**
     * @brief Return the buffer to the pool
     *
     * @param[in] buffer Pointer to the buffer to be returned
     */
    void doReturnBuffer(IasAudioBuffer* buffer);

    // Member variables
    uint32_t           mBufferSize;        //!< Size of the buffers in Float32
    IasRingBufferList     mRingBuffers;       //!< List of currently unused audio buffers
};

inline IasAudioProcessingResult IasAudioBufferPool::returnBuffer(IasAudioBuffer* buffer)
{
  IasAudioProcessingResult result = eIasAudioProcOK;

  if (NULL == buffer)
  {
    result = eIasAudioProcInvalidParam;
  }
  else
  {
    IasAudioBufferPool* pool =  buffer->getHomePool();
    IAS_ASSERT( NULL != pool );

    pool->doReturnBuffer(buffer);
    result = eIasAudioProcOK;
  }

  return result;
}

} //namespace IasAudio

#endif /* IASAUDIOBUFFERPOOL_HPP_ */
