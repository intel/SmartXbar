/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file   IasPipeline.hpp
 * @date   2016
 * @brief  Pipeline for hosting a cascade of processing modules.
 */

#ifndef IASPIPELINE_HPP_
#define IASPIPELINE_HPP_


#include "avbaudiomodules/audio/common/IasAudioCommonTypes.hpp"
#include "avbaudiomodules/internal/audio/common/IasAudioLogging.hpp"
#include "audio/smartx/rtprocessingfwx/IasBaseAudioStream.hpp"
#include "smartx/IasAudioTypedefs.hpp"
#include "model/IasProcessingModule.hpp"

/*!
 * @brief namespace IasAudio
 */
namespace IasAudio {

class IasAudioRingBuffer;
class IasAudioStream;
class IasAudioPort;

/*!
 * @brief Documentation for class IasPipeline
 *
 * A Pipeline can host a cascade of processing modules.
 */
class IAS_AUDIO_PUBLIC IasPipeline
{

  public:
    /**
     * @brief The result type for the IasPipeline methods
     */
    enum IasResult
    {
      eIasOk,               //!< Ok, Operation successful
      eIasFailed,           //!< Other error (operation failed)
    };


    /*!
     *  @brief Constructor.
     */
    IasPipeline(IasPipelineParamsPtr  params,
                IasPluginEnginePtr    pluginEngine,
                IasConfigurationPtr   configuration);

    /*!
     *  @brief Destructor, virtual by default.
     */
    virtual ~IasPipeline();

    /*!
     *  @brief Initialization.
     */
    IasResult init();

    /**
     * @brief Get access to the pipeline parameters
     *
     * @return A read-only pointer to the pipeline parameters
     */
    const IasPipelineParamsPtr getParameters() const { return mParams; }

    /**
     * @brief Add audio input pin to pipeline
     *
     * @param[in] pipelineInputPin Pin that shall be added as an input to the pipeline
     *
     * @return The result of the method call
     * @retval eIasOk Method succeeded.
     * @retval eIasFailed Method failed. Details can be found in the log.
     */
    IasResult addAudioInputPin(IasAudioPinPtr pipelineInputPin);

    /**
     * @brief Add audio output pin to pipeline
     *
     * @param[in] pipelineOutputPin Pin that shall be added as an output to the pipeline
     *
     * @return The result of the method call
     * @retval eIasOk Method succeeded.
     * @retval eIasFailed Method failed. Details can be found in the log.
     */
    IasResult addAudioOutputPin(IasAudioPinPtr pipelineOutputPin);

    /**
     * @brief Delete an audio pin (audio input pin or audio output pin) from pipeline
     *
     * The pin has to be added to the pipeline before by one of the methods
     * IasPipeline::addAudioInputPin or IasPipeline::addAudioOutputPin.
     *
     * @param[in] pipelineOutputPin Pin that shall be deleted from pipeline
     */
    void deleteAudioPin(IasAudioPinPtr pipelinePin);

    /**
     * @brief Add an audio input/output pin to the processing module
     *
     * An input/output pin has to be used for modules doing in-place processing on this pin.
     *
     * @param[in] module Module where the pin shall be added
     * @param[in] inOutPin Pin that shall be added to module
     *
     * @return The result of the method call
     * @retval eIasOk Method succeeded.
     * @retval eIasFailed Method failed. Details can be found in the log.
     */
    IasResult addAudioInOutPin(IasProcessingModulePtr module, IasAudioPinPtr inOutPin);

    /**
     * @brief Delete an audio input/output pin from the processing module
     *
     * @param[in] module Module where the pin shall be added
     * @param[in] inOutPin Pin that shall be added to module
     */
    void deleteAudioInOutPin(IasProcessingModulePtr module, IasAudioPinPtr inOutPin);

    /**
     * @brief Add audio pin mapping
     *
     * A pin mapping is required if the module is not able to do in-place processing, e.g. if the number of channels
     * for input and output pins is different (up-/down-mixer). Another example is a mixer module, where one output
     * pin is created from several input pins. If several input pins are required to create the signal for one output
     * pin like in the mixer example than this method has to be called multiple times with a different input pin but
     * with the same, single output pin.
     *
     * @param[in] module Module where the pin mapping shall be added
     * @param[in] inputPin Input pin of the mapping pair
     * @param[in] outputPin Output pin of the mapping pair
     *
     * @return The result of the method call
     * @retval eIasOk Method succeeded.
     * @retval eIasFailed Method failed. Details can be found in the log.
     */
    IasResult addAudioPinMapping(IasProcessingModulePtr module, IasAudioPinPtr inputPin, IasAudioPinPtr outputPin);

    /**
     * @brief Delete audio pin mapping
     *
     * @param[in] module Module where the pin mapping shall be deleted
     * @param[in] inputPin Input pin of the mapping pair to be deleted
     * @param[in] outputPin Output pin of the mapping pair to be deleted
     */
    void deleteAudioPinMapping(IasProcessingModulePtr module, IasAudioPinPtr inputPin, IasAudioPinPtr outputPin);

    /**
     * @brief Add processing module to pipeline
     *
     * @param[in] module The module that shall be added to the pipeline
     *
     * @return The result of the method call
     * @retval eIasOk Method succeeded.
     * @retval eIasFailed Method failed. Details can be found in the log.
     */
    IasResult addProcessingModule(IasProcessingModulePtr module);

    /**
     * @brief Delete processing module from the pipeline
     *
     * A previously added module can be deleted from a pipeline by using this method. Deleting a module
     * will also remove all links to and from the module.
     *
     * @param[in] module The module that shall be deleted from the pipeline
     */
    void deleteProcessingModule(IasProcessingModulePtr module);

    /**
     * @brief Link an audio output pin to an audio input pin
     *
     * The output pin and the input pin can also be combined input/output pins. These combined pins
     * are used for in-place processing components.
     *
     * By means of this function, the links between the processing modules can be configured,
     * in order to set up a cascade of processing modules.
     *
     * The linking between two audio pins must be biuniqe. This means that an input pin cannot
     * be linked to more than one output pins. Of course, also an output pin cannot receive
     * PCM frames from more than one input pin.
     *
     * There are two types how an output pin can be linked to an input pin:
     *
     * @li \ref IasAudioPinLinkType "IasAudioPinLinkType::eIasAudioPinLinkTypeImmediate"
     *          represents the normal linking from an output pin to an input pin.
     * @li \ref IasAudioPinLinkType "IasAudioPinLinkType::eIasAudioPinLinkTypeDelayed"
     *          represents a link with a delay of one period.
     *
     * The delayed link type is necessary to implement feed-back loops. A feed-back loop must
     * contain at least one delayed link, because otherwise there is no solution for breaking
     * up the closed loop of dependencies between the modules' input and output signals.
     * This is similar to the structure of a time-discrete IIR filter, which cannot have a
     * feed-back path with zero delay.
     *
     * @param[in] outputPin  Audio output or combined input/output pin
     * @param[in] inputPin   Audio input or combined input/output pin
     * @param[in] linkType   Link type: immediate or delayed
     *
     * @return The result of the method call
     * @retval eIasOk Method succeeded.
     * @retval eIasFailed Method failed. Details can be found in the log.
     */
    IasResult link(IasAudioPinPtr outputPin, IasAudioPinPtr inputPin, IasAudioPinLinkType linkType);

    /**
     * @brief Unlink an audio output pin from an audio input pin
     *
     * The output pin and the input pin can also be combined input/output pins. These combined pins
     * are used for in-place processing components.
     *
     * @param[in] outputPin Audio output or combined input/output pin
     * @param[in] inputPin Audio input or combined input/output pin
     */
    void unlink(IasAudioPinPtr outputPin, IasAudioPinPtr inputPin);

    /**
     * @brief Link an input port with a pipeline pin
     *
     * If a pipeline has been added to a routing zone, its input pins and output pins have to be
     * linked with the appropriate audio ports. For this purpose, this function can be used
     *
     * @li to link a routing zone input port to a pipeline input pin, or
     * @li to link a pipeline output pin to an audio sink device input port.
     *
     * @param[in] inputPort    Input port of the routing zone or of the audio sink device
     * @param[in] pipelinePin  Input pin or output pin of the pipeline
     *
     * @return The result of the method call
     * @retval eIasOk Method succeeded.
     * @retval eIasFailed Method failed. Details can be found in the log.
     */
    IasResult link(IasAudioPortPtr inputPort, IasAudioPinPtr pipelinePin);

    /**
     * @brief Unlink an input port from a pipeline pin
     *
     * @param[in] inputPort    Input port of the routing zone or of the audio sink device
     * @param[in] pipelinePin  Input pin or output pin of the pipeline
     *
     * @return The result of the method call
     * @retval eIasOk Method succeeded.
     * @retval eIasFailed Method failed. Details can be found in the log.
     */
    void unlink(IasAudioPortPtr inputPort, IasAudioPinPtr pipelinePin);

    /**
     * @brief Dump the connection parameters of all audio pins and of all processing modules.
     */
    void dumpConnectionParameters() const;

    void dumpProcessingSequence() const;

    /**
     * @brief Helper function to clear the configuration by setting the shared pointer to null
     */
    void clearConfig() { mConfiguration = nullptr;};

    /**
     * @brief Initialize the pipeline's audio chain of processing modules.
     *
     * After the topology of the pipeline has been defined, i.e., after
     *
     * @li all modules have been created and initialized
     * @li all audio pins have been created and linked with each other,
     *
     * This function has to be called before the pipeline can be processed. This function analyzes
     * the signal dependencies and identifies the scheduling order of the processing modules.
     * Furthermore, the function identifies how many audio streams have to be created and how the
     * audio streams are mapped to the audio pins. Finally, the audio chain is set up, i.e. all
     * processing modules and and audio streams are added to the audio chain.
     *
     * After this function has been executed successfully, the pipeline is ready to be used.
     *
     * @param[in,out] pipeline Pointer to the pipeline whose audio chain shall be initialized.
     *
     * @return The result of the method call
     * @retval eIasOk Method succeeded.
     * @retval eIasFailed Method failed. Details can be found in the log.
     */
    IasResult initAudioChain();

    /**
     * @brief Provide input data for the pipeline.
     *
     * @param[in]  routingZoneInputPort The audio input port whose PCM frames shall be transferred from
     *                                  the associated ring buffer to the audio stream associated with
     *                                  the pipeline input pin. The routingZoneInputPort must have been
     *                                  linked with a pipeline input pin before.
     * @param[in]  inputBufferOffset    Offset within the ring buffer associated with the routingZoneInputPort.
     * @param[in]  numFramesToRead      Number of PCM frames that shall be read from the routing zone input ports.
     * @param[in]  numFramesToWrite     Number of PCM frames that shall be written into the pipeline. The PCM
     *                                  frames will be padded with zeros, if numFramesToWrite > numFramesToRead.
     * @param[out] numFramesRemaining   Returns the number of missing PCM frames, which need to be provided before
     *                                  the IasPipeline::process method can be executed.
     * @return                          The result of the method call.
     */
    IasResult provideInputData(const IasAudioPortPtr routingZoneInputPort,
                               uint32_t              inputBufferOffset,
                               uint32_t              numFramesToRead,
                               uint32_t              numFramesToWrite,
                               uint32_t*             numFramesRemaining);

    /**
     * @brief Execute the audio chain of processing modules.
     *
     * This method iterates over all audio processing components and executes their processing function if
     * the module is enabled.
     */
    void process();

    /**
     * @brief Retrieve output data from the pipeline for a given sink device.
     *
     * @param[in]  sinkDevice           The sink device for which the output data shall be retrieved.
     * @param[in]  destinAreas          The areas of the ring buffer associated with the sink device.
     * @param[in]  destinFormat         The format of the ring buffer associated with the sink device.
     * @param[in]  destinNumFrames      The number of frames that shall be written into the ring buffer.
     * @param[in]  destinOffset         The offset within the ring buffer (the index where the first sample shall be stored).
     */
    void retrieveOutputData(IasAudioSinkDevicePtr     sinkDevice,
                            IasAudioArea const       *destinAreas,
                            IasAudioCommonDataFormat  destinFormat,
                            uint32_t                  destinNumFrames,
                            uint32_t                  destinOffset);

    /**
     * @brief Type definition for a vector of processing modules. This type is used by the method IasPipeline::getProcessingModules.
     */
    using IasProcessingModuleVector = std::vector<IasProcessingModulePtr>;

    /**
     * @brief Get a vector of all processing modules that belong to the pipeline.
     *
     * @param[out] processingModules  Vector of processing modules. The vector must be owned by the caller. The function
     *                                clears the vector and fills it with all modules that belong to the pipeline.
     */
    void getProcessingModules(IasProcessingModuleVector* processingModules);

    /**
     * @brief Type definition for a vector of audio pins. This type is used by the method IasPipeline::getAudioPins.
     */
    using IasAudioPinVector = std::vector<IasAudioPinPtr>;

    /**
     * @brief Get a vector of all audio pins that belong to the pipeline.
     *
     * @param[out] audioPins  Vector of audio pins. The vector must be owned by the caller. The function
     *                        clears the vector and fills it with all audio pins that belong to the pipeline.
     */
    void getAudioPins(IasAudioPinVector* audioPins);


    /**
     * @brief Trigger the start of probing at a certain pin
     *
     * @param[in] pin        pointer to the audio pin that where the data will be probed
     * @param[in] filename   the name of the wav file related to the probing
     * @param[in] numSeconds duration of probing in seconds
     * @param[in] inject     bool value, true = data is injected, false = data is recorded
     *
     * @return               Result of the function call
     */
    IasResult startPinProbing(IasAudioPinPtr pin,
                              std::string fileName,
                              uint32_t numSeconds,
                              bool inject);

    /**
     * @brief Trigger the stop of probing at a certain pin
     *
     * @param[in] pin        pointer to the audio pin that where the data will be probed
     */
    void stopPinProbing(IasAudioPinPtr pin);

    /*
     * @brief Get a vector to pipeline input pins.
     *
     * @return Vector of pipeline input pins
     */
    IasAudioPinVector getPipelineInputPins();

    /**
     * @brief Get audio port connected to pipeline pin
     *
     * @param[in] pipelinePin Pointer to the pipeline pin
     * @param[out] inputPort Pointer to the audio port
     *
     * @return              The result of the method call
     * @retval eIasOk       Method succeeded.
     * @retval eIasFailed   Method failed. Details can be found in the log.
     */
    IasResult getPipelinePinConnectionLink(IasAudioPinPtr pipelinePin, IasAudioPortPtr& inputPort);

    /**
     * @brief Get a vector to pipeline output pins.
     *
     *  @return Vector of pipeline output pins
     */
    IasAudioPinVector getPipelineOutputPins();

    /**
     * @brief Get a vector to pipeline sink/source pin name and connection type.
     *
     * @param[in] audioPin      Pin pointer
     * @param[out] audioPinMap   Vector to contain source/sink to the pin and connection type
     */
    void getAudioPinMap(IasAudioPinPtr audioPin, std::vector<std::string>& audioPinMap);

  private:

    /*!
     *  @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasPipeline(IasPipeline const &other);

    /*!
     *  @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasPipeline& operator=(IasPipeline const &other);

    /*!
     * @brief Private method: identify in which order the processing modules have to be scheduled.
     */
    void identifyProcessingSequence();

    /*!
     * @brief Private method: Identify all required audio streams and initialize them.
     *
     * @return              The result of the method call
     * @retval eIasOk       Method succeeded.
     * @retval eIasFailed   Method failed. Details can be found in the log.
     */
    IasResult initAudioStreams();

    /*!
     * @brief Private method to add an AudioPin to the pipeline's mAudioPinMap.
     *
     * The mAudioPinMap contains all audio pins that belong to this pipeline, and their
     * connection paramaters (such as the module, which the AudioPin is connected with).
     *
     * @param[in] audioPin  The audio pin that shall be added to the mAudioPinMap.
     * @param[in] module    The processing module, which the audio pin is connected with.
     *
     * @return              The result of the operation.
     * @retval eIasOk       In case of success.
     * @retval eIasFailed   If the audio pin has been already added with different connection parameters.
     */
    IasResult addAudioPinToMap(IasAudioPinPtr         audioPin, IasProcessingModulePtr module);

    //! Constant number representing an undefined audioStreamId.
    static const uint32_t cAudioStreamIdUndefined = 0xFFFFFFFF;


    /*!
     * @brief Parameters describing how an audio pin is connected with its neighbors.
     */
    struct IasAudioPinConnectionParams
    {
        /**
         * @brief Constructor
         */
        inline IasAudioPinConnectionParams()
          :thisPin(nullptr)
          ,sourcePin(nullptr)
          ,sinkPin(nullptr)
          ,isInputDataDelayed(false)
          ,processingModule(nullptr)
          ,audioPort(nullptr)
          ,inputDataAvailable(false)
          ,outputDataAvailable(false)
          ,audioStreamId(cAudioStreamIdUndefined)
          ,audioPortRingBuffer(nullptr)
          ,audioPortChannelIdx(0)
          ,numBufferedFramesInput(0)
        {
          audioChannelBuffers.clear();
        }

        IasAudioPinPtr         thisPin;                //!< handle of the pin itself
        IasAudioPinPtr         sourcePin;              //!< handle of the source pin (which sends PCM data to this pin)
        IasAudioPinPtr         sinkPin;                //!< handle of the sink pin (which receives PCM data from this pin)
        bool                   isInputDataDelayed;     //!< true, there is a delay element between the source pin and the input of this pin
        IasProcessingModulePtr processingModule;       //!< handle of the processing module (if the pin is linked to a module), otherwise nullptr
        IasAudioPortPtr        audioPort;              //!< input port of the routing zone (if pin is pipeline input)
                                                       //!< or input port of the audio sink device (if pin is pipeline output)

        bool                   inputDataAvailable;     //!< temporary flag, used while the function identifyProcessingSequence is running
        bool                   outputDataAvailable;    //!< temporary flag, used while the function identifyProcessingSequence is running

        uint32_t               audioStreamId;          //!< the ID of the audio stream associated with this audio pin

        IasAudioRingBuffer*    audioPortRingBuffer;    //!< ring buffer of the linked audio port, if pin is pipeline input/output
        uint32_t               audioPortChannelIdx;    //!< channel offset (the first channel of the ring buffer that belongs to this audio port)

        std::vector<float*>    audioChannelBuffers;    //!< vector of pointers to the channel buffers, used for pipeline input & output pins

        uint32_t               numBufferedFramesInput; //!< Number of PCM frames that are buffered at the input of the pipeline.

    };


    /**
     * Type definitions for a map of all pins that belong to this pipeline.
     * The second parameter of the pair/map describes how each pin is connected to its environment.
     */
    using IasAudioPinConnectionParamsPtr = std::shared_ptr<IasAudioPinConnectionParams>;
    using IasAudioPinConnectionParamsSet = std::set<IasAudioPinConnectionParamsPtr>;
    using IasAudioPinPair =  std::pair<IasAudioPinPtr, IasAudioPinConnectionParamsPtr>;
    using IasAudioPinMap  =   std::map<IasAudioPinPtr, IasAudioPinConnectionParamsPtr>;

    /**
     * Type definition for a map of all pins that are connected to one sink device.
     * For the pins we use a set, because a pin can only be added once per device.
     */
    using IasSinkPinMap = std::map<std::string, IasAudioPinConnectionParamsSet>;

    /**
     * @brief Parameters describing how a processing module is connected with its ewnvironment.
     */
    struct IasProcessingModuleConnectionParams
    {
        /**
         * @brief Constructor
         */
        inline IasProcessingModuleConnectionParams()
          :isAlsreadyProcessed(false)
        {}

        bool            isAlsreadyProcessed; //!< Flag indicating whether the sequencer has already scheduled the module for processing
    };

    /**
     * Type definitions for a map of all modules that belong to this pipeline.
     * The second parameter of the pair/map describes how each module is connected to its environment.
     */
    using IasProcessingModulePair = std::pair<IasProcessingModulePtr, IasProcessingModuleConnectionParams>;
    using IasProcessingModuleMap  =  std::map<IasProcessingModulePtr, IasProcessingModuleConnectionParams>;

    /**
     * Type definition for a list that contains all processing modules in their scheduling order.
     */
    using IasProcessingModuleSchedulingList = std::list<IasProcessingModulePtr>;


    /**
     * @brief Parameters describing how an audio stream is connected
     */
    struct IasAudioStreamConnectionParams
    {
      IasAudioStream*  audioStream;
      uint32_t      numChannels;
    };

    using IasAudioStreamVector = std::vector<IasAudioStreamConnectionParams>;


    /*!
     * @brief Private method: createAudioChannelBuffers
     *
     * Create the audioChannelBuffers an audio pin and add them to the pinConnectionParams.
     *
     * @param[in] pinConnectionParams  Connection parameters of the audio pin whose audio channel buffers shall be created.
     * @param[in] numChannels          Number of channels (number of buffers that shall be created).
     * @param[in] periodSize           Period size (size of the buffers that shall be created).
     */
    void createAudioChannelBuffers(IasAudioPinConnectionParamsPtr pinConnectionParams,
                                   uint32_t numChannels,
                                   uint32_t periodSize);

    /*!
     * @brief Private method: eraseAudioChannelBuffers
     *
     * Erase the audio channel buffers that belong to the audio pin that is specified by the given pinConnectionParams.
     *
     * @param[in] pinConnectionParams  Pin connection parameters of the audio pin whose audio channel buffers shall be erased.
     */
    void eraseAudioChannelBuffers(IasAudioPinConnectionParamsPtr pinConnectionParams);

    /*!
     * @brief Private method: eraseAudioChannelBuffers
     *
     * Erase the audio channel buffers that belong to the specified audio pin.
     *
     * @param[in] audioPin  Audio pin whose audio channel buffers shall be erased.
     */
    void eraseAudioChannelBuffers(IasAudioPinPtr audioPin);

    /**
     * @brief Retrieve output data from a given pin of the pipeline.
     *
     * @param[in]  pinConnectionParams  The pin connection params including the pin itself for which to retrieve the output data.
     * @param[in]  destinAreas          The areas of the ring buffer associated with the sink device.
     * @param[in]  destinFormat         The format of the ring buffer associated with the sink device.
     * @param[in]  destinNumFrames      The number of frames that shall be written into the ring buffer.
     * @param[in]  destinOffset         The offset within the ring buffer (the index where the first sample shall be stored).
     */
    void retrieveOutputData(const IasAudioPinConnectionParamsPtr pinConnectionParams,
                            IasAudioArea const       *destinAreas,
                            IasAudioCommonDataFormat  destinFormat,
                            uint32_t                  destinNumFrames,
                            uint32_t                  destinOffset);


    /**
     * @brief Get a vector to pipeline sink/source pin name and connection type.
     *
     *  @param[in] audioPin      Pin pointer
     *  @param[out] audioPinMap   Vector to contain source/sink to the pin and connection type
     */
    void getPinConnectionInfo(IasAudioPinPtr audioPin, std::vector<std::string>& audioPinMap);


    /**
     * @brief Retrieve the connection parameters for a given pin
     *
     * @param[in] pin pointer to the pin
     *
     * @returns pointer to the pinConnectionParams, nullptr if pin is not found
     **/
    const IasAudioPinConnectionParamsPtr getPinConnectionParams(IasAudioPinPtr pin) const;

    /*
     * Member Variables
     */
    DltContext*            mLog;                 //!< Handle for the DLT log object
    IasPipelineParamsPtr   mParams;              //!< Parameters of the pipeline.
    IasAudioPinMap         mAudioPinMap;         //!< Map with all pins that have been added.
    IasProcessingModuleMap mProcessingModuleMap; //!< Map with all modules and their associated audio pins
    IasProcessingModuleSchedulingList mProcessingModuleSchedulingList; //!< List with all modules (in scheduling order)
    IasAudioChainPtr       mAudioChain;          //!< Pointer to the audio chain that has been created by this pipeline.
    IasAudioStreamVector   mAudioStreams;        //!< Vector with the connection parameters of all audioStreams.
    IasPluginEnginePtr     mPluginEngine;        //!< Handle to the plugin engine.
    IasConfigurationPtr    mConfiguration;       //!< Handle to the central configuration.
    IasSinkPinMap          mSinkPinMap;          //!< Map with all pins linked to a sink device.
    IasAudioPinVector      mPipelineInputPins;   //!< Vector with pipeline input pins
    IasAudioPinVector      mPipelineOutputPins;  //!< Vector with pipeline output pins
};


/**
 * @brief Function to get a IasPipeline::IasResult as string.
 *
 * @return String carrying the result message.
 */
std::string IAS_AUDIO_PUBLIC toString(const IasPipeline::IasResult& type);


} // namespace IasAudio

#endif // IASPIPELINE_HPP_
