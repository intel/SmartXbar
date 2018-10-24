IasInitUnitTest( audio year_2038_tst )

  
 IasFindLibrary(GTEST_LIB gtest)

  IasAddSources(
    IasYear2038Test.cpp
  )

IasBuildUnitTest()
