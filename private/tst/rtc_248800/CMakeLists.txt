enable_testing()
find_package( GTest REQUIRED )

add_executable( rtc_248800
  private/tst/rtc_248800/src/rtc_248800.cpp
)

target_link_libraries( rtc_248800 ias-audio-common )
target_link_libraries( rtc_248800 ias-audio-smartx )
target_link_libraries( rtc_248800 GTest::GTest GTest::Main )

target_include_directories( rtc_248800 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/private/inc )

add_test(rtc_248800_tests rtc_248800 )

#install( TARGETS rtc_248800
#         RUNTIME
#            DESTINATION ${CMAKE_INSTALL_BINDIR}
#            COMPONENT Executables
#)
