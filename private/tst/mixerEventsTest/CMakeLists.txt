IasInitUnitTest( audio mixerEventsTest )

  IasUseEntity( audio modules )
  IasFindLibrary(GTEST_LIB gtest)
  
  IasAddSources(
    mixerEventsTest.cpp
  )

  IasFindPath( SNDFILE_INCLUDE "sndfile.h" )
  IasFindLibrary( SNDFILE_LIB sndfile )

IasBuildUnitTest()

add_dependencies( test_mixerEventsTest ias-audio-modules )
