/*
 * Copyright (c) 2015 - 2025, Nordic Semiconductor ASA
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

#include <nrfx.h>

#if NRFX_CHECK(NRFX_PWM_ENABLED)

#if !NRFX_FEATURE_PRESENT(NRFX_PWM, _ENABLED)
#error "No enabled PWM instances. Check <nrfx_config.h>."
#endif

#include <nrfx_pwm.h>
#include <haly/nrfy_gpio.h>

#define NRFX_LOG_MODULE PWM
#include <nrfx_log.h>

#if NRFX_CHECK(NRFX_PWM_NRF52_ANOMALY_109_WORKAROUND_ENABLED)
// The workaround uses interrupts to wake up the CPU and ensure it is active
// when PWM is about to start a DMA transfer. For initial transfer, done when
// a playback is started via PPI, a specific EGU instance is used to generate
// an interrupt. During the playback, the PWM interrupt triggered on SEQEND
// event of a preceding sequence is used to protect the transfer done for
// the next sequence to be played.
#include <hal/nrf_egu.h>
#define USE_DMA_ISSUE_WORKAROUND
#endif
#if defined(USE_DMA_ISSUE_WORKAROUND)
#define EGU_IRQn(i)         EGU_IRQn_(i)
#define EGU_IRQn_(i)        SWI##i##_EGU##i##_IRQn
#define EGU_IRQHandler(i)   EGU_IRQHandler_(i)
#define EGU_IRQHandler_(i)  nrfx_egu_##i##_irq_handler
#define DMA_ISSUE_EGU_IDX           NRFX_PWM_NRF52_ANOMALY_109_EGU_INSTANCE
#define DMA_ISSUE_EGU               NRFX_CONCAT_2(NRF_EGU, DMA_ISSUE_EGU_IDX)
#define DMA_ISSUE_EGU_IRQn          EGU_IRQn(DMA_ISSUE_EGU_IDX)
#define DMA_ISSUE_EGU_IRQHandler    EGU_IRQHandler(DMA_ISSUE_EGU_IDX)
#endif

// Control block - driver instance local data.
typedef struct
{
#if defined(USE_DMA_ISSUE_WORKAROUND)
    uint32_t                  starting_task_address;
#endif
    nrfx_pwm_handler_t        handler;
    void *                    p_context;
    nrfx_drv_state_t volatile state;
    uint32_t                  flags;
    bool                      skip_gpio_cfg;
} pwm_control_block_t;
static pwm_control_block_t m_cb[NRFX_PWM_ENABLED_COUNT];

static void pins_configure(nrfx_pwm_config_t const * p_config)
{
    // Nothing to do here if both GPIO configuration and pin selection are
    // to be skipped (the pin numbers may be then even not specified).
    if (p_config->skip_gpio_cfg && p_config->skip_psel_cfg)
    {
        return;
    }

    for (uint8_t i = 0; i < NRF_PWM_CHANNEL_COUNT; ++i)
    {
        uint32_t output_pin = p_config->output_pins[i];
        if (output_pin != NRF_PWM_PIN_NOT_CONNECTED)
        {
            if (p_config->pin_inverted[i])
            {
                nrfy_gpio_pin_set(output_pin);
            }
            else
            {
                nrfy_gpio_pin_clear(output_pin);
            }

            nrfy_gpio_cfg_output(output_pin);
        }
    }
}

static void pins_deconfigure(nrfx_pwm_t const * p_instance)
{
    for (uint8_t ch_idx = 0; ch_idx < NRF_PWM_CHANNEL_COUNT; ch_idx++)
    {
        uint32_t pin = nrfy_pwm_pin_get(p_instance->p_reg, ch_idx);
        if (pin != NRF_PWM_PIN_NOT_CONNECTED)
        {
            nrfy_gpio_cfg_default(pin);
        }
    }
}

#if defined(USE_DMA_ISSUE_WORKAROUND)
static void apply_errata_109(nrfx_pwm_config_t const * p_config)
{
    /* The workaround for nRF52 Anomaly 109 "protects" DMA transfers by handling
     * interrupts generated on SEQEND0 and SEQEND1 events (this ensures that
     * the 64 MHz clock is ready when data for the next sequence to be played
     * is read). Therefore, the PWM interrupt must be enabled even if the event
     * handler is not used.
     */
    NRFY_IRQ_PRIORITY_SET(DMA_ISSUE_EGU_IRQn, p_config->irq_priority);
    NRFY_IRQ_ENABLE(DMA_ISSUE_EGU_IRQn);
}
#endif

#define PWM_PIN_INIT(i, field) field[i]

static void pwm_configure(nrfx_pwm_t const * p_instance, nrfx_pwm_config_t const * p_config)
{
    if (!p_config->skip_gpio_cfg)
    {
        pins_configure(p_config);
    }

    nrfy_pwm_config_t nrfy_config =
    {
        .output_pins   =
        {
            NRFX_LISTIFY(NRF_PWM_CHANNEL_COUNT, PWM_PIN_INIT, (,), p_config->output_pins)
        },
        .top_value     = p_config->top_value,
        .base_clock    = p_config->base_clock,
        .count_mode    = p_config->count_mode,
        .load_mode     = p_config->load_mode,
        .step_mode     = p_config->step_mode,
        .skip_psel_cfg = p_config->skip_psel_cfg
    };

    nrfy_pwm_periph_configure(p_instance->p_reg, &nrfy_config);
    uint32_t to_clear = NRF_PWM_EVENT_LOOPSDONE | NRF_PWM_EVENT_SEQEND0 |
                        NRF_PWM_EVENT_SEQEND1 | NRF_PWM_EVENT_STOPPED;
    nrfy_pwm_int_init(p_instance->p_reg, to_clear, p_config->irq_priority, false);

#if NRF_PWM_HAS_IDLEOUT
    for (uint8_t channel = 0; channel < NRF_PWM_CHANNEL_COUNT; ++channel)
    {
        nrfy_pwm_channel_idle_set(p_instance->p_reg, channel, p_config->pin_inverted[channel]);
    }
#endif

#if defined(USE_DMA_ISSUE_WORKAROUND)
    apply_errata_109(p_config);
#endif
}

static bool pwm_stopped_check(nrfx_pwm_t const * p_instance)
{
    pwm_control_block_t * p_cb  = &m_cb[p_instance->instance_id];

    if (!p_cb->handler)
    {
        if (nrfy_pwm_events_process(p_instance->p_reg,
                                    NRFY_EVENT_TO_INT_BITMASK(NRF_PWM_EVENT_STOPPED)))
        {
            p_cb->state = NRFX_DRV_STATE_INITIALIZED;
        }
    }

    return p_cb->state != NRFX_DRV_STATE_POWERED_ON;
}

nrfx_err_t nrfx_pwm_init(nrfx_pwm_t const *        p_instance,
                         nrfx_pwm_config_t const * p_config,
                         nrfx_pwm_handler_t        handler,
                         void *                    p_context)
{
    nrfx_err_t err_code;

    pwm_control_block_t * p_cb  = &m_cb[p_instance->instance_id];

    if (p_cb->state != NRFX_DRV_STATE_UNINITIALIZED)
    {
#if NRFX_API_VER_AT_LEAST(3, 2, 0)
        err_code = NRFX_ERROR_ALREADY;
#else
        err_code = NRFX_ERROR_INVALID_STATE;
#endif
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }

    p_cb->handler = handler;
    p_cb->p_context = p_context;

    if (p_config)
    {
        p_cb->skip_gpio_cfg = p_config->skip_gpio_cfg;
        pwm_configure(p_instance, p_config);
    }

    p_cb->state = NRFX_DRV_STATE_INITIALIZED;

    err_code = NRFX_SUCCESS;
    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}

nrfx_err_t nrfx_pwm_reconfigure(nrfx_pwm_t const * p_instance, nrfx_pwm_config_t const * p_config)
{
    pwm_control_block_t * p_cb = &m_cb[p_instance->instance_id];

    NRFX_ASSERT(p_config);

    if (p_cb->state == NRFX_DRV_STATE_UNINITIALIZED)
    {
        return NRFX_ERROR_INVALID_STATE;
    }
    if (p_cb->state == NRFX_DRV_STATE_POWERED_ON)
    {
        return NRFX_ERROR_BUSY;
    }

    pwm_configure(p_instance, p_config);

    return NRFX_SUCCESS;
}

void nrfx_pwm_uninit(nrfx_pwm_t const * p_instance)
{
    pwm_control_block_t * p_cb = &m_cb[p_instance->instance_id];

    NRFX_ASSERT(p_cb->state != NRFX_DRV_STATE_UNINITIALIZED);

    nrfy_pwm_int_uninit(p_instance->p_reg);
#if defined(USE_DMA_ISSUE_WORKAROUND)
    NRFY_IRQ_DISABLE(DMA_ISSUE_EGU_IRQn);
#endif

    nrfy_pwm_disable(p_instance->p_reg);

    if (!p_cb->skip_gpio_cfg)
    {
        pins_deconfigure(p_instance);
    }

    p_cb->state = NRFX_DRV_STATE_UNINITIALIZED;
    NRFX_LOG_INFO("Uninitialized.");
}

bool nrfx_pwm_init_check(nrfx_pwm_t const * p_instance)
{
    pwm_control_block_t * p_cb = &m_cb[p_instance->instance_id];

    return (p_cb->state != NRFX_DRV_STATE_UNINITIALIZED);
}

static uint32_t start_playback(nrfx_pwm_t const *    p_instance,
                               pwm_control_block_t * p_cb,
                               uint32_t              flags,
                               uint8_t               seq_id)
{
    p_cb->state = NRFX_DRV_STATE_POWERED_ON;
    p_cb->flags = flags;

    nrfy_pwm_enable(p_instance->p_reg);

    if (p_cb->handler)
    {
        // The notification about finished playback is by default enabled,
        // but this can be suppressed.
        // The notification that the peripheral has stopped is always enabled.
        uint32_t int_mask = NRF_PWM_INT_LOOPSDONE_MASK |
                            NRF_PWM_INT_STOPPED_MASK;

        // The workaround for nRF52 Anomaly 109 "protects" DMA transfers by
        // handling interrupts generated on SEQEND0 and SEQEND1 events (see
        // 'nrfx_pwm_init'), hence these events must be always enabled
        // to generate interrupts.
        // However, the user handler is called for them only when requested
        // (see 'irq_handler').
#if defined(USE_DMA_ISSUE_WORKAROUND)
        int_mask |= NRF_PWM_INT_SEQEND0_MASK | NRF_PWM_INT_SEQEND1_MASK;
#else
        if (flags & NRFX_PWM_FLAG_SIGNAL_END_SEQ0)
        {
            int_mask |= NRF_PWM_INT_SEQEND0_MASK;
        }
        if (flags & NRFX_PWM_FLAG_SIGNAL_END_SEQ1)
        {
            int_mask |= NRF_PWM_INT_SEQEND1_MASK;
        }
#endif
        if (flags & NRFX_PWM_FLAG_NO_EVT_FINISHED)
        {
            int_mask &= (uint32_t)~NRF_PWM_INT_LOOPSDONE_MASK;
        }

        nrfy_pwm_int_set(p_instance->p_reg, int_mask);
    }
#if defined(USE_DMA_ISSUE_WORKAROUND)
    else
    {
        nrfy_pwm_int_set(p_instance->p_reg,
            NRF_PWM_INT_SEQEND0_MASK | NRF_PWM_INT_SEQEND1_MASK);
    }
#endif
    if (flags & NRFX_PWM_FLAG_START_VIA_TASK)
    {
        uint32_t starting_task_address =
            nrfy_pwm_task_address_get(p_instance->p_reg, nrfy_pwm_seqstart_task_get(seq_id));

#if defined(USE_DMA_ISSUE_WORKAROUND)
        // To "protect" the initial DMA transfer it is required to start
        // the PWM by triggering the proper task from EGU interrupt handler,
        // it is not safe to do it directly via PPI.
        p_cb->starting_task_address = starting_task_address;
        nrf_egu_int_enable(DMA_ISSUE_EGU, nrf_egu_channel_int_get(p_instance->instance_id));
        return nrf_egu_task_address_get(DMA_ISSUE_EGU,
                                        nrf_egu_trigger_task_get(p_instance->instance_id));
#else
        return starting_task_address;
#endif
    }

    nrfy_pwm_start(p_instance->p_reg, seq_id, false);
    return 0;
}

uint32_t nrfx_pwm_simple_playback(nrfx_pwm_t const *         p_instance,
                                  nrf_pwm_sequence_t const * p_sequence,
                                  uint16_t                   playback_count,
                                  uint32_t                   flags)
{
    pwm_control_block_t * p_cb = &m_cb[p_instance->instance_id];

    NRFX_ASSERT(p_cb->state != NRFX_DRV_STATE_UNINITIALIZED);
    NRFX_ASSERT(playback_count > 0);
    NRFX_ASSERT(nrfx_is_in_ram(p_sequence->values.p_raw));

    // To take advantage of the looping mechanism, we need to use both sequences
    // (single sequence can be played back only once).
    nrfy_pwm_sequence_set(p_instance->p_reg, 0, p_sequence);
    nrfy_pwm_sequence_set(p_instance->p_reg, 1, p_sequence);
    bool odd = (playback_count & 1);
    nrfy_pwm_loop_set(p_instance->p_reg,
        (uint16_t)((playback_count / 2UL) + (odd ? 1UL : 0UL)));

    uint32_t shorts_mask = 0;
    if (flags & NRFX_PWM_FLAG_STOP)
    {
        shorts_mask = NRF_PWM_SHORT_LOOPSDONE_STOP_MASK;
    }
    else if (flags & NRFX_PWM_FLAG_LOOP)
    {
#if NRF_PWM_HAS_SHORT_LOOPSDONE_SEQSTART
        shorts_mask = odd ? NRF_PWM_SHORT_LOOPSDONE_SEQSTART1_MASK
                          : NRF_PWM_SHORT_LOOPSDONE_SEQSTART0_MASK;
#else
        NRFX_ASSERT(0);
#endif
    }
    nrfy_pwm_shorts_set(p_instance->p_reg, shorts_mask);

    NRFX_LOG_INFO("Function: %s, sequence length: %d.",
                  __func__,
                  p_sequence->length);
    NRFX_LOG_DEBUG("Sequence data:");
    NRFX_LOG_HEXDUMP_DEBUG((uint8_t *)p_sequence->values.p_raw,
                           p_sequence->length * sizeof(uint16_t));
    return start_playback(p_instance, p_cb, flags, odd ? 1 : 0);
}

uint32_t nrfx_pwm_complex_playback(nrfx_pwm_t const *         p_instance,
                                   nrf_pwm_sequence_t const * p_sequence_0,
                                   nrf_pwm_sequence_t const * p_sequence_1,
                                   uint16_t                   playback_count,
                                   uint32_t                   flags)
{
    pwm_control_block_t * p_cb = &m_cb[p_instance->instance_id];

    NRFX_ASSERT(p_cb->state != NRFX_DRV_STATE_UNINITIALIZED);
    NRFX_ASSERT(playback_count > 0);
    NRFX_ASSERT(nrfx_is_in_ram(p_sequence_0->values.p_raw));
    NRFX_ASSERT(nrfx_is_in_ram(p_sequence_1->values.p_raw));

    nrfy_pwm_sequence_set(p_instance->p_reg, 0, p_sequence_0);
    nrfy_pwm_sequence_set(p_instance->p_reg, 1, p_sequence_1);
    nrfy_pwm_loop_set(p_instance->p_reg, playback_count);

    uint32_t shorts_mask = 0;
    if (flags & NRFX_PWM_FLAG_STOP)
    {
        shorts_mask = NRF_PWM_SHORT_LOOPSDONE_STOP_MASK;
    }
    else if (flags & NRFX_PWM_FLAG_LOOP)
    {
#if NRF_PWM_HAS_SHORT_LOOPSDONE_SEQSTART
        shorts_mask = NRF_PWM_SHORT_LOOPSDONE_SEQSTART0_MASK;
#else
        NRFX_ASSERT(0);
#endif
    }
    nrfy_pwm_shorts_set(p_instance->p_reg, shorts_mask);

    NRFX_LOG_INFO("Function: %s, sequence 0 length: %d.",
                  __func__,
                  p_sequence_0->length);
    NRFX_LOG_INFO("Function: %s, sequence 1 length: %d.",
                  __func__,
                  p_sequence_1->length);
    NRFX_LOG_DEBUG("Sequence 0 data:");
    NRFX_LOG_HEXDUMP_DEBUG(p_sequence_0->values.p_raw,
                           p_sequence_0->length * sizeof(uint16_t));
    NRFX_LOG_DEBUG("Sequence 1 data:");
    NRFX_LOG_HEXDUMP_DEBUG(p_sequence_1->values.p_raw,
                           p_sequence_1->length * sizeof(uint16_t));
    return start_playback(p_instance, p_cb, flags, 0);
}

bool nrfx_pwm_stop(nrfx_pwm_t const * p_instance, bool wait_until_stopped)
{
    pwm_control_block_t * p_cb = &m_cb[p_instance->instance_id];

    NRFX_ASSERT(p_cb->state != NRFX_DRV_STATE_UNINITIALIZED);

    bool ret_val = false;

    // Deactivate shortcuts before triggering the STOP task, otherwise the PWM
    // could be immediately started again if the LOOPSDONE event occurred in
    // the same peripheral clock cycle as the STOP task was triggered.
    nrfy_pwm_shorts_set(p_instance->p_reg, 0);

    // Trigger the STOP task even if the PWM appears to be already stopped.
    // It won't harm, but it could help if for some strange reason the stopped
    // status was not reported correctly.
    nrfy_pwm_abort(p_instance->p_reg, false);

    if (wait_until_stopped)
    {
        while (!pwm_stopped_check(p_instance))
        {}
        nrfy_pwm_disable(p_instance->p_reg);
        p_cb->state = NRFX_DRV_STATE_INITIALIZED;
    }

    ret_val = pwm_stopped_check(p_instance);

    NRFX_LOG_INFO("%s returned %d.", __func__, ret_val);
    return ret_val;
}

bool nrfx_pwm_stopped_check(nrfx_pwm_t const * p_instance)
{
    NRFX_ASSERT(m_cb[p_instance->instance_id].state != NRFX_DRV_STATE_UNINITIALIZED);

    bool ret_val = pwm_stopped_check(p_instance);

    NRFX_LOG_INFO("%s returned %d.", __func__, ret_val);
    return ret_val;
}

static void irq_handler(NRF_PWM_Type * p_pwm, pwm_control_block_t * p_cb)
{
    uint32_t evt_mask = nrfy_pwm_events_process(p_pwm,
                                                NRFY_EVENT_TO_INT_BITMASK(NRF_PWM_EVENT_SEQEND0)   |
                                                NRFY_EVENT_TO_INT_BITMASK(NRF_PWM_EVENT_SEQEND1)   |
                                                NRFY_EVENT_TO_INT_BITMASK(NRF_PWM_EVENT_LOOPSDONE) |
                                                NRFY_EVENT_TO_INT_BITMASK(NRF_PWM_EVENT_STOPPED));

    // The user handler is called for SEQEND0 and SEQEND1 events only when the
    // user asks for it (by setting proper flags when starting the playback).
    if (evt_mask & NRFY_EVENT_TO_INT_BITMASK(NRF_PWM_EVENT_SEQEND0))
    {
        if ((p_cb->flags & NRFX_PWM_FLAG_SIGNAL_END_SEQ0) && p_cb->handler)
        {
            p_cb->handler(NRFX_PWM_EVT_END_SEQ0, p_cb->p_context);
        }
    }
    if (evt_mask & NRFY_EVENT_TO_INT_BITMASK(NRF_PWM_EVENT_SEQEND1))
    {
        if ((p_cb->flags & NRFX_PWM_FLAG_SIGNAL_END_SEQ1) && p_cb->handler)
        {
            p_cb->handler(NRFX_PWM_EVT_END_SEQ1, p_cb->p_context);
        }
    }
    // For LOOPSDONE the handler is called by default, but the user can disable
    // this (via flags).
    if (evt_mask & NRFY_EVENT_TO_INT_BITMASK(NRF_PWM_EVENT_LOOPSDONE))
    {
        if (!(p_cb->flags & NRFX_PWM_FLAG_NO_EVT_FINISHED) && p_cb->handler)
        {
            p_cb->handler(NRFX_PWM_EVT_FINISHED, p_cb->p_context);
        }
    }

    // The STOPPED event is always propagated to the user handler.
    if (evt_mask & NRFY_EVENT_TO_INT_BITMASK(NRF_PWM_EVENT_STOPPED))
    {
        nrfy_pwm_disable(p_pwm);
        p_cb->state = NRFX_DRV_STATE_INITIALIZED;
        if (p_cb->handler)
        {
            p_cb->handler(NRFX_PWM_EVT_STOPPED, p_cb->p_context);
        }
    }
}

#if defined(USE_DMA_ISSUE_WORKAROUND)
// See 'start_playback' why this is needed.
void DMA_ISSUE_EGU_IRQHandler(void)
{
    for (uint8_t i = 0; i < NRFX_PWM_ENABLED_COUNT; i++)
    {
        nrf_egu_event_t event = nrf_egu_triggered_event_get(i);
        if (nrf_egu_event_check(DMA_ISSUE_EGU, event))
        {
            nrf_egu_event_clear(DMA_ISSUE_EGU, event);
            *(volatile uint32_t *)(m_cb[i].starting_task_address) = 1;
        }
    }
}
#endif

NRFX_INSTANCE_IRQ_HANDLERS(PWM, pwm)

#endif // NRFX_CHECK(NRFX_PWM_ENABLED)
