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

#include <nrfx_example.h>
#include <helpers/nrfx_gppi.h>
#include <nrfx_timer.h>
#include <nrfx_gpiote.h>

#define NRFX_LOG_MODULE                 EXAMPLE
#define NRFX_EXAMPLE_CONFIG_LOG_ENABLED 1
#define NRFX_EXAMPLE_CONFIG_LOG_LEVEL   3
#include <nrfx_log.h>

/**
 * @defgroup nrfx_gppi_one_to_one_example One-to-one GPPI example
 * @{
 * @ingroup nrfx_gppi_examples
 *
 * @brief Example showing basic functionality of a nrfx_gppi helper.
 *
 * @details Application initializes nrfx_gpiote, nrfx_timer drivers and nrfx_gppi helper in a way that
 *          TIMER compare event is set up to be forwarded via PPI/DPPI to GPIOTE and toggle a pin.
 */

/** @brief Symbol specifying timer instance to be used. */
#define TIMER_INST_IDX 0

/** @brief Symbol specifying time in milliseconds to wait for handler execution. */
#define TIME_TO_WAIT_MS 1000UL

/** @brief Symbol specifying GPIOTE instance to be used. */
#define GPIOTE_INST_IDX 0

/** @brief Symbol specifying ouput pin associated with the task. */
#define OUTPUT_PIN LED1_PIN

/**
 * @brief Function for handling TIMER driver events.
 *
 * @param[in] event_type Timer event.
 * @param[in] p_context  General purpose parameter set during initialization of the timer.
 *                       This parameter can be used to pass additional information to the handler
 *                       function for example the timer ID.
 */
static void timer_handler(nrf_timer_event_t event_type, void * p_context)
{
    if (event_type == NRF_TIMER_EVENT_COMPARE0)
    {
        char * p_msg = p_context;
        NRFX_LOG_INFO("Timer finished. Context passed to the handler: >%s<", p_msg);
        NRFX_LOG_INFO("GPIOTE output pin: %d is %s", OUTPUT_PIN,
                      nrfx_gpiote_in_is_set(OUTPUT_PIN) ? "high" : "low");
    }
}

/**
 * @brief Function for application main entry.
 *
 * @return Nothing.
 */
int main(void)
{
    nrfx_err_t status;
    (void)status;

    uint8_t out_channel;
    uint8_t gppi_channel;

#if defined(__ZEPHYR__)
    IRQ_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_TIMER_INST_GET(TIMER_INST_IDX)), IRQ_PRIO_LOWEST,
                NRFX_TIMER_INST_HANDLER_GET(TIMER_INST_IDX), 0, 0);
    IRQ_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_GPIOTE_INST_GET(GPIOTE_INST_IDX)), IRQ_PRIO_LOWEST,
                NRFX_GPIOTE_INST_HANDLER_GET(GPIOTE_INST_IDX), 0, 0);
#endif

    NRFX_EXAMPLE_LOG_INIT();

    NRFX_LOG_INFO("Starting nrfx_gppi basic one-to-one example.");
    NRFX_EXAMPLE_LOG_PROCESS();

    nrfx_gpiote_t const gpiote_inst = NRFX_GPIOTE_INSTANCE(GPIOTE_INST_IDX);
    status = nrfx_gpiote_init(&gpiote_inst, NRFX_GPIOTE_DEFAULT_CONFIG_IRQ_PRIORITY);
    NRFX_ASSERT(status == NRFX_SUCCESS);
    NRFX_LOG_INFO("GPIOTE status: %s",
                  nrfx_gpiote_init_check(&gpiote_inst) ? "initialized" : "not initialized");

    status = nrfx_gpiote_channel_alloc(&gpiote_inst, &out_channel);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    /*
     * Initialize output pin. The SET task will turn the LED on,
     * CLR will turn it off and OUT will toggle it.
     */
    static const nrfx_gpiote_output_config_t output_config =
    {
        .drive = NRF_GPIO_PIN_S0S1,
        .input_connect = NRF_GPIO_PIN_INPUT_DISCONNECT,
        .pull = NRF_GPIO_PIN_NOPULL,
    };

    const nrfx_gpiote_task_config_t task_config =
    {
        .task_ch = out_channel,
        .polarity = NRF_GPIOTE_POLARITY_TOGGLE,
        .init_val = NRF_GPIOTE_INITIAL_VALUE_HIGH,
    };

    status = nrfx_gpiote_output_configure(&gpiote_inst, OUTPUT_PIN, &output_config, &task_config);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    nrfx_gpiote_out_task_enable(&gpiote_inst, OUTPUT_PIN);

    nrfx_timer_t timer_inst = NRFX_TIMER_INSTANCE(TIMER_INST_IDX);
    uint32_t base_frequency = NRF_TIMER_BASE_FREQUENCY_GET(timer_inst.p_reg);
    nrfx_timer_config_t timer_config = NRFX_TIMER_DEFAULT_CONFIG(base_frequency);
    timer_config.bit_width = NRF_TIMER_BIT_WIDTH_32;
    timer_config.p_context = "Some context";

    status = nrfx_timer_init(&timer_inst, &timer_config, timer_handler);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    nrfx_timer_clear(&timer_inst);

    /* Creating variable desired_ticks to store the output of nrfx_timer_ms_to_ticks function. */
    uint32_t desired_ticks = nrfx_timer_ms_to_ticks(&timer_inst, TIME_TO_WAIT_MS);
    NRFX_LOG_INFO("Time to wait: %lu ms", TIME_TO_WAIT_MS);

    /*
     * Setting the timer channel NRF_TIMER_CC_CHANNEL0 in the extended compare mode to clear
     * the timer and to trigger an interrupt if the internal counter register is equal to
     * desired_ticks.
     */
    nrfx_timer_extended_compare(&timer_inst, NRF_TIMER_CC_CHANNEL0, desired_ticks,
                                NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);

    status = nrfx_gppi_channel_alloc(&gppi_channel);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    /*
     * Configure endpoints of the channel so that the input timer event is connected with the output
     * pin OUT task. This means that each time the timer interrupt occurs, the LED pin will be toggled.
     */
    nrfx_gppi_channel_endpoints_setup(gppi_channel,
        nrfx_timer_compare_event_address_get(&timer_inst, NRF_TIMER_CC_CHANNEL0),
        nrfx_gpiote_out_task_address_get(&gpiote_inst, OUTPUT_PIN));

    nrfx_gppi_channels_enable(BIT(gppi_channel));

    nrfx_timer_enable(&timer_inst);
    NRFX_LOG_INFO("Timer status: %s", nrfx_timer_is_enabled(&timer_inst) ? "enabled" : "disabled");

    while (1)
    {
        NRFX_EXAMPLE_LOG_PROCESS();
    }
}

/** @} */
