<!--
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
-->
# Redesign of audio_daemon

Today the audio subsystem is based on the jack-audio-connection kit. That means that alle the routing and crosbar
functionality is provided by the jack framework. This makes our life easier but also restricts the customers to use the
jack framework. Most of the customers have their own framework or don't want to use jack. Therefore the audio damon
should be able to be used also in an jackless environment (e.g. ALSA).
This would increase the customer acceptance lot. It would also increase the flexibility of usage of the audio subsystem.

This document decribes the architecture of the audio subsystem to be jackless

