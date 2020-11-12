/*
 * Copyright (c) 2020, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>
#include "nrf.h"

#include "fem/nrf_fem_protocol_api.h"

#ifdef __cplusplus
extern "C" {
#endif

int32_t nrf_802154_fal_pa_configuration_set(const nrf_802154_fal_event_t * const p_activate_event,
                                            const nrf_802154_fal_event_t * const p_deactivate_event)
{
    (void)p_activate_event;
    (void)p_deactivate_event;

    return NRFX_ERROR_FORBIDDEN;
}

int32_t nrf_802154_fal_pa_configuration_clear(void)
{
    return NRFX_ERROR_FORBIDDEN;
}

int32_t nrf_802154_fal_lna_configuration_set(const nrf_802154_fal_event_t * const p_activate_event,
                                             const nrf_802154_fal_event_t * const p_deactivate_event)
{
    (void)p_activate_event;
    (void)p_deactivate_event;

    return NRFX_ERROR_FORBIDDEN;
}

int32_t nrf_802154_fal_lna_configuration_clear(void)
{
    return NRFX_ERROR_FORBIDDEN;
}

void nrf_802154_fal_deactivate_now(nrf_fal_functionality_t type)
{
    (void)type;
}

#if defined(NRF52_SERIES)
// TODO: Uncomment this code when API is portable to nRF53
int32_t nrf_802154_fal_abort_set(uint32_t event, nrf_ppi_channel_group_t group)
{
    return NRFX_ERROR_FORBIDDEN;
}

int32_t nrf_802154_fal_abort_extend(nrf_ppi_channel_t       channel_to_add,
                                    nrf_ppi_channel_group_t group)
{
    return NRFX_ERROR_FORBIDDEN;
}

int32_t nrf_802154_fal_abort_reduce(nrf_ppi_channel_t       channel_to_remove,
                                    nrf_ppi_channel_group_t group)
{
    return NRFX_ERROR_FORBIDDEN;
}

int32_t nrf_802154_fal_abort_clear(void)
{
    return NRFX_ERROR_FORBIDDEN;
}

#endif // NRF52_SERIES

void nrf_802154_fal_cleanup(void)
{
    // Intentionally empty
}

void nrf_802154_fal_pa_is_configured(int8_t * const p_gain)
{
    (void)p_gain;
}

#if defined(NRF52_SERIES)
// TODO: Uncomment this code when API is portable to nRF53
bool nrf_fem_prepare_powerdown(NRF_TIMER_Type  * p_instance,
                               uint32_t          compare_channel,
                               nrf_ppi_channel_t ppi_id)
{
    (void)p_instance;
    (void)compare_channel;
    (void)ppi_id;

    return false;
}

#endif // NRF52_SERIES

int8_t nrf_802154_fal_tx_power_get(const uint8_t channel, const int8_t power)
{
    (void)channel;

    return power;
}

#ifdef __cplusplus
}
#endif
