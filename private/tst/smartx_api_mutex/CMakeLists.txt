enable_testing()
find_package( GTest REQUIRED )

add_executable( test_smartx_api_mutex
  private/tst/smartx_api_mutex/src/IasSmartX_API_Mutex_Test.cpp
)

target_link_libraries( test_smartx_api_mutex ias-audio-common )
target_link_libraries( test_smartx_api_mutex ias-audio-smartx )
target_link_libraries( test_smartx_api_mutex GTest::GTest GTest::Main )

target_include_directories( test_smartx_api_mutex PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/private/inc )

add_test(smartx_api_mutex_tests test_smartx_api_mutex )

add_dependencies( test_smartx_api_mutex ias-audio-modules )

#install( TARGETS rtc_248800
#         RUNTIME
#            DESTINATION ${CMAKE_INSTALL_BINDIR}
#            COMPONENT Executables
#)


