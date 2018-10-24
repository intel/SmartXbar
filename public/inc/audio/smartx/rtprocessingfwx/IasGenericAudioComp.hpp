/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasGenericAudioComp.hpp
 * @date   2012
 * @brief  The definition of the IasGenericAudioComp base class.
 */

#ifndef IASGENERICAUDIOCOMP_HPP_
#define IASGENERICAUDIOCOMP_HPP_

#include "audio/smartx/rtprocessingfwx/IasProcessingTypes.hpp"



namespace IasAudio {

class IasIModuleId;
class IasGenericAudioCompCore;
class IasGenericAudioComp;
class IasIGenericAudioCompConfig;

/**
 * @brief A vector of IasGenericAudioComp to manage multiple audio components
 */
typedef std::vector<IasGenericAudioComp*> IasGenericAudioCompVector;

/**
 * @class IasGenericAudioComp
 *
 * An audio processing component typically consists of three classes, an audio processing core class,
 * an audio processing command interface class and a wrapper class that combines these two to form the
 * complete audio processing component. The base class IasGenericAudioCompCore defined in this file is the
 * wrapper class that is used to combine an IasGenericAudioCompCore with an IasRpcModuleInterface to
 * form an audio processing component. Writing a typical audio processing component thus requires to define three
 * classes, one derived from each of the classes mentioned above and implementing all pure virtual methods.
 * A simple audio processing component that doesn't need to be controllable can consist only of the
 * IasGenericAudioCompCore class and the wrapper class. The main task of the wrapper class is to instantiate
 * the core and the command interface. It can also provide the core with a set of default parameters if necessary.
 */
class IAS_AUDIO_PUBLIC IasGenericAudioComp
{
  public:
    /**
     * @brief Constructor.
     *
     * @param[in] typeName The audio module type name
     */
    IasGenericAudioComp(const std::string &typeName, const std::string &instanceName);

    /**
     * @brief Destructor, virtual by default.
     */
    virtual ~IasGenericAudioComp();

    /**
     * @brief Getter method to access the command interface of the audio component.
     *
     * @returns A pointer to the command interface instance of the audio component.
     */
    inline IasIModuleId* getCmdInterface() const { return mCmdInterface; }

    /**
     * @brief Getter method to access the processing core of the audio component.
     *
     * @returns A pointer to the processing core instance of the audio component.
     */
    inline IasGenericAudioCompCore* getCore() const { return mCore; }

    /**
     * @brief Get the type name of the audio module
     *
     * @returns The type name as string
     */
    inline const std::string& getTypeName() const { return mTypeName; }

    /**
     * @brief Get the unique instance name of the audio module
     *
     * @returns The instance name as string
     */
    inline const std::string& getInstanceName() const { return mInstanceName; }

  protected:
    // Members
    IasIModuleId               *mCmdInterface;  //!< A pointer to the command interface instance.
    IasGenericAudioCompCore    *mCore;          //!< A pointer to the processing core instance.
    std::string                 mTypeName;      //!< The type name of the audio module.
    std::string                 mInstanceName;  //!< The unique instance name of the audio module.

  private:
    /**
     * @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasGenericAudioComp(IasGenericAudioComp const &other);

    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasGenericAudioComp& operator=(IasGenericAudioComp const &other);
};

} //namespace IasAudio

#endif /* IASGENERICAUDIOCOMP_HPP_ */
