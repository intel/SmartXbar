/*
  * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file    IasEqualizerConfiguration.hpp
 * @brief   Configuration class for the equalizer core module.
 * @date    October 30, 2012
 */


#ifndef IASEQUALIZERCONFIGURATION_HPP_
#define IASEQUALIZERCONFIGURATION_HPP_

#include "audio/smartx/rtprocessingfwx/IasAudioStream.hpp"
#include "audio/smartx/rtprocessingfwx/IasIModuleId.hpp"

namespace IasAudio {


struct IasEqConfigFilterParams
{
  uint32_t            freq;     ///< cut-off frequency or mid frequency of the filter
  float               gain;     ///< gain, linear value
  float               quality;  ///< quality, required only for band-pass and peak filters
  IasAudioFilterTypes type;     ///< filter type; the type IasAudioFilterTypes is defined in IasProcessingTypes.hpp
  uint32_t            order;    ///< filter order
};



struct IasEqConfigFilterInitParams
{
  IasEqConfigFilterParams params;  ///< equalizer config params
  bool               isInit;  ///< flag is param are initialized
};


using IasEqConfigStreamMap       = std::map<int32_t,std::vector<IasEqConfigFilterInitParams>>;
using IasEqConfigStreamMapPair   = std::pair<int32_t,std::vector<IasEqConfigFilterInitParams>>;

using IasEqFilterNumberMap       = std::map<int32_t,std::vector<uint32_t>>;
using IasEqFilterNumberMapPair   = std::pair<int32_t,std::vector<uint32_t>>;

using IasEqChannelFiltersMap     = std::map<uint32_t, std::vector<IasEqConfigFilterInitParams> >;
using IasEqChannelFiltersMapPair = std::pair<uint32_t, std::vector<IasEqConfigFilterInitParams> >;

using IasEqStreamChannelsMap     = std::map<int32_t, IasEqChannelFiltersMap>;
using IasEqStreamChannelsMapPair = std::pair<int32_t, IasEqChannelFiltersMap>;

/**
 * @class IasEqualizerCoreConfig
 * @brief This class defines the configuration of the Equalizer Core Module.
 *
 * This configuration class derives from the base class IasGenericAudioCompConfig
 * in order to have some common functionality like e.g. the feature to define
 * which streams the module has to process.
 *
 * Furthermore, this configuration class defines the maximum number of
 * filter stages that can be used for each audio channel. This is implemented
 * by the methods setNumFilterStagesMax() and getNumFilterStagesMax().
 */
class IAS_AUDIO_PUBLIC  IasEqualizerConfiguration
{
  public:
    /**
     *  @brief Constructor.
     */
    IasEqualizerConfiguration(const IasIGenericAudioCompConfig* config);

    /**
     *  @brief Destructor.
     */
    ~IasEqualizerConfiguration();

    /**
     * @brief  Get the maximum number of filter stages that can be used for
     *         each audio channel.
     *
     * @return The maximum number of filter stages.
     */
    inline uint32_t getNumFilterStagesMax() const
    {
      return mNumFilterStagesMax;
    }

    /**
     * @brief  Set the maximum number of filter stages that can be used for
     *         each audio channel.
     *
     * The maximum number of filter stages (filter sections), which is
     * declared by this method, must not be exceeded when the method
     * IasEqualizerCore::setFiltersSingleStream is called later on.
     *
     * Each filter section is able to implement up to two poles and zeros.
     * Therefore, higher order (Butterworth) filters (order > 2) require
     * more than one filter section. This must be considered when the
     * configuration is dimensioned.
     *
     * @param[in] numFilterStagesMax  The maximum number of filter stages.
     */
    void setNumFilterStagesMax(const uint32_t numFilterStagesMax);

    /**
     * @brief The function sets the configuration filter params for one stream of the equalizer
     *
     * @param[in] stream   pointer to the audio stream
     * @param[in] filterId the index of the filter
     * @param[in] params   pointer to the filter params
     *
     * @return                            error code
     * @retval eIasAudioProcOK            success
     * @retval eIasAudioProcInvalidParam  NULL pointer passed as parameter
     * @retval eIasAudioProcInvalidStream stream is not processed by module
     */
    IasAudioProcessingResult setConfigFilterParamsStream(IasAudioStream* stream, uint32_t filterId, const IasEqConfigFilterParams *params);

    /**
     * @brief the function return the configured filter params for the stream by filling the pointer
     *
     * @param[in]     stream   pointer to the audio stream
     * @param[in]     filterId the index of the filter
     * @param[in,out] params   pointer to the filter params
     *
     * @return                                error code
     * @retval eIasAudioProcOK                success
     * @retval eIasAudioProcInvalidParam      NULL pointer passed as parameter
     * @retval eIasAudioProcInvalidStream     stream is not processed by module
     * @retval eIasAudioProcNotInitialization no parameters were configured for this stream
     */
    IasAudioProcessingResult getConfigFilterParamsStream(IasAudioStream* stream, uint32_t filterId, IasEqConfigFilterParams *params) const;


  private:


    /*!
     *  @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasEqualizerConfiguration(IasEqualizerConfiguration const& other) = delete;

    /*!
     *  @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasEqualizerConfiguration& operator=(IasEqualizerConfiguration const& other) = delete;

    /**
     *  @brief Move constructor, private unimplemented to prevent misuse.
     */
    IasEqualizerConfiguration(IasEqualizerConfiguration&& other) = delete;

    /**
     *  @brief Move assignment operator, private unimplemented to prevent misuse.
     */
    IasEqualizerConfiguration& operator=(IasEqualizerConfiguration&& other) = delete;

    const IasIGenericAudioCompConfig*     mConfig;                         //!< Pointer to the configuration of the audio module
    uint32_t                              mNumFilterStagesMax;             //< maximum number of filter stages per channel
    bool                                  mNumFilterStagesMaxInitialized;  //< flag to indicate if mNumFilterStagesMax was set;
    IasEqConfigStreamMap                  mEqStreamMap;                    //< map with streamId as key, to indicate if a filter index for a stream has configured parameters
    IasEqFilterNumberMap                  mFilterNumberMap;                //< map indicating how many filters are configured for each channel of a stream
    IasEqStreamChannelsMap                mStreamChannelMap;               //< map containing the information about configuration parameter of each channel of each stream

}; // class IasEqualizerConfiguration
//
}  // namespace IasAudio

#endif /* IASEQUALIZERCONFIGURATION_HPP_ */
