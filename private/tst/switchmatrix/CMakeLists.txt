IasInitUnitTest( audio switchmatrix )

  IasUseEntity( audio common )
  IasUseTestEntity( audio smartx_test_support )
  IasFindLibrary(GTEST_LIB gtest)

  IasAddSources(
    main.cpp
    IasSwitchMatrixTest.cpp
  )

IasBuildUnitTest()

add_custom_command(
  TARGET test_switchmatrix
  POST_BUILD
  COMMAND cp ARGS /nfs/ka/proj/ias/organisation/teams/audio/TestWavRefFiles/2016-02-18/test_ch0.wav /tmp
)
add_custom_command(
  TARGET test_switchmatrix
  POST_BUILD
  COMMAND cp ARGS /nfs/ka/proj/ias/organisation/teams/audio/TestWavRefFiles/2016-02-18/test_ch1.wav /tmp
)