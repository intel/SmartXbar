IasInitUnitTest( audio equalizerCore )

  IasUseEntity( audio modules )
  IasFindLibrary(GTEST_LIB gtest)
  
  IasAddSources(
    equalizerCoreTest.cpp
  )

  IasFindPath( SNDFILE_INCLUDE "sndfile.h" )
  IasFindLibrary( SNDFILE_LIB sndfile )

IasBuildUnitTest()

add_dependencies( test_equalizerCore ias-audio-modules )
