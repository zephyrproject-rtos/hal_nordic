/*
 * Copyright (c) 2022 - 2025, Nordic Semiconductor ASA
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
#include <nrfx_timer.h>

#define NRFX_LOG_MODULE                 EXAMPLE
#define NRFX_EXAMPLE_CONFIG_LOG_ENABLED 1
#define NRFX_EXAMPLE_CONFIG_LOG_LEVEL   3
#include <nrfx_log.h>

/**
 * @defgroup nrfx_timer_counter_example Counter example
 * @{
 * @ingroup nrfx_timer_examples
 *
 * @brief Example showing basic functionality of nrfx_timer driver in Timer Mode and Counter Mode.
 *
 * @details Application initializes nrfx_timer driver. The @ref timer_handler() is executed
 *          regularly after specified time ( @ref TIME_TO_WAIT_MS ). Moreover, @ref timer_handler()
 *          is executed when the counter value is equal to @ref COUNTER_MAX_VAL.
 */

/** @brief Symbol specifying time in milliseconds to wait for handler execution. */
#define TIME_TO_WAIT_MS 1000UL

/** @brief Symbol specifying maximum value of the timer. */
#define TIMER_MAX_VAL 3UL

/** @brief Symbol specifying maximum value of the counter. */
#define COUNTER_MAX_VAL 3UL

/** @brief Symbol specifying the initial value of the timer. */
#define TIMER_START_VAL 1UL

/** @brief Global variable that is used to increment the value of a counter. */
static volatile bool m_counter_inc_flag = false;

/** @brief TIMER instance used in the example as a timer. */
static nrfx_timer_t timer_t_inst = NRFX_TIMER_INSTANCE(NRF_TIMER_INST_GET(TIMER_T_INST_IDX));

/** @brief TIMER instance used in the example as a counter. */
static nrfx_timer_t timer_c_inst = NRFX_TIMER_INSTANCE(NRF_TIMER_INST_GET(TIMER_C_INST_IDX));

#if !defined(__ZEPHYR__)
/* Define an IRQ handler named nrfx_timer_<TIMER_T_INST_IDX>_irq_handler. */
NRFX_INSTANCE_IRQ_HANDLER_DEFINE(timer, TIMER_T_INST_IDX, &timer_t_inst);

/* Define an IRQ handler named nrfx_timer_<TIMER_C_INST_IDX>_irq_handler. */
NRFX_INSTANCE_IRQ_HANDLER_DEFINE(timer, TIMER_C_INST_IDX, &timer_c_inst);
#endif

/**
 * @brief Function for handling TIMER driver events for TIMER instance in timer mode.
 *
 * @param[in] event_type Timer event.
 * @param[in] p_context  General purpose parameter set during initialization of
 *                       the timer. This parameter can be used to pass
 *                       additional information to the handler function, for
 *                       example, the timer ID. In this example p_context is not used.
 */
static void timer_handler(nrf_timer_event_t event_type, void * p_context)
{
    static uint32_t timer_val = TIMER_START_VAL;

    /* Configuring timer to count from TIMER_START_VAL to TIMER_MAX_VAL */
    NRFX_LOG_INFO("Timer: %u", timer_val);
    (timer_val == TIMER_MAX_VAL) ? timer_val = TIMER_START_VAL : timer_val++;

    if (timer_val == TIMER_START_VAL)
    {
        m_counter_inc_flag = true;
    }
}

/**
 * @brief Function for handling TIMER driver events for the TIMER instance in counter mode.
 *
 * @param[in] event_type Timer event.
 * @param[in] p_context  General purpose parameter set during initialization of
 *                       the timer. This parameter can be used to pass
 *                       additional information to the handler function, for
 *                       example, the timer ID. In this example p_context is not used.
 */
static void counter_handler(nrf_timer_event_t event_type, void * p_context)
{
    NRFX_LOG_INFO("Counter finished");
}

/**
 * @brief Function for application main entry.
 *
 * @return Nothing.
 */
int main(void)
{
    int status;
    (void)status;

#if defined(__ZEPHYR__)
    IRQ_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_TIMER_INST_GET(TIMER_T_INST_IDX)), IRQ_PRIO_LOWEST,
                nrfx_timer_irq_handler, &timer_t_inst, 0);
    IRQ_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_TIMER_INST_GET(TIMER_C_INST_IDX)), IRQ_PRIO_LOWEST,
                nrfx_timer_irq_handler, &timer_c_inst, 0);
#endif

    NRFX_EXAMPLE_LOG_INIT();

    NRFX_LOG_INFO("Starting nrfx_timer basic counter example.");
    NRFX_EXAMPLE_LOG_PROCESS();

    /* Configuring timer instances, one in timer mode and one in counter mode, assigning an event
     * handler for each of them and setting frequency to the base frequency value of the specified
     * timer instance.
     */
    uint32_t base_frequency = NRF_TIMER_BASE_FREQUENCY_GET(timer_t_inst.p_reg);
    nrfx_timer_config_t config = NRFX_TIMER_DEFAULT_CONFIG(base_frequency);
    config.bit_width = NRF_TIMER_BIT_WIDTH_32;
    status = nrfx_timer_init(&timer_t_inst, &config, timer_handler);
    NRFX_ASSERT(status == 0);

    config.frequency = NRF_TIMER_BASE_FREQUENCY_GET(timer_c_inst.p_reg);
    config.mode = NRF_TIMER_MODE_COUNTER;
    status = nrfx_timer_init(&timer_c_inst, &config, counter_handler);
    NRFX_ASSERT(status == 0);

    NRFX_LOG_INFO("Time between timer ticks: %lu ms", TIME_TO_WAIT_MS);

    /* Creating variable desired_ticks to store the output of nrfx_timer_ms_to_ticks function. */
    uint32_t desired_ticks = nrfx_timer_ms_to_ticks(&timer_t_inst, TIME_TO_WAIT_MS);

    /* Setting the timer (in timer mode) channel NRF_TIMER_CC_CHANNEL0 in the extended compare
     * mode to clear the timer and trigger an interrupt if internal counter register is equal
     * to desired_ticks.
     */
    nrfx_timer_extended_compare(&timer_t_inst, NRF_TIMER_CC_CHANNEL0, desired_ticks,
                                NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);

    /* Setting the timer (in counter mode) channel NRF_TIMER_CC_CHANNEL0 in the extended compare
     * mode to stop the timer and trigger an interrupt if internal counter register is equal
     * to COUNTER_MAX_VAL.
     */
    nrfx_timer_extended_compare(&timer_c_inst, NRF_TIMER_CC_CHANNEL0, COUNTER_MAX_VAL,
                                NRF_TIMER_SHORT_COMPARE0_STOP_MASK, true);

    nrfx_timer_enable(&timer_c_inst);
    nrfx_timer_enable(&timer_t_inst);
    NRFX_LOG_INFO("Timer status: %s",
                  nrfx_timer_is_enabled(&timer_t_inst) ? "enabled" : "disabled");
    NRFX_LOG_INFO("Counter status: %s",
                  nrfx_timer_is_enabled(&timer_c_inst) ? "enabled" : "disabled");

    uint32_t timer_c_curr_val = 0;

    while (1)
    {
       if (m_counter_inc_flag)
       {
            nrfx_timer_increment(&timer_c_inst);
            timer_c_curr_val = nrfx_timer_capture(&timer_c_inst, NRF_TIMER_CC_CHANNEL1);
            NRFX_LOG_INFO("Counter: %u / %lu", timer_c_curr_val, COUNTER_MAX_VAL);
            m_counter_inc_flag = false;

            if (timer_c_curr_val >= COUNTER_MAX_VAL)
            {
                nrfx_timer_disable(&timer_t_inst);
                nrfx_timer_disable(&timer_c_inst);
                NRFX_LOG_INFO("Timer status: %s",
                              nrfx_timer_is_enabled(&timer_t_inst) ? "enabled" : "disabled");
                NRFX_LOG_INFO("Counter status: %s",
                              nrfx_timer_is_enabled(&timer_c_inst) ? "enabled" : "disabled");
            }
        }
        NRFX_EXAMPLE_LOG_PROCESS();
    }
}

/** @} */
