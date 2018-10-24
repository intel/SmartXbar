IasInitPlugin( audio test_pipeline_plugin_library )

  
  IasUseEntity( audio smartx )
  IasFindLibrary(GTEST_LIB gtest)
 
  IasFindPath( TBB_INCLUDE "tbb/tbb.h" )
  IasFindLibrary( TBB_LIB  tbb)

  IasAddSources(
    IasSimpleVolumeCmd.cpp
    IasSimpleVolumeCore.cpp
    IasMyPluginLibrary.cpp
  )

IasBuildPlugin()


IasInitUnitTest( audio  pipeline )
 
  IasUseEntity( audio smartx )
  IasUseEntity( audio common )
  IasUseTestEntity( audio smartx_test_support )
  IasFindLibrary(GTEST_LIB gtest)

  IasAddSources(
    IasPipelineTestMain.cpp
    IasPipelineTest.cpp
  )

IasBuildUnitTest()

add_dependencies( test_pipeline ias-audio-test_pipeline_plugin_library )
