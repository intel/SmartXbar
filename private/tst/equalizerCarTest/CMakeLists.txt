IasInitUnitTest( audio equalizerCarTest )

  IasUseEntity( audio modules )
  IasFindLibrary(GTEST_LIB gtest)
  
  IasAddSources(
    equalizerCarTest.cpp
  )

  IasFindPath( SNDFILE_INCLUDE "sndfile.h" )
  IasFindLibrary( SNDFILE_LIB sndfile )

IasBuildUnitTest()

set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES 
"test_outstream1.wav"
"test_outstream2.wav"
"test_outstream3.wav"
"test_outstream4.wav"
)

add_dependencies( test_equalizerCarTest ias-audio-modules )
