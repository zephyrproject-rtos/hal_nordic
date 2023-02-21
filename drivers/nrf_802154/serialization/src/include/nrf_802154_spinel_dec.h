/*
 * Copyright (c) 2020 - 2023, Nordic Semiconductor ASA
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
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

/**
 * @defgroup nrf_802154_spinel_serialization_decoder
 * 802.15.4 radio driver spinel serialization decoder
 * @{
 *
 */

#ifndef NRF_802154_SPINEL_DEC_H_
#define NRF_802154_SPINEL_DEC_H_

#include <stddef.h>

#include "nrf_802154_serialization_error.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Decode and dispatch spinel command.
 *
 * @param[in]  p_packet_data    Pointer to a buffer that contains spinel packet to be decoded.
 * @param[in]  packet_data_len  Size of the @ref p_packet_data buffer.
 *
 * @returns zero on success or negative error value on failure.
 *
 */
nrf_802154_ser_err_t nrf_802154_spinel_decode_cmd(const void * p_packet_data,
                                                  size_t       packet_data_len);

/**
 * @brief Dispatches spinel command.
 *
 * @note Implementation of this function differs for an application core and
 * for network core.
 *
 * @param[in]  cmd       Spinel command received
 * @param[in]  p_data    Pointer to a buffer that contains data to be decoded.
 * @param[in]  data_len  Size of the @ref p_data buffer.
 *
 * @returns zero on success or negative error value on failure.
 */
extern nrf_802154_ser_err_t nrf_802154_spinel_dispatch_cmd(spinel_command_t cmd,
                                                           const void     * p_cmd_data,
                                                           size_t           cmd_data_len);

#ifdef __cplusplus
}
#endif

#endif /* NRF_802154_SPINEL_DEC_H_ */

/** @} */
