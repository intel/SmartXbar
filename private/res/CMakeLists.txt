#
# Copyright (C) 2018 Intel Corporation.All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
set( AUDIO_SMARTX_VERSION_MAJOR    4 )
set( AUDIO_SMARTX_VERSION_MINOR    0 )
set( AUDIO_SMARTX_VERSION_REVISION 7 )

include( CMakeLists.smartx.txt )

IasCreateComponentDocumentation( audio SmartXbar ${AUDIO_SMARTX_VERSION_MAJOR} ${AUDIO_SMARTX_VERSION_MINOR} ${AUDIO_SMARTX_VERSION_REVISION}
  common
  smartx
  volumex
  mixerx
  testfwx
  equalizerx
  configparser
)

IasCreateBbRecipeFile(audio ias-audio-smartx ${AUDIO_SMARTX_VERSION_MAJOR} ${AUDIO_SMARTX_VERSION_MINOR} ${AUDIO_SMARTX_VERSION_REVISION}
  SUMMARY  "SmartXbar - Smart Crossbar"
  DESCRIPTION "User-space component that provides configurable Audio routing and processing capabilities"
  GROUP "Multimedia/Audio"
  YOCTO_LAYER meta-ias-audio
  YOCTO_RECIPE_FOLDER recipes-multimedia/smartx
  YOCTO_GEN_EXTRA_FLAGS NO_BINBB NO_SRC
  PACKAGE ias-audio-smartx
    PACKAGE_SUMMARY "SmartXbar - Smart Crossbar"
    PACKAGE_DESCRIPTION "User-space component that provides configurable Audio routing and processing capabilities"
    PACKAGE_CONFIG_VERSION 1
    PACKAGE_ENTITIES smartx
    PACKAGE_ENTITIES SmartXbar
    PACKAGE_ENTITIES helperx
    PACKAGE_ENTITIES filterx
    PACKAGE_ENTITIES volumex
    PACKAGE_ENTITIES mixerx
    PACKAGE_ENTITIES equalizerx
    PACKAGE_ENTITIES modules
    PACKAGE_BUILD_REQUIRES tbb
    PACKAGE_BUILD_REQUIRES alsa-lib
    PACKAGE_BUILD_REQUIRES boost
  PACKAGE ias-audio-testfwx
    PACKAGE_SUMMARY "SmartXbar Test Framework"
    PACKAGE_DESCRIPTION "Test Framework for SmartXbar Plugin Modules"
    PACKAGE_ENTITIES testfwx
  PACKAGE ias-audio-configparser
    PACKAGE_SUMMARY "SmartXbar XML Parser"
    PACKAGE_DESCRIPTION "XML Parser for SmartXbar"
    PACKAGE_ENTITIES configparser
    PACKAGE_BUILD_REQUIRES libxml2
)
