/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasBundleSequencer.hpp
 * @date   2012
 * @brief  The definition of the IasBundleSequencer class.
 */

#ifndef IASBUNDLESEQUENCER_HPP_
#define IASBUNDLESEQUENCER_HPP_

#include "internal/audio/common/IasAudioLogging.hpp"

#include "smartx/IasAudioTypedefs.hpp"

#include "audio/smartx/rtprocessingfwx/IasProcessingTypes.hpp"

namespace IasAudio {

class IasAudioChannelBundle;

/**
 * @brief A vector of IasAudioChannelBundle to manage multiple audio channel bundles.
 */
typedef std::vector<IasAudioChannelBundle*> IasBundleVector;

/**
 * @class IasBundleSequencer
 *
 * This class is used to create bundle assignments for a stream with a given number of channels.
 * Depending on the number of channels a different vector is used to manage the already created bundles.
 * This is useful to avoid fragmentation when adding a new stream that has to be mapped to already existing
 * but not yet completely filled bundles. That means that we have one bundle vector for mono streams,
 * one bundle vector for stereo streams and one vector for multi-channel streams. By using this simple approach
 * we are sure that two stereo streams will always be mapped into the same bundle regardless on how many mono
 * or multi-channel streams are also created.
 */
class IAS_AUDIO_PUBLIC IasBundleSequencer
{
  public:
    /**
     * @brief Constructor.
     */
    IasBundleSequencer();

    /**
     * @brief Destructor, virtual by default.
     */
    virtual ~IasBundleSequencer();

    /**
     * @brief Initialize the bundle sequencer
     *
     * This method has to be called before using the bundle sequencer
     */
    IasAudioProcessingResult init(IasAudioChainEnvironmentPtr env);

    /**
     * @brief Get the bundle assignments from the bundle sequencer for the given number of channels.
     *
     * @param[in] numberChannels The number of channels for which new bundle assignments are requested.
     * @param[out] bundleAssignments The newly created bundle assignments.
     *
     * @returns The result indicating success (eIasAudioProcOK) or failure.
     */
    IasAudioProcessingResult getBundleAssignments(uint32_t numberChannels, IasBundleAssignmentVector *bundleAssignments);

    /**
     * @brief Clear audio sample buffers of all allocated and used bundles of this sequencer (mono, stereo and
     * multichannel.
     */
    void clearAllBundleBuffers() const;

  private:
    /**
     * @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasBundleSequencer(IasBundleSequencer const &other);

    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasBundleSequencer& operator=(IasBundleSequencer const &other);

    /**
     * @brief Get simple bundle assignments.
     *
     * The simple bundle assignments are used for mono and stereo streams.
     *
     * @param[in] numberChannels The number of channels of the stream.
     * @param[in,out] bundleVector A pointer to the vector of already created bundles.
     * @param[out] bundleAssignments A pointer to the vector where the bundle assignments will be added.
     *
     * @returns The result indicating success (eIasAudioProcOK) or failure.
     */
    IasAudioProcessingResult getSimpleBundleAssignments(uint32_t numberChannels,
                                                        IasBundleVector *bundleVector,
                                                        IasBundleAssignmentVector *bundleAssignments);

    /**
     * @brief Get multi-channel bundle assignments.
     *
     * The multi-channel bundle assignments are used for multi-channel streams.
     *
     * @param[in] numberChannels The number of channels of the stream.
     * @param[in,out] bundleVector A pointer to the vector of already created bundles.
     * @param[out] bundleAssignments A pointer to the vector where the bundle assignments will be added.
     *
     * @returns The result indicating success (eIasAudioProcOK) or failure.
     */
    IasAudioProcessingResult getMultiChannelBundleAssignments(uint32_t numberChannels,
                                                              IasBundleVector *bundleVector,
                                                              IasBundleAssignmentVector *bundleAssignments);


    // Member variables
    IasBundleVector             mMonoBundles;                 //!< A vector that holds all bundles created for mono streams.
    IasBundleVector             mStereoBundles;               //!< A vector that holds all bundles created for stereo streams.
    IasBundleVector             mMultiChannelBundles;         //!< A vector that holds all bundles created for multi-channel streams.
    DltContext                 *mLogContext;                  //!< The log context
    IasAudioChainEnvironmentPtr mEnv;                         //!< The audio chain environment
};

} //namespace IasAudio

#endif /* IASBUNDLESEQUENCER_HPP_ */
