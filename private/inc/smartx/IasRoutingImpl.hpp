/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasRoutingImpl.hpp
 * @date   2015
 * @brief
 */

#ifndef IASROUTINGIMPL_HPP
#define IASROUTINGIMPL_HPP


#include "avbaudiomodules/audio/common/IasAudioCommonTypes.hpp"
#include "audio/smartx/IasIRouting.hpp"
#include "avbaudiomodules/internal/audio/common/IasAudioLogging.hpp"

namespace IasAudio {

class IasConfiguration;

/**
 * @brief
 *
 */
class IAS_AUDIO_PUBLIC IasRoutingImpl : public IasIRouting
{

  using IasConnectionMap = std::multimap<IasAudioPortPtr,IasAudioPortPtr,std::less<IasAudioPortPtr>>;
  using IasConnectionPair =std::pair<IasConnectionMap::iterator, IasConnectionMap::iterator>;

  public:
    /**
     * @brief Constructor.
     */
    IasRoutingImpl(IasConfigurationPtr config);
    /**
     * @brief Destructor.
     */
    virtual ~IasRoutingImpl();

    /**
     * @brief Establish a connection between an audio source and an audio sink
     *
     * @param[in] sourceId The source ID to connect
     * @param[in] sinkId The sink ID to connect to
     * @return The result of establishing the connection
     * @retval IasIRouting::eIasOk Connection is established
     * @retval IasIRouting::eIasFailed Failed to establish connection
     */
    virtual IasResult connect(int32_t sourceId, int32_t sinkId);
    /**
     * @brief Remove a connection between an audio source and an audio sink
     *
     * @param[in] sourceId The source ID to disconnect
     * @param[in] sinkId The sink ID to disconnect
     * @return The result of removing the connection
     * @retval IasIRouting::eIasOk Connection is removed
     * @retval IasIRouting::eIasFailed Failed to remove connection
     */
    virtual IasResult disconnect(int32_t sourceId, int32_t sinkId);

    /**
     * @brief Get all the active getActiveConnections
     *
     * @return vector carrying pairs of the connected ports
     */
    virtual IasConnectionVector getActiveConnections() const;

    /**
     * @brief Get the dummy getDummySources
     *
     * The dummy sources are sources, which are not connected, but which data must
     * be read from the ALSA device (and then ignored) to avoid blocking of the ALSA device
     *
     * @return The set containg the IDs of the dummy sources
     */
    virtual IasDummySourcesSet getDummySources() const;


  private:
    /**
     * @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasRoutingImpl(IasRoutingImpl const &other);
    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasRoutingImpl& operator=(IasRoutingImpl const &other);

    /**
     * @brief Disconnection between an audio source and an audio sink but does not remove it
     * from the map. Must be removed by the caller.
     *
     * @param[in] sourceId The source ID to disconnect
     * @param[in] sinkId The sink ID to disconnect
     * @return The result of removing the connection
     * @retval IasIRouting::eIasOk Connection is removed
     * @retval IasIRouting::eIasFailed Failed to remove connection
     */
    IasResult disconnectSafe(int32_t sourceId, int32_t sinkId);

    DltContext          *mLog;          //!< DLT context
    IasConfigurationPtr  mConfig;       //!< Pointer to the configuration
    IasConnectionMap     mConnections;  //!< map containg the pairs of connected audio ports
    IasDummySourcesSet   mDummySources; //!< set containg the ids of the "dummy" sources
};

} //namespace IasAudio

#endif // IASROUTINGIMPL_HPP
