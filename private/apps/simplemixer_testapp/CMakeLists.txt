#
# Copyright (C) 2019 Intel Corporation.All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
add_executable( simplemixer_testapp
  private/apps/simplemixer_testapp/src/IasSimpleMixerTestApp.cpp
)

target_link_libraries( simplemixer_testapp ias-audio-common )
target_link_libraries( simplemixer_testapp ias-audio-smartx )
target_link_libraries( simplemixer_testapp ias-audio-testfwx )
target_link_libraries( simplemixer_testapp sndfile )
target_link_libraries( simplemixer_testapp asound )


target_include_directories( simplemixer_testapp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/private/inc )


add_dependencies( simplemixer_testapp ias-audio-modules )
add_dependencies( simplemixer_testapp ias-audio-plugin_library_example )


install( TARGETS simplemixer_testapp
         RUNTIME
            DESTINATION bin
            COMPONENT Executables
)
