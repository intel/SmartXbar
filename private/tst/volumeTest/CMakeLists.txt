IasInitUnitTest( audio volumeTest )

  IasUseEntity( audio smartx )
  IasFindLibrary(GTEST_LIB gtest)

  IasAddSources(
    volumeTestMain.cpp
    IasVolumeTest.cpp
  )

  IasFindPath( SNDFILE_INCLUDE "sndfile.h" )
  IasFindLibrary( SNDFILE_LIB sndfile )

IasBuildUnitTest()

add_dependencies( test_volumeTest ias-audio-modules )
