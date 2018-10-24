<!--
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
-->
# Ringbuffer Requirements

## 1. Purpose
This document describes the requirements of the ringbuffer object, which will be used to store and copy audio data.

### 2. Requirements

#### 2.1 Buffer specific parameters

The ringbuffer must have the following parameters:

* **period size**: the number of samples in one period
* **channel number**: the number of channels the ringbuffer can contain
* **data format**: the format of the audio data ( see section 2.2 )
* **number of periods**: this will define the length of the ringbuffer

#### 2.2 Data Format
The ringbuffer must be able to handle data with the following formats:

* Float 32 bit
* Signed Integer 32 bit
* Signed Integer 16 bit

#### 2.3 Channel Layout

The ringbuffer will store data in its buffer always as non-interleaved.
With its interface functions ( read and write) it must fullfill the following requirements;

* The ringbuffer must be able to read data from interleaved buffers
* The ringbuffer must be able to write data to interleaved buffers
* The ringbuffer must be able to read data from non-interleaved buffers
* The ringbuffer must be able to write data to non-interleaved buffers

#### 2.4 Shared memory

* It must be possible to locate the ringbuffer in a shared memory.
* The ringbuffer must be stored in shared memory under it's name, which must be passed to the factory
* Any process must be able to find a ringbuffer by it's name in the shared memory
* A process that found a ringbuffer in shared memory must be able to have access to the ringbuffer object

#### 2.5 Write interface

* It must be possible to write data to the ringbuffer via several write interfaces
* A general write interface that simply does a memcopy from a source memory to the internal memory, copying one period of all channels
* A write to copy data from another ringbuffer, copying one period of all channels
* A write to copy data from a source buffer, defining to copy data only for some specific block of channels (e.g. two consecutive channels out of eight)

#### 2.5 Read interface

* It must be possible to read data from the ringbuffer via several write interfaces
* A general read interface that simply does a memcopy to a destination memory from the internal memory, copying one period of all channels
* A read to copy data to another ringbuffer, copying one period of all channels
* A read to copy data to a destination buffer, defining to copy data only for some specific block of channels ((e.g. two consecutive channels out of eight)


