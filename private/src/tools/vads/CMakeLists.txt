add_executable( vads
  private/src/tools/vads/DevManager.cpp
  private/src/tools/vads/IDev.cpp
  private/src/tools/vads/IasRingBufferTestReader.cpp
  private/src/tools/vads/IasRingBufferTestWriter.cpp
  private/src/tools/vads/Sink.cpp
  private/src/tools/vads/Source.cpp
  private/src/tools/vads/Timing.cpp
  private/src/tools/vads/main.cpp
)

target_compile_options( vads PUBLIC -ffast-math )
target_compile_options( vads PUBLIC --std=c++11 )
target_compile_options( vads PUBLIC -msse )
target_compile_options( vads PUBLIC -msse2 )
target_compile_options( vads PUBLIC -O3 )

target_link_libraries( vads ias-audio-common )
target_link_libraries( vads ias-audio-smartx )
target_link_libraries( vads asound )
target_link_libraries( vads sndfile )
target_link_libraries( vads pthread )

install( TARGETS vads
         RUNTIME
            DESTINATION bin
            COMPONENT Executables
)

