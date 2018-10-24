IasInitUnitTest( audio mixerMultiChannelTest )

  IasUseEntity( audio modules )
  IasFindLibrary(GTEST_LIB gtest)

  IasAddSources(
    mixerMultiChannelTest.cpp
  )

  IasFindPath( SNDFILE_INCLUDE "sndfile.h" )
  IasFindLibrary( SNDFILE_LIB sndfile )

IasBuildUnitTest()

add_dependencies( test_mixerMultiChannelTest ias-audio-modules )
