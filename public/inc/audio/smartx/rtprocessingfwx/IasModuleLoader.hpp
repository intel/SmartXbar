/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file
 */

#ifndef _IASMODULELOADER_HPP_
#define _IASMODULELOADER_HPP_

#include <audio/smartx/rtprocessingfwx/IasModuleInterface.hpp>
#include "internal/audio/common/IasAudioLogging.hpp"

/**
 * @brief IasAudio
 */
namespace IasAudio
{

/**
 * Type definition for a list of IasModuleInterface pointers.
 */
typedef std::list<IasModuleInterface*> IasModuleInterfaceList;

/**
 * @brief Class to load shared libraries containing an IasModuleInterface.
 */
class IAS_AUDIO_PUBLIC IasModuleLoader
{
public:


  /*!
   * Default constructor.
   */
  IasModuleLoader();

  /*!
   * Destructor.
   */
  ~IasModuleLoader();

  /**
   * Loads a module.
   * @param lib the filename of the library to be loaded including the path.
   * @param create the create function name.
   * @param destroy the destroy function name.
   * @param moduleInfo the module info describing the module types to load.
   * @return != NULL on success.
   *
   * @warning Loading modules always imposes potential risks if a module from an unknown/untrusted source is loaded.
   */
  IasModuleInterface* loadModule( std::string const &lib, std::string const &createStr, std::string const &destroyStr, IasModuleInfo const &moduleInfo );

  /**
   * Load all modules contained in the specified directory.
   *
   * @param modulePath the directory that contains the modules to be loaded.
   * @param createStr the create function name.
   * @param destroyStr the destroy function name.
   * @param moduleInfo the module info describing the module types to load.
   * @return a (potentially empty) list of IasModuleInterface objects.
   *
   * @warning Loading modules always imposes potential risks if a module from an unknown/untrusted source is loaded.
   */
  IasModuleInterfaceList loadModules(std::string modulePath, std::string const & createStr, std::string const & destroyStr, IasModuleInfo const & moduleInfo);

  /**
   * Unloads a module
   *
   * @param lib the filename of the library to be unloaded including the path.
   * @returns cOk if the module was unloaded or an error otherwise.
   */
  IasAudioCommonResult unloadModule( std::string const &lib );

  /**
   * Unloads a module referenced by its interface.
   *
   * @params moduleInterface pointer to the interface of the module that must be unloaded
   * @returns cOk if the module was unloaded or an error otherwise.
   */
  IasAudioCommonResult unloadModule(IasModuleInterface* const moduleInterface);

private:

  typedef struct
  {
    std::string         mModuleName;        //!< The module name.
    void                *mDlHandle;         //!< The dl handle.
    IasModuleInterface  *mLoadedObject;     //!< Pointer to an instance of the loaded object.
    void*(*destroy)(IasModuleInterface*);   //!< Destructor function for the instance of the loaded object.
  } element;

  //<! typedef for element vector
  typedef std::vector<element> elementList;

  /*!
   * Helper to find a loaded library in the vector.
   * @param key the library name.
   * @return != -1 on success.
   */
  int32_t findKey(std::string const &key);

  elementList mLibs; //!< The loaded libs vector.

  /*!
   * @brief Used for logging trace output.
   */
  DltContext* mDltContext;
};

} // namespace IasAudio

#endif /* _IASMODULELOADER_HPP_ */
