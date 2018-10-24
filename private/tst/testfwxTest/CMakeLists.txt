IasInitUnitTest( audio testfwxTest )

  IasUseEntity( audio testfwx )
  IasUseEntity( audio smartx )
  IasUseEntity( audio modules )
  IasFindLibrary(GTEST_LIB gtest)
 

  IasAddSources(
    IasTestFrameworkTest.cpp
    IasTestFrameworkVolumeTest.cpp
    IasTestFrameworkTestMain.cpp
  )

  IasAddResourceFiles(
    "wav"
    ""
    "intel_masterbrand.wav"
  )
  
  IasFindPath( SNDFILE_INCLUDE "sndfile.h" )
  IasFindLibrary( SNDFILE_LIB sndfile )

  IasFindPath( TBB_INCLUDE "tbb/tbb.h" )
  IasFindLibrary( TBB_LIB  tbb)

IasBuildUnitTest()

add_dependencies( test_testfwxTest ias-audio-modules )
