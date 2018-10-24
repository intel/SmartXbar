/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file   IasMySimplePipeline.hpp
 * @date   2016
 * @brief  Class for setting up an example pipeline "mySimplePipeline".
 */

#ifndef IASMYSIMPLEPIPELINE_HPP_
#define IASMYSIMPLEPIPELINE_HPP_


#include "audio/common/IasAudioCommonTypes.hpp"
#include "internal/audio/common/IasAudioLogging.hpp"
#include "smartx/IasAudioTypedefs.hpp"

/*!
 * @brief namespace IasAudio
 */
namespace IasAudio {

class IasAudioRingBuffer;
class IasAudioStream;

/*!
 * @brief Documentation for class IasMySimplePipeline
 *
 * This class implements following pipeline by means of the SmartXBar Setup Interface.
 * @verbatim
 *
 *     +------------------------+
 *     |                        |
 *     |      +----------+      |
 *     |     0|          |0     |
 * in0 O----->O module 0 O----->O out0
 *     |      |          |      |
 *     |      +----------+      |
 *     |                        |
 *     +------------------------+
 *
 * @endverbatim
 */
class IAS_AUDIO_PUBLIC IasMySimplePipeline
{

  public:
    /**
     * @brief The result type for the IasMySimplePipeline methods
     */
    enum IasResult
    {
      eIasOk,               //!< Ok, Operation successful
      eIasFailed,           //!< Other error (operation failed)
    };

    /*!
     *  @brief Constructor.
     */
    IasMySimplePipeline(IasISetup*        setup,
                        uint32_t       sampleRate,
                        uint32_t       periodSize,
                        IasAudioPortPtr   routingZoneInputPort0,
                        IasAudioPortPtr   sinkDeviceInputPort0);

    /*!
     *  @brief Destructor, virtual by default.
     */
    virtual ~IasMySimplePipeline();

    inline IasPipelinePtr getPipeline() const { return mPipeline; };

    /*!
     *  @brief Initialization.
     */
    void init();

  private:

    /*!
     *  @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasMySimplePipeline(IasPipeline const &other);

    /*!
     *  @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasMySimplePipeline& operator=(IasPipeline const &other);

    /*
     * Member Variables
     */
    DltContext*                               mLog;                 //!< Handle for the DLT log object
    IasISetup*                                mSetup;
    uint32_t                               mSampleRate;
    uint32_t                               mPeriodSize;
    std::vector<IasProcessingModulePtr> mProcessingModules;
    std::vector<IasAudioPinPtr>         mInputPins;
    std::vector<IasAudioPinPtr>         mOutputPins;
    std::vector<IasAudioPinPtr>         mModulePins;
    IasPipelinePtr                            mPipeline;
    IasAudioPortPtr                           mRoutingZoneInputPort0;
    IasAudioPortPtr                           mRoutingZoneInputPort1;
    IasAudioPortPtr                           mSinkDeviceInputPort0;
    IasAudioPortPtr                           mSinkDeviceInputPort1;
};

} // namespace IasAudio

#endif // IASMYSIMPLEPIPELINE_HPP_
