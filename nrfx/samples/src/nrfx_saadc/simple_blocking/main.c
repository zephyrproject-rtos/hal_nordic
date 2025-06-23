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

#define NRFX_LOG_MODULE                 EXAMPLE
#define NRFX_EXAMPLE_CONFIG_LOG_ENABLED 1
#define NRFX_EXAMPLE_CONFIG_LOG_LEVEL   3
#include <nrfx_log.h>

/**
 * @defgroup nrfx_saadc_simple_blocking_example Simple blocking SAADC example
 * @{
 * @ingroup nrfx_saadc_examples
 *
 * @brief Example showing simple functionality of nrfx_saadc driver operating in the blocking mode.
 *
 * @details Application initializes nrfx_saadc driver and starts operating in the blocking mode.
 *          Program works as a simple state machine and starts in state @ref m_current_state == STATE_SINGLE_CONFIG.
 *          In the state ( @ref m_current_state ):
 *          - STATE_SINGLE_CONFIG - SAADC driver is configured to work with only one channel ( @ref m_single_channel )
 *            in the blocking mode. @ref m_current_state is changed to STATE_SINGLE_SAMPLING afterward.
 *          - STATE_SINGLE_SAMPLING - sampling on a single channel ( @ref m_single_channel ) is performed specified
 *            number of times ( @ref SAMPLING_ITERATIONS ), after that @ref m_current_state is changed to STATE_MULTIPLE_CONFIG.
 *          - STATE_MULTIPLE_CONFIG - SAADC driver is configured to work with multiple channels ( @ref m_multiple_channels )
 *            in the blocking mode. @ref m_current_state is changed to STATE_MULTIPLE_SAMPLING afterward.
 *          - STATE_MULTIPLE_SAMPLING - sampling on multiple channels ( @ref m_multiple_channels ) is performed specified
 *            number of times ( @ref SAMPLING_ITERATIONS ).
 *          Before every sampling, calibration in a blocking manner is performed. After it, sampling is invoked
 *          by @p nrfx_saadc_mode_trigger() function.
 *
 *          In the example there are GPIOTE tasks configured to toggle specified ( @ref m_out_pins ) loopback pins. Those tasks
 *          are being triggered between successive samplings to verify the functionality of the SAADC on the non-constant analog signal.
 */

/** @brief Symbol specifying analog input to be observed by SAADC channel 0. */
#define CH0_AIN ANALOG_INPUT_TO_SAADC_AIN(ANALOG_INPUT_A0)

/** @brief Symbol specifying analog input to be observed by SAADC channel 1. */
#define CH1_AIN ANALOG_INPUT_TO_SAADC_AIN(ANALOG_INPUT_A1)

/** @brief Symbol specifying analog input to be observed by SAADC channel 2. */
#define CH2_AIN ANALOG_INPUT_TO_SAADC_AIN(ANALOG_INPUT_A2)

/** @brief Declaration of enum containing a set of states for the simple state machine. */
typedef enum
{
    STATE_SINGLE_CONFIG,     ///< Configure a single SAADC channel and set the SAADC driver in the simple mode.
    STATE_SINGLE_SAMPLING,   ///< Trigger SAADC sampling on the single channel.
    STATE_MULTIPLE_CONFIG,   ///< Configure multiple SAADC channels and set the SAADC driver in the simple mode.
    STATE_MULTIPLE_SAMPLING, ///< Trigger SAADC sampling on multiple channels.
} state_t;

/** @brief SAADC channel configuration structure for single channel use. */
static const nrfx_saadc_channel_t m_single_channel = NRFX_SAADC_DEFAULT_CHANNEL_SE(CH0_AIN, 0);

/** @brief SAADC channel configuration structure for multiple channel use. */
static const nrfx_saadc_channel_t m_multiple_channels[] =
{
    NRFX_SAADC_DEFAULT_CHANNEL_SE(CH0_AIN, 0),
    NRFX_SAADC_DEFAULT_CHANNEL_SE(CH1_AIN, 1),
    NRFX_SAADC_DEFAULT_CHANNEL_SE(CH2_AIN, 2),
};

/** @brief Symbol specifying numbers of multiple channels ( @ref m_multiple_channels) used by SAADC. */
#define CHANNEL_COUNT NRFX_ARRAY_SIZE(m_multiple_channels)

/** @brief Symbol specifying the number of SAADC samplings to trigger. */
#define SAMPLING_ITERATIONS 8

/** @brief Symbol specifying the resolution of the SAADC. */
#define RESOLUTION NRF_SAADC_RESOLUTION_8BIT

/** @brief Symbol specifying GPIOTE instance to be used. */
#define GPIOTE_INST_IDX 0

/** @brief Array specifying GPIO pins used to test the functionality of SAADC. */
static uint8_t m_out_pins[CHANNEL_COUNT] = {LOOPBACK_PIN_1B, LOOPBACK_PIN_2B, LOOPBACK_PIN_3B};

/** @brief Samples buffer defined with the size of @ref CHANNEL_COUNT symbol to store values from each channel ( @ref m_multiple_channels). */
static uint16_t samples_buffer[CHANNEL_COUNT];

/** @brief Enum with the current state of the simple state machine. */
static state_t m_current_state = STATE_SINGLE_CONFIG;

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
    NRFX_LOG_INFO("Starting nrfx_saadc simple blocking example.");
    NRFX_EXAMPLE_LOG_PROCESS();

    status = nrfx_saadc_init(NRFX_SAADC_DEFAULT_CONFIG_IRQ_PRIORITY);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    nrfx_gpiote_t const gpiote_inst = NRFX_GPIOTE_INSTANCE(GPIOTE_INST_IDX);
    status = nrfx_gpiote_init(&gpiote_inst, NRFX_GPIOTE_DEFAULT_CONFIG_IRQ_PRIORITY);
    NRFX_ASSERT(status == NRFX_SUCCESS);
    NRFX_LOG_INFO("GPIOTE status: %s",
                  nrfx_gpiote_init_check(&gpiote_inst) ? "initialized" : "not initialized");

    uint8_t i;
    for (i = 0; i < CHANNEL_COUNT; i++)
    {
        gpiote_pin_toggle_task_setup(&gpiote_inst, m_out_pins[i]);
    }

    uint32_t sampling_index = 0;
    uint32_t channels_mask = 0;
    while (1)
    {
        switch (m_current_state)
        {
            case STATE_SINGLE_CONFIG:
                NRFX_LOG_INFO("Single channel SAADC test.");

                status = nrfx_saadc_channel_config(&m_single_channel);
                NRFX_ASSERT(status == NRFX_SUCCESS);

                channels_mask = nrfx_saadc_channels_configured_get();
                status = nrfx_saadc_simple_mode_set(channels_mask,
                                                    RESOLUTION,
                                                    NRF_SAADC_OVERSAMPLE_DISABLED,
                                                    NULL);
                NRFX_ASSERT(status == NRFX_SUCCESS);

                status = nrfx_saadc_buffer_set(samples_buffer, 1);
                NRFX_ASSERT(status == NRFX_SUCCESS);

                m_current_state = STATE_SINGLE_SAMPLING;
                break;

            case STATE_SINGLE_SAMPLING:
                if (sampling_index++ < SAMPLING_ITERATIONS)
                {
                    nrfx_gpiote_out_task_trigger(&gpiote_inst, m_out_pins[0]);

                    status = nrfx_saadc_offset_calibrate(NULL);
                    NRFX_ASSERT(status == NRFX_SUCCESS);
                    NRFX_LOG_INFO("Calibration in the blocking manner finished successfully.");

                    NRFX_LOG_INFO("Sampling %d / %d", sampling_index, SAMPLING_ITERATIONS);
                    NRFX_EXAMPLE_LOG_PROCESS();

                    status = nrfx_saadc_mode_trigger();
                    NRFX_ASSERT(status == NRFX_SUCCESS);

                    NRFX_LOG_INFO("[CHANNEL %u] Sampled value == %d",
                                  m_multiple_channels[0].channel_index,
                                  NRFX_SAADC_SAMPLE_GET(samples_buffer, 0));
                }
                else
                {
                    m_current_state = STATE_MULTIPLE_CONFIG;
                    sampling_index = 0;
                }
                break;

            case STATE_MULTIPLE_CONFIG:
                NRFX_LOG_INFO("Multiple channels SAADC test.");

                status = nrfx_saadc_channels_config(m_multiple_channels, CHANNEL_COUNT);
                NRFX_ASSERT(status == NRFX_SUCCESS);

                channels_mask = nrfx_saadc_channels_configured_get();
                status = nrfx_saadc_simple_mode_set(channels_mask,
                                                    RESOLUTION,
                                                    NRF_SAADC_OVERSAMPLE_DISABLED,
                                                    NULL);
                NRFX_ASSERT(status == NRFX_SUCCESS);

                status = nrfx_saadc_buffer_set(samples_buffer, CHANNEL_COUNT);
                NRFX_ASSERT(status == NRFX_SUCCESS);

                m_current_state = STATE_MULTIPLE_SAMPLING;
                break;

            case STATE_MULTIPLE_SAMPLING:
                if (sampling_index++ < SAMPLING_ITERATIONS)
                {
                    for (i = 0; i < CHANNEL_COUNT; i++)
                    {
                        nrfx_gpiote_out_task_trigger(&gpiote_inst, m_out_pins[i]);
                    }

                    status = nrfx_saadc_offset_calibrate(NULL);
                    NRFX_ASSERT(status == NRFX_SUCCESS);
                    NRFX_LOG_INFO("Calibration in the blocking manner finished successfully.");

                    NRFX_LOG_INFO("Sampling %d / %d", sampling_index, SAMPLING_ITERATIONS);
                    NRFX_EXAMPLE_LOG_PROCESS();

                    status = nrfx_saadc_mode_trigger();
                    NRFX_ASSERT(status == NRFX_SUCCESS);

                    for (i = 0; i < CHANNEL_COUNT; i++)
                    {
                        NRFX_LOG_INFO("[CHANNEL %u] Sampled value == %d",
                                      m_multiple_channels[i].channel_index,
                                      NRFX_SAADC_SAMPLE_GET(samples_buffer, i));
                    }
                }
                break;

            default:
                break;
        }
        NRFX_EXAMPLE_LOG_PROCESS();
    }
}

/** @} */
