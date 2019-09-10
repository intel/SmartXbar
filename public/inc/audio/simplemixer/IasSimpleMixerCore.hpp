/*
 * Copyright (C) 2019 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file  IasSimpleMixerCore.hpp
 * @date  2016
 * @brief The header file of the core of the simple mixer component.
 */

#ifndef IASSIMPLEMIXERCORE_HPP_
#define IASSIMPLEMIXERCORE_HPP_

#include "audio/smartx/rtprocessingfwx/IasGenericAudioCompCore.hpp"

// disable conversion warnings for tbb
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#include "tbb/tbb.h"
// turn the warnings back on
#pragma GCC diagnostic pop

namespace IasAudio{

class IasIGenericAudioCompConfig;


/** \class IasSimpleMixerCore
 *  This class is the core of the simple mixer component
 */
class IAS_AUDIO_PUBLIC IasSimpleMixerCore : public IasGenericAudioCompCore
{
  public:
    /**
     * @brief Constructor.
     *
     * @param config pointer to the component configuration.
     * @param componentName unique component name.
     */
    IasSimpleMixerCore(const IasIGenericAudioCompConfig *config, const std::string &componentName);

    /*!
     *  @brief Destructor, virtual by default.
     */
    virtual ~IasSimpleMixerCore();

    /*!
     *  @brief Reset method.
     *
     *  Reset the internal component states.
     *  Derived from base class.
     */
    virtual IasAudioProcessingResult reset();

    /*!
     *  @brief Initialize method.
     *
     *  Pass audio component specific initialization parameter.
     *  Derived from base class.
     */
    virtual IasAudioProcessingResult init();

    /**
     * @brief Set the gain for a stream that is processed in-place.
     *
     * @param[in] streamId  The id of the audio stream.
     * @param[in] newGain   The new gain (in linear representation).
     */
    void setInPlaceGain(int32_t   streamId,
                        float newGain);

    /**
     * @brief Set the gain for a stream mapping (from an input stream to an output stream).
     *
     * @param[in] inputStreamId   The id of the input stream.
     * @param[in] outputStreamId  The id of the output stream.
     * @param[in] newGain         The new gain (in linear representation).
     */
    void setXMixerGain(int32_t   inputStreamId,
                       int32_t   outputStreamId,
                       float newGain);


  private:

    /*!
     *  @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasSimpleMixerCore(IasSimpleMixerCore const &other); //lint !e1704

    /*!
     *  @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasSimpleMixerCore& operator=(IasSimpleMixerCore const &other); //lint !e1704

    /*!
     *  @brief Process method.
     *
     *  This method is periodically called by the AudioServer Core.
     *  Derived from base class.
     */
    virtual IasAudioProcessingResult processChild();

    /*!
     *  @brief Send an event to the user application via the event handler.
     *
     *  The event will be of eventType IasSimpleMixer::eIasGainUpdateFinished.
     *
     *  @param inputStreamId   Id of the input stream, will be used for the event property "input_pin".
     *  @param outputStreamId  Id of the output stream, will be used for the event property "output_pin".
     *  @param currentGain     Current gain value, will be used for the event property "gain".
     */
    void sendSimpleMixerEvent(int32_t inputStreamId, int32_t outputStreamId, float currentGain);

    /**
     *  @brief Message Ids used for the Message Queue.
     *
     *  The command for setting the module state is not required here,
     *  because this is handled by the IasGenericAudioCompCore.
     */
    enum IasMsgIds
    {
      eIasUndefined,                     //!< Undefined
      eIasSetInPlaceGain,                //!< Set new gain for all in-place streams
      eIasSetXMixerGain,                 //!< Set mew gain for all stream mappings
    };

    /**
     *  @brief Struct defining an element of the message queue.
     */
    struct IasMsgQueueEntry
    {
      // Constructor
      inline IasMsgQueueEntry()
        :msgId(IasMsgIds::eIasUndefined)
        ,streamId1(0)
        ,streamId2(0)
        ,gain(1.0f)
      {}

      IasMsgIds    msgId;
      int32_t   streamId1;
      int32_t   streamId2;
      float gain;
    };


    /**
     * @brief Type definition for an audio stream mapping.
     * An audio stream mapping is a struct containing the ids of the input stream and the output stream.
     */
    struct IasAudioStreamMapping
    {
      /**
       * @brief Constructor with initial values.
       */
      inline IasAudioStreamMapping()
        :inputStreamId(0)
        ,outputStreamId(0)
        {;}

      /**
       * @brief Constructor for initializer list.
       */
        inline IasAudioStreamMapping(int32_t p_inputStreamId, int32_t p_outputStreamId)
        :inputStreamId(p_inputStreamId)
        ,outputStreamId(p_outputStreamId)
      {;}

      /**
       * @brief Compare operator, required to build a set of audio pin mappings.
       */
      friend bool operator<(const IasAudioStreamMapping a, const IasAudioStreamMapping b)
      {
        if (a.inputStreamId < b.inputStreamId)
        {
          return true;
        }
        if (b.inputStreamId < a.inputStreamId)
        {
          return false;
        }
        // At this point, we know that the inputStreamIds are equal, so we have to compare the outputStreamIds
        if (a.outputStreamId < b.outputStreamId)
        {
          return true;
        }
        return false;
      }

      int32_t inputStreamId;
      int32_t outputStreamId;
    };

    /**
     * Type definition for a pair, where the key parameter is an audio stream mapping and the second parameter is a gain value.
     */
    using IasXMixerGainPair = std::pair<IasAudioStreamMapping, float>;

    /**
     * Type definition for a map, where the key parameter is an audio stream mapping and the second parameter is a gain value.
     */
    using IasXMixerGainMap = std::map<IasAudioStreamMapping, float>;

    /**
     * Type definition for a pair, where the key parameter is an (in-place) audio stream Id and the second parameter is a gain value.
     */
    using IasInplaceGainPair = std::pair<int32_t, float>;

    /**
     * Type definition for a map, where the key parameter is an (in-place) audio stream Id and the second parameter is a gain value.
     */
    using IasInplaceGainMap = std::map<int32_t, float>;


    // Member variables
    uint32_t                              mNumStreams;      //!< number of streams to be processed
    float                             mDefaultGain;     //!< The default gain (to be used after initialization).
    tbb::concurrent_queue<IasMsgQueueEntry>  mMsgQueue;        //!< Queue used to synchronize the Ctrl thread with processing thread.
    IasXMixerGainMap                        * mXMixerGainMap;   //!< Map with gains for all audio stream mappings.
    IasInplaceGainMap                       * mInplaceGainMap;  //!< Map with gains for all in-place audio streams.
    std::string                              mTypeName;        //!< Name of the module type
    std::string                              mInstanceName;    //!< Name of the module instance
};

} //namespace IasAudio

#endif /* IASSIMPLEMIXERCORE_HPP_ */
