/*
 * * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasIRouting.hpp
 * @date   2015
 * @brief
 */

#ifndef IASIROUTING_HPP_
#define IASIROUTING_HPP_


#include "avbaudiomodules/audio/common/IasAudioCommonTypes.hpp"

namespace IasAudio {

/**
  * @brief The routing interface class
  *
  * This class is used to establish and remove audio routing paths
  */
class IAS_AUDIO_PUBLIC IasIRouting
{
  public:
    /**
     * @brief The result type for the IasIRouting methods
     */
    enum IasResult
    {
      eIasOk,                     //!< Operation successful
      eIasFailed,                 //!< Operation failed
    };

    /**
    * @brief Destructor.
    */
    virtual ~IasIRouting() {}

    /**
    * @brief Establish a connection between an audio source port and an audio sink port
    *
    * @param[in] sourceId The source ID to connect
    * @param[in] sinkId The sink ID to connect to
    * @return The result of establishing the connection
    * @retval IasIRouting::eIasOk Connection is established
    * @retval IasIRouting::eIasFailed Failed to establish connection
    */
    virtual IasResult connect(int32_t sourceId, int32_t sinkId) = 0;
    /**
    * @brief Remove a connection between an audio source port and an audio sink port
    *
    * @param[in] sourceId The source ID to disconnect
    * @param[in] sinkId The sink ID to disconnect
    * @return The result of removing the connection
    * @retval IasIRouting::eIasOk Connection is removed
    * @retval IasIRouting::eIasFailed Failed to remove connection
    */
    virtual IasResult disconnect(int32_t sourceId, int32_t sinkId) = 0;

    /**
    * @brief Get the active connections
    *
    * @return The connection vector containing the audio port pairs
    */
    virtual IasConnectionVector getActiveConnections() const = 0;

    /**
     * @brief Get the dummy getDummySources
     *
     * The dummy sources are sources, which are not connected, but which data must
     * be read from the ALSA device (and then ignored) to avoid blocking of the ALSA device
     *
     * @return The set containg the IDs of the dummy sources
     */
    virtual IasDummySourcesSet getDummySources() const = 0;
};

/**
 * @brief Function to get a IasIRouting::IasResult as string.
 *
 * @param[in] type The IasIRouting::IasResult value
 *
 * @return Enum Member as string
 */
std::string toString(const IasIRouting::IasResult& type);


} //namespace IasAudio

#endif /* IASIROUTING_HPP_ */
