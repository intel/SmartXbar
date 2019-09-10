#
# Copyright (C) 2019 Intel Corporation.All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
enable_testing()
find_package( GTest REQUIRED )

add_executable( smartx_interactive_example
  private/apps/smartx_interactive_example/src/smartx_interactive_example.cpp
)

target_link_libraries( smartx_interactive_example ias-audio-common )
target_link_libraries( smartx_interactive_example ias-audio-smartx )
target_link_libraries( smartx_interactive_example ias-audio-configparser )
target_link_libraries( smartx_interactive_example sndfile )
#ToDo:need to enable if needed DebugAgent
#target_link_libraries( smartx_interactive_example DebugAgentIpcServer )

target_include_directories( smartx_interactive_example PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/private/inc )


add_dependencies( smartx_interactive_example ias-audio-modules )
#ToDO if needed
#add_dependencies( smartx_interactive_example core_libraries-foundation )


install( TARGETS smartx_interactive_example
         RUNTIME
            DESTINATION bin
            COMPONENT Executables
)
