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
#include <saadc_examples_common.h>
#include <nrfx_saadc.h>
#include <nrfx_gpiote.h>

#define NRFX_LOG_MODULE                 EXAMPLE
#define NRFX_EXAMPLE_CONFIG_LOG_ENABLED 1
#define NRFX_EXAMPLE_CONFIG_LOG_LEVEL   3
#include <nrfx_log.h>

/**
 * @defgroup nrfx_saadc_advanced_blocking_example Advanced blocking SAADC example
 * @{
 * @ingroup nrfx_saadc_examples
 *
 * @brief Example showing advanced functionality of nrfx_saadc driver operating in the blocking mode.
 *
 * @details Application initializes nrfx_saadc driver and starts operating in the blocking mode.
 *          In the example, @ref m_single_channel is configured and the SAADC driver is set to the advanced mode.
 *
 *          Before every sampling, calibration in a blocking manner is performed. Sampling is invoked
 *          by @p nrfx_saadc_mode_trigger() function afterwards.
 *
 *          In the example there is GPIOTE task configured to toggle specified ( @ref OUT_GPIO_PIN ) pin. This task
 *          is being triggered between successive samplings to verify the functionality of the SAADC on the non-constant analog signal.
 */

/** @brief Symbol specifying analog input to be observed by SAADC channel 0. */
#define CH0_AIN ANALOG_INPUT_TO_SAADC_AIN(ANALOG_INPUT_A0)

/** @brief Symbol specifying GPIOTE instance to be used. */
#define GPIOTE_INST_IDX 0

/** @brief Symbol specifying GPIO pin used to test the functionality of SAADC. */
#define OUT_GPIO_PIN LOOPBACK_PIN_1B

/** @brief Symbol specifying the size of singular sample buffer ( @ref m_samples_buffer ). */
#define BUFFER_SIZE 2UL

/** @brief Symbol specifying the number of SAADC samplings to trigger. */
#define SAMPLING_ITERATIONS 5UL

/** @brief Symbol specifying the resolution of the SAADC. */
#define RESOLUTION NRF_SAADC_RESOLUTION_10BIT

/** @brief SAADC channel configuration structure for single channel use. */
static const nrfx_saadc_channel_t m_single_channel = SAADC_CHANNEL_SE_ACQ_3US(CH0_AIN, 0);

/** @brief Samples buffer to store values from a SAADC channel. */
static uint16_t m_samples_buffer[BUFFER_SIZE];

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
    IRQ_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_GPIOTE_INST_GET(GPIOTE_INST_IDX)), IRQ_PRIO_LOWEST,
                NRFX_GPIOTE_INST_HANDLER_GET(GPIOTE_INST_IDX), 0, 0);
#endif

    NRFX_EXAMPLE_LOG_INIT();
    NRFX_LOG_INFO("Starting nrfx_saadc advanced blocking example.");
    NRFX_EXAMPLE_LOG_PROCESS();

    status = nrfx_saadc_init(NRFX_SAADC_DEFAULT_CONFIG_IRQ_PRIORITY);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    nrfx_gpiote_t const gpiote_inst = NRFX_GPIOTE_INSTANCE(GPIOTE_INST_IDX);
    status = nrfx_gpiote_init(&gpiote_inst, NRFX_GPIOTE_DEFAULT_CONFIG_IRQ_PRIORITY);
    NRFX_ASSERT(status == NRFX_SUCCESS);
    NRFX_LOG_INFO("GPIOTE status: %s",
                  nrfx_gpiote_init_check(&gpiote_inst) ? "initialized" : "not initialized");

    gpiote_pin_toggle_task_setup(&gpiote_inst, OUT_GPIO_PIN);

    status = nrfx_saadc_channel_config(&m_single_channel);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    nrfx_saadc_adv_config_t adv_config = NRFX_SAADC_DEFAULT_ADV_CONFIG;

    uint32_t channel_mask = nrfx_saadc_channels_configured_get();
    status = nrfx_saadc_advanced_mode_set(channel_mask,
                                          RESOLUTION,
                                          &adv_config,
                                          NULL);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    status = nrfx_saadc_buffer_set(m_samples_buffer, BUFFER_SIZE);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    uint16_t sampling_index = 0;
    while (1)
    {
        if (sampling_index < SAMPLING_ITERATIONS)
        {
            nrfx_gpiote_out_task_trigger(&gpiote_inst, OUT_GPIO_PIN);

            status = nrfx_saadc_offset_calibrate(NULL);
            NRFX_ASSERT(status == NRFX_SUCCESS);
            NRFX_LOG_INFO("Calibration in the blocking manner finished successfully.");

            /*
             * If function nrfx_saadc_mode_trigger() returns NRFX_ERROR_BUSY it means that the conversion in the advanced
             * blocking mode is still being performed. Then the function must be called again to continue the conversion.
             */
            while ((status = nrfx_saadc_mode_trigger()) == NRFX_ERROR_BUSY)
            {}
            NRFX_ASSERT(status == NRFX_SUCCESS);
            NRFX_LOG_INFO("Sampling %d / %ld", sampling_index + 1, SAMPLING_ITERATIONS);

            for (uint32_t buffer_index = 0; buffer_index < BUFFER_SIZE; buffer_index++)
            {
                NRFX_LOG_INFO("[Sample %u] value == %d",
                              buffer_index,
                              NRFX_SAADC_SAMPLE_GET(m_samples_buffer, buffer_index));
            }

            status = nrfx_saadc_buffer_set(m_samples_buffer, BUFFER_SIZE);
            NRFX_ASSERT(status == NRFX_SUCCESS);

            sampling_index++;
        }
        NRFX_EXAMPLE_LOG_PROCESS();
    }
}

/** @} */
