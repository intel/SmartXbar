IasInitUnitTest( audio debugFascadeTest )

  IasUseEntity( audio configparser)
  IasUseEntity( audio smartx )
  IasUseEntity( audio modules )
  IasUseEntity( core_libraries foundation )
  IasUseTestEntity( core_libraries test_support )

  IasAddSources(
    SmartXDebugFacadeTest.cpp
    main.cpp
  )

  IasFindPath( XML_INCLUDE "libxml/tree.h" HINTS /usr/include/libxml2)
  IasFindLibrary( XML_LIB xml2 )
  

IasBuildUnitTest()

add_dependencies( test_debugFascadeTest ias-audio-modules )