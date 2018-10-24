IasInitUnitTest( audio rampTest )

  IasUseEntity( audio helperx )
  IasUseEntity( audio smartx )
  IasFindLibrary(GTEST_LIB gtest)
  
  IasAddSources(
    main.cpp
  )

  IasFindPath( SNDFILE_INCLUDE "sndfile.h" )
  IasFindLibrary( SNDFILE_LIB sndfile )

IasBuildUnitTest()
