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
#include <nrf_erratas.h>

#define NRFX_LOG_MODULE CLOCK
#include <nrfx_log.h>

#if NRFX_CHECK(NRFX_POWER_ENABLED)
extern bool nrfx_power_irq_enabled;
#endif

#if defined(CLOCK_LFCLKSRC_SRC_RC) || defined(__NRFX_DOXYGEN__)
    #define LF_SRC_RC CLOCK_LFCLKSRC_SRC_RC
#elif defined(CLOCK_LFCLKSRC_SRC_LFRC)
    #define LF_SRC_RC CLOCK_LFCLKSRC_SRC_LFRC
#else
    #define LF_SRC_RC CLOCK_LFCLK_SRC_SRC_LFRC
#endif
#if NRFX_CHECK(NRFX_CLOCK_CONFIG_LF_CAL_ENABLED)
    #if (NRF_CLOCK_HAS_CALIBRATION == 0 && NRFX_CHECK(NRF_LFRC_HAS_CALIBRATION) == 0)
        #error "Calibration is not available in the SoC that is used."
    #endif
    #if (NRFX_CLOCK_CONFIG_LF_SRC != LF_SRC_RC)
        #error "Calibration can be performed only for the RC Oscillator."
    #endif
#endif

#if !defined(USE_WORKAROUND_FOR_ANOMALY_132) && \
    (defined(NRF52832_XXAA) || defined(NRF52832_XXAB))
    // ANOMALY 132 - LFCLK needs to avoid frame from 66us to 138us after LFCLK stop. This solution
    //               applies delay of 138us before starting LFCLK.
    #define USE_WORKAROUND_FOR_ANOMALY_132 1

    // Convert time to cycles (nRF52832 is clocked with 64 MHz, use delay of 138 us).
    #define ANOMALY_132_DELAY_CYCLES (64UL * 138)
#endif

#if !defined(USE_WORKAROUND_FOR_ANOMALY_192) && \
    (defined(NRF52810_XXAA) || \
     defined(NRF52832_XXAA) || defined(NRF52832_XXAB) || \
     defined(NRF52840_XXAA))
    // Enable workaround for nRF52 anomaly 192 (LFRC oscillator frequency is wrong
    // after calibration, exceeding 500 ppm).
    #define USE_WORKAROUND_FOR_ANOMALY_192 1
#endif

#if !defined(USE_WORKAROUND_FOR_ANOMALY_201) && \
    (defined(NRF52810_XXAA) || \
     defined(NRF52832_XXAA) || defined(NRF52832_XXAB) || \
     defined(NRF52840_XXAA))
    // Enable workaround for nRF52 anomaly 201 (EVENTS_HFCLKSTARTED might be generated twice).
    #define USE_WORKAROUND_FOR_ANOMALY_201 1
#endif

#if defined(CLOCK_LFCLKSRC_SRC_Xtal)
    #define LF_SRC_LFXO CLOCK_LFCLKSRC_SRC_Xtal
#elif NRF_CLOCK_HAS_LFCLK_TYPE
    #define LF_SRC_LFXO CLOCK_LFCLK_SRC_SRC_LFXO
#else
    #define LF_SRC_LFXO CLOCK_LFCLKSRC_SRC_LFXO
#endif

#if defined(NRF_CLOCK_USE_EXTERNAL_LFCLK_SOURCES)
    #define LF_SRC_XTAL_LOW  (CLOCK_LFCLKSRC_SRC_Xtal | \
                             (CLOCK_LFCLKSRC_EXTERNAL_Enabled << CLOCK_LFCLKSRC_EXTERNAL_Pos))
    #define LF_SRC_XTAL_FULL (CLOCK_LFCLKSRC_SRC_Xtal | \
                             (CLOCK_LFCLKSRC_BYPASS_Enabled   << CLOCK_LFCLKSRC_BYPASS_Pos) | \
                             (CLOCK_LFCLKSRC_EXTERNAL_Enabled << CLOCK_LFCLKSRC_EXTERNAL_Pos))
#else
    #define LF_SRC_XTAL_LOW  LF_SRC_LFXO
    #define LF_SRC_XTAL_FULL LF_SRC_LFXO
#endif

#if NRFX_CHECK(NRFX_CLOCK_CONFIG_LFXO_TWO_STAGE_ENABLED) && \
    NRFX_CLOCK_CONFIG_LF_SRC != LF_SRC_LFXO && \
    NRFX_CLOCK_CONFIG_LF_SRC != LF_SRC_XTAL_LOW && \
    NRFX_CLOCK_CONFIG_LF_SRC != LF_SRC_XTAL_FULL
    #error "Two-stage LFXO start procedure enabled but LFCLK source is not set to LFXO!"
#endif

#if !defined(NRFX_CLOCK_CONFIG_CT_ENABLED) && NRF_CLOCK_HAS_CALIBRATION_TIMER
#define NRFX_CLOCK_CONFIG_CT_ENABLED 1
#endif

#if NRFX_CHECK(NRFX_CLOCK_CONFIG_CT_ENABLED) && !NRF_CLOCK_HAS_CALIBRATION_TIMER
    #error "Calibration timer is not available in the SoC that is used."
#endif

#define INTERRUPT_MASK                                                                                                                                              \
    NRF_CLOCK_INT_LF_STARTED_MASK |                                                                                                                                 \
    NRFX_COND_CODE_1(NRF_CLOCK_HAS_LFCLK_SRC_CHANGED, (NRF_CLOCK_INT_LF_SRC_CHANGED_MASK |), ())                                                                    \
    NRFX_COND_CODE_1(NRF_CLOCK_HAS_CALIBRATION, (NRF_CLOCK_INT_DONE_MASK |), ())                                                                                    \
    NRFX_COND_CODE_1(NRF_CLOCK_HAS_CALIBRATION_TIMER, (NRF_CLOCK_INT_CTTO_MASK |), ())                                                                              \
    NRFX_COND_CODE_1(defined(CLOCK_INTENSET_CTSTARTED_Msk) || defined(__NRFX_DOXYGEN__), (NRF_CLOCK_INT_CTSTARTED_MASK | NRF_CLOCK_INT_CTSTOPPED_MASK |), ())       \
    NRFX_COND_CODE_1(NRF_CLOCK_HAS_HFCLKAUDIO, (NRF_CLOCK_INT_HFAUDIO_STARTED_MASK |), ())                                                                          \
    NRFX_COND_CODE_1(NRF_CLOCK_HAS_HFCLK24M, (NRF_CLOCK_INT_HFCLK24M_STARTED_MASK |), ())                                                                           \
    NRFX_COND_CODE_1(NRF_CLOCK_HAS_HFCLK192M, (NRF_CLOCK_INT_HF192M_STARTED_MASK |), ())                                                                            \
    0

#if NRFX_CHECK(NRFX_CLOCK_CONFIG_LF_CAL_ENABLED)
typedef enum
{
    CAL_STATE_IDLE,
    CAL_STATE_CAL
} nrfx_clock_cal_state_t;
#endif

#if NRFX_CHECK(NRF_CLOCK_HAS_XO_TUNE)
typedef enum
{
    XO_STATE_NOT_TUNED,
    XO_STATE_TUNING,
    XO_STATE_TUNED
} nrfx_clock_xo_state_t;
#endif

#define NRFX_CLOCK_EVT2STR(evt_type)                                                        \
    evt_type == NRFX_CLOCK_EVT_HFCLK_STARTED ? "HFCLK_STARTED" :                            \
    NRFX_COND_CODE_1(NRF_CLOCK_HAS_PLL,                                                     \
            (evt_type == NRFX_CLOCK_EVT_PLL_STARTED ? "PLL_STARTED" : ), ())                \
    evt_type == NRFX_CLOCK_EVT_LFCLK_STARTED ? "LFCLK_STARTED" :                            \
    NRFX_COND_CODE_1(NRF_CLOCK_HAS_CALIBRATION_TIMER,                                       \
            (evt_type == NRFX_CLOCK_EVT_CTTO ? "CTTO" : ), ())                              \
    NRFX_COND_CODE_1(NRF_CLOCK_HAS_CALIBRATION,                                             \
            (evt_type == NRFX_CLOCK_EVT_CAL_DONE ? "CAL_DONE" : ), ())                      \
    NRFX_COND_CODE_1(NRF_CLOCK_HAS_HFCLKAUDIO,                                              \
            (evt_type == NRFX_CLOCK_EVT_HFCLKAUDIO_STARTED ? "HFAUDIO_STARTED" : ), ())     \
    NRFX_COND_CODE_1(NRF_CLOCK_HAS_HFCLK24M,                                                \
            (evt_type == NRFX_CLOCK_EVT_HFCLK24M_STARTED ? "HFCLK24M_STARTED" : ), ())      \
    NRFX_COND_CODE_1(NRF_CLOCK_HAS_HFCLK192M,                                               \
            (evt_type == NRFX_CLOCK_HFCLK192M_EVT_HFCLK192M_STARTED ? "HF192M_STARTED" : ), ())       \
    NRFX_COND_CODE_1(NRF_CLOCK_HAS_XO_TUNE,                                                 \
            (evt_type == NRFX_CLOCK_EVT_XO_TUNED ? "XO_TUNED" :                             \
             evt_type == NRFX_CLOCK_EVT_XO_TUNE_ERROR ? "XO_TUNE_ERROR" :                   \
             evt_type == NRFX_CLOCK_EVT_XO_TUNE_FAILED ? "XO_TUNE_FAILED" :), ()) "Unknown"

/** @brief CLOCK control block. */
typedef struct
{
    nrfx_clock_hfclks192m_event_handler_t      event_handler;
    bool                            module_initialized; /*< Indicate the state of module */
#if NRFX_CHECK(USE_WORKAROUND_FOR_ANOMALY_201)
    bool                            hfclk_started;      /*< Anomaly 201 workaround. */
#endif

#if NRFX_CHECK(NRFX_CLOCK_CONFIG_LF_CAL_ENABLED)
    volatile nrfx_clock_cal_state_t cal_state;
#endif

#if NRFX_CHECK(NRF_CLOCK_HAS_XO_TUNE)
    volatile nrfx_clock_xo_state_t xo_state;
#endif
} nrfx_clock_cb_t;

static nrfx_clock_cb_t m_clock_cb;

/**
 * This variable is used to check whether common POWER_CLOCK common interrupt
 * should be disabled or not if @ref nrfx_power tries to disable the interrupt.
 */
#if NRFX_CHECK(NRFX_POWER_ENABLED)
bool nrfx_clock_irq_enabled;
#endif

#if NRFX_CHECK(USE_WORKAROUND_FOR_ANOMALY_132)
/**
 * @brief Function for applying delay of 138us before starting LFCLK.
 */
static void nrfx_clock_anomaly_132(void)
{
    uint32_t cyccnt_inital;
    uint32_t core_debug;
    uint32_t dwt_ctrl;

    // Preserve DEMCR register to do not influence into its configuration. Enable the trace and
    // debug blocks. It is required to read and write data to DWT block.
    core_debug = CoreDebug->DEMCR;
    CoreDebug->DEMCR = core_debug | CoreDebug_DEMCR_TRCENA_Msk;

    // Preserve CTRL register in DWT block to do not influence into its configuration. Make sure
    // that cycle counter is enabled.
    dwt_ctrl = DWT->CTRL;
    DWT->CTRL = dwt_ctrl | DWT_CTRL_CYCCNTENA_Msk;

    // Store start value of cycle counter.
    cyccnt_inital = DWT->CYCCNT;

    // Delay required time.
    while ((DWT->CYCCNT - cyccnt_inital) < ANOMALY_132_DELAY_CYCLES)
    {}

    // Restore preserved registers.
    DWT->CTRL = dwt_ctrl;
    CoreDebug->DEMCR = core_debug;
}
#endif // NRFX_CHECK(USE_WORKAROUND_FOR_ANOMALY_132)

static void clock_stop(nrf_clock_domain_t domain)
{
    uint32_t          int_mask;
    nrf_clock_event_t event;
    nrf_clock_task_t  task;
    switch (domain)
    {
        case NRF_CLOCK_DOMAIN_LFCLK:
            int_mask = NRF_CLOCK_INT_LF_STARTED_MASK;
            task = NRF_CLOCK_TASK_LFCLKSTOP;
            event = NRF_CLOCK_EVENT_LFCLKSTARTED;
            break;
        case NRF_CLOCK_DOMAIN_HFCLK:
            int_mask = NRF_CLOCK_INT_HF_STARTED_MASK |
#if NRF_CLOCK_HAS_XO_TUNE
                       NRF_CLOCK_INT_XOTUNED_MASK |
                       NRF_CLOCK_INT_XOTUNEFAILED_MASK |
                       NRF_CLOCK_INT_XOTUNEERROR_MASK |
#endif
                       0;
            task = NRF_CLOCK_TASK_HFCLKSTOP;
            event = NRF_CLOCK_EVENT_HFCLKSTARTED;
            break;
#if NRF_CLOCK_HAS_HFCLK192M
        case NRF_CLOCK_DOMAIN_HFCLK192M:
            int_mask = NRF_CLOCK_INT_HF192M_STARTED_MASK;
            task = NRF_CLOCK_TASK_HFCLK192MSTOP;
            event = NRF_CLOCK_EVENT_HFCLK192MSTARTED;
            break;
#endif
#if NRF_CLOCK_HAS_HFCLKAUDIO
        case NRF_CLOCK_DOMAIN_HFCLKAUDIO:
            int_mask = NRF_CLOCK_INT_HFAUDIO_STARTED_MASK;
            task = NRF_CLOCK_TASK_HFCLKAUDIOSTOP;
            event = NRF_CLOCK_EVENT_HFCLKAUDIOSTARTED;
            break;
#endif
#if NRF_CLOCK_HAS_HFCLK24M
        case NRF_CLOCK_DOMAIN_HFCLK24M:
            int_mask = NRF_CLOCK_INT_HFCLK24M_STARTED_MASK;
            task = NRF_CLOCK_TASK_HFCLK24MSTOP;
            event = NRF_CLOCK_EVENT_HFCLK24MSTARTED;
            break;
#endif
        default:
            NRFX_ASSERT(0);
            return;
    }

    nrf_clock_int_disable(NRF_CLOCK, int_mask);
    nrf_clock_task_trigger(NRF_CLOCK, task);
    nrf_clock_event_clear(NRF_CLOCK, event);
#if NRFX_CHECK(NRF54L_ERRATA_39_ENABLE_WORKAROUND)
    if (nrf54l_errata_39() && (domain == NRF_CLOCK_DOMAIN_HFCLK))
    {
        nrf_clock_task_trigger(NRF_CLOCK, NRF_CLOCK_TASK_PLLSTOP);
    }
#endif

    bool stopped;
    nrf_clock_hfclk_t clk_src = NRF_CLOCK_HFCLK_HIGH_ACCURACY;
    nrf_clock_hfclk_t *p_clk_src = (domain == NRF_CLOCK_DOMAIN_HFCLK) ? &clk_src : NULL;
    NRFX_WAIT_FOR((!nrfx_clock_is_running(domain, p_clk_src) ||
                      (p_clk_src && clk_src != NRF_CLOCK_HFCLK_HIGH_ACCURACY)), 10000, 1, stopped);
    if (!stopped)
    {
        NRFX_LOG_ERROR("Failed to stop clock domain: %d.", domain);
    }
    else if (domain == NRF_CLOCK_DOMAIN_HFCLK)
    {
#if NRF_CLOCK_HAS_XO_TUNE
        m_clock_cb.xo_state = XO_STATE_NOT_TUNED;
#endif
#if NRFX_CHECK(USE_WORKAROUND_FOR_ANOMALY_201)
        m_clock_cb.hfclk_started = false;
#endif
    }
}

static nrf_clock_lfclk_t clock_initial_lfclksrc_get(void)
{
#if NRFX_CHECK(NRFX_CLOCK_CONFIG_LFXO_TWO_STAGE_ENABLED)
    return NRF_CLOCK_LFCLK_RC;
#else
    return (nrf_clock_lfclk_t)NRFX_CLOCK_CONFIG_LF_SRC;
#endif
}

#if NRF_CLOCK_HAS_HFCLK
static void hfclk_event_handler(void)
{
    m_clock_cb.event_handler(NRFX_CLOCK_EVT_HFCLK_STARTED);
}
#endif // NRF_CLOCK_HAS_HFCLK

#if NRF_CLOCK_HAS_XO
static void xo_event_handler(nrfx_clock_xo_evt_type_t event)
{
    m_clock_cb.event_handler((nrfx_clock_hfclk192m_evt_type_t)event);
}
#endif // NRF_CLOCK_HAS_XO

static void lfclk_event_handler(nrfx_clock_lfclk_evt_type_t event)
{
    m_clock_cb.event_handler((nrfx_clock_hfclk192m_evt_type_t)event);
}

nrfx_err_t nrfx_clock_init(nrfx_clock_hfclks192m_event_handler_t event_handler)
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
#if NRFX_CHECK(NRFX_CLOCK_CONFIG_LF_CAL_ENABLED)
        m_clock_cb.cal_state = CAL_STATE_IDLE;
#endif
        m_clock_cb.event_handler = event_handler;
        m_clock_cb.module_initialized = true;
#if NRFX_CHECK(USE_WORKAROUND_FOR_ANOMALY_201)
        m_clock_cb.hfclk_started = false;
#endif
    }
//Temp code, until nordic_nrf_clock compat is present
#if CONFIG_CLOCK_CONTROL_NRF_COMMON
    return err_code;
#endif

#if NRF_CLOCK_HAS_HFCLK
        err_code |= m_clock_cb.event_handler ?  nrfx_clock_hfclk_init(&hfclk_event_handler) :
                                                nrfx_clock_hfclk_init(NULL);
#endif // NRF_CLOCK_HAS_HFCLK

#if NRF_CLOCK_HAS_XO
        err_code |= m_clock_cb.event_handler ?  nrfx_clock_xo_init(&xo_event_handler) :
                                                nrfx_clock_xo_init(NULL);
#endif // NRF_CLOCK_HAS_XO

        err_code |= m_clock_cb.event_handler ?  nrfx_clock_lfclk_init(&lfclk_event_handler) :
                                                nrfx_clock_lfclk_init(NULL);

    if(err_code != NRFX_SUCCESS)
    {
        NRFX_LOG_INFO("Function: %s, error code: %s.", __func__,
                        NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }

    return err_code;
}

void nrfx_clock_enable(void)
{
    NRFX_ASSERT(m_clock_cb.module_initialized);
    if (m_clock_cb.event_handler)
    {
        nrfx_power_clock_irq_init();
    }
    nrf_clock_lf_src_set(NRF_CLOCK, clock_initial_lfclksrc_get());
#if NRF_CLOCK_HAS_HFCLK192M
    nrf_clock_hfclk192m_src_set(NRF_CLOCK, (nrf_clock_hfclk_t)NRFX_CLOCK_CONFIG_HFCLK192M_SRC);
#endif
#if NRFX_CHECK(NRFX_POWER_ENABLED)
    nrfx_clock_irq_enabled = true;
#endif
    NRFX_LOG_INFO("Module enabled.");
}

void nrfx_clock_disable(void)
{
    NRFX_ASSERT(m_clock_cb.module_initialized);

    if (m_clock_cb.event_handler)
    {
#if NRFX_CHECK(NRFX_POWER_ENABLED)
        NRFX_ASSERT(nrfx_clock_irq_enabled);
        if (!nrfx_power_irq_enabled)
#endif
        {
#if defined(NRF54L05_XXAA) || defined(NRF54L10_XXAA) || defined(NRF54L15_XXAA)
            IRQn_Type irqn = CLOCK_POWER_IRQn;
#else
            IRQn_Type irqn = nrfx_get_irq_number(NRF_CLOCK);
#endif
            NRFX_IRQ_DISABLE(irqn);
        }
    }
    nrf_clock_int_disable(NRF_CLOCK, NRF_CLOCK_INT_HF_STARTED_MASK |
                                     NRF_CLOCK_INT_LF_STARTED_MASK |
#if NRFX_CHECK(NRFX_CLOCK_CONFIG_LF_CAL_ENABLED) && (NRF_CLOCK_HAS_CALIBRATION)
                                     NRF_CLOCK_INT_DONE_MASK |
#if NRF_CLOCK_HAS_CALIBRATION_TIMER
                                     NRF_CLOCK_INT_CTTO_MASK |
#endif
#endif // NRFX_CHECK(NRFX_CLOCK_CONFIG_LF_CAL_ENABLED)
                          0);
#if NRFX_CHECK(NRFX_POWER_ENABLED)
    nrfx_clock_irq_enabled = false;
#endif
#if NRFX_CHECK(NRF_LFRC_HAS_CALIBRATION) && NRFX_CHECK(NRFX_CLOCK_CONFIG_LF_CAL_ENABLED)
    NRFX_IRQ_DISABLE(LFRC_IRQn);
    nrf_lfrc_int_disable(NRF_LFRC, NRF_LFRC_INT_CALDONE_MASK);
#endif
    NRFX_LOG_INFO("Module disabled.");
}

void nrfx_clock_uninit(void)
{
    NRFX_ASSERT(m_clock_cb.module_initialized);

    nrfx_clock_lfclk_uninit();

#if NRF_CLOCK_HAS_HFCLK192M
    clock_stop(NRF_CLOCK_DOMAIN_HFCLK192M);
#endif
#if NRF_CLOCK_HAS_HFCLKAUDIO
    clock_stop(NRF_CLOCK_DOMAIN_HFCLKAUDIO);
#endif
#if NRF_CLOCK_HAS_HFCLK24M
    clock_stop(NRF_CLOCK_DOMAIN_HFCLK24M);
#endif
#if NRF_CLOCK_HAS_HFCLK
    nrfx_clock_hfclk_uninit();
#endif
#if NRF_CLOCK_HAS_XO
    nrfx_clock_xo_uninit();
#endif
    m_clock_cb.module_initialized = false;
    NRFX_LOG_INFO("Uninitialized.");
}

bool nrfx_clock_init_check(void)
{
    return m_clock_cb.module_initialized;
}

void nrfx_clock_start(nrf_clock_domain_t domain)
{
    uint32_t          int_mask;
    nrf_clock_event_t event;
    nrf_clock_task_t  task;

    NRFX_ASSERT(m_clock_cb.module_initialized);
    switch (domain)
    {
        case NRF_CLOCK_DOMAIN_LFCLK:
            nrfx_clock_lfclk_start();
            return;
        case NRF_CLOCK_DOMAIN_HFCLK:
#if NRF_CLOCK_HAS_XO
            nrfx_clock_xo_start();
#elif NRF_CLOCK_HAS_HFCLK
            nrfx_clock_hfclk_start();
#endif // NRF_CLOCK_HAS_HFCLK
            return;

#if NRF_CLOCK_HAS_HFCLK192M
        case NRF_CLOCK_DOMAIN_HFCLK192M:
            event    = NRF_CLOCK_EVENT_HFCLK192MSTARTED;
            int_mask = NRF_CLOCK_INT_HF192M_STARTED_MASK;
            task     = NRF_CLOCK_TASK_HFCLK192MSTART;
            break;
#endif
#if NRF_CLOCK_HAS_HFCLKAUDIO
        case NRF_CLOCK_DOMAIN_HFCLKAUDIO:
            event    = NRF_CLOCK_EVENT_HFCLKAUDIOSTARTED;
            int_mask = NRF_CLOCK_INT_HFAUDIO_STARTED_MASK;
            task     = NRF_CLOCK_TASK_HFCLKAUDIOSTART;
            break;
#endif
#if NRF_CLOCK_HAS_HFCLK24M
        case NRF_CLOCK_DOMAIN_HFCLK24M:
            event    = NRF_CLOCK_EVENT_HFCLK24MSTARTED;
            int_mask = NRF_CLOCK_INT_HFCLK24M_STARTED_MASK;
            task     = NRF_CLOCK_TASK_HFCLK24MSTART;
            break;
#endif
        default:
            NRFX_ASSERT(0);
            return;
    }

    nrf_clock_event_clear(NRF_CLOCK, event);
#if NRFX_CHECK(USE_WORKAROUND_FOR_ANOMALY_132)
    if (event == NRF_CLOCK_EVENT_LFCLKSTARTED)
    {
        nrfx_clock_anomaly_132();
    }
#endif
    nrf_clock_task_trigger(NRF_CLOCK, task);
    if (m_clock_cb.event_handler)
    {
        nrf_clock_int_enable(NRF_CLOCK, int_mask);
    }
    else
    {
        while (!nrf_clock_event_check(NRF_CLOCK, event))
        {}
        nrf_clock_event_clear(NRF_CLOCK, event);
    }
}

void nrfx_clock_stop(nrf_clock_domain_t domain)
{
    NRFX_ASSERT(m_clock_cb.module_initialized);
    switch(domain)
    {
    case NRF_CLOCK_DOMAIN_HFCLK:
#if NRF_CLOCK_HAS_XO
        nrfx_clock_xo_stop();
#elif NRF_CLOCK_HAS_HFCLK
        nrfx_clock_hfclk_stop();
#endif // NRF_CLOCK_HAS_XO
        break;
    case NRF_CLOCK_DOMAIN_LFCLK:
        nrfx_clock_lfclk_stop();
        break;
    default:
        clock_stop(domain);
        break;
    }
}

#if ((NRF_CLOCK_HAS_CALIBRATION || NRFX_CHECK(NRF_LFRC_HAS_CALIBRATION)) && \
     NRFX_CHECK(NRFX_CLOCK_CONFIG_LF_CAL_ENABLED))
nrfx_err_t nrfx_clock_calibration_start(void)
{
    return nrfx_clock_lfclk_calibration_start();
}

nrfx_err_t nrfx_clock_is_calibrating(void)
{
    return nrfx_clock_lfclk_is_calibrating();
}

#if NRF_CLOCK_HAS_CALIBRATION_TIMER && NRFX_CHECK(NRFX_CLOCK_CONFIG_CT_ENABLED)
void nrfx_clock_calibration_timer_start(uint8_t interval)
{
    nrfx_clock_lfclk_calibration_timer_start(interval);
}

void nrfx_clock_calibration_timer_stop(void)
{
    nrfx_clock_lfclk_calibration_timer_stop();
}
#endif // NRF_CLOCK_HAS_CALIBRATION_TIMER && NRFX_CHECK(NRFX_CLOCK_CONFIG_CT_ENABLED)
#endif /* ((NRF_CLOCK_HAS_CALIBRATION || NRFX_CHECK(NRF_LFRC_HAS_CALIBRATION)) &&
            NRFX_CHECK(NRFX_CLOCK_CONFIG_LF_CAL_ENABLED)) */

#if defined(CLOCK_FEATURE_HFCLK_DIVIDE_PRESENT) || NRF_CLOCK_HAS_HFCLK192M
nrfx_err_t nrfx_clock_divider_set(nrf_clock_domain_t domain,
                                  nrf_clock_hfclk_div_t div)
{
    switch(domain)
    {
#if defined(CLOCK_FEATURE_HFCLK_DIVIDE_PRESENT)
#if NRF_CLOCK_HAS_HFCLK
        case NRF_CLOCK_DOMAIN_HFCLK:
            return nrfx_clock_hfclk_divider_set(div);
#endif // NRF_CLOCK_HAS_HFCLK
#endif // CLOCK_FEATURE_HFCLK_DIVIDE_PRESENT
#if NRF_CLOCK_HAS_HFCLK192M
        case NRF_CLOCK_DOMAIN_HFCLK192M:
            if (div > NRF_CLOCK_HFCLK_DIV_4)
            {
                return NRFX_ERROR_INVALID_PARAM;
            }
            else
            {
                nrf_clock_hfclk192m_div_set(NRF_CLOCK, div);
            }
            return NRFX_SUCCESS;
#endif
        default:
            NRFX_ASSERT(0);
            return NRFX_ERROR_NOT_SUPPORTED;
    }
}
#endif

void nrfx_clock_irq_handler(void)
{
#if NRFX_CHECK(NRFX_CLOCK_CONFIG_USE_LFRC_CALIBRATION) && \
    NRFX_CHECK(NRFX_CLOCK_CONFIG_LF_CAL_ENABLED)
    if (nrf_lfrc_event_check(NRF_LFRC, NRF_LFRC_EVENT_CALDONE))
    {
        nrf_lfrc_event_clear(NRF_LFRC, NRF_LFRC_EVENT_CALDONE);
        nrf_lfrc_int_disable(NRF_LFRC, NRF_LFRC_INT_CALDONE_MASK);
        m_clock_cb.cal_state = CAL_STATE_IDLE;
        m_clock_cb.event_handler(NRFX_CLOCK_EVT_CAL_DONE);
    }
#endif
#if NRF_CLOCK_HAS_INTPEND
    uint32_t intpend = nrf_clock_int_pending_get(NRF_CLOCK);
#else
    uint32_t intpend = nrf_clock_int_enable_check(NRF_CLOCK, UINT32_MAX);
#endif
    intpend &=  INTERRUPT_MASK;

    while (intpend != 0)
    {
        uint32_t int_bit = 31 - NRF_CLZ(intpend);
        nrf_clock_int_mask_t int_mask = (nrf_clock_int_mask_t)NRFX_BIT(int_bit);
        nrf_clock_event_t evt = (nrf_clock_event_t)NRFY_INT_BITPOS_TO_EVENT(int_bit);
        nrfx_clock_hfclk192m_evt_type_t evt_type = (nrfx_clock_hfclk192m_evt_type_t)int_bit;
        bool call_handler = true;
#if !NRF_CLOCK_HAS_INTPEND
        intpend &= ~int_mask;
        // Check if event is set for that interrupt and if not continue.
        if (!nrf_clock_event_check(NRF_CLOCK, evt)) {
            continue;
        }
#endif

        nrf_clock_event_clear(NRF_CLOCK, evt);
        nrf_clock_int_disable(NRF_CLOCK, int_mask);

        NRFX_LOG_DEBUG("Event: %s", NRFX_CLOCK_EVT2STR(evt_type));
        switch (int_mask)
        {
            case NRF_CLOCK_INT_LF_STARTED_MASK:
            {
#if NRFX_CHECK(NRFX_CLOCK_CONFIG_LFXO_TWO_STAGE_ENABLED)
                nrf_clock_lfclk_t lfclksrc;
                (void)nrf_clock_is_running(NRF_CLOCK, NRF_CLOCK_DOMAIN_LFCLK, &lfclksrc);
                if (lfclksrc == NRF_CLOCK_LFCLK_RC)
                {
                    // After the LFRC oscillator start switch to external source.
                    nrf_clock_lf_src_set(NRF_CLOCK, (nrf_clock_lfclk_t)NRFX_CLOCK_CONFIG_LF_SRC);
                    nrf_clock_int_enable(NRF_CLOCK, int_mask);
                    nrf_clock_task_trigger(NRF_CLOCK, NRF_CLOCK_TASK_LFCLKSTART);
                    call_handler = false;
                }
                else
#endif
                {
                    // After the LF clock external source start invoke user callback.
                }
                break;
            }
#if NRFX_CHECK(NRFX_CLOCK_CONFIG_LF_CAL_ENABLED) && NRF_CLOCK_HAS_CALIBRATION
#if NRF_CLOCK_HAS_CALIBRATION_TIMER && NRFX_CHECK(NRFX_CLOCK_CONFIG_CT_ENABLED)
            case NRF_CLOCK_INT_CTTO_MASK:
                break;
#endif
            case NRF_CLOCK_INT_DONE_MASK:
#if NRFX_CHECK(USE_WORKAROUND_FOR_ANOMALY_192)
                *(volatile uint32_t *)0x40000C34 = 0x00000000;
#endif
                m_clock_cb.cal_state = CAL_STATE_IDLE;
                break;
#endif // (NRFX_CLOCK_CONFIG_LF_CAL_ENABLED && NRF_CLOCK_HAS_CALIBRATION)
#if NRF_CLOCK_HAS_HFCLKAUDIO
            case NRF_CLOCK_INT_HFAUDIO_STARTED_MASK:
                break;
#endif
#if NRF_CLOCK_HAS_HFCLK24M
            case NRF_CLOCK_INT_HFCLK24M_STARTED_MASK:
                break;
#endif
#if NRF_CLOCK_HAS_HFCLK192M
            case NRF_CLOCK_INT_HF192M_STARTED_MASK:
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

        intpend &=  INTERRUPT_MASK;
#endif
    }
}

#endif // NRFX_CHECK(NRFX_CLOCK_ENABLED)
