IasInitUnitTest( audio equalizerUserTest )

  IasUseEntity( audio modules )
  IasFindLibrary(GTEST_LIB gtest)

  IasAddSources(
    equalizerUserTest.cpp
  )

  IasFindPath( SNDFILE_INCLUDE "sndfile.h" )
  IasFindLibrary( SNDFILE_LIB sndfile )

IasBuildUnitTest()

add_dependencies( test_equalizerUserTest ias-audio-modules )
