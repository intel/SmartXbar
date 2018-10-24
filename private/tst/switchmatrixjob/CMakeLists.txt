IasInitUnitTest( audio switchmatrixjob )

  IasUseEntity( audio common )
  IasUseTestEntity( audio smartx_test_support )
  IasFindLibrary(GTEST_LIB gtest)

  IasAddSources(
    main.cpp
    IasSwitchMatrixJobTest.cpp
  )

IasBuildUnitTest()