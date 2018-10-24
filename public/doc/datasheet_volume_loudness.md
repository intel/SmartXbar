Volume/Loudness
================
@page md_datasheet_volume_loudness

The volume/loudness module implements the volume control, which can be adjusted by the user.
Furthermore, the module implements loudness equalization filters that compensate for the fact
that the human hearing system is less sensitive for low frequencies (below 200 Hz) and for
high frequencies (above 6000 Hz). For this reason, the loudness equalization filters implement
volume dependent amplifications for low frequencies and for high frequencies.

The relationship of how the loudness amplification depends on the user-controlled volume can be
configured by means of a table. In general, the loudness function is configured such that small volumes
result in high loudness amplifications, while high volumes result in flat frequency responses of the loudness filters.

The volume/loudness module also provides a speed-controlled volume functionality. The SmartXbar
supports speed control volume on the base of a volume curve. This means that the output volume
is increased with a value assigned to the current speed. This assignment is done in a volume curve
that can be modified during configuration. The user volume setting is not affected and remains
the same, independent of the speed and the offset. The control interface provides a method for
switching speed controlled volume handling on or off. This allows implementing a customer-specific
volume handling outside of the SmartXbar (for example, as part of a policy controller).

All gain factors that are involved in the volume/loudness function, are internally updated smoothly
to guarantee the absence of any click or plop artifacts that may result from a sudden change of the gain.

###############################
@section ds_vl_general_info General Information

<table class="doxtable">
<tr><th align=left> Type name <td> "ias.volume"
<tr><th align=left> Pin configuration <td> IasAudio::IasISetup::addAudioInOutPin
<tr><th align=left> Default module state <td> on
<tr><th align=left> Default loudness state <td> on
<tr><th align=left> Default mute state <td> off
<tr><th align=left> Default speed controlled volume state <td> off
</table>

The pin configuration for this module is done via the method IasAudio::IasISetup::addAudioInOutPin, which means
this module does in-place processing. The module applies volume/loudness to every pin added. There is no limit in
the number of pins being added to an instance of the volume/loudness module. In general it is most efficient to
add all pins being processed to one single instance of the processing module.

###############################
@section ds_vl_configuration Configuration Properties

The following table describes all available properties that can be set via IasAudio::IasISetup::setProperties:

<table class="doxtable">
<tr><th> Struct                           <th> Key              <th> Value type               <th> Value range <th> Mandatory <th> Description
<tr><td> -                                <td> "numFilterBands" <td> int32_t               <td> 0 to 3      <td> yes <td> The number of filter bands that shall be used for the loudness filters.
<tr><td rowspan=2> Loudness table         <td> "ld.gains.x"     <td> IasAudio::IasInt32Vector <td> 0 to 240    <td> no <td> The gains for the filter in 1/10th dB.<br>
    The "x" in the key string is used as a placeholder for the filter band index. For filter band 0 the key has to be "ld.gains.0".<br>
    If "numFilterBands" is 2, then the possible filter band indices are 0 and 1. If "numFilterBands" is 3, then the possible<br>
    filter band indices are 0, 1 and 2.<br>
    The size of the gains vector has to match the size of the volumes vector.
<tr>                                      <td> "ld.volumes.x"   <td> IasAudio::IasInt32Vector <td> 0 to -1440  <td> no <td> The volumes for the filter in 1/10th dB.<br>
    The "x" in the key string is used as a placeholder for the filter band index. For filter band 0 the key has to be "ld.volumes.0".<br>
    If "numFilterBands" is 2, then the possible filter band indices are 0 and 1. If "numFilterBands" is 3, then the possible<br>
    filter band indices are 0, 1 and 2.<br>
    The size of the volumes vector has to match the size of the gains vector.
<tr><td rowspan=7> Loudness filter params <td rowspan=4> "ld.freq_order_type.x" <td> IasAudio::IasInt32Vector <td> size == 3 <td> no <td> Tuple of filter frequency, order and type.<br>
    The "x" in the key string is used as a placeholder for the filter band index. For filter band 0 the key has to be "ld.freq_order_type.0".<br>
    If "numFilterBands" is 2, then the possible filter band indices are 0 and 1. If "numFilterBands" is 3, then the possible<br>
    filter band indices are 0, 1 and 2.<br>
<tr>                                      <td> [0] int32_t <td> 10 to 20000              <td> no <td> The filter frequency in Hz.
<tr>                                      <td> [1] int32_t <td> 1, 2                     <td> no <td> The filter order.
<tr>                                      <td> [2]&nbsp;IasAudio::IasAudioFilterTypes <td> IasAudio::eIasFilterTypePeak, IasAudio::eIasFilterTypeLowShelving, IasAudio::eIasFilterTypeHighShelving <td> no <td> The filter type.
<tr>                                      <td rowspan=3> "ld.gain_quality.x" <td> IasAudio::IasFloat32Vector <td> size == 2 <td> no <td> Tuple of filter gain and quality.<br>
    The "x" in the key string is used as a placeholder for the filter band index. For filter band 0 the key has to be "ld.gain_quality.0".<br>
    If "numFilterBands" is 2, then the possible filter band indices are 0 and 1. If "numFilterBands" is 3, then the possible<br>
    filter band indices are 0, 1 and 2.<br>
<tr>                                      <td> [0] float <td> 1.0                      <td> no <td> The filter gain. Fixed to 1.0.
<tr>                                      <td> [1] float <td> 0.01 to 100.0            <td> no <td> The filter quality.
<tr><td> -                                <td> "activePinsForBand.x" <td> IasAudio::IasStringVector <td> - <td> no <td> A vector with all pin names that shall be active for
    the specified band.<br>
    The "x" in the key string is used as a placeholder for the filter band index. For filter band 0 the key has to be "activePinsForBand.0".<br>
    If "numFilterBands" is 2, then the possible filter band indices are 0 and 1. If "numFilterBands" is 3, then the possible<br>
    filter band indices are 0, 1 and 2.<br>
<tr><td rowspan=3> Speed dependent volume table <td> "sdv.speed"     <td> IasAudio::IasInt32Vector <td> Meaningful speed values in increasing order <td> no <td> The speed value in km/h.
    For every speed value in this vector, there has to be a minimum gain in the "sdv.gain_inc" vector and a maximum gain in the "sdv.gain_dec" vector.
    This tuple of three values allows the module to interpolate two gain curves. The increase curve defines the minimal gain which is used for a specific speed.
    The decrease curve defines the maximal gain which is used for a specific speed. This allows to define a hysteresis for the gain changes.
    The actual gain can be on every value between increase and decrease (depending on the history of the speed changes). The value for decrease  must be equal
    or greater than the value for increase. If the values are equal for every node no hysteresis will be used. The vectors can have a maximum of 40 entries.
<tr>                                            <td> "sdv.gain_inc"  <td> IasAudio::IasInt32Vector <td> 0 to 200 <td> no <td> The minimum gain in 1/10th dB for the speed defined by "sdv.speed"
<tr>                                            <td> "sdv.gain_dec"  <td> IasAudio::IasInt32Vector <td> 0 to 200 <td> no <td> The maximum gain in 1/10th dB for the speed defined by "sdv.speed". Has to be greater
    or equal than the value for "sdv.gain_inc".
</table>

###############################
@section ds_vl_control Runtime Processing Control Properties

The following table describes all commands that can be received by an instance of the volume/loudness
module via the method IasAudio::IasIProcessing::sendCmd. They are typically used during runtime to control
the processing:

<table class="doxtable">
<tr><th> Command <th> Key <th> Value type <th> Value range <th> Description
<tr><td rowspan=2> Set module state <td> "cmd"            <td> int32_t  <td> IasAudio::IasVolume::eIasSetModuleState <td> Set the module state, i.e. turn the module on or off.
<tr>                                <td> "moduleState"    <td> std::string <td> "on", "off"                             <td> "on" to turn the module on, or "off" to turn it off.
<tr><td rowspan=6> Set volume       <td> "cmd"            <td> int32_t  <td> IasAudio::IasVolume::eIasSetVolume      <td> Set a new volume level.
<tr>                                <td> "pin"            <td> std::string <td> Valid pin name                          <td> A valid input or combined input/output pin name for which the volume shall be changed.
<tr>                                <td> "volume"         <td> int32_t  <td> <=0                                     <td> The new volume in 1/10th dB. Values <= -1440 will be treated as mute.
<tr>                                <td rowspan=3> "ramp" <td> IasAudio::IasInt32Vector <td> size == 2                  <td> The ramp parameters for the fade ramp.
<tr>                                                      <td> [0] int32_t <td> 1 to 10000                           <td> The ramp time in ms.
<tr>                                                      <td> [1]&nbsp;IasAudio::IasRampShapes <td> IasAudio::IasRampShapes <td> The ramp shape. (Logarithmic shape not supported, this will return error.)
<tr><td rowspan=6> Set mute state   <td> "cmd"            <td> int32_t  <td> IasAudio::IasVolume::eIasSetMuteState   <td> Set the mute state for a specific pin.
<tr>                                <td> "pin"            <td> std::string <td> Valid pin name                          <td> A valid input or combined input/output pin name for which the mute state shall be changed.
<tr>                                <td rowspan=4> "params" <td> IasAudio::IasInt32Vector <td> size == 3                <td> The params for setting the mute state.
<tr>                                                      <td> [0] bool <td> true, false                           <td> pin muted => true or unmuted => false.
<tr>                                                      <td> [1] uint32_t <td> 1 to 10000                          <td> The ramp time in ms.
<tr>                                                      <td> [2]&nbsp;IasAudio::IasRampShapes <td> IasAudio::IasRampShapes <td> The ramp shape. (Logarithmic shape not supported, this will return error.)
<tr><td rowspan=3> Set loudness state <td> "cmd"          <td> int32_t  <td> IasAudio::IasVolume::eIasSetLoudness    <td> Set the loudness state for a specific pin.
<tr>                                <td> "pin"            <td> std::string <td> Valid pin name                          <td> A valid input or combined input/output pin name for which the loudness state shall be changed.
<tr>                                <td> "loudness"       <td> std::string <td> "on", "off"                             <td> "on" to turn loudness on for a specific pin, or "off" to turn it off.
<tr><td rowspan=3> Set speed controlled volume state <td> "cmd"            <td> int32_t  <td> IasAudio::IasVolume::eIasSetSpeedControlledVolume <td> Set the speed controlled volume state for a specific pin.
<tr>                                <td> "pin"            <td> std::string <td> Valid pin name                          <td> A valid input or combined input/output pin name for which the speed controlled volume state shall be changed.
<tr>                                <td> "sdv"             <td> std::string <td> "on", "off"                             <td> "on" to turn speed controlled volume on for a specific pin, or "off" to turn it off.
<tr><td rowspan=2> Set speed  <td> "cmd"      <td> int32_t <td> IasAudio::IasVolume::eIasSetSpeed <td> Set the current speed.
<tr>                          <td> "speed"    <td> int32_t <td> -                                 <td> The speed in km/h.
</table>

###############################
@section ds_vl_return Runtime Processing Return Properties

The following table describes all return properties that can be received from an instance of the volume/loudness
module when the IasAudio::IasIProcessing::sendCmd method is called.

<table class="doxtable">
<tr><th> Response for command <th> Key           <th> Value type  <th> Value range <th> Description
<tr><td> Set module state     <td> "moduleState" <td> std::string <td> "on", "off" <td> Property that is returned by the "set module state" command. Provides information whether the module is on or off.
</table>

###############################
@section ds_vl_tuning Tuning Properties

The following table describes all commands that can be received by an instance of the volume/loudness
module via the method IasAudio::IasIProcessing::sendCmd. They are typically used during start-up to set tuning parameters.
There are also commands to retrieve the currently set tuning parameters. For the getter methods the returned values can be
found in the returnProperties parameter of the IasAudio::IasIProcessing::sendCmd method:

<table class="doxtable">
<tr><th> Command <th> Key <th> Value type <th> Value range <th> Description
<tr><td rowspan=3> Set loudness table     <td> "cmd"            <td> int32_t <td> IasAudio::IasVolume::eIasSetLoudnessTable <td> Set the loudness table
<tr>                                      <td> "ld.gains.x"     <td> IasAudio::IasInt32Vector <td> 0 to 240    <td> The gains for the filter in 1/10th dB.<br>
    The "x" in the key string is used as a placeholder for the filter band index. For filter band 0 the key has to be "ld.gains.0".<br>
    If "numFilterBands" is 2, then the possible filter band indices are 0 and 1. If "numFilterBands" is 3, then the possible<br>
    filter band indices are 0, 1 and 2.<br>
    The size of the gains vector has to match the size of the volumes vector.
<tr>                                      <td> "ld.volumes.x"   <td> IasAudio::IasInt32Vector <td> 0 to -1440  <td> The volumes for the filter in 1/10th dB.<br>
    The "x" in the key string is used as a placeholder for the filter band index. For filter band 0 the key has to be "ld.volumes.0".<br>
    If "numFilterBands" is 2, then the possible filter band indices are 0 and 1. If "numFilterBands" is 3, then the possible<br>
    filter band indices are 0, 1 and 2.<br>
    The size of the volumes vector has to match the size of the gains vector.
<tr><td rowspan=3> Get loudness table     <td> "cmd"            <td> int32_t <td> IasAudio::IasVolume::eIasGetLoudnessTable <td> Get the loudness table
<tr>                                      <td> "ld.gains.x"     <td> IasAudio::IasInt32Vector <td> 0 to 240    <td> The gains for the filter in 1/10th dB.<br>
    The "x" in the key string is used as a placeholder for the filter band index. For filter band 0 the key has to be "ld.gains.0".<br>
    If "numFilterBands" is 2, then the possible filter band indices are 0 and 1. If "numFilterBands" is 3, then the possible<br>
    filter band indices are 0, 1 and 2.<br>
    The size of the gains vector has to match the size of the volumes vector.
<tr>                                      <td> "ld.volumes.x"   <td> IasAudio::IasInt32Vector <td> 0 to -1440  <td> The volumes for the filter in 1/10th dB.<br>
    The "x" in the key string is used as a placeholder for the filter band index. For filter band 0 the key has to be "ld.volumes.0".<br>
    If "numFilterBands" is 2, then the possible filter band indices are 0 and 1. If "numFilterBands" is 3, then the possible<br>
    filter band indices are 0, 1 and 2.<br>
    The size of the volumes vector has to match the size of the gains vector.
<tr><td rowspan=8> Set loudness filter params <td> "cmd"      <td> int32_t   <td> IasAudio::IasVolume::eIasSetLoudnessFilter <td> Set the loudness filter params
<tr>                                      <td rowspan=4> "ld.freq_order_type.x" <td> IasAudio::IasInt32Vector <td> size == 3 <td> Tuple of filter frequency, order and type.<br>
    The "x" in the key string is used as a placeholder for the filter band index. For filter band 0 the key has to be "ld.freq_order_type.0".<br>
    If "numFilterBands" is 2, then the possible filter band indices are 0 and 1. If "numFilterBands" is 3, then the possible<br>
    filter band indices are 0, 1 and 2.<br>
<tr>                                      <td> [0] int32_t <td> 10 to 20000              <td> The filter frequency in Hz.
<tr>                                      <td> [1] int32_t <td> 1, 2                     <td> The filter order.
<tr>                                      <td> [2]&nbsp;IasAudio::IasAudioFilterTypes <td> IasAudio::eIasFilterTypePeak, IasAudio::eIasFilterTypeLowShelving, IasAudio::eIasFilterTypeHighShelving <td> The filter type.
<tr>                                      <td rowspan=3> "ld.gain_quality.x" <td> IasAudio::IasFloat32Vector <td> size == 2 <td> Tuple of filter gain and quality.<br>
    The "x" in the key string is used as a placeholder for the filter band index. For filter band 0 the key has to be "ld.gain_quality.0".<br>
    If "numFilterBands" is 2, then the possible filter band indices are 0 and 1. If "numFilterBands" is 3, then the possible<br>
    filter band indices are 0, 1 and 2.<br>
<tr>                                      <td> [1] float <td> 1.0                      <td> The filter gain. Fixed to 1.0.
<tr>                                      <td> [0] float <td> 0.01 to 100.0            <td> The filter quality.
<tr><td rowspan=8> Get loudness filter params <td> "cmd"      <td> int32_t   <td> IasAudio::IasVolume::eIasGetLoudnessFilter <td> Get the loudness filter params
<tr>                                      <td rowspan=4> "ld.freq_order_type.x" <td> IasAudio::IasInt32Vector <td> size == 3 <td> Tuple of filter frequency, order and type.<br>
    The "x" in the key string is used as a placeholder for the filter band index. For filter band 0 the key has to be "ld.freq_order_type.0".<br>
    If "numFilterBands" is 2, then the possible filter band indices are 0 and 1. If "numFilterBands" is 3, then the possible<br>
    filter band indices are 0, 1 and 2.<br>
<tr>                                      <td> [0] int32_t <td> 10 to 20000              <td> The filter frequency in Hz.
<tr>                                      <td> [1] int32_t <td> 1, 2                     <td> The filter order.
<tr>                                      <td> [2]&nbsp;IasAudio::IasAudioFilterTypes <td> IasAudio::IasAudioFilterTypes <td> The filter type.
<tr>                                      <td rowspan=3> "ld.gain_quality.x" <td> IasAudio::IasFloat32Vector <td> size == 2 <td> Tuple of filter gain and quality.<br>
    The "x" in the key string is used as a placeholder for the filter band index. For filter band 0 the key has to be "ld.gain_quality.0".<br>
    If "numFilterBands" is 2, then the possible filter band indices are 0 and 1. If "numFilterBands" is 3, then the possible<br>
    filter band indices are 0, 1 and 2.<br>
<tr>                                      <td> [1] float <td> 1.0                      <td> The filter gain. Fixed to 1.0.
<tr>                                      <td> [0] float <td> 0.01 to 100.0            <td> The filter quality.
<tr><td rowspan=4> Set speed dependent volume table <td> "cmd"       <td> int32_t               <td> IasAudio::IasVolume::eIasSetSdvTable <td> Set the speed dependent volume table
<tr>                                            <td> "sdv.speed"     <td> IasAudio::IasInt32Vector <td> Meaningful speed values in increasing order <td> The speed value in km/h.
    For every speed value in this vector, there has to be a minimum gain in the "sdv.gain_inc" vector and a maximum gain in the "sdv.gain_dec" vector.
    This tuple of three values allows the module to interpolate two gain curves. The increase curve defines the minimal gain which is used for a specific speed.
    The decrease curve defines the maximal gain which is used for a specific speed. This allows to define a hysteresis for the gain changes.
    The actual gain can be on every value between increase and decrease (depending on the history of the speed changes). The value for decrease  must be equal
    or greater than the value for increase. If the values are equal for every node no hysteresis will be used. The vectors can have a maximum of 40 entries.
<tr>                                            <td> "sdv.gain_inc"  <td> IasAudio::IasInt32Vector <td> 0 to 200 <td> The minimum gain in 1/10th dB for the speed defined by "sdv.speed"
<tr>                                            <td> "sdv.gain_dec"  <td> IasAudio::IasInt32Vector <td> 0 to 200 <td> The maximum gain in 1/10th dB for the speed defined by "sdv.speed". Has to be greater
    or equal than the value for "sdv.gain_inc".
<tr><td rowspan=4> Get speed dependent volume table <td> "cmd"       <td> int32_t          <td> IasAudio::IasVolume::eIasGetSdvTable <td> Get the speed dependent volume table
<tr>                                            <td> "sdv.speed"     <td> IasAudio::IasInt32Vector <td> Meaningful speed values in increasing order <td> The speed value in km/h.
    For every speed value in this vector, there has to be a minimum gain in the "sdv.gain_inc" vector and a maximum gain in the "sdv.gain_dec" vector.
    This tuple of three values allows the module to interpolate two gain curves. The increase curve defines the minimal gain which is used for a specific speed.
    The decrease curve defines the maximal gain which is used for a specific speed. This allows to define a hysteresis for the gain changes.
    The actual gain can be on every value between increase and decrease (depending on the history of the speed changes). The value for decrease  must be equal
    or greater than the value for increase. If the values are equal for every node no hysteresis will be used. The vectors can have a maximum of 40 entries.
<tr>                                            <td> "sdv.gain_inc"  <td> IasAudio::IasInt32Vector <td> 0 to 200 <td> The minimum gain in 1/10th dB for the speed defined by "sdv.speed"
<tr>                                            <td> "sdv.gain_dec"  <td> IasAudio::IasInt32Vector <td> 0 to 200 <td> The maximum gain in 1/10th dB for the speed defined by "sdv.speed". Has to be greater
    or equal than the value for "sdv.gain_inc".
</table>


###############################
@section ds_vl_events Event Types

The following table describes all event types that can be generated by the volume/loudness module.


<table class="doxtable">
<tr><th> Event type <th> Key <th> Value type <th> Value range <th> Description
<tr><td rowspan=5> Volume Fading Finished           <td> "typeName"        <td> std::string  <td> "ias.volume"                                 <td> Module type name
<tr>                                                <td> "instanceName"    <td> std::string  <td> Valid instance name                          <td> Instance name
<tr>                                                <td> "eventType"       <td> int32_t   <td> IasVolume::eIasVolumeFadingFinished          <td> Indicates that volume fading has been finished
<tr>                                                <td> "pin"             <td> std::string  <td> Valid pin name                               <td> Name of the pin whose volume fading has been finished
<tr>                                                <td> "volume"          <td> int32_t <td> 0 to -1440                                     <td> The new volume in representation of [dB/10]
<tr><td rowspan=5> Loudness Switch Finished         <td> "typeName"        <td> std::string  <td> "ias.volume"                                 <td> Module type name
<tr>                                                <td> "instanceName"    <td> std::string  <td> Valid instance name                          <td> Instance name
<tr>                                                <td> "eventType"       <td> int32_t   <td> IasVolume::eIasLoudnessSwitchFinished        <td> Indicates that loudness has been switched
<tr>                                                <td> "pin"             <td> std::string  <td> Valid pin name                               <td> Name of the pin whose loudness state has been switched on or off
<tr>                                                <td> "loudnessState"   <td> int32_t   <td> 0 (loudness off) or 1 (loudness on)          <td> The new loudness state
<tr><td rowspan=5> Speed Controlled Volume Finished <td> "typeName"        <td> std::string  <td> "ias.volume"                                 <td> Module type name
<tr>                                                <td> "instanceName"    <td> std::string  <td> Valid instance name                          <td> Instance name
<tr>                                                <td> "eventType"       <td> int32_t   <td> IasVolume::eIasSpeedControlledVolumeFinished <td> Indicates that speed controlled volume has been switched on or off
<tr>                                                <td> "pin"             <td> std::string  <td> Valid pin name                               <td> Name of the pin whose speed controlled volume has been switched
<tr>                                                <td> "sdvState"        <td> int32_t   <td> 0 (speed dependent volume off) or 1 (on)     <td> The new state of the speed dependent volume
<tr><td rowspan=5> Set Mute State Finished          <td> "typeName"        <td> std::string  <td> "ias.volume"                                 <td> Module type name
<tr>                                                <td> "instanceName"    <td> std::string  <td> Valid instance name                          <td> Instance name
<tr>                                                <td> "eventType"       <td> int32_t   <td> IasVolume::eIasSetMuteStateFinished          <td> Indicates that mute state has been updated
<tr>                                                <td> "pin"             <td> std::string  <td> Valid pin name                               <td> Name of the pin whose mute state has been updated
<tr>                                                <td> "muteState"       <td> int32_t <td> 0 (not muted) or 1 (muted)                     <td> The new mute state
</table>
