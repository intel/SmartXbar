IasInitUnitTest( audio configparserTest )

  IasUseEntity( audio configparser)
  IasUseEntity( audio smartx )
  IasUseEntity( audio modules )
  IasUseEntity( core_libraries foundation )
  IasUseTestEntity( core_libraries test_support )

  IasAddSources(
    IasConfigParserTest.cpp
    IasAudioDataComparer.cpp
    IasAudioDevicesComparer.cpp
    IasPortsComparer.cpp
    IasRoutingZonesComparer.cpp
  )


  IasFindPath( XML_INCLUDE "libxml/tree.h" HINTS /usr/include/libxml2)
  IasFindLibrary( XML_LIB xml2 )
  

IasBuildUnitTest()

add_dependencies( test_configparserTest ias-audio-modules )



