/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file
 */

#ifndef IASAUDIODEVICE_HPP_
#define IASAUDIODEVICE_HPP_


#include "audio/common/IasAudioCommonTypes.hpp"
#include "internal/audio/common/IasAudioLogging.hpp"
#include "audio/smartx/IasISetup.hpp"
#include "smartx/IasAudioTypedefs.hpp"
#include "model/IasAudioPortOwner.hpp"

/*!
 * @brief namespace IasAudio
 */
namespace IasAudio {

class IasAudioRingBuffer;

/*!
 * @brief Documentation for class IasAudioDevice
 */
class IAS_AUDIO_PUBLIC IasAudioDevice :  public IasAudioPortOwner
{
  public:

    /**
     * @brief The event type
     *
     * This enum is used to signal which command was received from the external client
     */
    enum IasEventType
    {
      eIasNoEvent,        //!< No event available in event queue
      eIasStart,          //!< The device was started
      eIasStop,           //!< The device was stopped
    };

    /*!
     * @brief Type definition for a set of audio ports.
     */
    using IasAudioPortSet = std::set<IasAudioPortPtr>;

    /**
     * @brief Shared ptr type for a set of audio ports.
     */
    using IasAudioPortSetPtr = std::shared_ptr<IasAudioPortSet>;

    /*!
     *  @brief Constructor.
     */
    IasAudioDevice(IasAudioDeviceParamsPtr params);

    /*!
     *  @brief Destructor, virtual to allow inheritance.
     */
    virtual ~IasAudioDevice();

    const IasAudioDeviceParamsPtr getDeviceParams() const;

    void setConcreteDevice(IasSmartXClientPtr smartXClient);
    void setConcreteDevice(IasAlsaHandlerPtr alsaHandler);

    virtual bool isSmartXClient() const;
    virtual bool isAlsaHandler() const;

    virtual IasAudioCommonResult getConcreteDevice(IasSmartXClientPtr* smartXClient);
    virtual IasAudioCommonResult getConcreteDevice(IasAlsaHandlerPtr* alsaHandler);

    inline virtual uint32_t getNumChannels() const { return mDeviceParams->numChannels;};

    inline virtual IasClockType getClockType() const { return mDeviceParams->clockType;};

    inline virtual IasAudioCommonDataFormat getSampleFormat() const { return mDeviceParams->dataFormat;};

    inline virtual uint32_t getSampleRate() const { return mDeviceParams->samplerate;};

    inline virtual uint32_t getPeriodSize() const { return mDeviceParams->periodSize;};

    inline virtual uint32_t getNumPeriods() const { return mDeviceParams->numPeriods;};

    inline virtual uint32_t getPeriodsAsrcBuffer() const { return mDeviceParams->numPeriodsAsrcBuffer;};

    inline virtual IasSwitchMatrixPtr getSwitchMatrix() const { return nullptr; }

    virtual IasRoutingZoneWorkerThreadPtr getWorkerThread() const {return nullptr;}

    virtual std::string getName() const { return mDeviceParams->name; }

    virtual IasAudioRingBuffer* getRingBuffer();

    virtual IasRoutingZonePtr getRoutingZone() const {return mRoutingZone;}

    /*!
     *  @brief Add the audio port to the set of audio ports that belong to this device.
     *
     *  @param[in] audioPort  The audio port that shall be added.
     *
     *  @return Result of the operation.
     *  @retval eIasResultOk                   if the operation was successful.
     *  @retval eIasResultObjectAlreadyExists  if the specified audio port has already been added to this device.
     */
    virtual IasAudioCommonResult addAudioPort(IasAudioPortPtr audioPort);

    /*!
     *  @brief Delete the audio port from the set of audio ports that belong to this device.
     *
     *  @param[in] audioPort  The audio port that shall be deleted.
     */
    virtual void deleteAudioPort(IasAudioPortPtr audioPort);

    /*!
     *  @brief Get access to the set of audio ports that have been added to this device
     *
     *  @return  Handle to the set of audio ports.
     */
    virtual const IasAudioPortSetPtr getAudioPortSet() const { return mAudioPortSet; }

    /**
     * @brief Enable the event queue
     *
     * This method has to be used to enable or disable the event queue of the audio device. By default the event queue
     * is disabled. The event queue is currently only supported by the SmartXClient to inform about the commands received by
     * the external client. For the AlsaHandler this functionality is currently not used.
     *
     * @param[in] enable If true, the event queue mechanism will be enabled. False will disable the event queue mechanism.
     */
    virtual void enableEventQueue(bool enable);

    /**
     * @brief Get the next entry from the event queue
     *
     * If there is an event type entry in the event queue, it will be removed from the queue and returned as return value.
     * In case the event queue was not enabled (see IasAudioDevice::enableEventQueue) or the device does not support the
     * event queue, the returned values is IasEventType::eIasNoEvent.
     *
     * @return The event type from the event queue.
     */
    virtual IasEventType getNextEventType();

  protected:
    DltContext               *mLog;
    IasAudioDeviceParamsPtr   mDeviceParams;
    IasSmartXClientPtr        mSmartXClient;
    IasAlsaHandlerPtr         mAlsaHandler;
    IasRoutingZonePtr         mRoutingZone;
    IasAudioPortSetPtr        mAudioPortSet;  //!< Set of audio ports that have been added to this audio device.

  private:
    /*!
     *  @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasAudioDevice(IasAudioDevice const &other);

    /*!
     *  @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasAudioDevice& operator=(IasAudioDevice const &other);
};

std::string toString(const IasAudioDevice::IasEventType&  eventType);


} // namespace IasAudio

#endif // IASAUDIODEVICE_HPP_
