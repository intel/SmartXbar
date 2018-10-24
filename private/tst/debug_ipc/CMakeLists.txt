#------------------------------------------------------------------
# Build the library
#------------------------------------------------------------------

IasInitLibrary( audio debug_ipc ${AUDIO_SMARTX_VERSION_MAJOR} ${AUDIO_SMARTX_VERSION_MINOR} ${AUDIO_SMARTX_VERSION_REVISION} )

  IasSetEntityInstallPrefix( audio )

  IasUsePrivateEntity( audio common )
  IasUsePrivateEntity( audio smartx )

  IasAddSources(
    IasDebugCommunicator.cpp
  )

  IasFindLibrary( BOOST_FILESYSTEM_LIB  boost_filesystem )

IasBuildLibrary( )
