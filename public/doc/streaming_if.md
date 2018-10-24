Streaming Interface
===================
@page md_streaming_if

The streaming interface is a pure ALSA based solution and can be separated into two general mechanisms:

* ALSA interface is provided by the SmartXbar, i.e. the SmartXbar provides ALSA playback and ALSA capture devices that the application can use like
standard ALSA devices.
* ALSA interface is used by the SmartXbar, i.e. the SmartXbar uses externally provided ALSA devices, like e.g. a real ALSA hw driver or the ALSA AVB devices.

For both kinds of mechanisms it is necessary to add the audio device with the desired parameters to the SmartXbar configuration via the IasAudio::IasISetup interface.

################################################
@section alsa_if_provided ALSA Interfaces Provided by SmartXbar

The ALSA interfaces that are provided by the SmartXbar are based on the
[External Plugin: I/O Plugin](http://www.alsa-project.org/alsa-doc/alsa-lib/pcm_external_plugins.html#pcm_ioplug) and are called alsa-smartx-plugin.
That means they are not implemented as a real Kernel driver but as a user-space ALSA plugin and thus all constraints of the ALSA user-space plugins equally apply
to the alsa-smartx-plugin. An audio device of this type can also be seen like it **provides** a clock to the external application.
After the audio device was added to the SmartXbar configuration an audio application can use the standard ALSA library
interface to access the ALSA device and playback or capture audio. The name to be used by the application always has to be prefixed by **smartx:** and then the name
used during creation via the IasAudio::IasISetup interface has to be appended. Here is an example if the standard ALSA player application aplay is used to play a wave file to a
previously created source device named *stereo0*:

    aplay -Dsmartx:stereo0 YOUR_WAVE_FILE.wav

################################################
@section alsa_if_used ALSA Interfaces Used by SmartXbar

The SmartXbar is able to handle external ALSA devices as playback and as capture devices. An audio device of this type can also be seen like it **receives** a clock
from the external ALSA device and is thus able to drive the SmartXbar routing and processing engine. The ALSA handler inside the SmartXbar dealing with the ALSA devices
always uses the [direct transfer](http://www.alsa-project.org/alsa-doc/alsa-lib/pcm.html#alsa_mmap_rw) mechanism also called zero-copy to avoid unnecessary copy transactions.
