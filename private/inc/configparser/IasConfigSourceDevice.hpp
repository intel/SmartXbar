/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasConfigSourcedevice.hpp
 * @date   2017
 * @brief  smartXConfigParser: Parsing class for the source devices
 */

#ifndef IASCONFIGSOURCEDEVICE_HPP
#define IASCONFIGSOURCEDEVICE_HPP

#include <stdio.h>
#include <stdlib.h>
#include <libxml2/libxml/parser.h>
#include <libxml2/libxml/tree.h>
#include <libxml2/libxml/xmlmemory.h>
#include "avbaudiomodules/audio/common/IasAudioCommonTypes.hpp"
#include "audio/smartx/IasISetup.hpp"
#include "avbaudiomodules/internal/audio/common/IasAudioLogging.hpp"
#include "audio/smartx/IasSetupHelper.hpp"
#include "avbaudiomodules/audio/common/IasAudioCommonTypes.hpp"
#include "avbaudiomodules/audio/common/audiobuffer/IasMemoryAllocator.hpp"
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
 * @brief The IasConfigSourceDevice private implementation class
 */
class IAS_AUDIO_PUBLIC IasConfigSourceDevice
{

  public:
    /**
     * @brief The result type for the IasConfigSourceDevice methods
     */
     enum IasResult
     {
       eIasOk,               //!< Operation successful
       eIasFailed            //!< Operation failed
     };


     /*!
      * @brief Constructor.
      *
      * @param[in] numPorts Number of ports.
      */
     explicit IasConfigSourceDevice(std::int32_t numPorts);

     /*!
      *  @brief Destructor
      */
     ~IasConfigSourceDevice();

     /*!
      * @brief Set source device params
      *
      * @param[in] sourceDeviceNode Pointer to the source device node.
      * @return The result of setting source parameters
      * @retval IasConfigSourcekDevice::eIasOk  operation Successful
      * @retval IasConfigSourceDevice::eIasFailed operation Failed
      */
     IasResult setSourceParams(xmlNodePtr sourceDeviceNode);

     /*!
      * @brief Set source port params
      *
      * @param[in]  setup     Pointer to the smartx setup
      * @param[in]  sourceDevicePortNode      Pointer to the source device port node.
      * @return The result of setting source port parameters
      * @retval IasConfigSourcekDevice::eIasOk  operation Successful
      * @retval IasConfigSourceDevice::eIasFailed operation Failed
      */
     IasResult setSourcePortParams(IasISetup *setup, xmlNodePtr sourceDevicePortNode);


     /*!
      * @brief Returns source device params
      *
      * @return Pointer to the source device parameter
      */
     IasAudioDeviceParams getSourceDeviceParams() const;

     /*!
      * @brief Increment source port counter
      *
      */
     void incrementSourcePortCounter();


     /*!
      * @brief Set source device pointer
      *
      */
     void setSourceDevicePtr(IasAudioSourceDevicePtr sourceDevicePtr);


  private:
    //private members to store each source device information
    IasAudioDeviceParams mSourceParams;              //!< Member variable to store source device params
    IasAudioSourceDevicePtr mSourceDevicePtr;        //!< Pointer to the sourceDevicePtr
    IasAudioPortParams mSourcePortParams;            //!< Member variable to store source device port params
    std::int32_t mNumSourcePorts;                      //!< Number of source ports
    std::int32_t mSourcePortCounter;                   //!< To count source ports
    std::vector<IasAudioPortPtr> mSourcePortVec;     //!< Vector for source ports
    DltContext *mLog;                                //!< The DLT log context

};

} // end of IasAudio namespace

#endif // IASCONFIGSOURCEDEVICEHPP
