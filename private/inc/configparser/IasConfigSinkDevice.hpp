/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasConfigSinkDevice.hpp
 * @date   2017
 * @brief  smartXConfigParser: Parsing class for the sink devices
 */

#ifndef IASCONFIGSINKDEVICE_HPP
#define IASCONFIGSINKDEVICE_HPP

#include <libxml2/libxml/parser.h>
#include <libxml2/libxml/tree.h>
#include <libxml2/libxml/xmlmemory.h>
#include "audio/common/IasAudioCommonTypes.hpp"
#include "audio/smartx/IasISetup.hpp"
#include "internal/audio/common/IasAudioLogging.hpp"
#include "audio/smartx/IasSetupHelper.hpp"
#include "audio/common/IasAudioCommonTypes.hpp"
#include "audio/common/audiobuffer/IasMemoryAllocator.hpp"
#include "smartx/IasAudioTypedefs.hpp"
#include "model/IasAudioPortOwner.hpp"
#include "smartx/IasSmartXClient.hpp"
#include "alsahandler/IasAlsaHandler.hpp"
#include "model/IasAudioPort.hpp"
#include "model/IasAudioDevice.hpp"
#include <vector>

/*!
 * @brief namespace IasAudio
 */
namespace IasAudio {

class IasAudioDevice;
/**
 * @brief The IasConfigSinkDevice private implementation class
 */
class IAS_AUDIO_PUBLIC IasConfigSinkDevice
{
  public:
    /**
     * @brief The result type for the IasConfigSinkDevice methods
     */
    enum IasResult
    {
      eIasOk,               //!< Operation successful
      eIasFailed            //!< Operation failed
    };


    /*!
     * @brief Constructor.
     *
     * @param[in] numPorts  Number of ports.
     */
    explicit IasConfigSinkDevice(std::int32_t numPorts);

    /*!
     * @brief Destructor.
     */
    ~IasConfigSinkDevice();

    /*!
     * @brief Set sink device params
     *
     * @param[in] sinkDeviceNode  Pointer to the sink device node.
     * @return The result of setting sink parameters
     * @retval IasConfigSinkDevice::eIasOk  operation Successful
     * @retval IasConfigSinkDevice::eIasFailed operation Failed
     */
    IasResult setSinkParams(xmlNodePtr sinkDeviceNode);

    /*!
     * @brief Set sink port params
     *
     * @param[in]  setup  Pointer to the smartx setup
     * @param[in]  sinkDevicePortNode  Pointer to the sink device port node.
     * @return The result of setting sink port parameters
     * @retval IasConfigSinkDevice::eIasOk  operation Successful
     * @retval IasConfigSinkDevice::eIasFailed operation Failed
     */
    IasResult setSinkPortParams(IasISetup *setup, xmlNodePtr sinkDevicePortNode);

    /*!
     * @brief Create and link routing zone
     *
     * @param[in]  setup Pointer to the smartx setup
     * @return The result of creating and linking routing zone
     * @retval IasConfigSinkDevice::eIasOk  operation Successful
     * @retval IasConfigSinkDevice::eIasFailed operation Failed
     */
    IasResult createLinkRoutingZone(IasISetup *setup);

    /*!
     * @brief Set routing zone port params
     *
     * @param[in]  setup Pointer to the smartx setup
     * @param[in]  routingZoneNode     Pointer to routing zone node.
     * @param[in]  setupLinksNode     Pointer to setupLinks node.
     * @return The result of setting zone parameters
     * @retval IasConfigSinkDevice::eIasOk  operation Successful
     * @retval IasConfigSinkDevice::eIasFailed operation Failed
     */
    IasResult setZonePortParams(IasISetup *setup, xmlNodePtr routingZoneNode, xmlNodePtr setupLinksNode);

    /*!
     * @brief Returns sink device params
     *
     * @return Pointer to the sink device parameter
     */
    IasAudioDeviceParams getSinkDeviceParams() const;

    /*!
     * @brief Returns sink device port params vector
     *
     * @return Vector to the sink device port parameter
     */
    std::vector<IasAudioPortParams> getSinkPortParams();

    /*!
     * @brief Returns sink device port vector
     *
     * @return Vector to the sink device port
     */
    std::vector<IasAudioPortPtr> getSinkPorts();

    /*!
     * @brief set sink device pointer
     *
     * @param[in] sinkDevice pointer to the sink device
     */
    void setSinkDevicePtr(IasAudioSinkDevicePtr sinkDevicePtr);

    /*!
     * @brief Returns sink routing zone params
     *
     * @return Pointer to routing zone parameters
     */
    IasRoutingZoneParams getRoutingZoneParams() const;

    /*!
     * @brief set routing zone param name
     *
     * @param[in] name routingzone string name
     */
    void setRoutingZoneParamsName(std::string name);

    /*!
     * @brief Returns sink routing zone params
     *
     * @return Pointer to routing zone
     */
    IasRoutingZonePtr getMyRoutingZone() const;

    /*!
     * @brief Returns routing zone ports vector
     *
     * @return Vector to routing zone ports
     */
    std::vector<IasAudioPortPtr> getMyRoutingZonePorts();

    /*!
     * @brief Returns routing zone ports params vector
     *
     * @return Vector to routing zone port params
     */
    std::vector<IasAudioPortParams> getMyRoutingZonePortParams();

    /*!
     * @brief Increment sink port counter
     *
     */
    void incrementSinkPortCounter();

    /*!
     * @brief Increment zone port counter
     *
     */
    void incrementZonePortCounter();


  private:
    //Data members for each sink device
    IasAudioSinkDevicePtr mSinkDevicePtr;                    //!< Pointer to the sinkDevicePtr
    IasAudioDeviceParams mSinkParams;                        //!< Member variable to store sink device params
    std::vector<IasAudioPortParams> mSinkPortParams;         //!< Member variable to store sink device port params
    std::int32_t mNumSinkPorts;                                //!< Number of sink ports
    std::int32_t mSinkPortCounter;                             //!< To count sink ports
    std::vector<IasAudioPortPtr> mSinkPort;                  //!< Vector for sink ports

    //Routing Zone data members for each sink device
    IasRoutingZoneParams mRoutingZoneParams;                 //!< Member variable to store routing zone params
    std::vector<IasAudioPortParams> mZonePortParams;         //!< Vector to store routing zone port params
    IasRoutingZonePtr mRoutingZone;                          //!< Routing zone pointer
    std::int32_t mNumZonePorts;                                //!< Number of routing zone ports
    std::int32_t mZonePortCounter;                             //!< To count routing zone ports
    std::vector<IasAudioPortPtr> mZonePorts;                 //!< Vector for sink zone ports

    DltContext *mLog;                                        //!< The DLT log context
};

} // end of IasAudio namespace



#endif // IASCONFIGSINKDEVICEHPP
