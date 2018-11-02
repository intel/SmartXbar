/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasConnectionEvent.hpp
 * @date   2015
 * @brief
 */

#ifndef IASCONNECTIONEVENT_HPP
#define IASCONNECTIONEVENT_HPP


#include "avbaudiomodules/audio/common/IasAudioCommonTypes.hpp"
#include "avbaudiomodules/internal/audio/common/IasAudioLogging.hpp"
#include "audio/smartx/IasEvent.hpp"

namespace IasAudio {

class IasEventHandler;

/**
 * @brief The connection event class
 *
 * It is used to inform about any event related to audio connections between
 * an audio source port and an audio sink port or an audio source port and a routing zone port
 */
class IAS_AUDIO_PUBLIC IasConnectionEvent : public IasEvent
{
  public:
    /**
     * @brief The event type
     */
    enum IasEventType
    {
      eIasUninitialized,            //!< Event not initialized yet
      eIasConnectionEstablished,    //!< Connection between source and sink successfully established
      eIasConnectionRemoved,        //!< Connection between source and sink successfully removed
      eIasSourceDeleted,            //!< Connection lost because source was deleted
      eIasSinkDeleted               //!< Connection lost because sink was deleted @deprecated Deprecated, left in for binary compatibility.
    };

    /**
     * @brief Constructor.
     */
    IasConnectionEvent();
    /**
     * @brief Destructor.
     */
    virtual ~IasConnectionEvent();

    /**
     * @brief The dispatch method
     *
     * @param[in] eventHandler A reference to the customized IasEventHandler used for dispatching
     */
    virtual void accept(IasEventHandler &eventHandler);

    /**
     * @name Getter methods
     *
     * This methods have to be used to get the informations of the connection event.
     *
     * @note Not all informations are always set or relevant. If e.g. a source deleted event was received the sink id
     * does not contain any meaningful information.
     */
    ///@{
    /**
     * @brief Get the event type
     *
     * @return The event type of the connection event. See IasConnectionEvent::IasEventType for details.
     */
    inline IasEventType getEventType() const { return mEvenType; }

    /**
     * @brief Get the source id
     *
     * @return The source id related to the connection event. Will be -1 if the information is not relevant
     * for the event.
     */
    inline int32_t getSourceId() const { return mSourceId; }

    /**
     * @brief Get the sink id
     *
     * @return The sink id related to the connection event. Will be -1 if the information is not relevant
     * for the event.
     */
    inline int32_t getSinkId() const { return mSinkId; }
    ///@}

    /**
     * @name Setter methods
     *
     * This setter methods only have to be used by the SmartXbar internally to set the relevant parts of the event.
     */
    ///@{
    /**
     * @brief Set the event type
     *
     * @param[in] eventType The event type of the connection event
     */
    inline void setEventType(IasEventType eventType) { mEvenType = eventType; }

    /**
     * @brief Set the source id
     *
     * @param[in] sourceId The source id of the connection
     */
    inline void setSourceId(int32_t sourceId) { mSourceId = sourceId; }

    /**
     * @brief Set the sink id
     *
     * @param[in] sinkId The sink id of the connection
     */
    inline void setSinkId(int32_t sinkId) { mSinkId = sinkId; }
    ///@}

  private:
    /**
     * @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasConnectionEvent(IasConnectionEvent const &other);
    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasConnectionEvent& operator=(IasConnectionEvent const &other);

    IasEventType    mEvenType;      //!< The event type
    int32_t      mSourceId;      //!< The source id
    int32_t      mSinkId;        //!< The sink id
};

/**
 * @brief Function to get a IasEventType as string.
 *
 * @param[in] type The event type to translate to a string
 *
 * @return Enum Member as string
 */
std::string toString(const IasConnectionEvent::IasEventType&  type);


} //namespace IasAudio

#endif // IASCONNECTIONEVENT_HPP
