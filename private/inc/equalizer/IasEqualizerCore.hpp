/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasEqualizerCore.hpp
 * @date   October 2016
 * @brief  the header for the equalizer core module
 */

#ifndef IASXEQUALIZERCORE_HPP_
#define IASXEQUALIZERCORE_HPP_


#include "helper/IasRamp.hpp"
#include "audio/smartx/rtprocessingfwx/IasAudioStream.hpp"
#include "audio/smartx/rtprocessingfwx/IasGenericAudioCompCore.hpp"
#include "audio/common/IasAudioCommonTypes.hpp"
#include "filter/IasAudioFilter.hpp"
#include "audio/equalizerx/IasEqualizerCmd.hpp"


namespace IasAudio {


/**
 *  @brief Struct defining the parameters of one channel, which is
 *         processed by the IasEqualizerCore. This struct is for
 *         internal use of the IasEqualizerCore component.
 */
struct IasEqualizerCoreChannelParams
{
  uint32_t            numActiveFilters;      ///< number of active filters
  uint32_t            numActiveFilterStages; ///< number of associated 2nd order stages
  IasAudioFilterParams  *filterParamsTab;       ///< table, contains for each filter: the filter parameters
  uint32_t           *filterStageIndexTab;   ///< table, contains for each filter: the index of the associated 2nd order stage
};
/**
 *  @brief Struct defining the paraneters of one bundle, which is
 *         processed by the IasEqualizerCore. This struct is for
 *         internal use of the IasEqualizerCore component.
 */
struct IasEqualizerCoreBundleParams
{
  uint32_t            numActiveFilterStages;
};


struct IasEqualizerCoreStreamStatus
{
  bool *isGainRamping; ///< vector[mNumFilterStagesMax] defining for each filter stage, whether gain is currently ramping
};


using IasEqualizerCoreStreamStatusMap = std::map<int32_t, IasEqualizerCoreStreamStatus>;

/*!
 *  @class IasEqualizerCore
 *  @brief This class implements the Equalizer Core Module.
 *
 *  The Equalizer Core Module provides the functions that are used for
 *  implementing a Car Equalizer or a User Equalizer (Bass/Treble/Mid/...).
 *
 *  It inherits from the IasGenericAudioCompCore, because it represents an
 *  audio processing component, as well as from the IasAudioFilterCallback
 *  because it has to implement the callback function for the underlying
 *  IasAudioFilter component.
 */
class IAS_AUDIO_PUBLIC IasEqualizerCore : public IasGenericAudioCompCore, IasAudioFilterCallback
{
  public:
   /**
    * @brief Constructor.
    *
    * @param[in] config                      pointer to the component configuration.
    * @param[in] componentName               unique component name.
    */
    IasEqualizerCore(const IasIGenericAudioCompConfig* config, const std::string& componentName);

    /**
     * @brief the reset function, inherited by IasGenericAudioComp
     * @return error                         the error code, eIasAudioProcOK if all is ok
     */
    IasAudioProcessingResult reset() override;

    /**
     * @brief get max number of filters
     * @return                               max number of filters
     */
    inline uint32_t getNumFilterStagesMax() const
    {
      return mNumFilterStagesMax;
    }

    /**
     * @brief the initialization function, inherited by IasGenericAudioComp
     * @return error                              the error code
     * @retval eIasAudioProcInitializationFailed  the core could not be initialized
     * @retval eIasAudioProcNotEnoughMemory       out of memory, the core could not be initialized
     * @retval eIasAudioProcOK                    no error
     */
    IasAudioProcessingResult init() override;

    /*!
     * @brief Return the parameters of a specific filter in a channel
     *
     * @param[in] streamId The stream for which the information is desired
     * @param[in] filterIdx The index of the filter in the channel
     * @param[in] channel The channel for which the information is desired
     * @param[in] params pointer, where the filter params will be stored
     *
     * @return    errorCode
     * @retval    eIasAudioProcOK Sucess, in this case the pointer will carry the number of configured filters
     * @retval    eIasAudioProcInvalidParam One of the parameters were not valid or pointer was NULL
     */
    IasAudioProcessingResult getFilterParamsForChannel(int32_t streamId, uint32_t filterIdx, uint32_t channel,
                                                       IasAudioFilterParams *params);

    /*!
     * @brief Return the parameters of a specific filter in a channel
     *
     * @param[in] streamId The stream for which the information is desired
     * @param[in] filterIdx The index of the filter in the channel
     * @param[in] channel The channel for which the information is desired
     * @param[in] params pointer, where the filter params will be stored
     *
     * @return    errorCode
     * @retval    eIasAudioProcOK Sucess, in this case the pointer will carry the number of configured filters
     * @retval    eIasAudioProcInvalidParam One of the parameters were not valid or pointer was NULL
     */
    IasAudioProcessingResult getNumFiltersForChannel(int32_t streamId, uint32_t channel, uint32_t* numFilters);

    /*!
     *  @brief Set the gradient that is used for ramping the filter gain.
     *
     *  The gradient specifies by how many dB the filter gain is ramped up/down
     *  for each audio frame.
     *
     *  @param[in] streamId  Stream, whose ramp gardient shall be updated.
     *  @param[in] gradient  Gain increase/decrease, expressed in dB per frame.
     *                       Must be between 0.01 dB and 6.0 dB.
     */
    IasAudioProcessingResult setRampGradientSingleStream(int32_t streamId, float gradient);

    /*!
     *  @brief Ramp the filter such that it approaches the new target gain.
     *
     *  This function triggers a ramping of the filter gain for a single filter within
     *  a single channel such that the filter gain approaches the specified darget gain.
     *  The use of this function is reasonable only for peak filters or shelving
     *  filters. For any other filter type, this function does not have any effect.
     *
     *  @return                             Error code.
     *  @retval eIasAudioProcOK             Success!
     *  @retval eIasAudioProcInvalidParam   The specified parameters are not valid.
     *
     *  @param[in] streamId           ID of the stream, whose filter gain shall be updated.
     *  @param[in] filterId           ID of the filter stage, whose gain shall be updated.
     *                                ID=0 denotes the first filter within the cascade of filters,
     *                                ID=1 denotes the second filter, and so on.
     *  @param[in] targetGain         New gain value in linear representation (i.e., not in dB).
     */
    IasAudioProcessingResult rampGainSingleStreamSingleFilter(int32_t streamId, uint32_t filterId,
                                                              float targetGain);

    /*!
     *  @brief Declare the filter parameters for one stream.
     *
     *  This function sets the parameters for the cascade of filters for one single
     *  stream. Depending on the parameter @a channelIdTable, the function sets the filter
     *  parameters either for all channels of this stream (if @a channelIdTable is empty)
     *  or only for those channels that are listed in @a channelIdTable.
     *
     *  The filter cascade is specified by the parameter @a filterParamsTable. Each
     *  element of this vector describes the parameters of one single filter of
     *  the cascade. The following filter types are supported: peak filters (2nd order),
     *  shelving filters (1st or 2nd order), low-pass and high-pass filters (any order).
     *  Higher order filters (order > 2) are implemented as a cascade of 2nd order
     *  sections (this is done internally by the IasEqualizerCore component). See the
     *  type definition of IasAudioFilterParams for details.
     *
     *  The number of requested filter sections must not exceed the number of filter sections
     *  that have been configured by IasEqualizerCoreConfig::setNumFilterStagesMax().
     *
     *  @return                           Error code.
     *  @retval eIasAudioProcOK           Success!
     *  @retval eIasAudioProcNoSpaceLeft  The number of requested filters exceeds the number
     *                                    of filter sections that have been specified by the
     *                                    equalizer configuration, method
     *                                    IasEqualizerCoreConfig::setNumFilterStagesMax().
     *
     *  @param[in] streamId               ID of the stream, whose filter parameters shall be updated.
     *  @param[in] channelIdTable         Vector containing the channel IDs, whose filter parameters
     *                                    shall be updated. ID=0 denotes the first channel belonging
     *                                    to this stream, ID=1 denotes the second channel, and so on.
     *                                    An empty vector indicates that all channels belonging to
     *                                    this stream shall be updated.
     *  @param[in] filterParamsTable      Vector containing the parameters of each filter.
     */
    IasAudioProcessingResult setFiltersSingleStream(int32_t streamId,
                                                    std::vector<uint32_t> const &channelIdTable,
                                                    std::vector<IasAudioFilterParams> const &filterParamsTable);
    /**
     *  @brief Destructor, virtual by default.
     */
    virtual ~IasEqualizerCore();


  private:

    /*
     *  @brief Implementation of the callback function, which is called by the underlying IasAudioFilter
     */
    void gainRampingFinished(uint32_t channel, float gain, uint64_t callbackUserData) override;

    /*!
     *  @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasEqualizerCore(IasEqualizerCore const& other) = delete;
    /*!
     *  @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasEqualizerCore& operator=(IasEqualizerCore const& other) = delete;

    /**
     *  @brief Move constructor, private unimplemented to prevent misuse.
     */
    IasEqualizerCore(IasEqualizerCore&& other) = delete;

    /**
     *  @brief Move assignment operator, private unimplemented to prevent misuse.
     */
    IasEqualizerCore& operator=(IasEqualizerCore&& other) = delete;

    /**
     * @brief the processing function, inherited by IasGenericAudioComp
     * @return error an error code of type IasAudioProcessingResult
     */
    IasAudioProcessingResult processChild() override;

    // Member variables
    std::list<IasAudioChannelBundle*>  mOutputBundlesList;  //!< Sorted+unique list of all output bundles
    uint32_t                              mNumFilterStagesMax; //!< Maximum number of filter stages per channel
    uint32_t                              mNumBundles;         //!< Number of bundles
    uint32_t                              mNumStreams;         //!< Number of streams
    IasEqualizerCoreChannelParams          **mChannelParams;      //!< Channel-specific parameters [mNumBundles][cIasNumChannelsPerBundle]
    IasAudioFilter                        ***mFilters;            //!< Two-dim. array [mNumBundles][mNumFilterStagesMax] with pointers to the filter objects
    IasEqualizerCoreBundleParams            *mBundleParams;       //!< Bundle params
    IasEqualizerCoreStreamStatusMap          mStreamStatusMap;    //!< <ap carrying the stream-specific status flags
    std::string                              mTypeName;           //!< Name of the module type
    std::string                              mInstanceName;       //!< Name of the module instance
    DltContext                              *mLogContext;         //!< The log context for the equalizer
};


} //namespace IasAudio

#endif /* IASXEQUALIZERCORE_HPP_ */
