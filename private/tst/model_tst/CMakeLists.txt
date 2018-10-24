IasInitUnitTest( audio model_tst )

  IasUseEntity( audio smartx )
  IasFindLibrary(GTEST_LIB gtest)

  IasAddSources(
    IasAudioModelTest.cpp
  )

IasBuildUnitTest()
