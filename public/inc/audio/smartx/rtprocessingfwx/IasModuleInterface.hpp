/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file
 */

#ifndef _IASMODULEINTERFACE_HPP
#define _IASMODULEINTERFACE_HPP

#include "avbaudiomodules/audio/common/IasAudioCommonTypes.hpp"

/**
 * @brief IasAudio
 */
namespace IasAudio
{

/**
 * @brief Class to hold information about a module.
 */
class IAS_AUDIO_PUBLIC IasModuleInfo
{
  public:
    /**
     * @brief Constructor.
     *
     * @param name the name of the module.
     */
    explicit IasModuleInfo(std::string const &name);

    /**
     * @brief Destructor.
     */
    virtual ~IasModuleInfo(){};

    /**
     * @brief The module type name.
     */
    std::string mModuleTypeName;
};

/**
 * @brief Defines the Interface of a module.
 */
class IAS_AUDIO_PUBLIC IasModuleInterface
{
public:

  /*!
   * Constructor.
   */
  IasModuleInterface();

  /*!
   * Destructor.
   */
  virtual ~IasModuleInterface();

};

/**
 * @brief Function declaration to retrieve the module information.
 * @return the module info.
 */
#ifdef __clang__
#pragma clang diagnostic ignored "-Wreturn-type-c-linkage"
#endif //#ifdef __clang__
extern "C" IasModuleInfo getModuleInfo();

} // namespace IasAudio



#endif /* _IASMODULEINTERFACE_HPP_ */
