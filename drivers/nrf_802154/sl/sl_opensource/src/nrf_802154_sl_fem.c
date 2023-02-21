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

#include <nrfx.h>
#include "nrf_errno.h"
#include <stdbool.h>
#include <stdint.h>

#include "nrf_802154_fal.h"
#include "nrf_802154_types.h"

#include "protocol/mpsl_fem_protocol_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**@brief Macro to get the number of elements in an array.
 *
 * @param[in] X   Array.
 */
#define NUMELTS(X) (sizeof((X)) / sizeof(X[0]))

/**
 * Converts TX power integer values to RADIO TX power allowed values.
 *
 * @param[in]  integer_tx_power  TX power integer value.
 *
 * @retval     RADIO TX power allowed value.
 */
static nrf_radio_txpower_t to_radio_tx_power_convert(int8_t integer_tx_power)
{
    static const nrf_radio_txpower_t allowed_values[] =
    {
#if defined(RADIO_TXPOWER_TXPOWER_Neg40dBm)
        NRF_RADIO_TXPOWER_NEG40DBM, /**< -40 dBm. */
#endif
        NRF_RADIO_TXPOWER_NEG20DBM, /**< -20 dBm. */
        NRF_RADIO_TXPOWER_NEG16DBM, /**< -16 dBm. */
        NRF_RADIO_TXPOWER_NEG12DBM, /**< -12 dBm. */
        NRF_RADIO_TXPOWER_NEG8DBM,  /**< -8 dBm. */
        NRF_RADIO_TXPOWER_NEG4DBM,  /**< -4 dBm. */
        NRF_RADIO_TXPOWER_0DBM,     /**< 0 dBm. */
#if defined(RADIO_TXPOWER_TXPOWER_Pos2dBm)
        NRF_RADIO_TXPOWER_POS2DBM,  /**< 2 dBm. */
#endif
#if defined(RADIO_TXPOWER_TXPOWER_Pos3dBm)
        NRF_RADIO_TXPOWER_POS3DBM,  /**< 3 dBm. */
#endif
#if defined(RADIO_TXPOWER_TXPOWER_Pos4dBm)
        NRF_RADIO_TXPOWER_POS4DBM,  /**< 4 dBm. */
#endif
#if defined(RADIO_TXPOWER_TXPOWER_Pos5dBm)
        NRF_RADIO_TXPOWER_POS5DBM,  /**< 5 dBm. */
#endif
#if defined(RADIO_TXPOWER_TXPOWER_Pos6dBm)
        NRF_RADIO_TXPOWER_POS6DBM,  /**< 6 dBm. */
#endif
#if defined(RADIO_TXPOWER_TXPOWER_Pos7dBm)
        NRF_RADIO_TXPOWER_POS7DBM,  /**< 7 dBm. */
#endif
#if defined(RADIO_TXPOWER_TXPOWER_Pos8dBm)
        NRF_RADIO_TXPOWER_POS8DBM,  /**< 8 dBm. */
#endif
    };

    for (uint32_t i = NUMELTS(allowed_values) - 1; i > 0; i--)
    {
        if (integer_tx_power >= (int8_t)allowed_values[i])
        {
            return allowed_values[i];
        }
    }

    return allowed_values[0];
}

int32_t mpsl_fem_disable(void)
{
    return 0;
}

int32_t mpsl_fem_pa_configuration_set(const mpsl_fem_event_t * const p_activate_event,
                                      const mpsl_fem_event_t * const p_deactivate_event)
{
    (void)p_activate_event;
    (void)p_deactivate_event;

    return -NRF_EPERM;
}

int32_t mpsl_fem_pa_configuration_clear(void)
{
    return -NRF_EPERM;
}

int32_t mpsl_fem_lna_configuration_set(const mpsl_fem_event_t * const p_activate_event,
                                       const mpsl_fem_event_t * const p_deactivate_event)
{
    (void)p_activate_event;
    (void)p_deactivate_event;

    return -NRF_EPERM;
}

int32_t mpsl_fem_lna_configuration_clear(void)
{
    return -NRF_EPERM;
}

void mpsl_fem_deactivate_now(mpsl_fem_functionality_t type)
{
    (void)type;
}

int32_t mpsl_fem_abort_set(uint32_t event, uint32_t group)
{
    return -NRF_EPERM;
}

int32_t mpsl_fem_abort_extend(uint32_t channel_to_add, uint32_t group)
{
    (void)channel_to_add;
    (void)group;
    return 0;
}

int32_t mpsl_fem_abort_reduce(uint32_t channel_to_remove, uint32_t group)
{
    (void)channel_to_remove;
    (void)group;
    return 0;
}

int32_t mpsl_fem_abort_clear(void)
{
    return -NRF_EPERM;
}

void mpsl_fem_cleanup(void)
{
    // Intentionally empty
}

void mpsl_fem_tx_power_split(const mpsl_tx_power_t         power,
                             mpsl_tx_power_split_t * const p_tx_power_split)
{
    p_tx_power_split->radio_tx_power      = to_radio_tx_power_convert(power);
    p_tx_power_split->fem.gain_db         = 0;
    p_tx_power_split->fem.private_setting = 0;
}

int32_t mpsl_fem_pa_gain_set(const mpsl_fem_gain_t * p_gain)
{
    (void)p_gain;

    return 0;
}

void mpsl_fem_pa_is_configured(int8_t * const p_gain)
{
    (void)p_gain;
}

bool mpsl_fem_prepare_powerdown(NRF_TIMER_Type * p_instance,
                                uint32_t         compare_channel,
                                uint32_t         ppi_id,
                                uint32_t         event_addr)
{
    (void)p_instance;
    (void)compare_channel;
    (void)ppi_id;

    return false;
}

bool mpsl_fem_device_config_254_apply_get(void)
{
    return false;
}

int8_t nrf_802154_fal_tx_power_split(const uint8_t                           channel,
                                     const int8_t                            power,
                                     nrf_802154_fal_tx_power_split_t * const p_tx_power_split)
{
    (void)channel;

    p_tx_power_split->radio_tx_power      = to_radio_tx_power_convert(power);
    p_tx_power_split->fem.gain_db         = 0;
    p_tx_power_split->fem.private_setting = 0;

    return p_tx_power_split->radio_tx_power;
}

#ifdef __cplusplus
}
#endif
