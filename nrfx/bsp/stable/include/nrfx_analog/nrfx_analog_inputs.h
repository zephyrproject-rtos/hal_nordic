/*
 * Copyright (c) 2026, Nordic Semiconductor ASA
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

#ifndef NRFX_ANALOG_INPUTS_H__
#define NRFX_ANALOG_INPUTS_H__

/*
 * Values of the generic analog inputs (nrfx_analog_input_t).
 *
 * This file must contain preprocessor definitions only, so that it can also be
 * included from devicetree sources and assembly files.
 */

/** @brief Symbol specifying test inputs offset. */
#define NRFX_ANALOG_AIN_TEST_OFFSET     64

/** @brief Symbol specifying internal inputs offset. */
#define NRFX_ANALOG_AIN_INTERNAL_OFFSET 128

#define NRFX_ANALOG_EXTERNAL_AIN0_VAL   0
#define NRFX_ANALOG_EXTERNAL_AIN1_VAL   1
#define NRFX_ANALOG_EXTERNAL_AIN2_VAL   2
#define NRFX_ANALOG_EXTERNAL_AIN3_VAL   3
#define NRFX_ANALOG_EXTERNAL_AIN4_VAL   4
#define NRFX_ANALOG_EXTERNAL_AIN5_VAL   5
#define NRFX_ANALOG_EXTERNAL_AIN6_VAL   6
#define NRFX_ANALOG_EXTERNAL_AIN7_VAL   7
#define NRFX_ANALOG_EXTERNAL_AIN8_VAL   8
#define NRFX_ANALOG_EXTERNAL_AIN9_VAL   9
#define NRFX_ANALOG_EXTERNAL_AIN10_VAL  10
#define NRFX_ANALOG_EXTERNAL_AIN11_VAL  11
#define NRFX_ANALOG_EXTERNAL_AIN12_VAL  12
#define NRFX_ANALOG_EXTERNAL_AIN13_VAL  13

#define NRFX_ANALOG_TEST_AVSS_VAL          (NRFX_ANALOG_AIN_TEST_OFFSET + 0)

#define NRFX_ANALOG_INTERNAL_VDD_VAL       (NRFX_ANALOG_AIN_INTERNAL_OFFSET + 0)
#define NRFX_ANALOG_INTERNAL_VDDDIV2_VAL   (NRFX_ANALOG_AIN_INTERNAL_OFFSET + 1)
#define NRFX_ANALOG_INTERNAL_AVDD_VAL      (NRFX_ANALOG_AIN_INTERNAL_OFFSET + 2)
#define NRFX_ANALOG_INTERNAL_DVDD_VAL      (NRFX_ANALOG_AIN_INTERNAL_OFFSET + 3)
#define NRFX_ANALOG_INTERNAL_VDDHDIV5_VAL  (NRFX_ANALOG_AIN_INTERNAL_OFFSET + 4)
#define NRFX_ANALOG_INTERNAL_VDDL_VAL      (NRFX_ANALOG_AIN_INTERNAL_OFFSET + 5)
#define NRFX_ANALOG_INTERNAL_DECB_VAL      (NRFX_ANALOG_AIN_INTERNAL_OFFSET + 6)
#define NRFX_ANALOG_INTERNAL_VSS_VAL       (NRFX_ANALOG_AIN_INTERNAL_OFFSET + 7)
#define NRFX_ANALOG_INTERNAL_VDDAO3V0_VAL  (NRFX_ANALOG_AIN_INTERNAL_OFFSET + 8)
#define NRFX_ANALOG_INTERNAL_VDDAO1V8_VAL  (NRFX_ANALOG_AIN_INTERNAL_OFFSET + 9)
#define NRFX_ANALOG_INTERNAL_VDDAO0V8_VAL  (NRFX_ANALOG_AIN_INTERNAL_OFFSET + 10)
#define NRFX_ANALOG_INTERNAL_VDDRF_VAL     (NRFX_ANALOG_AIN_INTERNAL_OFFSET + 11)
#define NRFX_ANALOG_INTERNAL_VBAT_VAL      (NRFX_ANALOG_AIN_INTERNAL_OFFSET + 12)
#define NRFX_ANALOG_INTERNAL0_VAL          (NRFX_ANALOG_AIN_INTERNAL_OFFSET + 13)
#define NRFX_ANALOG_INTERNAL1_VAL          (NRFX_ANALOG_AIN_INTERNAL_OFFSET + 14)
#define NRFX_ANALOG_INTERNAL2_VAL          (NRFX_ANALOG_AIN_INTERNAL_OFFSET + 15)
#define NRFX_ANALOG_INTERNAL3_VAL          (NRFX_ANALOG_AIN_INTERNAL_OFFSET + 16)

#define NRFX_ANALOG_INPUT_DISABLED_VAL     255

#endif /* NRFX_ANALOG_INPUTS_H__ */
