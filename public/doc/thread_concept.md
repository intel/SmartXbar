Thread concept
================
@page md_thread_concept

In the SmartXbar there are two different kinds of threads used:

* non-real-time threads and
* real-time threads

The non-real-time threads are only used for setup and for command and control jobs. All threads being involved in
moving or processing audio data are scheduled by the real-time scheduler. The default real-time scheduling policy is *fifo*.
The default scheduling policy, priority and CPU affinity can be changed in the
[SmartXbar configuration file](@ref scheduling_rt).

Details about thread names can be found in the [analysis chapter](@ref thread_names).

###############################
@section non_real_time_threads Non-real-time Threads

For every audio source or audio sink device of clock type IasAudio::eIasClockProvided, an additional communicator thread is created.
It is only used to send data from an ALSA plug-in to the SmartXbar and vice versa.

###############################
@section real_time_threads Real-time Threads

There are two scenarios when a real-time thread is created:

* when a new base routing zone is created
* when an audio source or sink device of clock type IasAudio::eIasClockReceivedAsync is created

That means for a strict synchronous system, there will be only one real-time thread. Asynchronous
sample rate converters wouldn't be required and there is only one base routing zone and none or
several derived routing zones.
