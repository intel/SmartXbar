IasInitUnitTest( audio diagnostic_test )

  IasUseEntity( core_libraries foundation )
  IasUseTestEntity( core_libraries test_support )

  IasAddSources(
    diagnostic_test.cpp
  )
  IasAddResourceFiles(
    "res/config_ok"
    "config_ok"
    smartx_config.txt
  )
  IasAddResourceFiles(
    "res/min_config_ok"
    "min_config_ok"
    smartx_config.txt
  )

IasBuildUnitTest()
