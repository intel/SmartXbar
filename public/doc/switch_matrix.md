Switch Matrix
=============
@page md_switch_matrix

The switch matrix is the part of the SmartXbar, where the sources are connected to the sinks. The sources can be from an alsa-smartx-plugin or an ALSA hw driver (please see @ref arch_overview "Architectural Overview"). The sinks will be input ports of the different routing zones.

The switch matrix performs a copy (which contains format conversion and synchronous sample rate conversion if necessary) from the source buffer to the sink buffer.

@note Each source can only be connected to one frequency domain, which means that it is not possible to connect the same source at the same time to two independent routing zones.

################################################
@section swm_format_conv Format Conversion

If the sample rate of the source and the sink is the same, the switch matrix performs a data format conversion automatically. The decision, which format conversion is needed, is taken from the port settings.
The following conversions are supported

**Supported format conversions**
Input format | Output format |
-------------|---------------|
Int32 | Int32 |
Int32 | Int16 |
Int32 | Float32 |
Int16 | Int16 |
Int16 | Int32 |
Int16 | Float32 |
Float32 | Float32 |
Float32 | Int32 |
Float32 | Int16 |

Other data formats are not supported.

################################################
@section swm_sync_src Synchronous Sample Rate Conversion Inside SmartXbar

The SmartXbar provides the functionality to do synchronous sample rate conversion, when connection is established where the source has a different sample rate than the sink.
The synchronous sample rate conversion is done inside the switch matrix, where the source data is already in the synchronous frequency domain, but may be at a different rate. When establishing a connection, the switch matrix decides depending on the information of the two audio ports if a synchronous sample rate conversion is necessary. If the two ports have different sample rates, the synchronous sample rate conversion will be activated. On top of that, there will be also a data format conversion (only if necessary, depending on the port settings).

Example:

- Source port: sample rate: 44100 Hz, data format: Float32
- Sink port  : sample rate: 48000 Hz, data format: Int16

If a connection of these two ports is done, there will be synchronous sample rate conversion + the format conversion from Float32 to Int16.

The following combinations of sample rates are allowed:

- if input rate <= output rate: all ratios for frequencies in the range of [8000 Hz .. 96000 Hz]

- if input rate > output rate: see table

**Input rate > output rate**

Input rate [Hz] | Output rates [Hz] |
----------------|-------------------|
48000 | 8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100 |
44100 | 16000 |
24000 | 8000, 16000 |


@note The sample rates of source and sink must be inside the range of [8000 Hz .. 96000 Hz]. If not, then the connection will be refused.

It is possible to connect one source to more than one routing zone, as long as all the routing zones are clocked from the same device. This implies that the routing zones have sampling rates with integer multipliers.

Every routing zone needs to have a full conversion buffer when it is triggered. The size of the conversion buffer is equal to the period size of the routing zone. Depending on the configuration, each routing zone can have its own conversion buffer size. So it must be ensured that in case of sample rate conversion the correct size of data is copied to these buffers.

A simple example should explain how the calculation of the copy sizes is done in case of sample rate conversion:
Let's assume we have the following setup:

|   | Sampling Rate [Hz] | Period Size [sample] | Period Time [ms] |
|---|----------------|-------------------|----------------|
|Base zone | 48000 | 192 | 4 |
|Derived1 | 24000 | 192 | 8 |
|Derived2 | 16000 | 192 | 12 |

And lets' assume we want to connect a 44.1 kHz source with 192 sample period size to all the zones.
We start connecting it to Derived1. The calculation takes a base factor, which is calculated by

BaseFactor = BasePeriodSize * (sourceRate/BaseZoneRate) ,

which in our example is 192 * (44100/48000) = 176.4

To get the copy size for Derived1, we calculate  BaseFactor * Derived1Rate/sourceRate, which is 176.4 * 24000/44100 = 96

Same example for Derived2: 176.4 * 16000/44100 = 64

The periodSize of Derived1 is 192 samples, but we copy only chunks of 96 samples, which is a time of 4ms. So every call of base routingzone we copy chunks of 4 ms to all other connected routing zones.

**Important: The buffer of the input devices must be chosen so that there is always enough input to serve the conversion buffers of the routing zones. For example, if the base routing zone needs 192 samples, and one wants to convert from 44.1 kHz to 48 kHz, the source device input buffer should be chosen to be at least 2*192 samples big**



