/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file   IasTestFrameworkRoutingZone.hpp
 * @date   2017
 * @brief  The Routing Zone of the Test Framework.
 */

#ifndef AUDIO_DAEMON2_PRIVATE_INC_TESTFWX_IASTESTFRAMEWORKROUTINGZONE_HPP_
#define AUDIO_DAEMON2_PRIVATE_INC_TESTFWX_IASTESTFRAMEWORKROUTINGZONE_HPP_

#include "audio/common/IasAudioCommonTypes.hpp"
#include "internal/audio/common/IasAudioLogging.hpp"
#include "testfwx/IasTestFrameworkTypes.hpp"




namespace IasAudio {


// Forward declaration of test framework worker classes.
class IasAudioRingBuffer;


/*!
 * @brief Documentation for class IasTestFrameworkRoutingZone
 */
class IasTestFrameworkRoutingZone
{
  public:
    /**
     * @brief  Result type of the class IasRoutingZone.
     */
    enum IasResult
    {
      eIasOk,               //!< Ok, Operation successful
      eIasFinished,         //!< Processing of all input files finished, operation successful
      eIasInvalidParam,     //!< Invalid parameter, e.g., out of range or NULL pointer
      eIasInitFailed,       //!< Initialization of the component failed
      eIasNotInitialized,   //!< Component has not been initialized appropriately
      eIasFailed,           //!< other error
    };

    /**
     * @brief Struct for collecting all parameters for each ring buffer that is serviced by the test framework routing zone.
     */
    struct IasRingBufferParams
    {
      //! The default constructor
      IasRingBufferParams()
        :ringBuffer(nullptr)
        ,waveFile(nullptr)
      {}

      IasAudioRingBuffer*          ringBuffer;          //!< ring buffer handle
      IasTestFrameworkWaveFilePtr  waveFile;            //!< wave file associated with ring buffer
    };

    /**
     * @brief Type definition for pairs of audio ports and their associated input buffers.
     */
    using IasRingBufferParamsPair = std::pair<IasAudioPortPtr, IasRingBufferParams>;

    /**
     * @brief Type definition for maps with all audio ports and their associated input buffers.
     */
    using IasRingBufferParamsMap = std::map<IasAudioPortPtr,  IasRingBufferParams>;

    /*!
     *  @brief Constructor.
     */
    IasTestFrameworkRoutingZone(IasTestFrameworkRoutingZoneParamsPtr params);

    /*!
     *  @brief Destructor.
     */
    ~IasTestFrameworkRoutingZone();

    /*!
     *  @brief Initialize test framework routing zone
     */
    IasResult init();

    /**
     * @brief Add a new input port to the test framework routing zone and create an input buffer for this input port.
     *
     * There must be a 1:1 relationship between the input ports and the associated input buffers.
     * Therefore, it is not allowed that an input port is added to a routing zone more than once.
     *
     * @param[in]  audioPort         The audio port that shall be added to the routing zone.
     * @param[in]  dataFormat        The data format that shall be used for the input buffer.
     *                               If the data format is undefined the output buffer will be created
     *                               using IasAudioCommonDataFormat::eIasFormatFloat32.
     * @param[in]  waveFile          Wave file that will be associated with this buffer
     * @param[out] inputBuffer       The input buffer that has been created by this method for the specified port.
     * @return                       The result of the function call.
     */
    IasResult createInputBuffer(const IasAudioPortPtr         audioPort,
                                     IasAudioCommonDataFormat dataFormat,
                                     IasTestFrameworkWaveFilePtr waveFile,
                                     IasAudioRingBuffer**     inputBuffer);

    /**
     * @brief Destroy an input buffer for an audio port.
     *
     * @param[in]  audioPort         The audio port that shall be removed
     *
     * @return                       The result of the function call.
     */
    IasResult destroyInputBuffer(const IasAudioPortPtr audioPort);

    /**
      * @brief Add a new output port to the test framework routing zone and create the output buffer for this output port.
      *
      * There must be a 1:1 relationship between the output ports and the associated output buffers.
      * Therefore, it is not allowed that an output port is added to a routing zone more than once.
      *
      * @param[in]  audioPort         The audio port that shall be added to the routing zone.
      * @param[in]  dataFormat        The data format that shall be used for the output buffer.
      *                               If the data format is undefined the output buffer will be created
     *                                using IasAudioCommonDataFormat::eIasFormatFloat32.
      * @param[in]  waveFile          Wave file that will be associated with this buffer
      * @param[out] outputBuffer      The output buffer that has been created by this method for the specified port.
      * @return                       The result of the function call.
      */
    IasResult createOutputBuffer(const IasAudioPortPtr         audioPort,
                                      IasAudioCommonDataFormat dataFormat,
                                      IasTestFrameworkWaveFilePtr waveFile,
                                      IasAudioRingBuffer**     outputBuffer);

    /**
     * @brief Destroy an output buffer for an audio port.
     *
     * @param[in]  audioPort         The audio port that shall be removed
     *
     * @return                       The result of the function call.
     */
    IasResult destroyOutputBuffer(const IasAudioPortPtr audioPort);

    /**
     * @brief Add pipeline to the test framework routing zone.
     *
     * There is only one pipeline per routing zone allowed. Adding a second pipeline will fail.
     *
     * @param[in] pipeline Pipeline that shall be added to routing zone.
     *
     * @return The result of the method call
     */
    IasResult addPipeline(IasPipelinePtr pipeline);

    /**
     * @brief Delete pipeline from the routing zone.
     */
    void deletePipeline();

    /**
     * @brief Start test framework worker
     *
     * @return The result of the method call
     */
    IasResult start();

    /**
     * @brief Stop test framework worker
     *
     * @return The result of the method call
     */
    IasResult stop();

    /**
     * @brief Process a number of periods of wave file
     *
     * This method executes processing of given number of periods of wave file.
     * First function call starts processing from the beginning of file. Next calls
     * take periods from the position in the file where previous call has finished.
     *
     * @param[in] numPeriods Number of periods to be processed
     * @return The result of the process call
     * @retval eIasOk Number of periods processed successfully
     * @retval eIasFailed Processing failed
     * @retval eIasEndOfFile End of file reached, assuming operation successful
     */
    IasResult processPeriods(uint32_t numPeriods);

    /*!
     *  @brief Get test framework routing zone parameters.
     */
    const IasTestFrameworkRoutingZoneParamsPtr getParameters() const {return mParams;}

  private:

    /*!
     *  @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasTestFrameworkRoutingZone(IasTestFrameworkRoutingZone const &other);

    /*!
     *  @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasTestFrameworkRoutingZone& operator=(IasTestFrameworkRoutingZone const &other);

    /*!
     *  @brief Create ring buffer, helper function.
     */
    IasResult createRingBuffer(const IasAudioPortPtr audioPort, IasAudioCommonDataFormat dataFormat, IasAudioRingBuffer** ringBuffer);

    /**
     * @brief Transfer one period of PCM frames from the input wave file(s) to the output wave file(s)
     */
    IasResult transferPeriod();

    /**
     * @brief Copy output data from pipeline to output ring buffer
     */
    IasResult retrieveDataFromPipeline(IasAudioSinkDevicePtr sinkDevice, IasAudioRingBuffer* buffer, uint32_t numFrames);

    /**
     * @brief Copy data from input wave file to input ring buffer
     */
    IasResult readDataFromWaveFile(IasTestFrameworkWaveFilePtr waveFile, IasAudioRingBuffer* buffer, uint32_t numFrames, uint32_t& numFramesRead);

    /**
     * @brief Copy data from output ring buffer to output wave file
     */
    IasResult writeDataIntoWaveFile(IasTestFrameworkWaveFilePtr waveFile, IasAudioRingBuffer* buffer, uint32_t numFrames, uint32_t& numFramesWritten);

    DltContext                            *mLog;
    IasTestFrameworkRoutingZoneParamsPtr  mParams;
    IasRingBufferParamsMap                mInputBufferParamsMap;
    IasRingBufferParamsMap                mOutputBufferParamsMap;
    IasPipelinePtr                        mPipeline;
    bool                             mIsRunning;
};

/**
 * @brief Function to get a IasTestFrameworkRoutingZone::IasResult as string.
 *
 * @return String carrying the result message.
 */
std::string toString(const IasTestFrameworkRoutingZone::IasResult& type);

} // namespace IasAudio

#endif // AUDIO_DAEMON2_PRIVATE_INC_TESTFWX_IASTESTFRAMEWORKROUTINGZONE_HPP_
