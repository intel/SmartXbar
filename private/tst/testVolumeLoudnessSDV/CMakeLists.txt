IasInitUnitTest( audio testVolumeLoudnessSDV NO_WARNING_AS_ERROR )

  IasUseEntity( audio smartx )
  IasFindLibrary(GTEST_LIB gtest)

  IasAddSources(
    main.cpp
  )

  IasFindPath( SNDFILE_INCLUDE "sndfile.h" )
  IasFindLibrary( SNDFILE_LIB sndfile )

IasBuildUnitTest()

add_dependencies( test_testVolumeLoudnessSDV ias-audio-modules )
