/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasIGenericAudioCompConfig.hpp
 * @date   2013
 * @brief  The definition of the IasIGenericAudioCompConfig base class.
 */

#ifndef IASIGENERICAUDIOCOMPCONFIG_HPP_
#define IASIGENERICAUDIOCOMPCONFIG_HPP_

#include "audio/smartx/IasProperties.hpp"
#include "audio/smartx/rtprocessingfwx/IasProcessingTypes.hpp"


namespace IasAudio {

class IasAudioStream;
class IasAudioChannelBundle;
class IasStreamParams;

/**
 * @brief A list of IasAudioChannelBundle to manage multiple audio channel bundles.
 */
typedef std::list<IasAudioChannelBundle*> IasBundlePointerList;

/**
 * @brief A list of IasAudioStream to manage multiple audio streams.
 */
typedef std::list<IasAudioStream*> IasStreamPointerList;

/**
 * @brief A list of float to manage multiple sample pointers.
 */
typedef std::list<float*> IasSamplePointerList;

/**
 * @brief Map one audio stream to multiple audio streams.
 *
 * This is used for the output stream to input streams mapping.
 */
typedef std::map<IasAudioStream*, IasStreamPointerList> IasStreamMap;

/**
 * @brief Map a stream ID to Stream params.
 *
 * The same stream ID can have multiple stream params associated that's why a multimap is
 * required here.
 */
typedef std::multimap<int32_t, IasStreamParams*> IasStreamParamsMultimap;

/**
 * @class IasIGenericAudioCompConfig
 *
 * Every module has to define a configuration class that derives from this base class in order to have
 * some common functionality like e.g. the feature to define which streams the module has to process.
 */
class IAS_AUDIO_PUBLIC IasIGenericAudioCompConfig
{
  public:
    /**
     * @brief Destructor, virtual by default.
     */
    virtual ~IasIGenericAudioCompConfig() {}

    /**
     * @brief Add stream that has to be processed.
     *
     * This method is used for the configuration of in-place processing audio components. The stream provided as parameter
     * has to be processed by the audio component.
     *
     * @param[in] streamToProcess The stream that has to be processed by the in-place processing component.
     * @param[in] pinName The pin name where this stream enters and leaves the audio module
     */
    virtual void addStreamToProcess(IasAudioStream *streamToProcess, const std::string &pinName) = 0;

    /**
     * @brief Add stream mapping from input to output stream
     *
     * This method is used for the configuration of modules that have a different amount of input and output streams
     * like e.g. a mixer. To configure a mixer you simply call this method for every input stream that you like to mix
     * to an output stream.
     *
     * @param[in] inputStream An input stream
     * @param[in] inputPinName Name of the input pin where the inputStream enters the audio module
     * @param[in] outputStream An output stream
     * @param[in] outputPinName Name of the output pin where the outputStream leaves the audio module
     */
    virtual void addStreamMapping(IasAudioStream *inputStream, const std::string &inputPinName, IasAudioStream *outputStream, const std::string &outputPinName) = 0;

    /**
     * @brief Getter method for the bundle list that has to be processed.
     *
     * Only the bundles which this component has to process are returned. The active
     * bundles returned by this method are defined by calling the method #addStreamToProcess.
     *
     * @returns The bundle list that this component has to process.
     */
    virtual const IasBundlePointerList& getBundles() const = 0;

    /**
     * @brief Getter method for the audio sample pointer list that has to be processed.
     *
     * Only the sample pointers to the sample data which this component has to process are returned.
     * The sample pointers returned by this method are defined by calling the method #addStreamToProcess.
     *
     * @returns The sample pointer list that this component has to process.
     */
    virtual const IasSamplePointerList& getAudioFrame() const = 0;

    /**
     * @brief Getter method for the stream pointer list that has to be processed.
     *
     * Only the streams which this component has to process are returned.
     * The stream pointers returned by this method are defined by calling the method #addStreamToProcess.
     *
     * @returns The stream pointer list that this component has to process.
     */
    virtual const IasStreamPointerList& getStreams() const = 0;

    /**
     * @brief Getter method for the stream mappings.
     *
     * A stream mapping maps one output stream to one or multiple input streams. This is useful
     * for mixer components to decide which input streams has to be mixed to which output streams.
     *
     * @returns The stream mapping that this component has to process.
     */
    virtual const IasStreamMap& getStreamMapping() const = 0;

    /**
     * @brief Getter method for the stream params multimap.
     *
     */
    virtual const IasStreamParamsMultimap& getStreamParams() const = 0;

    /**
     * @brief Getter method for the generic configuration properties.
     *
     * Depending on the audio module several user-defined properties can
     * be set that are required during initialization of the module.
     *
     * @returns A reference to the generic configuration properties.
     */
    virtual const IasProperties& getProperties() const = 0;

    /**
     * @brief Setter method for the generic configuration properties.
     *
     * Depending on the audio module several user-defined properties can
     * be set that are required during initialization of the module.
     */
    virtual void setProperties(IasProperties &properties) = 0;

    /**
     * @brief Set the module ID
     *
     * @param[in] moduleId The module ID of the module
     */
    virtual void setModuleId(int32_t moduleId) = 0;

    /**
     * @brief Get the module ID
     *
     * @returns The module ID of the module
     */
    virtual int32_t getModuleId() const = 0;

    /**
     * @brief Get the stream ID for a given pin name
     *
     * In case of success the stream ID is returned via the stream ID parameter.
     * In case of error the stream ID is not touched. The return value of the method
     * has to be evaluated to check if the returned stream ID is valid or not.
     *
     * @param[in] pinName The pin name for which the stream ID is requested
     * @param[out] streamId The stream ID that is related to the given pin name
     *
     * @returns The result of the method call
     * @retval eIasAudioProcOK Stream ID for given pin name successfully returned
     * @retval eIasAudioProcInvalidParam No stream ID for given pin name found
     */
    virtual IasAudioProcessingResult getStreamId(const std::string &pinName, int32_t &streamId) const = 0;

    /**
     * @brief Get the pin name for a given stream ID
     *
     * In case of success the pin name is returned via the pinName parameter.
     * In case of error the pinName is not touched. The return value of the method
     * has to be evaluated to check if the returned pinName is valid or not.
     *
     * @param[in] streamId The stream ID for which the pin name is requested
     * @param[out] pinName The pin name that is related to the given stream ID
     *
     * @returns The result of the method call
     * @retval eIasAudioProcOK Pin name for given pin name successfully returned
     * @retval eIasAudioProcInvalidParam No pin name for given stream ID found
     */
    virtual IasAudioProcessingResult getPinName(int32_t streamId, std::string &pinName) const = 0;

    /**
     * @brief Get the stream pointer for a given stream ID
     *
     * @param[in][out] stream   pointer to the stream
     * @param[in]      id       the stream id
     *
     * @returns Error code, non zero in case of error
     */
    virtual IasAudioProcessingResult getStreamById(IasAudioStream** stream, int32_t id) const = 0;
};

} //namespace IasAudio

#endif /* IASIGENERICAUDIOCOMPCONFIG_HPP_ */
