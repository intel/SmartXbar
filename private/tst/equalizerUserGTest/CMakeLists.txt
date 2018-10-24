IasInitUnitTest( audio equalizerUserGTest )

  IasUseEntity( audio smartx )
  IasFindLibrary(GTEST_LIB gtest)

  IasAddSources(
    equalizerUserGTestMain.cpp
    IasEqualizerUserGTest.cpp
  )

  IasFindPath( SNDFILE_INCLUDE "sndfile.h" )
  IasFindLibrary( SNDFILE_LIB sndfile )

IasBuildUnitTest()

add_dependencies( test_equalizerUserGTest ias-audio-modules )
