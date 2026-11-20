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

#ifndef NRFX_AIN_MAP_H__
#define NRFX_AIN_MAP_H__

/*
 * Selects the external analog input (AIN) table of the SoC in use.
 *
 * Each SoC header defines NRFX_AIN_TABLE(AIN, NA), which expands to one entry
 * per AIN, in AIN number order:
 *  - AIN(idx, port, pin, saadc, comp, lpcomp) for an existing AIN, where the last
 *    three arguments are 1 if the AIN can be used by the given peripheral and 0 otherwise;
 *  - NA(idx) for an AIN that does not exist.
 *
 * The SoC headers contain preprocessor definitions only and can be included
 * directly from devicetree sources.
 */

#if defined(NRF52805_XXAA)
#include "nrf52805_ain.h"
#elif defined(NRF52840_XXAA)
#include "nrf52840_ain.h"
#elif defined(NRF54L05_XXAA) || defined(NRF54L10_XXAA) || defined(NRF54L15_XXAA)
#include "nrf54l_05_10_15_ain.h"
#elif defined(NRF7120_ENGA_XXAA) || defined(NRF7120E_ENGA_XXAA) || \
    defined(NRF7120_XXAA) || defined(NRF7120E_XXAA)
#include "nrf7120_ain.h"
#endif

#endif /* NRFX_AIN_MAP_H__ */
