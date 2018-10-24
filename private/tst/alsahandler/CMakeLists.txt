IasInitUnitTest( audio  alsahandler )

  
  IasUseEntity( audio common )
  IasUseTestEntity( audio smartx_test_support )
  IasFindLibrary(GTEST_LIB gtest)
  
  IasAddSources(
    IasAlsaHandlerTestMain.cpp
    IasAlsaHandlerTest.cpp
    IasAlsaHandlerTestPlayback.cpp
    IasAlsaHandlerTestPlaybackStartStop.cpp
    IasAlsaHandlerTestPlaybackAsync.cpp
    IasAlsaHandlerTestCapture.cpp
    IasAlsaHandlerTestCaptureAsync.cpp
    IasDrift.cpp
  )

   IasAddResourceFiles(
     "resources"
     ""
     "drift.txt"
   )

IasBuildUnitTest()
