IasInitUnitTest( audio mixerBalanceTest )

  IasUseEntity( audio modules )
  IasFindLibrary(GTEST_LIB gtest)
  
  IasAddSources(
    mixerBalanceTest.cpp
  )

  IasFindPath( SNDFILE_INCLUDE "sndfile.h" )
  IasFindLibrary( SNDFILE_LIB sndfile )

IasBuildUnitTest()

add_dependencies( test_mixerBalanceTest ias-audio-modules )
