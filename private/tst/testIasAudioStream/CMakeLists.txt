IasInitUnitTest( audio IasAudioStream )

  IasUseEntity( audio smartx )
  IasFindLibrary(GTEST_LIB gtest)

  IasAddSources(
    testIasAudioStream.cpp
  )

IasBuildUnitTest()
