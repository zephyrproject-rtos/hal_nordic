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

#include <saadc_examples_common.h>
#include <helpers/nrfx_gppi.h>
#include <nrfx_gpiote.h>

void gpiote_pin_toggle_task_setup(nrfx_gpiote_t const * p_gpiote, nrfx_gpiote_pin_t pin)
{
    nrfx_err_t status;
    (void)status;

    uint8_t gpiote_channel;
    status = nrfx_gpiote_channel_alloc(p_gpiote, &gpiote_channel);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    static const nrfx_gpiote_output_config_t output_config =
    {
        .drive = NRF_GPIO_PIN_H0H1,
        .input_connect = NRF_GPIO_PIN_INPUT_DISCONNECT,
        .pull = NRF_GPIO_PIN_NOPULL,
    };

    const nrfx_gpiote_task_config_t task_config =
    {
        .task_ch = gpiote_channel,
        .polarity = NRF_GPIOTE_POLARITY_TOGGLE,
        .init_val = NRF_GPIOTE_INITIAL_VALUE_LOW,
    };

    status = nrfx_gpiote_output_configure(p_gpiote, pin, &output_config, &task_config);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    nrfx_gpiote_out_task_enable(p_gpiote, pin);
}

void pin_on_event_toggle_setup(nrfx_gpiote_t const * p_gpiote, nrfx_gpiote_pin_t pin, uint32_t eep)
{
    nrfx_err_t status;
    (void)status;

    uint8_t gppi_channel;

    gpiote_pin_toggle_task_setup(p_gpiote, pin);

    status = nrfx_gppi_channel_alloc(&gppi_channel);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    nrfx_gppi_channel_endpoints_setup(gppi_channel,
                                      eep,
                                      nrfx_gpiote_out_task_address_get(p_gpiote, pin));

    nrfx_gppi_channels_enable(NRFX_BIT(gppi_channel));
}
