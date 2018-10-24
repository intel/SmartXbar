/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasSetupEvent.hpp
 * @date   2015
 * @brief
 */

#ifndef IASSETUPEVENT_HPP
#define IASSETUPEVENT_HPP


#include "audio/common/IasAudioCommonTypes.hpp"
#include "audio/smartx/IasEvent.hpp"

namespace IasAudio {

class IasEventHandler;

/**
 * @brief The setup event class
 *
 * It is used to inform about any event related to the setup of the SmartXbar
 */
class IAS_AUDIO_PUBLIC IasSetupEvent : public IasEvent
{
  public:
    /**
     * @brief The event type
     */
    enum IasEventType
    {
      eIasUninitialized,                    //!< Event not initialized yet
      eIasUnrecoverableSourceDeviceError,   //!< There was an unrecoverable source device error. Device is stopped.
      eIasUnrecoverableSinkDeviceError,     //!< There was an unrecoverable sink device error. Device is stopped.
    };

    /**
     * @brief Constructor.
     */
    IasSetupEvent();
    /**
     * @brief Destructor.
     */
    virtual ~IasSetupEvent();

    /**
     * @brief The dispatch method
     *
     * @param[in] eventHandler A reference to the customized IasEventHandler used for dispatching
     */
    virtual void accept(IasEventHandler &eventHandler);

    /**
     * @name Getter methods
     *
     * This methods have to be used to get the informations of the setup event.
     *
     * @note Not all informations are always set or relevant. If e.g. a source error occured the IasSetupEvent::getSinkDevice will return a nullptr
     */
    ///@{
    /**
     * @brief Get the event type
     *
     * @return The event type of the connection event. See IasConnectionEvent::IasEventType for details.
     */
    inline IasEventType getEventType() const { return mEvenType; }

    /**
     * @brief Get the source device
     *
     * @return A pointer to the source device
     */
    inline IasAudioSourceDevicePtr getSourceDevice() const { return mSourceDevice; }

    /**
     * @brief Get the sink device
     *
     * @return A pointer to the sink device
     */
    inline IasAudioSinkDevicePtr getSinkDevice() const { return mSinkDevice; }
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
     * @brief Set the source device
     *
     * @param[in] sourceDevice A pointer to the source device
     */
    inline void setSourceDevice(IasAudioSourceDevicePtr sourceDevice) { mSourceDevice = sourceDevice; }

    /**
     * @brief Set the sink device
     *
     * @param[in] sinkDevice A pointer to the sink device
     */
    inline void setSinkDevice(IasAudioSinkDevicePtr sinkDevice) { mSinkDevice = sinkDevice; }
    ///@}

  private:
    /**
     * @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasSetupEvent(IasSetupEvent const &other);
    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasSetupEvent& operator=(IasSetupEvent const &other);

    IasEventType              mEvenType;        //!< The event type
    IasAudioSourceDevicePtr   mSourceDevice;    //!< A pointer to the source device
    IasAudioSinkDevicePtr     mSinkDevice;      //!< A pointer to the sink device
};

/**
 * @brief Function to get a IasEventType as string.
 *
 * @param[in] type The event type to translate to a string
 *
 * @return Enum Member as string
 */
std::string toString(const IasSetupEvent::IasEventType&  type);


} //namespace IasAudio

#endif // IASSETUPEVENT_HPP
