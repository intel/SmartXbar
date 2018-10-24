/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasPluginLibrary.hpp
 * @date   2016
 * @brief
 */

#ifndef IASPLUGINLIBRARY_HPP_
#define IASPLUGINLIBRARY_HPP_

#include <dlt/dlt.h>

#include "audio/smartx/rtprocessingfwx/IasModuleInterface.hpp"

namespace IasAudio {

class IasGenericAudioComp;
class IasIGenericAudioCompConfig;

/**
 * @brief Typedef for module create function prototype
 */
typedef IasGenericAudioComp* (*IasCreateFunc)(const IasIGenericAudioCompConfig*, const std::string &typeName, const std::string &instanceName);

/**
 * @brief Typedef for module destroy function prototype
 */
typedef void (*IasDestroyFunc)(IasGenericAudioComp*);

/**
 * @brief A list of strings to keep the type names of the audio modules
 */
using IasTypeNameList = std::list<std::string>;

/**
 * @class IasPluginLibrary
 *
 * This class is used to create a plug-in library. It contains all required methods
 * to (un)register factory methods for different processing module types, to create
 * and destroy modules and to get some information about the registered processing
 * module types.
 */
class IAS_AUDIO_PUBLIC IasPluginLibrary : public IasModuleInterface
{
  public:
    enum IasResult
    {
      eIasOk,     //!< Method successful
      eIasFailed, //!< Method failed
    };

    /**
     * @brief Constructor
     *
     * @param[in] libraryName The name of the library. Can be freely chosen.
     */
    IasPluginLibrary(const std::string &libraryName);

    /**
     * @brief Destructor
     */
    virtual ~IasPluginLibrary();

    /**
     * @brief Register factory methods at the plugin library
     *
     * @param[in] typeName The type name of the module being registered
     * @param[in] create The create function of the module
     * @param[in] destroy The destroy function of the module
     */
    void registerFactoryMethods(const std::string &typeName, IasCreateFunc create, IasDestroyFunc destroy);

    /**
     * @brief Unregister factory methods at the plugin library
     *
     * @param[in] typeName The type name of the module being unregistered
     */
    void unregisterFactoryMethods(const std::string &typeName);

    /**
     * @brief Create a new instance of the audio module
     *
     * @param[in] config A pointer to a previously created and initialized configuration
     * @param[in] typeName The type name of the module to create
     * @param[in] instanceName A unique instance name of the module instance
     *
     * @returns A new instance of the corresponding type or nullptr in case the module type wasn't registered before
     */
    IasGenericAudioComp* createModule(const IasIGenericAudioCompConfig *config, const std::string &typeName, const std::string &instanceName);

    /**
     * @brief Destroy a previously created module
     *
     * @param[in] module Pointer to a previously created module
     */
    IasResult destroyModule(IasGenericAudioComp *module);

    /**
     * @brief Get a list of all registered module type names
     *
     * @returns A list of all registered module type names
     */
    IasTypeNameList getModuleTypeNames() const;

    /**
     * @brief Get library name
     *
     * @returns The library name
     */
    inline const std::string& getLibraryName() const { return mLibraryName; }

  private:
    struct IasFactoryFunctions
    {
      IasCreateFunc create;
      IasDestroyFunc destroy;
    };
    using IasModuleMap = std::map<std::string, IasFactoryFunctions>;

    std::string       mLibraryName;   //!< The plugin library name
    IasModuleMap      mModuleMap;     //!< The map containing all registered module types and their factory functions
    DltContext       *mLog;           //!< The DLT context
};

} /* namespace IasAudio */

#endif /* IASPLUGINLIBRARY_HPP_ */
