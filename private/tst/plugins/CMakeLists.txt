IasInitPlugin( audio test_plugin_library )

  IasUseEntity( audio smartx )
  
  IasAddSources(
    IasTestComp.cpp
    IasTestPluginLibrary1.cpp
  )

IasBuildPlugin()

IasInitPlugin( audio test_plugin_library2 )

  
  IasUseEntity( audio smartx )

  IasAddSources(
    IasTestComp.cpp
    IasTestPluginLibrary2.cpp
  )

IasBuildPlugin()

IasInitPlugin( audio test_plugin_library3 )

  
  IasUseEntity( audio smartx )

  IasAddSources(
    IasTestComp.cpp
    IasTestPluginLibrary3.cpp
  )

IasBuildPlugin()

IasInitUnitTest( audio plugins )

  IasUseEntity( audio common )
  IasUseTestEntity( audio smartx_test_support )
  IasFindLibrary(GTEST_LIB gtest)
  
  IasAddSources(
    IasPluginsTest.cpp
  )

IasBuildUnitTest()

add_dependencies( test_plugins ias-audio-test_plugin_library )
add_dependencies( test_plugins ias-audio-test_plugin_library2 )
add_dependencies( test_plugins ias-audio-test_plugin_library3 )
