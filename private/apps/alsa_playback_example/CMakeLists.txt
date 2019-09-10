#
# Copyright (C) 2019 Intel Corporation.All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
add_executable( alsa_playback_example
  private/apps/alsa_playback_example/src/alsa_playback_example.cpp
)

target_link_libraries( alsa_playback_example ias-audio-common )
target_link_libraries( alsa_playback_example ias-audio-smartx )
target_link_libraries( alsa_playback_example sndfile )
target_link_libraries( alsa_playback_example asound )

target_include_directories( alsa_playback_example PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/private/inc )

add_dependencies( alsa_playback_example ias-audio-modules )
add_dependencies(alsa_playback_example smartx_interactive_example)

install( TARGETS alsa_playback_example
         RUNTIME
            DESTINATION bin
            COMPONENT Executables
)
