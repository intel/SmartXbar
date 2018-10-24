IasInitUnitTest( audio testVolumeLoudnessCore )

  IasUseEntity( audio modules )
  IasFindLibrary(GTEST_LIB gtest)

  IasAddSources(
    main.cpp
  )

  IasFindPath( SNDFILE_INCLUDE "sndfile.h" )
  IasFindLibrary( SNDFILE_LIB sndfile )

IasBuildUnitTest()

add_dependencies( test_testVolumeLoudnessCore ias-audio-modules )
