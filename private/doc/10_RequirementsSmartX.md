<!--
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
-->
# Requirement Analysis

## General requirements

<ol>
 <li>SmartX needs his own audio clock scheduling engine</li>
 <li>rtprocessingfw shall be reused and adjusted or extended if necessary</li>
 <li>Audio processing modules shall be reused and adjusted or extended if necessary</li>
</ol>

## Configuration options

<ol>
 <li>The period size of a procesing chain shall be configurable</li>
 <li>Support of multiple processing chains with seperate sampling rate shall be possible</li>
 <li>Processing of different data formats ( fS32_LE, fS16_LE, SND_PCM_FORMAT_FLOAT_LE )</li>
 <li>Booting of audio to several stages shall be provided (Early routing only, processing capable, cmd/ctrl capable,...)</li>
 <li>Startup shall be possible without available cmd/ctrl SW stack running</li>
 <li>The audio processing chains shall be capable of being run asynchronously to each other</li>
 <li>Crossbar functionality has to be available in the audio daemon.</li>
 <li>Output of chains shall be usable as inputs of other chains </li>
 <li>Real time scheduling parameters like scheduling policy and scheduling priority shall be configurable</li>
</ol>

## Scheduling

<ol>
 <li>Audio routing or processing threads shall be run with real-time scheduler</li>
</ol>

## Performance requirements

<ol>
 <li>Synchronous or asynchronous sample rate conversion shall only be done on demand</li>
 <li>Processing modules shall be executed only on demand</li>
</ol>

## Runtime changes

<ol>
 <li>Audio chains have to be addable/removable while runtime</li>
 <li>Processing components have to be addable/removable while runtime</li>
 <li>Additional sources should be addable while runtime and should be added to the processing graph</li>
 <li>Adding SRC automatically on demand due to connected chain sample rate</li>
</ol>

## Connection handling

<ol>
 <li>All sources shall be connectable to all chains even if the chains have different sample rates and period sizes.</li>
</ol>

## Latency

<ol>
 <li>All audio processing and routing paths shall be able to run on different period sizes</li>
 <li>Support of low latency paths (direct connect from input to output) without processing</li>
 <li>All signal routing and processing paths shall have a constant latency</li>
 <li>Measurement capabilities for signal latency shall be available</li>
 <li>Interface method shall be provided to retrieve the latency of a stream.</li>
 <li>Latency shall be adjustable to compensate different latencies of different paths, e.g. speakers vs. wireless headphones</li>
</ol>

## Debugging

<ol>
 <li>Extended audio recording(between modules, ALSA Handlers) shall be provided</li>
 <li>Audio stream injection shall be possible</li>
 <li>Measurement capabilities to detect timing/scheduling issues shall be provided</li>
 <li>Support profiling for CPU/Mem shall be included and results shall be available via interface methods</li>
 <li>A tool shall be provided that records profiling data and captures in a csv file for long running analysis</li> 
 <li>Generic Debug interface shall be provided. Free definitions of queries shall be possible</li>
</ol>

## Error handling

<ol>
 <li>Watchdog and emergency mute mechanism shall be available for audio streaming threads</li>
</ol>

## Command and control

<ol>
 <li>Session ID for cmds</li>
</ol>

## Meta-data buffer

<ol>
 <li>Support of user-defined typed meta-data support</li>
 <li>Meta-data buffers shall be constructable in heap memory</li>
 <li>Meta-data buffers shall be constructable in shared memory</li>
 <li>Meta-data buffers shall be constructable without the dependency to the user-data struct so that the framework implementation can remain closed source</li>
</ol>

## ALSA Handler

<ol>
 <li>Shall support either playback or capture or playback and capture direction</li>
 <li>Shall be able to open ALSA PCM device by either by name or by card ID and device ID</li>
</ol>

## External requirements

<ol>
 <li>No dependency on any audio streaming server is needed. The audio daemon can act as an audio server</li>
 <li>ALSA interface are provided to stream audio data to the server</li>
 <li>Startup must be synchronized with any process that audio server depends on (e.g. ASoC driver )</li>
 <li>Meta data support in ALSA audio_daemon plugin</li>
</ol>

