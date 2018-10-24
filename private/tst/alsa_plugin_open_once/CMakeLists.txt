IasInitUnitTest( audio alsa_plugin_open_once )

  IasUseEntity( core_libraries foundation )
  IasUseTestEntity( core_libraries test_support )
  IasUseEntity( audio smartx )
  
  IasFindPath( ASOUND_INCLUDE "sound/asound.h" )
  IasFindLibrary( ASOUND_LIB asound )

  IasAddSources(
    alsa_plugin_open_once.cpp
  )

IasBuildUnitTest()

if ( "$ENV{IAS_CONTINUOUS_INTEGRATION_BUILD}" STREQUAL "" )

  add_dependencies( test_alsa_plugin_open_once libasound_module_pcm_smartx )

  add_custom_command(
    TARGET test_alsa_plugin_open_once
    POST_BUILD
    COMMAND cp ARGS $<TARGET_FILE:libasound_module_pcm_smartx> /tmp
  )

endif()
