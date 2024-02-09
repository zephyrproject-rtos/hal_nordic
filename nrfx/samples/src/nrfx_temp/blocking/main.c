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
#include <nrfx_temp.h>

#define NRFX_LOG_MODULE                 EXAMPLE
#define NRFX_EXAMPLE_CONFIG_LOG_ENABLED 1
#define NRFX_EXAMPLE_CONFIG_LOG_LEVEL   3
#include <nrfx_log.h>

/**
 * @defgroup nrfx_temp_blocking_example Blocking TEMP example
 * @{
 * @ingroup nrfx_temp_examples
 *
 * @brief Example shows the functionality of nrfx_temp driver operating in the blocking mode.
 *
 * @details Application initializes nrfx_temp driver and starts operating in the blocking mode.
 *          The temperature measurement and relevant message logging are performed, until a number
 *          of measurements taken is equal to the value specified by the user.
 */

/** @brief Symbol specifying the number of temperature measurements to take. */
#define TEMP_ITERATIONS 5UL

/**
 * @brief Function for application main entry.
 *
 * @return Nothing.
 */
int main(void)
{
    nrfx_err_t status;
    (void)status;

    uint32_t i = 0;

    NRFX_EXAMPLE_LOG_INIT();

    NRFX_LOG_INFO("Starting nrfx_temp blocking example.");
    NRFX_EXAMPLE_LOG_PROCESS();

    nrfx_temp_config_t config = NRFX_TEMP_DEFAULT_CONFIG;
    status = nrfx_temp_init(&config, NULL);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    while (1)
    {
        if (i < TEMP_ITERATIONS)
        {
            status = nrfx_temp_measure();
            NRFX_ASSERT(status == NRFX_SUCCESS);

            int32_t temperature = nrfx_temp_result_get();
            int32_t celsius_temperature = nrfx_temp_calculate(temperature);

            int32_t whole_celsius = celsius_temperature / 100;
            uint8_t fraction_celsius = NRFX_ABS(celsius_temperature % 100);

            NRFX_LOG_INFO("Measured temperature: %d.%02u [C]", whole_celsius, fraction_celsius);

            i++;
        }
        else if (i == TEMP_ITERATIONS)
        {
            nrfx_temp_uninit();
            i++;
        }

        NRFX_EXAMPLE_LOG_PROCESS();
    }
}

/** @} */
