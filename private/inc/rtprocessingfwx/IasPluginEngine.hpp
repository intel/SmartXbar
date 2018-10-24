/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasPluginEngine.hpp
 * @date   2013
 * @brief
 */

#ifndef IASPLUGINENGINE_HPP_
#define IASPLUGINENGINE_HPP_

#include <audio/smartx/rtprocessingfwx/IasModuleLoader.hpp>
#include "internal/audio/common/IasAudioLogging.hpp"


#include "audio/smartx/rtprocessingfwx/IasProcessingTypes.hpp"
#include "smartx/IasAudioTypedefs.hpp"

namespace IasAudio {

class IasGenericAudioComp;
class IasIGenericAudioCompConfig;
class IasPluginLibrary;

class IAS_AUDIO_PUBLIC IasPluginEngine
{
  public:
    /**
     * @brief Constructor.
     */
    IasPluginEngine(IasICmdRegistrationPtr cmdRegistration);

    /**
     * @brief Destructor, virtual by default.
     */
    virtual ~IasPluginEngine();

    /**
     * @brief Load plug-in libraries.
     *
     * This method tries to load all SmartXbar audio plugin libraries from the
     * default folder /opt/audio/plugin or from the directory specified by the environment
     * variable AUDIO_PLUGIN_DIR. After the plugin libraries are loaded, module instances
     * from the included audio modules can be created.
     *
     * @returns    The result indicating success (eIasAudioProcOK) or failure.
     */
    IasAudioProcessingResult loadPluginLibraries();

    /**
     * @brief Create a module configuration.
     *
     * @param[in,out] config A pointer to the location where the new config pointer shall be stored.
     *
     * @returns    The result indicating success (eIasAudioProcOK) or failure.
     */
    IasAudioProcessingResult createModuleConfig(IasIGenericAudioCompConfig **config);

    /**
     * @brief Create a new instance of a module.
     *
     * The typeName is a predefined type name of the audio module that exists in one of the plugin libraries.
     * The instanceName is a unique name for the specific instance and can be freely chosen.
     *
     * @param[in] config A pointer to the previously created and initialized configuration.
     * @param[in] typeName The name of the module type to create.
     * @param[in] instanceName A unique instance name.
     * @param[in,out] module A pointer to the location where the newly created module will be stored.
     *
     * @returns The result indicating success (eIasAudioProcOK) or failure.
     */
    IasAudioProcessingResult createModule(IasIGenericAudioCompConfig *config, const std::string &typeName, const std::string& instanceName, IasGenericAudioComp **module);

    /**
     * @brief Destroy previously created audio module
     *
     * @param[in] module The audio module to be destroyed
     */
    void destroyModule(IasGenericAudioComp *module);

  private:
    using IasCompConfigList = std::list<IasIGenericAudioCompConfig*>;
    using IasPluginLibraryList = std::list<IasPluginLibrary*>;

    /**
     * @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasPluginEngine(IasPluginEngine const &other);

    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasPluginEngine& operator=(IasPluginEngine const &other);

    // Member variables
    IasModuleInterfaceList        mModules;         //!< List of all loaded plugin libraries
    IasCompConfigList             mCompConfigs;     //!< List of all created plug-in configs
    IasModuleLoader               mModuleLoader;    //!< The plug-in module loader
    IasPluginLibraryList          mPluginLibs;      //!< List of all loaded plugin libraries (concrete type)
    DltContext                   *mLogContext;      //!< The log context
    IasICmdRegistrationPtr        mCmdRegistration; //!< The command and control registration instance
};

} //namespace IasAudio

#endif /* IASPLUGINENGINE_HPP_ */
