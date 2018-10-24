/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file
 */

#include <audio/smartx/rtprocessingfwx/IasModuleLoader.hpp>
#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <iostream>

namespace IasAudio
{

template<int32_t> struct CompileError;
template<> struct CompileError<true>{};

#define COMPILER_CHECK(expr, msg)                                                 \
  class Error_##msg {};                                                           \
  { CompileError<((expr) != 0)> Error_##msg; static_cast<void>(Error_##msg); }


IasModuleLoader::IasModuleLoader()
  : mLibs()
  , mDltContext(IasAudioLogging::registerDltContext("PFW", "Module Loader"))
{
}

//---------------------------------------------------------------------------------------------------------------------
IasModuleLoader::~IasModuleLoader()
//---------------------------------------------------------------------------------------------------------------------
{
  while ( 0 < mLibs.size() )
  {
    DLT_LOG_CXX(*mDltContext, DLT_LOG_ERROR, "IasModuleLoader: Not all modules unloaded by user, freeing memory");
    // Create temporary key so we can self reference the vector.
    std::string str = mLibs.back().mModuleName;
    unloadModule(str);
  }
}

IasModuleInterface* IasModuleLoader::loadModule( std::string const &lib,
                                                 std::string const &createStr,
                                                 std::string const &destroyStr,
                                                 IasModuleInfo const &moduleInfo)
{
  IasModuleInterface* (*createFunc)() = NULL;
  void* (*destroyFunc)(IasModuleInterface*) = NULL;

  bool moduleInfoMatches = false;

  const char *error = NULL;
  void *hdl = dlopen(lib.c_str(), RTLD_NOW);

  if(hdl == NULL)
  {
    error = dlerror();
  }

  // The return value.
  IasModuleInterface *create = NULL;

  // First do check if the conversion from function pointer to void is allowed.
  COMPILER_CHECK((sizeof(void*) <= sizeof(createFunc)), Function_Pointer_Size_to_small)


  if ( NULL != hdl)
  {
    // Clear error.
    static_cast<void>(dlerror());

    typedef IasModuleInfo (*IasGetModuleInfoType)();

    IasGetModuleInfoType getModuleInfoFunc = NULL;

    // Workaround for type-punned pointer warning/error (compiler option -O for O >=2).
    getModuleInfoFunc = reinterpret_cast<IasGetModuleInfoType>(dlsym(hdl, "getModuleInfo"));
    error = dlerror();

    if((error == NULL) && (getModuleInfoFunc != NULL))
    {
      IasModuleInfo info = getModuleInfoFunc();
      if(info.mModuleTypeName.compare(moduleInfo.mModuleTypeName) == 0)
      {
        moduleInfoMatches = true;
      }
    }
  }


  if ( NULL != hdl
      &&(NULL == error))
  {
    // Clear error.
    static_cast<void>(dlerror());

    // Workaround for type-punned pointer warning/error (compiler option -O for O >=2).
    typedef IasModuleInterface* (*IasCreateFuncType)();
    createFunc = reinterpret_cast<IasCreateFuncType>(dlsym(hdl, createStr.c_str()));
    error = dlerror();
  }

  if (  (NULL != hdl)
      &&(NULL == error) ) // dlsym error should be checked with dlerror, NULL from dlsysm could be a valid value.
  {
    // Clear error.
    static_cast<void>(dlerror());


    // Workaround for type-punned pointer warning/error (compiler option -O for O >=2).
    typedef void* (*IasDestroyFuncType)(IasModuleInterface*);
    destroyFunc = reinterpret_cast<IasDestroyFuncType>(dlsym(hdl, destroyStr.c_str()));
    error = dlerror();
  }

  // Checks if there are any errors pending that require unloading the library.
  bool mustUnloadDynamicLibrary = false;
  if(NULL == error)
  {
    // Checks if the module has correct type and valid construction/destruction pointers.
    if((NULL != createFunc) && (NULL != destroyFunc) && moduleInfoMatches)
    {
      // Checks if the pointer to the instance of the module is valid.
      create = createFunc();
      if(NULL != create)
      {
        // Adds the loaded module to the lib vector.
        element elem;
        elem.mModuleName = lib;
        elem.mDlHandle = hdl;
        elem.mLoadedObject = create;
        elem.destroy = destroyFunc;
        mLibs.push_back(elem);
      }
      else
      {
        // The pointer to an instance of the loaded object is invalid (NULL).
        mustUnloadDynamicLibrary = true;
        DLT_LOG_CXX(*mDltContext, DLT_LOG_ERROR, "IasModuleLoader: Invalid pointer to instance for module ", lib);
      }
    }
    else
    {
      // The loaded module cannot be used: an invalid pointer and/or an invalid type were found.
      mustUnloadDynamicLibrary = true;
      DLT_LOG_CXX(*mDltContext, DLT_LOG_ERROR, "IasModuleLoader: Invalid type or missing info for module ", lib);
    }
  }
  else
  {
    // At least one dl loader operation failed. Prints the relevant message.
    mustUnloadDynamicLibrary = true;
    DLT_LOG_CXX(*mDltContext, DLT_LOG_ERROR, "IasModuleLoader: Could not load module ", lib,", error = ", error);
  }

  if(mustUnloadDynamicLibrary && (NULL != hdl))
  {
    if(dlclose(hdl) != 0)
    {
      error = dlerror();
      (void) error;
      DLT_LOG_CXX(*mDltContext, DLT_LOG_ERROR, "IasModuleLoader: Could not force the unload of module ", lib, ", error = ", error);
    }
    else
    {
      DLT_LOG_CXX(*mDltContext, DLT_LOG_WARN, "IasModuleLoader: Forced unload of module ", lib);
    }
  }
  return create;
}

IasModuleInterfaceList IasModuleLoader::loadModules(std::string modulePath,
                                                    std::string const & createStr,
                                                    std::string const & destroyStr,
                                                    IasModuleInfo const & moduleInfo)
{
  IasModuleInterfaceList result;

  // If the module path is empty set it to the current directory.
  if (modulePath.empty())
  {
    modulePath = "./";
  }
  // Ensure that the module path ends with a "/".
  if (modulePath.at(modulePath.length() - 1) != '/')
  {
    modulePath = modulePath + "/";
  }

  // Open the plugin directory.
  DIR *directory = NULL;
  directory = opendir(modulePath.c_str());

  if (directory != NULL)
  {
    /**
     * These two sets store the plugin files that have actually been loaded
     * and symbolic links that will be loaded later if the don't point to files
     * in the plugin directory.
    */
    std::set<std::string> loaded_plugins;
    std::set<std::string> symlinks;

    // Get the first directory entry.
    struct dirent *dir_entry = readdir(directory);
    while (dir_entry != NULL)
    {
      const std::string extension = ".so";
      const std::string name = dir_entry->d_name;
      if (name.length() >= extension.length() && name.substr(name.length() - extension.length(), extension.length()) == extension)
      {
        if (dir_entry->d_type == DT_LNK)
        {
          // Store symlinks for later processing.
          symlinks.insert(modulePath + std::string(dir_entry->d_name));
        }
        else if ((dir_entry->d_type == DT_REG) || (dir_entry->d_type == DT_UNKNOWN))
        {
          // Try to load the directory entry as a plugin.
          std::string mModuleName = modulePath + std::string(dir_entry->d_name);

          IasModuleInterface * plugin = loadModule(mModuleName, createStr, destroyStr, moduleInfo);
          if(plugin != NULL)
          {
            loaded_plugins.insert(mModuleName);
            result.push_back(plugin);
          }
        }
      }

      // Get the next directory entry.
      dir_entry = readdir(directory);
    }

    // Process symlinks.
    for (std::set<std::string>::const_iterator it = symlinks.begin(); it != symlinks.end(); ++it)
    {
      // Try to resolve the symlink.
      char resolvedFilename[PATH_MAX];
      if (realpath(it->c_str(), resolvedFilename) == NULL)
      {
        std::string moduleFilename(resolvedFilename);

        // Check if the linked file has already been loaded.
        if (loaded_plugins.find(moduleFilename) == loaded_plugins.end())
        {
          IasModuleInterface * plugin = loadModule(moduleFilename, createStr, destroyStr, moduleInfo);
          if(plugin != NULL)
          {
            loaded_plugins.insert(moduleFilename);
            result.push_back(plugin);
          }
        }
      }
    }
  }

  return result;
}

IasAudioCommonResult IasModuleLoader::unloadModule( std::string const &lib )
{
  IasAudioCommonResult result = eIasResultOk;
  int32_t error = 0;

  const int32_t pos = findKey(lib);

  if (  (-1 != pos   )
      &&(NULL != mLibs[pos].mDlHandle) )
  {
    mLibs[pos].destroy(mLibs[pos].mLoadedObject);
    error = dlclose(mLibs[pos].mDlHandle);

    if ( 0 == error )
    {
      mLibs[pos].mDlHandle = 0;
      mLibs.erase(mLibs.begin() + pos);
      result = eIasResultOk;
    }
    else
    {
      const char * errorValue = dlerror();
      DLT_LOG_CXX(*mDltContext, DLT_LOG_ERROR, "IasModuleLoader: Failed to unload module ", lib);
      if(errorValue != NULL)
      {
        DLT_LOG_CXX(*mDltContext, DLT_LOG_ERROR, "... error value was ", errorValue);
      }
      result = eIasResultUnknown;
    }
  }
  else
  {
    DLT_LOG_CXX(*mDltContext, DLT_LOG_ERROR, "IasModuleLoader: Could not find module ", lib);
    result = eIasResultObjectNotFound;
  }

  return result;
}

IasAudioCommonResult IasModuleLoader::unloadModule(IasModuleInterface* const moduleInterface)
{
  IasAudioCommonResult result = eIasResultUnknown;

  if(!mLibs.empty())
  {
    // Scan mLibs to search the interface.
    result = eIasResultObjectNotFound;
    for (elementList::size_type i = 0; i < mLibs.size(); ++i)
    {
      // Unloads the module, if the interface is present.
      if (mLibs[i].mLoadedObject == moduleInterface)
      {
        result = unloadModule(mLibs[i].mModuleName);
        break;
      }
    }
  }
  else
  {
    // The loaded module vector is empty.
    result = eIasResultInvalidParam;
  }

  return result;
}

int32_t IasModuleLoader::findKey(std::string const &key)
{
  int32_t pos = -1;
  for (uint32_t i = 0u; i < mLibs.size(); ++i)
  {
    if ( 0 == mLibs[i].mModuleName.compare(key) )
    {
      pos = i;
      break;
    }
  }
  return pos;
}

} // namespace IasAudio
