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

#ifndef NRF54L_05_10_15_AIN_H__
#define NRF54L_05_10_15_AIN_H__

/*
 * External analog inputs of nRF54L05, nRF54L10 and nRF54L15.
 * AIN(idx, port, pin, saadc, comp, lpcomp) / NA(idx), see nrfx_ain_map.h.
 */

#define NRFX_AIN_TABLE(AIN, NA) \
    AIN(0, 1, 4, 1, 1, 1)       \
    AIN(1, 1, 5, 1, 1, 1)       \
    AIN(2, 1, 6, 1, 1, 1)       \
    AIN(3, 1, 7, 1, 1, 1)       \
    AIN(4, 1, 11, 1, 1, 1)      \
    AIN(5, 1, 12, 1, 1, 1)      \
    AIN(6, 1, 13, 1, 1, 1)      \
    AIN(7, 1, 14, 1, 1, 1)

#endif /* NRF54L_05_10_15_AIN_H__ */
