#
# Copyright (C) 2018 Intel Corporation.All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
CMAKE_MINIMUM_REQUIRED( VERSION 3.4.3 )
project(SmartXbar)

set( AUDIO_SMARTX_VERSION_MAJOR    4 )
set( AUDIO_SMARTX_VERSION_MINOR    0 )
set( AUDIO_SMARTX_VERSION_REVISION 7 )

set( AUDIO_SMARTX_VERSION_STRING ${AUDIO_SMARTX_VERSION_MAJOR}.${AUDIO_SMARTX_VERSION_MINOR}.${AUDIO_SMARTX_VERSION_REVISION})

# create a version.h file
set(CURRENT_VERSION_MAJOR ${AUDIO_SMARTX_VERSION_MAJOR})
set(CURRENT_VERSION_MINOR ${AUDIO_SMARTX_VERSION_MINOR})
set(CURRENT_VERSION_REVISION ${AUDIO_SMARTX_VERSION_REVISION})
set(VERSION_H_INCLUDE_GUARD "AUDIO_SMARTX_VERSION_H")
string(TOUPPER ${VERSION_H_INCLUDE_GUARD} VERSION_H_INCLUDE_GUARD)
string(REGEX REPLACE "[^a-zA-Z0-9_]" "_" VERSION_H_INCLUDE_GUARD ${VERSION_H_INCLUDE_GUARD})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/private/inc/helper/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/version.h)
unset(CURRENT_VERSION_MAJOR)
unset(CURRENT_VERSION_MINOR)
unset(CURRENT_VERSION_REVISION)
unset(VERSION_H_INCLUDE_GUARD)

if( NOT CMAKE_INSTALL_LIBDIR )
  # This can be overwritten externally by defining it to lib64 if required
  set( CMAKE_INSTALL_LIBDIR lib )
  message( "CMAKE_INSTALL_LIBDIR=${CMAKE_INSTALL_LIBDIR}" )
endif()

include_directories(/usr/include/dlt)

include( CMakeLists.smartx.txt )
include( private/tst/rtc_248800/CMakeLists.txt )
include( private/tst/smartx_api_mutex/CMakeLists.txt )
include( private/tst/plugin_use_cases_tst/CMakeLists.txt )
include( private/src/tools/vads/CMakeLists.txt )

#add_custom_command( TARGET public-doc-audio-SmartXbar-copy-api
#                    PRE_BUILD
#                    COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/public/doc/Parser_2_7.py ${CMAKE_CURRENT_SOURCE_DIR}/../..)


#IasGlobalPropertyAppend(IAS_COVERAGE_OFF_DIRS
# ${CMAKE_CURRENT_SOURCE_DIR}/private/tst/**
#)
