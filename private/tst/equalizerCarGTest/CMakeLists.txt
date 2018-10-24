IasInitUnitTest( audio equalizerCarGTest )

  IasUseEntity( audio smartx )
  IasFindLibrary(GTEST_LIB gtest)
  
  IasAddSources(
    equalizerCarGTestMain.cpp
    IasEqualizerCarGTest.cpp
  )

  IasFindPath( SNDFILE_INCLUDE "sndfile.h" )
  IasFindLibrary( SNDFILE_LIB sndfile )

IasBuildUnitTest()

add_dependencies( test_equalizerCarGTest ias-audio-modules )
