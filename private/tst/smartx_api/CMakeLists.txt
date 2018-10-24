IasInitUnitTest( audio smartx_api NO_WARNING_AS_ERROR )

  
 
  IasUseEntity( audio smartx )
  IasUseEntity( audio common )
  IasFindLibrary(GTEST_LIB gtest)
  
  IasAddSources(
    main.cpp
    IasSmartX_API_Test.cpp
    IasCustomerEventHandler.cpp
  )
  IasAddResourceFiles(
    "res/rr"
    "."
    smartx_config.txt
  )
  IasAddResourceFiles(
    "res/rr"
    "rr"
    smartx_config.txt
  )
  IasAddResourceFiles(
    "res/cfs"
    "cfs"
    smartx_config.txt
  )
  IasAddResourceFiles(
    "res/cfs_1"
    "cfs_1"
    smartx_config.txt
  )
  IasAddResourceFiles(
    "res/cfs_1"
    "cfs_1"
    smartx_config.txt
  )
  IasAddResourceFiles(
    "res/fifo"
    "fifo"
    smartx_config.txt
  )
  IasAddResourceFiles(
    "res/fifo_100"
    "fifo_100"
    smartx_config.txt
  )
  IasAddResourceFiles(
    "res/inv"
    "inv"
    smartx_config.txt
  )
  IasAddResourceFiles(
    "res/empty_file"
    "empty_file"
    smartx_config.txt
  )
  IasAddResourceFiles(
    "res/corrupt"
    "corrupt"
    smartx_config.txt
  )
  IasAddResourceFiles(
    "res/cpu_affinities_1"
    "cpu_affinities_1"
    smartx_config.txt
  )
  IasAddResourceFiles(
    "res/unregistered_key"
    "unregistered_key"
    smartx_config.txt
  )
  IasAddResourceFiles(
    "res/runner_all_disabled"
    "runner_all_disabled"
    smartx_config.txt
  )
  IasAddResourceFiles(
    "res/runner_all_enabled"
    "runner_all_enabled"
    smartx_config.txt
  )
  IasAddResourceFiles(
    "res/runner_not_included"
    "runner_not_included"
    smartx_config.txt
  )
  IasAddResourceFiles(
    "res/runner_specific_enabled"
    "runner_specific_enabled"
    smartx_config.txt
  )

IasBuildUnitTest()

add_dependencies( test_smartx_api ias-audio-modules )

if ( "$ENV{IAS_CONTINUOUS_INTEGRATION_BUILD}" STREQUAL "" )

  add_dependencies( test_smartx_api libasound_module_pcm_smartx )

  add_custom_command(
    TARGET test_smartx_api
    POST_BUILD
    COMMAND cp ARGS $<TARGET_FILE:libasound_module_pcm_smartx> /tmp
  )

endif()
