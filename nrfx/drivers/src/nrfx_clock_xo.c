/*
 * Copyright (c) 2016 - 2025, Nordic Semiconductor ASA
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

#if NRFX_CHECK(NRFX_CLOCK_ENABLED)

#include <nrfx_clock.h>
#include <nrfx_clock_xo.h>
#include <nrf_erratas.h>

#define NRFX_LOG_MODULE CLOCK_XO
#include <nrfx_log.h>

#if !defined(USE_WORKAROUND_FOR_ANOMALY_201) &&          \
    (defined(NRF52810_XXAA) ||                           \
     defined(NRF52832_XXAA) || defined(NRF52832_XXAB) || \
     defined(NRF52840_XXAA))
// Enable workaround for nRF52 anomaly 201 (EVENTS_HFCLKSTARTED might be generated twice).
#define USE_WORKAROUND_FOR_ANOMALY_201 1
#endif

#define INTERRUPT_MASK                                                                                                                                              \
    NRFX_COND_CODE_1(NRF_CLOCK_HAS_HFDOMAIN, (NRF_CLOCK_INT_HF_STARTED_MASK |), ())                                                                                  \
    NRFX_COND_CODE_1(NRF_CLOCK_HAS_XO_TUNE, (NRF_CLOCK_INT_XOTUNED_MASK | NRF_CLOCK_INT_XOTUNEERROR_MASK | NRF_CLOCK_INT_XOTUNEFAILED_MASK |), ())                  \
    0

#if NRFX_CHECK(NRF_CLOCK_HAS_XO_TUNE)
typedef enum
{
    XO_STATE_NOT_TUNED,
    XO_STATE_TUNING,
    XO_STATE_TUNED
} nrfx_clock_xo_state_t;
#endif

#define NRFX_CLOCK_EVT2STR(evt_type)                                                                            \
    evt_type == NRFX_CLOCK_XO_EVT_HFCLK_STARTED ? "XO_STARTED" :                                                \
    NRFX_COND_CODE_1(NRF_CLOCK_HAS_PLL, (evt_type == NRFX_CLOCK_XO_EVT_PLL_STARTED ? "PLL_STARTED" :), ())      \
    NRFX_COND_CODE_1(NRF_CLOCK_HAS_XO_TUNE, (evt_type == NRFX_CLOCK_XO_EVT_XO_TUNED ? "XO_TUNED" :              \
        evt_type == NRFX_CLOCK_XO_EVT_XO_TUNE_ERROR ? "XO_TUNE_ERROR" :                                         \
        evt_type == NRFX_CLOCK_XO_EVT_XO_TUNE_FAILED  ? "XO_TUNE_FAILED" :), ()) "Unknown"

/** @brief CLOCK control block. */
typedef struct
{
    nrfx_clock_xo_event_handler_t event_handler;
    bool module_initialized; /*< Indicate the state of module */
#if NRFX_CHECK(USE_WORKAROUND_FOR_ANOMALY_201)
    bool hfclk_started; /*< Anomaly 201 workaround. */
#endif

#if NRFX_CHECK(NRF_CLOCK_HAS_XO_TUNE)
    volatile nrfx_clock_xo_state_t xo_state;
#endif
} nrfx_clock_xo_cb_t;

static nrfx_clock_xo_cb_t m_clock_cb;

static void clock_stop(void)
{
    nrf_clock_int_disable(NRF_CLOCK, INTERRUPT_MASK);
    nrf_clock_task_trigger(NRF_CLOCK, NRF_CLOCK_TASK_HFCLKSTOP);
    nrf_clock_event_clear(NRF_CLOCK, NRF_CLOCK_EVENT_HFCLKSTARTED);
#if NRFX_CHECK(NRF54L_ERRATA_39_ENABLE_WORKAROUND)
    if (nrf54l_errata_39())
    {
        nrf_clock_task_trigger(NRF_CLOCK, NRF_CLOCK_TASK_PLLSTOP);
    }
#endif

    bool stopped;
    nrf_clock_hfclk_t clk_src = NRF_CLOCK_HFCLK_HIGH_ACCURACY;
    NRFX_WAIT_FOR(!nrfx_clock_xo_running_check(&clk_src), 10000, 1, stopped);
    if (!stopped)
    {
        NRFX_LOG_ERROR("Failed to stop clock XO.");
    }
    else
    {
#if NRF_CLOCK_HAS_XO_TUNE
        m_clock_cb.xo_state = XO_STATE_NOT_TUNED;
#endif
#if NRFX_CHECK(USE_WORKAROUND_FOR_ANOMALY_201)
        m_clock_cb.hfclk_started = false;
#endif
    }
}

nrfx_err_t nrfx_clock_xo_init(nrfx_clock_xo_event_handler_t event_handler)
{
    nrfx_err_t err_code = NRFX_SUCCESS;
    if (m_clock_cb.module_initialized)
    {
        err_code = NRFX_ERROR_ALREADY;
        NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }
    else
    {
        m_clock_cb.event_handler = event_handler;
        m_clock_cb.module_initialized = true;
#if NRFX_CHECK(USE_WORKAROUND_FOR_ANOMALY_201)
        m_clock_cb.hfclk_started = false;
#endif
    }

    return err_code;
}

void nrfx_clock_xo_uninit(void)
{
    NRFX_ASSERT(m_clock_cb.module_initialized);
    clock_stop();

    m_clock_cb.module_initialized = false;
    NRFX_LOG_INFO("Uninitialized.");
}

bool nrfx_clock_xo_init_check(void)
{
    return m_clock_cb.module_initialized;
}

void nrfx_clock_xo_start(void)
{
    uint32_t int_mask;

    NRFX_ASSERT(m_clock_cb.module_initialized);

    int_mask = NRF_CLOCK_INT_HF_STARTED_MASK |
#if NRF_CLOCK_HAS_XO_TUNE
               NRF_CLOCK_INT_XOTUNED_MASK |
               NRF_CLOCK_INT_XOTUNEERROR_MASK |
               NRF_CLOCK_INT_XOTUNEFAILED_MASK |
#endif
               0;
#if NRF_CLOCK_HAS_XO_TUNE
    nrf_clock_event_clear(NRF_CLOCK, NRF_CLOCK_EVENT_XOTUNED);
    nrf_clock_event_clear(NRF_CLOCK, NRF_CLOCK_EVENT_XOTUNEFAILED);
    nrf_clock_event_clear(NRF_CLOCK, NRF_CLOCK_EVENT_XOTUNEERROR);
#endif

#if NRFX_CHECK(NRF54L_ERRATA_39_ENABLE_WORKAROUND)
    if (nrf54l_errata_39())
    {
        nrf_clock_task_trigger(NRF_CLOCK, NRF_CLOCK_TASK_PLLSTART);
    }
#endif

    nrf_clock_event_clear(NRF_CLOCK, NRF_CLOCK_EVENT_HFCLKSTARTED);
    nrf_clock_task_trigger(NRF_CLOCK, NRF_CLOCK_TASK_HFCLKSTART);
    if (m_clock_cb.event_handler)
    {
        nrf_clock_int_enable(NRF_CLOCK, INTERRUPT_MASK);
    }
    else
    {
        while (!nrf_clock_event_check(NRF_CLOCK, NRF_CLOCK_EVENT_HFCLKSTARTED))
        {
        }
        nrf_clock_event_clear(NRF_CLOCK, NRF_CLOCK_EVENT_HFCLKSTARTED);
    }
}

void nrfx_clock_xo_stop(void)
{
    NRFX_ASSERT(m_clock_cb.module_initialized);

    clock_stop();
}

#if NRF_CLOCK_HAS_XO_TUNE
nrfx_err_t nrfx_clock_xo_tune_start(void)
{
    nrf_clock_hfclk_t hfclksrc = nrf_clock_hf_src_get(NRF_CLOCK);
    if ((hfclksrc != NRF_CLOCK_HFCLK_HIGH_ACCURACY) || (m_clock_cb.xo_state == XO_STATE_TUNING))
    {
        return NRFX_ERROR_INVALID_STATE;
    }

    nrf_clock_event_clear(NRF_CLOCK, NRF_CLOCK_EVENT_XOTUNED);
    nrf_clock_event_clear(NRF_CLOCK, NRF_CLOCK_EVENT_XOTUNEFAILED);

    if (m_clock_cb.event_handler)
    {
        uint32_t int_mask = NRF_CLOCK_INT_XOTUNED_MASK | NRF_CLOCK_INT_XOTUNEFAILED_MASK;
        nrf_clock_int_enable(NRF_CLOCK, int_mask);
    }

    // XOTUNEERROR can occur at any moment and it is not related to this operation
    m_clock_cb.xo_state = XO_STATE_TUNING;
    nrf_clock_task_trigger(NRF_CLOCK, NRF_CLOCK_TASK_XOTUNE);

    if (!m_clock_cb.event_handler)
    {
        bool evt_xotuned;
        bool evt_xotunefailed;
        do
        {
            evt_xotuned = nrf_clock_event_check(NRF_CLOCK, NRF_CLOCK_EVENT_XOTUNED);
            evt_xotunefailed = nrf_clock_event_check(NRF_CLOCK, NRF_CLOCK_EVENT_XOTUNEFAILED);
        } while (!(evt_xotuned | evt_xotunefailed));
        m_clock_cb.xo_state = evt_xotuned ? XO_STATE_TUNED : XO_STATE_NOT_TUNED;

        nrf_clock_event_clear(NRF_CLOCK, NRF_CLOCK_EVENT_XOTUNED);
        nrf_clock_event_clear(NRF_CLOCK, NRF_CLOCK_EVENT_XOTUNEFAILED);

        if (evt_xotunefailed)
        {
            return NRFX_ERROR_INTERNAL;
        }
    }

    return NRFX_SUCCESS;
}

nrfx_err_t nrfx_clock_xo_tune_abort(void)
{
    nrf_clock_hfclk_t hfclksrc = nrf_clock_hf_src_get(NRF_CLOCK);
    if ((hfclksrc != NRF_CLOCK_HFCLK_HIGH_ACCURACY) || (m_clock_cb.xo_state != XO_STATE_TUNING))
    {
        return NRFX_ERROR_FORBIDDEN;
    }

    nrf_clock_task_trigger(NRF_CLOCK, NRF_CLOCK_TASK_XOTUNEABORT);
    m_clock_cb.xo_state = XO_STATE_NOT_TUNED;

    if (m_clock_cb.event_handler)
    {
        uint32_t int_mask = NRF_CLOCK_INT_XOTUNED_MASK | NRF_CLOCK_INT_XOTUNEFAILED_MASK;
        nrf_clock_int_disable(NRF_CLOCK, int_mask);
    }

    return NRFX_SUCCESS;
}

bool nrfx_clock_xo_tune_error_check(void)
{
    NRFX_ASSERT(!m_clock_cb.event_handler);

    bool quality_issue = nrf_clock_event_check(NRF_CLOCK, NRF_CLOCK_EVENT_XOTUNEERROR);
    if (quality_issue)
    {
        nrf_clock_event_clear(NRF_CLOCK, NRF_CLOCK_EVENT_XOTUNEERROR);
    }
    return quality_issue;
}

bool nrfx_clock_xo_tune_status_check(void)
{
    return m_clock_cb.xo_state == XO_STATE_TUNED;
}

#endif // NRF_CLOCK_HAS_XO

void nrfx_clock_xo_irq_handler(void)
{

#if NRF_CLOCK_HAS_INTPEND
    uint32_t intpend = nrf_clock_int_pending_get(NRF_CLOCK);
#else
    uint32_t intpend = nrf_clock_int_enable_check(NRF_CLOCK, UINT32_MAX);
#endif

    intpend &= INTERRUPT_MASK;

    while (intpend != 0)
    {
        uint32_t int_bit = 31 - NRF_CLZ(intpend);
        nrf_clock_int_mask_t int_mask = (nrf_clock_int_mask_t)NRFX_BIT(int_bit);
        nrf_clock_event_t evt = (nrf_clock_event_t)NRFY_INT_BITPOS_TO_EVENT(int_bit);
        nrfx_clock_xo_evt_type_t evt_type = (nrfx_clock_xo_evt_type_t)int_bit;
        bool call_handler = true;
#if !NRF_CLOCK_HAS_INTPEND
        intpend &= ~int_mask;
        // Check if event is set for that interrupt and if not continue.
        if (!nrf_clock_event_check(NRF_CLOCK, evt))
        {
            continue;
        }
#endif

        nrf_clock_event_clear(NRF_CLOCK, evt);
        nrf_clock_int_disable(NRF_CLOCK, int_mask);

        NRFX_LOG_DEBUG("Event: %s", NRFX_CLOCK_EVT2STR(evt_type));
        switch (int_mask)
        {
        case NRF_CLOCK_INT_HF_STARTED_MASK:
#if NRFX_CHECK(USE_WORKAROUND_FOR_ANOMALY_201)
            if (!m_clock_cb.hfclk_started)
            {
                m_clock_cb.hfclk_started = true;
            }
            else
            {
                call_handler = false;
            }
#endif
            break;
#if NRFX_CHECK(NRF_CLOCK_HAS_XO_TUNE)
        case NRF_CLOCK_INT_XOTUNED_MASK:
            m_clock_cb.xo_state = XO_STATE_TUNED;
            // Enable XOTUNEERROR interrupt to handle situation when XO is out of tune.
            nrf_clock_event_clear(NRF_CLOCK, NRF_CLOCK_EVENT_XOTUNEERROR);
            nrf_clock_int_enable(NRF_CLOCK, NRF_CLOCK_INT_XOTUNEERROR_MASK);
            break;
        case NRF_CLOCK_INT_XOTUNEERROR_MASK:
            break;
        case NRF_CLOCK_INT_XOTUNEFAILED_MASK:
            m_clock_cb.xo_state = XO_STATE_NOT_TUNED;
            break;
#endif
        default:
            NRFX_ASSERT(0);
            break;
        }

        if (call_handler)
        {
            m_clock_cb.event_handler(evt_type);
        }
#if NRF_CLOCK_HAS_INTPEND
        intpend = nrf_clock_int_pending_get(NRF_CLOCK);

        intpend &= INTERRUPT_MASK;
#endif
    }
}

#endif // NRFX_CHECK(NRFX_CLOCK_ENABLED)
