IasInitUnitTest( audio mixerGainOffsetTest )

  IasUseEntity( audio modules )
  IasFindLibrary(GTEST_LIB gtest)
  
  IasAddSources(
    mixerGainOffsetTest.cpp
  )

  IasFindPath( SNDFILE_INCLUDE "sndfile.h" )
  IasFindLibrary( SNDFILE_LIB sndfile )

IasBuildUnitTest()

add_dependencies( test_mixerGainOffsetTest ias-audio-modules )
