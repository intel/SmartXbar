/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasProperties.cpp
 * @date   2013
 * @brief  This file contains the IasProperties class definition.
 */

#include "audio/smartx/IasProperties.hpp"
#include "avbaudiomodules/internal/audio/common/IasAudioLogging.hpp"

using namespace std;

#define DECLARE_SET_GET(prop_ns, prop_type) template <> \
  void IasProperties::set(const std::string &key, const prop_ns::prop_type &value) \
  { \
    mKeyDataTypes.insert(make_pair(key, TOSTRING(prop_ns::prop_type))); \
    PROPERTY_MAP_MEMBER(prop_type)[key] = value; \
    mPropertyKeys.push_back(key); \
    mPropertyKeys.unique(); \
    mHasProperties = true; \
  } \
  template <> \
  IasProperties::IasResult IasProperties::get(const std::string &key, prop_ns::prop_type *value) const \
  { \
    return get(PROPERTY_MAP_MEMBER(prop_type), key, value); \
  }

namespace IasAudio {

static const std::string cClassName = "IasProperties::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"

DECLARE_SET_GET(std, int64_t);
DECLARE_SET_GET(std, int32_t);
DECLARE_SET_GET(IasAudio, float64);
DECLARE_SET_GET(IasAudio, float32);
DECLARE_SET_GET(std, string);
DECLARE_SET_GET(IasAudio, IasInt64Vector);
DECLARE_SET_GET(IasAudio, IasInt32Vector);
DECLARE_SET_GET(IasAudio, IasFloat64Vector);
DECLARE_SET_GET(IasAudio, IasFloat32Vector);
DECLARE_SET_GET(IasAudio, IasStringVector);

void IasProperties::clear(const std::string &key)
{
  // We don't know to which type the key belongs, so try to erase it in every map.
  mMap_int64_t.erase(key);
  mMap_int32_t.erase(key);
  mMap_float64.erase(key);
  mMap_float32.erase(key);
  mMap_string.erase(key);
  mMap_IasInt64Vector.erase(key);
  mMap_IasInt32Vector.erase(key);
  mMap_IasFloat64Vector.erase(key);
  mMap_IasFloat32Vector.erase(key);
  mMap_IasStringVector.erase(key);
  mPropertyKeys.remove(key);
  if (  mMap_int64_t.empty()
    &&  mMap_int32_t.empty()
    &&  mMap_float64.empty()
    &&  mMap_float32.empty()
    &&  mMap_string.empty()
    &&  mMap_IasInt64Vector.empty()
    &&  mMap_IasInt32Vector.empty()
    &&  mMap_IasFloat64Vector.empty()
    &&  mMap_IasFloat32Vector.empty()
    &&  mMap_IasStringVector.empty())
  {
    // If the last key was deleted also set the bool flag to false,
    // indicating that there are no more properties set
    mHasProperties = false;
  }
}

void IasProperties::clearAll()
{
  mMap_int64_t.clear();
  mMap_int32_t.clear();
  mMap_float64.clear();
  mMap_float32.clear();
  mMap_string.clear();
  mMap_IasInt64Vector.clear();
  mMap_IasInt32Vector.clear();
  mMap_IasFloat64Vector.clear();
  mMap_IasFloat32Vector.clear();
  mMap_IasStringVector.clear();
  mPropertyKeys.clear();
  mHasProperties = false;
}

IasProperties::IasProperties()
  :mHasProperties(false)
  ,mLog(IasAudioLogging::registerDltContext("PFW", "Log of rtprocessing framework"))
{
}

IasProperties::~IasProperties()
{
}

template <typename MAP, typename VALUE>
IasProperties::IasResult IasProperties::get(const MAP &map, const std::string &key, VALUE *value) const
{
  auto valueIt = map.find(key);
  if (valueIt != map.end())
  {
    if (value != nullptr)
    {
      *value = (*valueIt).second;
      return eIasOk;
    }
    else
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter. value == nullptr");
      return eIasInvalidParam;
    }
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Key", key, "not found in properties");
    return eIasKeyNotFound;
  }
}

void IasProperties::dump(const std::string &name) const
{
  if (hasProperties() == true)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX, "Keys of", name,":");
    for (auto key: mPropertyKeys)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX, "Key=", key);
    }
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX, name, "are empty");
  }
}

IasProperties::IasResult IasProperties::getKeyDataType(std::string key, std::string &dataType) const
{
  auto valueIt = mKeyDataTypes.find(key);
  if (valueIt != mKeyDataTypes.end())
  {
    if (key != "")
    {
      dataType = valueIt->second;
      return eIasOk;
    }
    else
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter. key == empty");
      return eIasInvalidParam;
    }
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Key", key, "not found in properties");
    return eIasKeyNotFound;
  }

}

} // namespace IasAudio
