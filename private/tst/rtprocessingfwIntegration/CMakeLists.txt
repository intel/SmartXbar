IasInitUnitTest( audio rtprocessingfwIntegration )

  IasFindLibrary(GTEST_LIB gtest)

  IasAddSources(
    IasRtProcessingFwTest.cpp
    IasRtProcessingFwTestMain.cpp
    IasAudioChainEnvironmentTest.cpp
    IasAudioChannelBundleTest.cpp
    IasAudioStreamTest.cpp
    IasBundleAssignmentTest.cpp
    IasBundleSequencerTest.cpp
    IasTestComp.cpp
    IasAudioChainTest.cpp
    IasPropertiesTest.cpp
    IasModuleEventTest.cpp
    IasAudioBufferTest.cpp
    IasProcessingTypesTest.cpp
    IasCmdDispatcherTest.cpp
    IasGenericAudioCompConfigTest.cpp
)

IasBuildUnitTest()
