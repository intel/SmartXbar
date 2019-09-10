#
# Copyright (C) 2019 Intel Corporation.All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
add_executable( alsa_capture_example
  private/apps/alsa_capture_example/src/alsa_capture_example.cpp
)

target_link_libraries( alsa_capture_example ias-audio-common )
target_link_libraries( alsa_capture_example ias-audio-smartx )
target_link_libraries( alsa_capture_example sndfile )


target_include_directories( alsa_capture_example PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/private/inc )


add_dependencies( alsa_capture_example ias-audio-modules )
add_dependencies(alsa_capture_example smartx_interactive_example)

install( TARGETS alsa_capture_example
         RUNTIME
            DESTINATION bin
            COMPONENT Executables
)
