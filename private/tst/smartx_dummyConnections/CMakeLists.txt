IasInitUnitTest( audio smartx_dummyConnections )

  IasUseEntity( audio smartx )
  IasFindLibrary(GTEST_LIB gtest)

  IasAddSources(
    main.cpp
     IasDummyConnectionTest.cpp
  )

IasBuildUnitTest()

