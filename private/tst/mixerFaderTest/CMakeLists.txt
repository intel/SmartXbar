IasInitUnitTest( audio mixerFaderTest )

  IasUseEntity( audio modules )
  IasFindLibrary(GTEST_LIB gtest)
  
  IasAddSources(
    mixerFaderTest.cpp
  )

  IasFindPath( SNDFILE_INCLUDE "sndfile.h" )
  IasFindLibrary( SNDFILE_LIB sndfile )

IasBuildUnitTest()

add_dependencies( test_mixerFaderTest ias-audio-modules )