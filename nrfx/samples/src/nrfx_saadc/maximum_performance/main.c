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
#include <helpers/nrfx_gppi.h>
#include <nrfx_timer.h>
#include <nrfx_gpiote.h>

#define NRFX_LOG_MODULE                 EXAMPLE
#define NRFX_EXAMPLE_CONFIG_LOG_ENABLED 1
#define NRFX_EXAMPLE_CONFIG_LOG_LEVEL   3
#include <nrfx_log.h>

/**
 * @defgroup nrfx_saadc_maximum_performance_example Maximum performance SAADC example
 * @{
 * @ingroup nrfx_saadc_examples
 *
 * @brief Example showing advanced functionality of nrfx_saadc driver operating at its peak performance.
 *
 * @details Application initializes nrfx_saadc driver and starts operating in the non-blocking mode. Sampling is
 *          performed at the highest supported frequency.
 *          In the example, @ref m_single_channel is configured and the SAADC driver is set to the advanced mode.
 *          To achieve maximum performance:
 *              - Performing sampling at @ref MAX_SAADC_SAMPLE_FREQUENCY requires an external timer. It is done
 *                by setting up endpoints of the channel @ref m_gppi_channels [ @p gppi_channels_purpose_t::SAADC_SAMPLING ]
 *                to trigger SAADC sample task ( @p nrf_saadc_task_t::NRF_SAADC_TASK_SAMPLE ) on the timer compare event.
 *              - Hardware start-on-end must be provided. It is done by setting up endpoints of the channel
 *                @ref m_gppi_channels [ @p gppi_channels_purpose_t::SAADC_START_ON_END ] to trigger SAADC task start
 *                ( @p nrf_saadc_task_t::NRF_SAADC_TASK_START ) on the SAADC event end ( @p nrf_saadc_event_t::NRF_SAADC_EVENT_END ).
 *
 *          Calibration in a non-blocking manner is triggered by @p nrfx_saadc_offset_calibrate. Then at @p NRFX_SAADC_EVT_CALIBRATEDONE
 *          event in @ref saadc_handler() sampling is initiated by calling @p nrfx_saadc_mode_trigger() function.
 *          Consecutive sample tasks are triggered by the external timer at the sample rate specified in  @ref SAADC_SAMPLE_FREQUENCY symbol.
 *
 *          In the example there is GPPI channel configured to test the functionality of SAADC. The endpoints are setup up in a way
 *          that @p NRF_SAADC_EVENT_RESULTDONE event is connected with the GPIOTE task that toggles the @ref OUT_GPIO_PIN pin.
 */

/** @brief Symbol specifying timer instance to be used. */
#define TIMER_INST_IDX 0

/** @brief Symbol specifying GPIOTE instance to be used. */
#define GPIOTE_INST_IDX 0

/** @brief Symbol specifying analog input to be observed by SAADC channel 0. */
#define CH0_AIN ANALOG_INPUT_TO_SAADC_AIN(ANALOG_INPUT_A0)

/** @brief Symbol specifying GPIO pin used to test the functionality of SAADC. */
#define OUT_GPIO_PIN LOOPBACK_PIN_1B

/** @brief Acquisition time [us] for source resistance <= 10 kOhm (see SAADC electrical specification). */
#define ACQ_TIME_10K 3UL

/** @brief Conversion time [us] (see SAADC electrical specification). */
#define CONV_TIME 2UL

/** @brief Symbol specifying maximal possible SAADC sample rate (see SAADC electrical specification). */
#define MAX_SAADC_SAMPLE_FREQUENCY 200000UL

/** @brief Symbol specifying SAADC sample frequency for the continuous sampling. */
#define SAADC_SAMPLE_FREQUENCY MAX_SAADC_SAMPLE_FREQUENCY

/** @brief Symbol specifying time in microseconds to wait for timer handler execution. */
#define TIME_TO_WAIT_US (uint32_t)(1000000UL / SAADC_SAMPLE_FREQUENCY)

/**
 * @brief Symbol specifying the number of sample buffers ( @ref m_sample_buffers ).
 *        Two buffers are required for performing double-buffered conversions.
 */
#define BUFFER_COUNT 2UL

/** @brief Symbol specifying the size of singular sample buffer ( @ref m_sample_buffers ). */
#define BUFFER_SIZE 2UL

/** @brief Symbol specifying the number of SAADC samplings to trigger. */
#define SAMPLING_ITERATIONS 3UL

/** @brief Symbol specifying the resolution of the SAADC. */
#define RESOLUTION NRF_SAADC_RESOLUTION_10BIT

/** @brief SAADC channel configuration structure for single channel use. */
static const nrfx_saadc_channel_t m_single_channel = SAADC_CHANNEL_SE_ACQ_3US(CH0_AIN, 0);

/** @brief Samples buffer to store values from a SAADC channel. */
static uint16_t m_sample_buffers[BUFFER_COUNT][BUFFER_SIZE];

/** @brief Array of the GPPI channels. */
static uint8_t m_gppi_channels[2];

/** @brief Enum with intended uses of GPPI channels defined as @ref m_gppi_channels. */
typedef enum
{
    SAADC_SAMPLING,     ///< Triggers SAADC sampling task on external timer event.
    SAADC_START_ON_END, ///< Triggers SAADC start task on SAADC end event.
} gppi_channels_purpose_t;

/** Maximum sampling rate of SAADC is 200 [kHz]. */
NRFX_STATIC_ASSERT(SAADC_SAMPLE_FREQUENCY <= (MAX_SAADC_SAMPLE_FREQUENCY));

/** For continuous sampling the sample rate SAADC_SAMPLE_FREQUENCY should fulfill the following criteria (see SAADC Continuous sampling). */
NRFX_STATIC_ASSERT(SAADC_SAMPLE_FREQUENCY <= (1000000UL / (ACQ_TIME_10K + CONV_TIME)));

/**
 * @brief Function for handling SAADC driver events.
 *
 * @param[in] p_event Pointer to an SAADC driver event.
 */
static void saadc_handler(nrfx_saadc_evt_t const * p_event)
{
    nrfx_err_t status;
    (void)status;

    static uint16_t buffer_index = 1;
    static uint16_t buf_req_evt_counter;
    uint16_t samples_number;

    switch(p_event->type)
    {
        case NRFX_SAADC_EVT_CALIBRATEDONE:
            NRFX_LOG_INFO("SAADC event: CALIBRATEDONE");

            status = nrfx_saadc_mode_trigger();
            NRFX_ASSERT(status == NRFX_SUCCESS);
            break;

        case NRFX_SAADC_EVT_READY:
            NRFX_LOG_INFO("SAADC event: READY");

            nrfx_gppi_channels_enable(NRFX_BIT(m_gppi_channels[SAADC_SAMPLING]));
            break;

        case NRFX_SAADC_EVT_BUF_REQ:
            NRFX_LOG_INFO("SAADC event: BUF_REQ");

            if (++buf_req_evt_counter < SAMPLING_ITERATIONS)
            {
                /* Next available buffer must be set on the NRFX_SAADC_EVT_BUF_REQ event to achieve the continuous conversion. */
                status = nrfx_saadc_buffer_set(m_sample_buffers[buffer_index++], BUFFER_SIZE);
                NRFX_ASSERT(status == NRFX_SUCCESS);
                buffer_index = buffer_index % BUFFER_COUNT;
            }
            else
            {
                nrfx_gppi_channels_disable(NRFX_BIT(m_gppi_channels[SAADC_START_ON_END]));
            }
            break;

        case NRFX_SAADC_EVT_DONE:
            NRFX_LOG_INFO("SAADC event: DONE");
            NRFX_LOG_INFO("Sample buffer address == %p", p_event->data.done.p_buffer);

            samples_number = p_event->data.done.size;
            for (uint16_t i = 0; i < samples_number; i++)
            {
                NRFX_LOG_INFO("[Sample %u] value == %d",
                              i, NRFX_SAADC_SAMPLE_GET(p_event->data.done.p_buffer, i));
            }
            break;

        case NRFX_SAADC_EVT_FINISHED:
            NRFX_LOG_INFO("FINISHED");

            nrfx_gppi_channels_disable(NRFX_BIT(m_gppi_channels[SAADC_SAMPLING]));
            break;

        default:
            break;
    }
}

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
    (void)event_type;
    (void)p_context;
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
    IRQ_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_SAADC), IRQ_PRIO_LOWEST, nrfx_saadc_irq_handler, 0, 0);
    IRQ_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_GPIOTE_INST_GET(GPIOTE_INST_IDX)), IRQ_PRIO_LOWEST,
                NRFX_GPIOTE_INST_HANDLER_GET(GPIOTE_INST_IDX), 0, 0);
#endif

    NRFX_EXAMPLE_LOG_INIT();
    NRFX_LOG_INFO("Starting nrfx_saadc maximum performance example.");
    NRFX_EXAMPLE_LOG_PROCESS();

    status = nrfx_saadc_init(NRFX_SAADC_DEFAULT_CONFIG_IRQ_PRIORITY);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    nrfx_timer_t timer_inst = NRFX_TIMER_INSTANCE(TIMER_INST_IDX);
    uint32_t base_frequency = NRF_TIMER_BASE_FREQUENCY_GET(timer_inst.p_reg);
    nrfx_timer_config_t timer_config = NRFX_TIMER_DEFAULT_CONFIG(base_frequency);
    timer_config.bit_width = NRF_TIMER_BIT_WIDTH_32;
    timer_config.p_context = &timer_inst;

    status = nrfx_timer_init(&timer_inst, &timer_config, timer_handler);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    nrfx_timer_clear(&timer_inst);

    /* Creating variable desired_ticks to store the output of nrfx_timer_us_to_ticks function. */
    uint32_t desired_ticks = nrfx_timer_us_to_ticks(&timer_inst, TIME_TO_WAIT_US);

    /*
     * Setting the timer channel NRF_TIMER_CC_CHANNEL0 in the extended compare mode to clear
     * the timer and to not trigger an interrupt if the internal counter register is equal to
     * desired_ticks.
     */
    nrfx_timer_extended_compare(&timer_inst,
                                NRF_TIMER_CC_CHANNEL0,
                                desired_ticks,
                                NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK,
                                false);

    status = nrfx_saadc_channel_config(&m_single_channel);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    /*
     * Setting the advanced configuration with triggering sampling by the internal timer disabled
     * (internal_timer_cc = 0) and without software start task on end event (start_on_end = false).
     */
    nrfx_saadc_adv_config_t adv_config = NRFX_SAADC_DEFAULT_ADV_CONFIG;
    adv_config.internal_timer_cc = 0;
    adv_config.start_on_end = false;

    uint32_t channel_mask = nrfx_saadc_channels_configured_get();
    status = nrfx_saadc_advanced_mode_set(channel_mask,
                                          RESOLUTION,
                                          &adv_config,
                                          saadc_handler);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    status = nrfx_saadc_buffer_set(m_sample_buffers[0], BUFFER_SIZE);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    /*
     * Allocate a dedicated channel and configure endpoints of that channel so that the timer compare event
     * is connected with the SAADC sample task. This means that each time the timer interrupt occurs,
     * the SAADC sampling will be triggered.
     */
    status = nrfx_gppi_channel_alloc(&m_gppi_channels[SAADC_SAMPLING]);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    nrfx_gppi_channel_endpoints_setup(m_gppi_channels[SAADC_SAMPLING],
        nrfx_timer_compare_event_address_get(&timer_inst, NRF_TIMER_CC_CHANNEL0),
        nrf_saadc_task_address_get(NRF_SAADC, NRF_SAADC_TASK_SAMPLE));

    /*
     * Allocate a dedicated channel and configure endpoints of that so that the SAADC event end is connected
     * with the SAADC task start. This means that each time the SAADC fills up the result buffer,
     * the SAADC will be restarted and the result buffer will be prepared in RAM.
     */
    status = nrfx_gppi_channel_alloc(&m_gppi_channels[SAADC_START_ON_END]);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    nrfx_gppi_channel_endpoints_setup(m_gppi_channels[SAADC_START_ON_END],
        nrf_saadc_event_address_get(NRF_SAADC, NRF_SAADC_EVENT_END),
        nrf_saadc_task_address_get(NRF_SAADC, NRF_SAADC_TASK_START));

    nrfx_gpiote_t const gpiote_inst = NRFX_GPIOTE_INSTANCE(GPIOTE_INST_IDX);
    status = nrfx_gpiote_init(&gpiote_inst, NRFX_GPIOTE_DEFAULT_CONFIG_IRQ_PRIORITY);
    NRFX_ASSERT(status == NRFX_SUCCESS);
    NRFX_LOG_INFO("GPIOTE status: %s",
                  nrfx_gpiote_init_check(&gpiote_inst) ? "initialized" : "not initialized");

    pin_on_event_toggle_setup(&gpiote_inst, OUT_GPIO_PIN,
                        nrf_saadc_event_address_get(NRF_SAADC, NRF_SAADC_EVENT_RESULTDONE));

    nrfx_timer_enable(&timer_inst);

    nrfx_gppi_channels_enable(NRFX_BIT(m_gppi_channels[SAADC_START_ON_END]));

    status = nrfx_saadc_offset_calibrate(saadc_handler);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    while (1)
    {
        NRFX_EXAMPLE_LOG_PROCESS();
    }
}

/** @} */
