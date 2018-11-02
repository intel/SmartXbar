/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasProperties.hpp
 * @date   2013
 * @brief  This file contains the IasProperties class declaration.
 */

#ifndef IASPROPERTIES_HPP_
#define IASPROPERTIES_HPP_

#include <dlt/dlt.h>

#include "avbaudiomodules/audio/common/IasAudioCommonTypes.hpp"


#define TOSTRING(x) #x
#define PROPERTY_MAP_TYPE(prop_type) Ias##prop_type##Map
#define PROPERTY_MAP_MEMBER(prop_type) mMap_##prop_type
#define DEFINE_PROPERTY_TYPE(prop_ns, prop_type) using PROPERTY_MAP_TYPE(prop_type) = std::map<std::string, prop_ns::prop_type>; \
  PROPERTY_MAP_TYPE(prop_type)     PROPERTY_MAP_MEMBER(prop_type)


namespace IasAudio {

/**
 * @brief std::string list type for the keys of all included properties
 */
using IasKeyList = std::list<std::string>;

using float32 = float;
using float64 = double;

class IAS_AUDIO_PUBLIC IasProperties
{
  public:
    enum IasResult
    {
      eIasOk,               //!< No error
      eIasKeyNotFound,      //!< Key not found
      eIasInvalidParam      //!< Invalid param
    };

    /**
     * @brief Constructor.
     */
    IasProperties();

    /**
     * @brief Destructor, virtual.
     */
    virtual ~IasProperties();

    /**
     * @brief Set a property of type VALUE
     *
     * @param[in] key The name of the property
     * @param[in] value The new value of the property
     */
    template <typename VALUE>
    void set(const std::string &key, const VALUE &value);

    /**
     * @brief Get a property of type VALUE
     *
     * @param[in] key The name of the property
     * @param[in,out] value A location where the value shall be stored
     *
     * @returns eIasOk in case of success or any other value in case of an error.
     */
    template <typename VALUE>
    IasResult get(const std::string &key, VALUE *value) const;

    /**
     * @brief Clear a specific property referenced by key.
     *
     * @param[in] key The key of the property to be cleared.
     */
    void clear(const std::string &key);

    /**
     * @brief Clear all properties.
     */
    void clearAll();

    /**
     * @brief Check if this instance has at least one property set.
     *
     * @returns Returns true if at least one property is set or false if no property is set.
     */
    bool hasProperties() const { return mHasProperties; }

    /**
     * @brief Get a list containing all keys that are set within the IasProperties instance
     *
     * @returns A list containing all keys that are set within the IasProperties instance
     */
    const IasKeyList& getPropertyKeys() const { return mPropertyKeys; }

    /**
     * @brief Dump the properties to DLT
     *
     * @param[in] name Name to be used in log for describing the properties
     */
    void dump(const std::string &name) const;

    /**
     * @brief Get a key data type
     *
     * @param[in] key The name of the property
     * @param[in,out] dataType dataType of the key
     *
     * @returns eIasOk in case of success or any other value in case of an error.
     */
    IasResult getKeyDataType(std::string key, std::string &dataType) const;

  private:
    /**
     * @brief Generic method to get the value for a given key from a given map
     *
     * @param[in] map The map where to look for the key and the value
     * @param[in] key The key to look for in the map
     * @param[out] value The value stored for the given key in the given map
     */
    template <typename MAP, typename VALUE>
    IasResult get(const MAP &map, const std::string &key, VALUE *value) const;

    bool            mHasProperties;                     //!< True if this instance has some properties set, false otherwise
    IasKeyList      mPropertyKeys;                      //!< List of all property keys
    std::map<std::string, std::string> mKeyDataTypes;   //!< map of key data types
    DEFINE_PROPERTY_TYPE(std, int64_t);                 //!< Property type for Ias::Int64
    DEFINE_PROPERTY_TYPE(std, int32_t);                 //!< Property type for Ias::Int32
    DEFINE_PROPERTY_TYPE(IasAudio, float64);            //!< Property type for Ias::Float64
    DEFINE_PROPERTY_TYPE(IasAudio, float32);            //!< Property type for Ias::Float32
    DEFINE_PROPERTY_TYPE(std, string);                  //!< Property type for Ias::String
    DEFINE_PROPERTY_TYPE(IasAudio, IasInt64Vector);     //!< Property type for IasInt64Vector
    DEFINE_PROPERTY_TYPE(IasAudio, IasInt32Vector);     //!< Property type for IasInt32Vector
    DEFINE_PROPERTY_TYPE(IasAudio, IasFloat64Vector);   //!< Property type for IasFloat64Vector
    DEFINE_PROPERTY_TYPE(IasAudio, IasFloat32Vector);   //!< Property type for IasFloat32Vector
    DEFINE_PROPERTY_TYPE(IasAudio, IasStringVector);    //!< Property type for IasStringVector
    DltContext     *mLog;                               //!< The DLT context
};

/**
 * @brief Function to get a IasProperties::IasResult as string.
 *
 * @param[in] type The IasProperties::IasResult value
 *
 * @return Enum Member as string
 */
std::string toString(const IasProperties::IasResult& type);


} //namespace IasAudio

#endif /* IASPROPERTIES_HPP_ */
