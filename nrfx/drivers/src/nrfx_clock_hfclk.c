/*$$$LICENCE_NORDIC_STANDARD<2025>$$$*/

#include <nrfx.h>
#include <nrfx_clock_hfclk.h>

#if NRFX_CHECK(NRFX_CLOCK_ENABLED)

#define NRFX_LOG_MODULE CLOCK_HFCLK
#include <nrfx_log.h>

#if !defined(USE_WORKAROUND_FOR_ANOMALY_201) && \
    (defined(NRF52810_XXAA) || \
     defined(NRF52832_XXAA) || defined(NRF52832_XXAB) || \
     defined(NRF52840_XXAA))
    // Enable workaround for nRF52 anomaly 201 (EVENTS_HFCLKSTARTED might be generated twice).
    #define USE_WORKAROUND_FOR_ANOMALY_201 1
#endif

/** @brief CLOCK control block. */
typedef struct
{
    nrfx_clock_hfclk_event_handler_t event_handler;
    bool                             module_initialized; /*< Indicate the state of module */
#if NRFX_CHECK(USE_WORKAROUND_FOR_ANOMALY_201)
    bool                             hfclk_started;      /*< Anomaly 201 workaround. */
#endif
} nrfx_clock_hfclk_cb_t;

static nrfx_clock_hfclk_cb_t m_clock_cb;

static void clock_stop()
{
    nrf_clock_int_disable(NRF_CLOCK, NRF_CLOCK_INT_HF_STARTED_MASK);
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
    NRFX_WAIT_FOR(!nrfx_clock_hfclk_running_check(&clk_src), 10000, 1, stopped);
    if (!stopped)
    {
        NRFX_LOG_ERROR("Failed to stop clock domain: NRF_CLOCK_DOMAIN_HFCLK.");
    }
    else
    {
#if NRFX_CHECK(USE_WORKAROUND_FOR_ANOMALY_201)
        m_clock_cb.hfclk_started = false;
#endif
    }
}

nrfx_err_t nrfx_clock_hfclk_init(nrfx_clock_hfclk_event_handler_t event_handler)
{
    nrfx_err_t err_code = NRFX_SUCCESS;
    if (m_clock_cb.module_initialized)
    {
        err_code = NRFX_ERROR_ALREADY;
    }
    else
    {
        m_clock_cb.event_handler = event_handler;
        m_clock_cb.module_initialized = true;
#if NRFX_CHECK(USE_WORKAROUND_FOR_ANOMALY_201)
        m_clock_cb.hfclk_started = false;
#endif
    }
#if NRF_CLOCK_HAS_HFCLKSRC
    nrf_clock_hf_src_set(NRF_CLOCK, NRF_CLOCK_HFCLK_HIGH_ACCURACY);
#endif // NRF_CLOCK_HAS_HFCLKSRC
    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}

void nrfx_clock_hfclk_uninit(void)
{
    NRFX_ASSERT(m_clock_cb.module_initialized);
    clock_stop();

    m_clock_cb.module_initialized = false;
    NRFX_LOG_INFO("Uninitialized.");
}

void nrfx_clock_hfclk_start(void)
{
    NRFX_ASSERT(m_clock_cb.module_initialized);

    nrf_clock_event_clear(NRF_CLOCK, NRF_CLOCK_EVENT_HFCLKSTARTED);
    nrf_clock_task_trigger(NRF_CLOCK, NRF_CLOCK_TASK_HFCLKSTART);
    if (m_clock_cb.event_handler)
    {
        nrf_clock_int_enable(NRF_CLOCK, NRF_CLOCK_INT_HF_STARTED_MASK);
    }
    else
    {
        while (!nrf_clock_event_check(NRF_CLOCK, NRF_CLOCK_EVENT_HFCLKSTARTED))
        {}
        nrf_clock_event_clear(NRF_CLOCK, NRF_CLOCK_EVENT_HFCLKSTARTED);
    }
}

void nrfx_clock_hfclk_stop(void)
{
    NRFX_ASSERT(m_clock_cb.module_initialized);
    clock_stop();
}

#if defined(CLOCK_FEATURE_HFCLK_DIVIDE_PRESENT)
nrfx_err_t nrfx_clock_hfclk_divider_set(nrf_clock_hfclk_div_t div)
{
    switch (div)
    {
        case NRF_CLOCK_HFCLK_DIV_2:
#if !defined(NRF_TRUSTZONE_NONSECURE) && NRFX_CHECK(NRF53_ERRATA_4_ENABLE_WORKAROUND)
            if (nrf53_errata_4())
            {
                NRFX_CRITICAL_SECTION_ENTER();
                __DSB();

                nrf_clock_hfclk_div_set(NRF_CLOCK, div);

                *(volatile uint32_t *)0x5084450C = 0x0;
                *(volatile uint32_t *)0x50026548 = 0x0;
                *(volatile uint32_t *)0x50081EE4 = 0x0D;

                NRFX_CRITICAL_SECTION_EXIT();
            }
            else
#endif
            {
                nrf_clock_hfclk_div_set(NRF_CLOCK, div);
            }
            break;
        case NRF_CLOCK_HFCLK_DIV_1:
#if !defined(NRF_TRUSTZONE_NONSECURE) && NRFX_CHECK(NRF53_ERRATA_4_ENABLE_WORKAROUND)
            if (nrf53_errata_4())
            {
                NRFX_CRITICAL_SECTION_ENTER();
                __DSB();

                *(volatile uint32_t *)0x5084450C = 0x4040;
                *(volatile uint32_t *)0x50026548 = 0x40;
                *(volatile uint32_t *)0x50081EE4 = 0x4D;

                nrf_clock_hfclk_div_set(NRF_CLOCK, div);

                NRFX_CRITICAL_SECTION_EXIT();
            }
            else
#endif
            {
                nrf_clock_hfclk_div_set(NRF_CLOCK, div);
            }
            break;
        default:
            return NRFX_ERROR_INVALID_PARAM;
    }
    SystemCoreClockUpdate();
    return NRFX_SUCCESS;
}
#endif

void nrfx_clock_hfclk_irq_handler(void)
{
#if NRF_CLOCK_HAS_INTPEND
    uint32_t intpend = nrf_clock_int_pending_get(NRF_CLOCK);
#else
    uint32_t intpend = nrf_clock_int_enable_check(NRF_CLOCK, UINT32_MAX);
#endif

    if(!(intpend & NRF_CLOCK_INT_HF_STARTED_MASK))
    {
        return;
    }

    uint32_t int_bit = 31 - NRF_CLZ(NRF_CLOCK_INT_HF_STARTED_MASK);
    nrf_clock_event_t evt = (nrf_clock_event_t)NRFY_INT_BITPOS_TO_EVENT(int_bit);
    bool call_handler = true;

#if !NRF_CLOCK_HAS_INTPEND
    // Check if event is set for that interrupt and if not continue.
    if (!nrf_clock_event_check(NRF_CLOCK, evt)) {
        return;
    }
#endif

    nrf_clock_event_clear(NRF_CLOCK, evt);
    nrf_clock_int_disable(NRF_CLOCK, NRF_CLOCK_INT_HF_STARTED_MASK);

    NRFX_LOG_DEBUG("Event: HFCLK_STARTED");

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

    if (call_handler)
    {
        m_clock_cb.event_handler();
    }
}

#endif // NRFX_CHECK(NRFX_CLOCK_ENABLED)
