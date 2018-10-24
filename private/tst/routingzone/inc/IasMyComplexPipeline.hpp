/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file   IasMyComplexPipeline.hpp
 * @date   2016
 * @brief  Class for setting up an example pipeline "myComplexPipeline".
 */

#ifndef IASMYCOMPLEXPIPELINE_HPP_
#define IASMYCOMPLEXPIPELINE_HPP_


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
 * @brief Documentation for class IasMyComplexPipeline
 *
 * This class implements following pipeline by means of the SmartXBar Setup Interface.
 * @verbatim
 *
 *     +------------------------------------------------------------------------------+
 *     |                                                                              |
 *     |      +----------+      +----------+      +----------+                        |
 *     |     0|          |0    1|          |1    5| module 4 |7                       |
 * in0 O----->O module 0 O----->O module 1 O----->O..........O----------------------->O out0
 *     |      |          |      |          |      |        .°|                        |
 *     |      +----------+      +----------+      |      .°  |                        |
 *     |                                          |    .°    |                        |
 *     |                        +----------+      |  .°      |      +----------+      |
 *     |                       9| module 5 |11   6|.°        |8    3|          |3     |
 * in1 O------------------------O..........O----->O..........O----->O          O----->O out1
 *     |                        |  °.  .°  |      |          |      |          |      |
 *     |      +----------+      |    ::    |      +----------+      | module 3 |      |
 *     |     2|          |2   10|  .°  °.  |12                     4|          |4     |
 *     |  +-->O module 2 O----->O.:......:.O----------------------->O          O---+  |
 *     |  |   |          |      |          |                        |          |   |  |
 *     |  |   +----------+      +----------+                        +----------+   |  |
 *     |  |                                  +---+                                 |  |
 *     |  +----------------------------------| T |<--------------------------------+  |
 *     |                                     +---+                                    |
 *     |                                                                              |
 *     +------------------------------------------------------------------------------+
 *
 * @endverbatim
 */
class IAS_AUDIO_PUBLIC IasMyComplexPipeline
{

  public:
    /**
     * @brief The result type for the IasMyComplexPipeline methods
     */
    enum IasResult
    {
      eIasOk,               //!< Ok, Operation successful
      eIasFailed,           //!< Other error (operation failed)
    };

    /*!
     *  @brief Constructor.
     */
    IasMyComplexPipeline(IasISetup*        setup,
                         uint32_t       sampleRate,
                         uint32_t       periodSize,
                         IasAudioPortPtr   routingZoneInputPort0,
                         IasAudioPortPtr   routingZoneInputPort1,
                         IasAudioPortPtr   sinkDeviceInputPort0,
                         IasAudioPortPtr   sinkDeviceInputPort1);

    /*!
     *  @brief Destructor, virtual by default.
     */
    virtual ~IasMyComplexPipeline();

    inline IasPipelinePtr getPipeline() const { return mPipeline; };

    /*!
     *  @brief Initialization.
     */
    void init();

    void unlinkPorts();

  private:

    /*!
     *  @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasMyComplexPipeline(IasPipeline const &other);

    /*!
     *  @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasMyComplexPipeline& operator=(IasPipeline const &other);

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

#endif // IASMYCOMPLEXPIPELINE_HPP_
