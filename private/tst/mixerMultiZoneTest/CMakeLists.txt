IasInitUnitTest( audio mixerMultiZoneTest )

  IasUseEntity( audio modules )
  IasFindLibrary(GTEST_LIB gtest)
 
  IasAddSources(
    mixerMultiZoneTest.cpp
  )

  IasFindPath( SNDFILE_INCLUDE "sndfile.h" )
  IasFindLibrary( SNDFILE_LIB sndfile )

IasBuildUnitTest()

add_dependencies( test_mixerMultiZoneTest ias-audio-modules )
