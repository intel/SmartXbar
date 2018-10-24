# Analysis

#################################################################################
@section general_analysis General Analysis Hints

All parts of the SmartXbar are instrumented using DLT logging. However due to the fact that the SmartXbar is delivered as shared library the application using
the SmartXbar is responsible to register itself at the DLT daemon using the standard DLT registration method. The smartx_interactive_example e.g. does this by
including the following lines:

~~~~~~~~~~{.cpp}
DLT_REGISTER_APP("XBAR", "SmartXbar example");
DLT_VERBOSE_MODE();
~~~~~~~~~~

@subsection initial_log_levels Setting the initial log level

Setting the initial log level is directly supported by DLT. To use it simply set the environment variable
DLT\_INITIAL\_LOG\_LEVEL. The syntax is as follows:

    DLT_INITIAL_LOG_LEVEL="APP-ID:CONTEXT-ID:LOG-LEVEL"

E.g.

    DLT_INITIAL_LOG_LEVEL="XBAR:SMX:4"

This sets the initial log level for the application with application id **XBAR** and context id **SMX** to log level info. You can also omit the application and/or the context id, which matches all application and/or context ids. If you want to set several dedicated context ids to a specific log level you have to separate the entries with a semicolon like this:

    DLT_INITIAL_LOG_LEVEL="XBAR:SMX:4;XBAR:SXC:5"


#################################################################################
@section smartx_plugin_analysis DLT Logging of the alsa-smartx-plugin

The alsa-smartx-plugin is also instrumented via DLT logging. If the application playing sound via the alsa-smartx-plugin is DLT-aware and already registered
itself at the DLT daemon, then the DLT contexts of the alsa-smartx-plugin will be available like the applications own contexts. However there might be applications
that are not DLT-aware like e.g. the standard aplay utility. To enable DLT logging for such applications you have to use two environment variables and
start the application like this:

    DBG_ID=APP1 DBG_LVL=4 aplay -Dsmartx:stereo0 YOUR_WAVE_FILE.wav

The **DBG_ID** is used to set the DLT Application ID for registration at the DLT daemon. It has to be a 4 character string.
The **DBG_LVL** is used to set the initial log level of the alsa-smartx-plugin. The value matches with the value defined in the DLT header
file, e.g. DLT_LOG_INFO has the value 4. In the above example aplay is started
and will show up in the DLT viewer under the Application ID **APP1** with its DLT contexts log level initially set to **DLT_LOG_INFO**.

#################################################################################
@section dlt_log_messages DLT Logging Message Description

The application ID for the SmartXbar is *XBAR*. In the following tables you will find a description of all possible DLT logs
and a more detailed description if the message itself is not sufficient for debugging purposes.

################################################
@subsection log_error DLT Logging Messages for Log Level DLT_LOG_ERROR

@htmlinclude dlt_error.html


################################################
@subsection log_warn DLT Logging Messages for Log Level DLT_LOG_WARN

@htmlinclude dlt_warning.html


#################################################################################
@section thread_names Thread Naming

The Linux kernel has a limitation for the thread names of 15 characters. For being able to distinguish the different threads being created by the SmartXbar a naming convention was introduced. All threads being scheduled by the standard scheduler are named
**xbar\_std\_xxx**. All threads being scheduled by the real-time scheduler are named **xbar\_rt\_xxx**. The *xxx* in the name is a consecutive number of the threads starting with 000.
More details about a specific thread can be found in the file <b>/tmp/smartx_threads.txt</b> on the target.

@note All thread names created by the SmartXbar are listed in this file, regardless if the thread currently exists or if it already has been shutdown. To get a list of all active threads you can use the standard ps command on the target with the following options:

    ps Haxo 'comm class rtprio pri cmd' | grep xbar


#################################################################################
@section no_audio_analysis Analyzing 'no audio' issues

This chapter describes how to analyze 'no audio' issues using the DLT log.

################################################
@subsection naa_prerequisites Prerequisites

The initial log level of some DLT contexts have to be changed in order to get a complete log of all important subcomponents of the SmartXbar. Changing the initial log level for DLT is done as specified in the @ref general_analysis chapter. The environment variable specifying the initial DLT log level has to be set in the correct execution environment. E.g., if the application is started in a shell, then you can either export the environment variable and then start the application:

    $ export DLT_INITIAL_LOG_LEVEL=":SMX:4;:SMJ:4;:SXC:4;:SXP:4;:RZN:4;:BFT:4;:SMW:4;:AHD:4;:CFG:4;:ROU:4;:RBM:4;:EVT:4;:MDL:4;:PFW:4"
    $ ./smartxApplication

or you can specify it on the same command line like that:

    $ DLT_INITIAL_LOG_LEVEL=":SMX:4;:SMJ:4;:SXC:4;:SXP:4;:RZN:4;:BFT:4;:SMW:4;:AHD:4;:CFG:4;:ROU:4;:RBM:4;:EVT:4;:MDL:4;:PFW:4" ./smartxApplication

If the application is started using systemd, then you have to add the following line to the corresponding service file:

    Environment=“DLT_INITIAL_LOG_LEVEL=:SMX:4;:SMJ:4;:SXC:4;:SXP:4;:RZN:4;:BFT:4;:SMW:4;:AHD:4;:CFG:4;:ROU:4;:RBM:4;:EVT:4;:MDL:4;:PFW:4“

@note The setting of the quotes is critical and different here than specifying the environment variable in the shell.

################################################
@subsection naa_connect Verify that the connection is established

In the trace you can find the connection commands for the audio path of the SmartXbar to check that the commands are received. Search for **IasRoutingImpl** and you will find e.g.:

    IasRoutingImpl::connect(54): Connect source Id 260 to sink Id 394
    IasRoutingImpl::disconnectSafe(197): Disconnect source Id 260 from sink Id 394

################################################
@subsection naa_rzn_activated Verify that the routing zone is activated

Search for **Change state** and you will find e.g.:

    IasRoutingZoneWorkerThread::changeState(1129): zone=MyRoutingZone: Change state from active pending to active

In this case the routing zone is *active* and can route/process data. If the state is *active_pending* or *inactive*, the routing zone does not route/process data.

################################################
@subsection naa_copy_job Verify that the switch matrix job is unlocked

Check if the copy job of the switch matrix is locked:

Search for **Job between** and you will find e.g.:

    IasSwitchMatrixJob::execute(174): Job between Source_port and Sink_port is locked, unlock to execute

This message might only be repeated for a short period of time until all buffers are correctly filled and the path is fully ready to route audio data. However, if this message is repeated over and over again, then no copying of that specific job between the source and sink is done and no audio in this path will be audible.

################################################
@subsection naa_device_provided_started Verify that the ALSA devices of clock type eIasClockTypeProvided are started

Search for **Received eIas** and you will find e.g.:

    IasSmartXClient::ipcThread(170): device=MyDevice: Received eIasAudioIpcStart control
    IasSmartXClient::ipcThread(177): device=MyDevice: Received eIasAudioIpcStop control

Only if the *eIasAudioIpcStart* control is received, the ALSA device is able to transmit or receive data.

################################################
@subsection naa_source_device_received_started Verify that the ALSA source devices of clock type eIasClockTypeReceived(Async) are started

Search for **startAudioSource|stopAudioSource** and you will find e.g.:

    IasSetupImpl::startAudioSourceDevice(650): Starting audio source device (ALSA handler) MySourceDevice
    IasSetupImpl::startAudioSourceDevice(664): Successfully started audio source device MySourceDevice

If you see the following messages, then the device is stopped and audio won't be audible for that source device:

    IasSetupImpl::stopAudioSourceDevice(680): Stopping audio source device (ALSA handler) MySourceDevice
    IasSetupImpl::stopAudioSourceDevice(689): Successfully stopped audio source device MySourceDevice
