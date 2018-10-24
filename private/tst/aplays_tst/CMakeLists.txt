IasInitUnitTest( audio aplays_tst )

  IasUseEntity( audio smartx )
  IasFindLibrary(GTEST_LIB gtest)
  
  IasAddSources(
    aplays.cpp
  )

IasBuildUnitTest()
