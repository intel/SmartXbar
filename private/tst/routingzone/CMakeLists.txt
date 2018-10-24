IasInitPlugin( audio test_routingzone_plugin_library )

  
  IasUseEntity( audio smartx )
  
  IasFindPath( TBB_INCLUDE "tbb/tbb.h" )
  IasFindLibrary( TBB_LIB  tbb)

  IasAddSources(
    IasSimpleVolumeCmd.cpp
    IasSimpleVolumeCore.cpp
    IasMyPluginLibrary.cpp
  )

IasBuildPlugin()


IasInitUnitTest( audio routingzone NO_WARNING_AS_ERROR )

  IasUseEntity( audio common )
  IasUseTestEntity( audio smartx_test_support )
  IasFindLibrary(GTEST_LIB gtest)

  IasAddSources(
    main.cpp
    IasRoutingZoneTest.cpp
    IasMyComplexPipeline.cpp
    IasMySimplePipeline.cpp
  )

IasBuildUnitTest()

add_dependencies( test_routingzone ias-audio-test_routingzone_plugin_library )
add_dependencies( test_routingzone ias-audio-modules )
