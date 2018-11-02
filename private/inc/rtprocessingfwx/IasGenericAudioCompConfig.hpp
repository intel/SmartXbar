/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasGenericAudioCompConfig.hpp
 * @date   2012
 * @brief  The definition of the IasGenericAudioCompConfig base class.
 */

#ifndef IASGENERICAUDIOCOMPCONFIG_HPP_
#define IASGENERICAUDIOCOMPCONFIG_HPP_

#include "audio/smartx/rtprocessingfwx/IasIGenericAudioCompConfig.hpp"
#include "avbaudiomodules/internal/audio/common/IasAudioLogging.hpp"


#include "audio/smartx/IasProperties.hpp"

namespace IasAudio {

class IasAudioStream;
class IasAudioChannelBundle;
class IasStreamParams;
class IasGenericAudioCompConfig;

/**
 * @brief The pair for easier access as used in the IasStreamParamsMultimap.
 */
typedef std::pair<int32_t, IasStreamParams*> IasStreamParamsPair;

/**
 * @brief This pair is used when fetching a range from the multimap via equal_range.
 */
typedef std::pair<IasStreamParamsMultimap::const_iterator, IasStreamParamsMultimap::const_iterator> IasStreamParamsIteratorPair;

/**
 * @brief A vector that contains pointers to generic audio component configurations.
 */
typedef std::vector<IasGenericAudioCompConfig*> IasGenericAudioCompConfigVector;

/**
 * @class IasGenericAudioCompConfig
 *
 * Every module has to define a configuration class that derives from this base class in order to have
 * some common functionality like e.g. the feature to define which streams the module has to process.
 */
class IAS_AUDIO_PUBLIC IasGenericAudioCompConfig : public virtual IasIGenericAudioCompConfig
{
  public:
    /**
     * @brief Constructor.
     */
    IasGenericAudioCompConfig();

    /**
     * @brief Destructor, virtual by default.
     */
    virtual ~IasGenericAudioCompConfig();

    /**
     * @brief Add stream that has to be processed.
     *
     * This method is used for the configuration of in-place processing audio components. The stream provided as parameter
     * has to be processed by the audio component.
     *
     * @param[in] streamToProcess The stream that has to be processed by the in-place processing component.
     * @param[in] pinName The pin name where this stream enters and leaves the audio module
     */
    virtual void addStreamToProcess(IasAudioStream *streamToProcess, const std::string &pinName);

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
    virtual void addStreamMapping(IasAudioStream *inputStream, const std::string &inputPinName, IasAudioStream *outputStream, const std::string &outputPinName);

    /**
     * @brief Getter method for the bundle list that has to be processed.
     *
     * Only the bundles which this component has to process are returned. The active
     * bundles returned by this method are defined by calling the method #addStreamToProcess.
     *
     * @returns The bundle list that this component has to process.
     */
    inline virtual const IasBundlePointerList& getBundles() const { return mActiveBundlePointers; }

    /**
     * @brief Getter method for the audio sample pointer list that has to be processed.
     *
     * Only the sample pointers to the sample data which this component has to process are returned.
     * The sample pointers returned by this method are defined by calling the method #addStreamToProcess.
     *
     * @returns The sample pointer list that this component has to process.
     */
    inline virtual const IasSamplePointerList& getAudioFrame() const { return mActiveAudioFrame; }

    /**
     * @brief Getter method for the stream pointer list that has to be processed.
     *
     * Only the streams which this component has to process are returned.
     * The stream pointers returned by this method are defined by calling the method #addStreamToProcess.
     *
     * @returns The stream pointer list that this component has to process.
     */
    inline virtual const IasStreamPointerList& getStreams() const { return mActiveStreamPointers; }

    /**
     * @brief Getter method for the stream mappings.
     *
     * A stream mapping maps one output stream to one or multiple input streams. This is useful
     * for mixer components to decide which input streams has to be mixed to which output streams.
     *
     * @returns The stream mapping that this component has to process.
     */
    inline virtual const IasStreamMap& getStreamMapping() const { return mStreamMap; }

    /**
     * @brief Getter method for the module identifier.
     *
     * Get the Id which can be set by the customer and used to address a
     * module via setProperties command interface.
     *
     * @returns The module identifier set by the customer or -1 if not set by the customer.
     */
    inline virtual int32_t getModuleId() const { return mModuleId; }

    /**
     * @brief Setter method for the module identifier.
     *
     * Set the Id which can be set by the customer and used to address a
     * module via setProperties command interface. -1 is an invalid module id.
     */
    inline virtual void setModuleId(int32_t moduleId) { mModuleId = moduleId; }

    /**
     * @brief Getter method for the stream params multimap.
     *
     */
    inline virtual const IasStreamParamsMultimap& getStreamParams() const { return mStreamParams; }

    /**
     * @brief Getter method for the generic configuration properties.
     *
     * Depending on the audio module several user-defined properties can
     * be set that are required during initialization of the module.
     *
     * @returns A reference to the generic configuration properties.
     */
    inline virtual const IasProperties& getProperties() const { return mProperties; }

    /**
     * @brief Setter method for the generic configuration properties.
     *
     * Depending on the audio module several user-defined properties can
     * be set that are required during initialization of the module.
     */
    inline virtual void setProperties(IasProperties &properties) { mProperties = properties; }

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
    virtual IasAudioProcessingResult getStreamId(const std::string &pinName, int32_t &streamId) const;

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
    virtual IasAudioProcessingResult getPinName(int32_t streamId, std::string &pinName) const;

    /**
     * @brief Get a stream pointer for a given ID
     *
     * @param[in][out] stream  pointer to the stream
     * @param[in]      id      the id of the stream
     * 
     * @return The result of the method call
     * @retval eIasAudioProcOK             all went good, stream is found
     * @retval eIasAudioProcInvalidStream  stream was not found
     */
    virtual IasAudioProcessingResult getStreamById(IasAudioStream** stream, int32_t id) const;

  protected:
    using IasPinNameStreamIdMap = std::map<std::string, int32_t>;
    using IasStreamIdPinNameMap = std::map<int32_t, std::string>;

    IasStreamPointerList                mActiveStreamPointers;    //!< Pointers to the streams to be processed
    IasBundlePointerList                mActiveBundlePointers;    //!< Pointers to the bundles to be processed
    IasSamplePointerList                mActiveAudioFrame;        //!< Pointers to the samples to be processed
    IasStreamMap                        mStreamMap;               //!< Maps one output stream to one or multiple input streams
    IasStreamParamsMultimap             mStreamParams;            //!< Stream params multimap for easy access to bundle index, etc.
    IasProperties                       mProperties;              //!< Generic properties of the component.
    int32_t                          mModuleId;                //!< Id which can be set by the customer and used to address a module via setProperties cmd interface.
    IasPinNameStreamIdMap               mPinNameStreamIdMap;      //!< A map to get the stream id from a given pin name
    IasStreamIdPinNameMap               mStreamIdPinNameMap;      //!< A map to get the pin name from a given stream id
    DltContext                         *mLogContext;              //!< The log context

  private:
    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasGenericAudioCompConfig& operator=(
        IasGenericAudioCompConfig const &other);
};

} //namespace IasAudio

#endif /* IASGENERICAUDIOCOMPCONFIG_HPP_ */
