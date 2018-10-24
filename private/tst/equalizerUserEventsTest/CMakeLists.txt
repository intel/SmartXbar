IasInitUnitTest( audio equalizerUserEventsTest )

  IasUseEntity( audio modules )
  IasFindLibrary(GTEST_LIB gtest)
 
  IasAddSources(
    equalizerUserEventsTest.cpp
  )

  IasFindPath( SNDFILE_INCLUDE "sndfile.h" )
  IasFindLibrary( SNDFILE_LIB sndfile )

IasBuildUnitTest()

add_dependencies( test_equalizerUserEventsTest ias-audio-modules )
