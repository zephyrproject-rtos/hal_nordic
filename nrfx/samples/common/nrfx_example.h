/*
 * Copyright (c) 2022 - 2024, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef NRFX_EXAMPLE_H__
#define NRFX_EXAMPLE_H__

#if defined(__ZEPHYR__)
#include <zephyr/logging/log_ctrl.h>
#define NRFX_EXAMPLE_LOG_INIT()    LOG_INIT()
#define NRFX_EXAMPLE_LOG_PROCESS() while (LOG_PROCESS())
#else
#define NRFX_EXAMPLE_LOG_INIT()
#define NRFX_EXAMPLE_LOG_PROCESS()
#endif

#if defined(NRF52_SERIES)
#define LOOPBACK_PIN_1A 3
#define LOOPBACK_PIN_1B 31
#define LOOPBACK_PIN_2A 4
#define LOOPBACK_PIN_2B 30
#define LOOPBACK_PIN_3A 28
#define LOOPBACK_PIN_3B 29
#define LOOPBACK_PIN_4A 26
#define LOOPBACK_PIN_4B 27
#define ANALOG_INPUT_A0 1
#define ANALOG_INPUT_A1 2
#define ANALOG_INPUT_A2 4
#if defined(NRF52832_XXAA) || defined (NRF52832_XXAB)
#define LED1_PIN        17
#define LED2_PIN        18
#define LED3_PIN        19
#define LED4_PIN        20
#else
#define LED1_PIN        13
#define LED2_PIN        14
#define LED3_PIN        15
#define LED4_PIN        16
#endif
#elif defined(NRF53_SERIES)
#define LOOPBACK_PIN_1A 4
#define LOOPBACK_PIN_1B 26
#define LOOPBACK_PIN_2A 5
#define LOOPBACK_PIN_2B 25
#define LOOPBACK_PIN_3A 6
#define LOOPBACK_PIN_3B 7
#define LOOPBACK_PIN_4A 34
#define LOOPBACK_PIN_4B 35
#define ANALOG_INPUT_A0 0
#define ANALOG_INPUT_A1 1
#define ANALOG_INPUT_A2 2
#define LED1_PIN        28
#define LED2_PIN        29
#define LED3_PIN        30
#define LED4_PIN        31
#elif defined(NRF91_SERIES)
#define LOOPBACK_PIN_1A 14
#define LOOPBACK_PIN_1B 19
#define LOOPBACK_PIN_2A 15
#define LOOPBACK_PIN_2B 18
#define LOOPBACK_PIN_3A 16
#define LOOPBACK_PIN_3B 17
#define LOOPBACK_PIN_4A 30
#define LOOPBACK_PIN_4B 31
#define ANALOG_INPUT_A0 1
#define ANALOG_INPUT_A1 2
#define ANALOG_INPUT_A2 3
#define LED1_PIN        2
#define LED2_PIN        3
#define LED3_PIN        4
#define LED4_PIN        5
#else
#error "Unknown device."
#endif

#define ANALOG_INPUT_TO_SAADC_AIN(x) ((x) + 1)

#define ANALOG_INPUT_TO_COMP_AIN(x) (x)

#endif // NRFX_EXAMPLE_H__
