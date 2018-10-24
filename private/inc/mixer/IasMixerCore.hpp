/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasMixerCore.hpp
 * @date   August 2016
 * @brief  the header for the mixer core module
 */

#ifndef IASXMIXERCORE_HPP_
#define IASXMIXERCORE_HPP_


#include "helper/IasRamp.hpp"
#include "audio/smartx/rtprocessingfwx/IasAudioStream.hpp"
#include "audio/smartx/rtprocessingfwx/IasGenericAudioCompCore.hpp"
#include "audio/common/IasAudioCommonTypes.hpp"
#include "mixer/IasMixerElementary.hpp"


namespace IasAudio {


class IAS_AUDIO_PUBLIC IasMixerCore : public IasGenericAudioCompCore
{

  /**
   * @brief multimap that defines which stream belong to which elementary mixer
   */
  using IasMixerCoreStreamMap = std::multimap<int32_t, IasMixerElementaryPtr>;

  /**
   * @brief pair to with the IasMixerCoreStreamMap type
   */
  using IasMixerCoreStreamPair = std::pair<int32_t,IasMixerElementary*>;

  public:
   /**
    * @brief Constructor.
    *
    * @param[in] config                      pointer to the component configuration.
    * @param[in] componentName               unique component name.
    */
    IasMixerCore(const IasIGenericAudioCompConfig* config, const std::string& componentName);

    /**
     *  @brief Destructor, virtual by default.
     */
    virtual ~IasMixerCore();

    /**
     * @brief the reset function, inherited by IasGenericAudioComp
     * @return error                         the error code, eIasAudioProcOK if all is ok
     */
    IasAudioProcessingResult reset() override;

    /**
     * @brief the initialization function, inherited by IasGenericAudioComp
     * @return error                              the error code
     * @retval eIasAudioProcInitializationFailed  the core could not be initialized
     * @retval eIasAudioProcNotEnoughMemory       out of memory, the core could not be initialized
     * @retval eIasAudioProcOK                    no error
     */
    IasAudioProcessingResult init() override;

    /**
     * @brief the function is used to start a balance change
     *
     * @param[in] stramId                   the stream to be affected
     * @param[in] balanceLeft               the balance value for the left hand side, as linear value
     * @param[in] balanceLeft               the balance value for the right hand side, as linear value
     * @return error                        the error code (if the elementary mixer function return an error, this is returned here)
     * @retval eIasAudioProcInvalidParam    the stream was out of bounds
     * @retval eIasAudioProcOK              no error
     *
     */
    IasAudioProcessingResult setBalance(int32_t streamId, float balanceLeft, float balanceRight);

    /**
     * @brief the function is used to start a fader change
     *
     * @param[in] stramId                   the stream to be affected
     * @param[in] faderFront                the fader value for the front, as linear value
     * @param[in] faderRear                 the fader value for the rear, as linear value
     * @return error                        the error code (if the elementary mixer function return an error, this is returned here)
     * @retval eIasAudioProcInvalidParam    the stream was out of bounds
     * @retval eIasAudioProcOK              no error
     *
     */
    IasAudioProcessingResult setFader(int32_t streamId, float faderFront, float faderRear);

    /**
     * @brief the function is used to start an input gain change
     *
     * @param[in]                           stramId the stream to be affected
     * @param[in]                           gain the new gain, as linear value
     * @return error                        the error code (if the elementary mixer function return an error, this is returned here)
     * @retval eIasAudioProcInvalidParam    the stream was out of bounds
     * @retval eIasAudioProcOK              no error
     *
     */
    IasAudioProcessingResult setInputGainOffset(int32_t streamId, float gain);


  private:

    /*!
     *  @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasMixerCore(IasMixerCore const& other) = delete;
    /*!
     *  @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasMixerCore& operator=(IasMixerCore const& other) = delete;

    /**
     *  @brief Move constructor, private unimplemented to prevent misuse.
     */
    IasMixerCore(IasMixerCore&& other) = delete;

    /**
     *  @brief Move assignment operator, private unimplemented to prevent misuse.
     */
    IasMixerCore& operator=(IasMixerCore&& other) = delete;

    /**
     * @brief the processing function, inherited by IasGenericAudioComp
     * @return error an error code of type IasAudioProcessingResult
     */
    IasAudioProcessingResult processChild() override;

    // Member variables
    IasMixerElementaryVector                 mElementaryMixers;  //!< vector containing the elementary mixers
    IasMixerCoreStreamMap                    mCoreStreamMap;     //!< map that connects the elementary mixer and the belonging streams
    std::list<IasAudioChannelBundle*>  mOutputBundlesList; //!< Sorted+unique list of all output bundles
    DltContext                              *mLogContext;        //!< The log context for the mixer
};


} //namespace IasAudio

#endif /* IASMIXERCORE_HPP_ */
