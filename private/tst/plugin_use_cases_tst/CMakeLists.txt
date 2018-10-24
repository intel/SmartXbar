enable_testing()
find_package( GTest REQUIRED )

add_executable( test_plugin_use_cases
  private/tst/plugin_use_cases_tst/src/IasAlsaPluginUseCasesTest.cpp
)

target_link_libraries( test_plugin_use_cases ias-audio-common )
target_link_libraries( test_plugin_use_cases ias-audio-smartx )
target_link_libraries( test_plugin_use_cases GTest::GTest GTest::Main )
target_link_libraries( test_plugin_use_cases sndfile )
target_link_libraries( test_plugin_use_cases tbb )


target_include_directories( test_plugin_use_cases PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/private/inc )

add_test(smartx_plugin_use_cases_tests test_plugin_use_cases )

add_dependencies( test_plugin_use_cases ias-audio-modules )
