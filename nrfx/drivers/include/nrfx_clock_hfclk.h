/*$$$LICENCE_NORDIC_STANDARD<2025>$$$*/

#ifndef NRFX_CLOCK_HFCLK_H__
#define NRFX_CLOCK_HFCLK_H__

#include <hal/nrf_clock.h>

#define NRFX_CLOCK_HFCLK_EVT_HFCLK_STARTED NRFX_BITMASK_TO_BITPOS(NRF_CLOCK_INT_HF_STARTED_MASK)

/**
 * @brief Clock event handler.
 *
 * @param[in] event Event.
 */
typedef void (*nrfx_clock_hfclk_event_handler_t)(void);

/**
 * @brief Function for initializing internal structures in the nrfx_clock_hfclk module.
 *
 * After initialization, the module is in power off state (clock is not started).
 *
 * @param[in] event_handler Event handler provided by the user.
 *                          If not provided, driver works in blocking mode.
 *
 * @retval NRFX_SUCCESS       The procedure is successful.
 * @retval NRFX_ERROR_ALREADY The driver is already initialized.
 */
nrfx_err_t nrfx_clock_hfclk_init(nrfx_clock_hfclk_event_handler_t event_handler);

/** @brief Function for uninitializing the clock module. */
void nrfx_clock_hfclk_uninit(void);

/** @brief Function for starting the hfclk clock domain. */
void nrfx_clock_hfclk_start(void);

/** @brief Function for stopping the hfclk clock domain. */
void nrfx_clock_hfclk_stop(void);

#if defined(CLOCK_FEATURE_HFCLK_DIVIDE_PRESENT)
/**
 * @brief Function for setting the hfclk divider.
 *
 * @param[in] div    New divider for the hfclk.
 *
 * @retval NRFX_SUCCESS             Divider successfully set.
 * @retval NRFX_ERROR_INVALID_PARAM Divider not supported by the specified domain.
 */
nrfx_err_t nrfx_clock_hfclk_divider_set(nrf_clock_hfclk_div_t div);
#endif

/**
 * @brief Function for checking the hfclk state.
 *
 * @param[out] p_clk_src Pointer to a clock source that is running.
 *
 * @retval true  The hfclk is running.
 * @retval false The hfclk is not running.
 */
NRFX_STATIC_INLINE bool nrfx_clock_hfclk_running_check(nrf_clock_hfclk_t * p_clk_src);

void nrfx_clock_hfclk_irq_handler(void);

NRFX_STATIC_INLINE bool nrfx_clock_hfclk_running_check(nrf_clock_hfclk_t * p_clk_src)
{
    bool ret = nrf_clock_is_running(NRF_CLOCK, NRF_CLOCK_DOMAIN_HFCLK, p_clk_src);
    return (ret && (*p_clk_src == NRF_CLOCK_HFCLK_HIGH_ACCURACY));
}

#endif // NRFX_CLOCK_HFCLK_H__
