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
#include <nrfx_pwm.h>

#define NRFX_LOG_MODULE                 EXAMPLE
#define NRFX_EXAMPLE_CONFIG_LOG_ENABLED 1
#define NRFX_EXAMPLE_CONFIG_LOG_LEVEL   3
#include <nrfx_log.h>

/**
 * @defgroup nrfx_pwm_common_example Common mode PWM example
 * @{
 * @ingroup nrfx_pwm_examples
 *
 * @brief Example showing basic functionality of nrfx_pwm driver for sequence loaded in common mode.
 *
 * @details Application initializes nrfx_pmw driver. It plays a simple sequence on LEDs ("breath"
 *          effect) and replays this sequence @ref NUM_OF_LOOPS times. The @ref pwm_handler() is
 *          executed with relevant log message after every loop.
 */

/** @brief Symbol specifying PWM instance to be used. */
#define PWM_INST_IDX 0

/**
 * @brief Symbol specifying number of times that each duty cycle is to be repeated (after being
 *        played once) and is strictly correlated with the "breath" effect speed.
 */
#define VALUE_REPEATS 150UL

/** @brief Symbol specifying number of loops (breaths) to be performed. */
#define NUM_OF_LOOPS 3UL

/**
 * @brief Symbol specifying number of playbacks to be performed. In this example couple of
 *        playbacks might be considered as one loop.
 */
#define PLAYBACK_COUNT 1UL

/** @brief Array with duty cycle values. */
nrf_pwm_values_common_t pwm_val []=
{
    0, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000,
    900, 800, 700, 600, 500, 400, 300, 200, 100, 0
};

/**
 * @brief Function for handling PWM driver events.
 *
 * @param[in] event_type PWM event.
 * @param[in] p_context  General purpose parameter set during initialization of
 *                       the timer. This parameter can be used to pass
 *                       additional information to the handler function.
 */
static void pwm_handler(nrfx_pwm_evt_type_t event_type, void * p_context)
{
    nrfx_pwm_t * inst = p_context;
    static uint32_t curr_loop = 1;

    NRFX_LOG_INFO("Loops: %u / %lu", curr_loop, NUM_OF_LOOPS);

    if (curr_loop == NUM_OF_LOOPS)
    {
        NRFX_LOG_INFO("PWM finished");
        nrfx_pwm_uninit(inst);
    }
    curr_loop++;
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

#if defined(__ZEPHYR__)
    IRQ_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_PWM_INST_GET(PWM_INST_IDX)), IRQ_PRIO_LOWEST,
                NRFX_PWM_INST_HANDLER_GET(PWM_INST_IDX), 0, 0);
#endif

    NRFX_EXAMPLE_LOG_INIT();

    NRFX_LOG_INFO("Starting nrfx_pwm example for sequence loaded in common mode.");
    NRFX_EXAMPLE_LOG_PROCESS();

    nrfx_pwm_t pwm_instance = NRFX_PWM_INSTANCE(PWM_INST_IDX);
    nrfx_pwm_config_t config = NRFX_PWM_DEFAULT_CONFIG(LED1_PIN, LED2_PIN, LED3_PIN, LED4_PIN);
    status = nrfx_pwm_init(&pwm_instance, &config, pwm_handler, &pwm_instance);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    nrf_pwm_sequence_t seq =
    {
        .values = {pwm_val},
        .length = NRFX_ARRAY_SIZE(pwm_val),
        .repeats = VALUE_REPEATS,
        .end_delay = 0
    };

    nrfx_pwm_simple_playback(&pwm_instance, &seq, PLAYBACK_COUNT, NRFX_PWM_FLAG_LOOP);

    while (1)
    {
        NRFX_EXAMPLE_LOG_PROCESS();
    }
}

/** @} */
