/*$$$LICENCE_NORDIC_STANDARD<2025>$$$*/

#ifndef NRFX_ANALOG_LPCOMP_COMMON_H__
#define NRFX_ANALOG_LPCOMP_COMMON_H__

#include <helpers/nrfx_analog_common.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrfx_analog_lpcomp_common Generic LPCOMP inputs layer.
 * @{
 * @ingroup nrfx
 * @ingroup nrf_lpcomp
 *
 * @brief Helper layer that provides the common functionality of LPCOMP driver.
 */

#if (NRF_LPCOMP_HAS_AIN_AS_PIN)
static const uint32_t nrfx_analog_lpcomp_psels[] = {
#if defined(HALTIUM_XXAA)
    NRF_PIN_PORT_TO_PIN_NUMBER(0U, 1),
    NRF_PIN_PORT_TO_PIN_NUMBER(1U, 1),
    NRF_PIN_PORT_TO_PIN_NUMBER(2U, 1),
    NRF_PIN_PORT_TO_PIN_NUMBER(3U, 1),
    NRF_PIN_PORT_TO_PIN_NUMBER(4U, 1),
    NRF_PIN_PORT_TO_PIN_NUMBER(5U, 1),
    NRF_PIN_PORT_TO_PIN_NUMBER(6U, 1),
    NRF_PIN_PORT_TO_PIN_NUMBER(7U, 1),
#elif defined(NRF54L05_XXAA) || defined(NRF54L10_XXAA) || defined(NRF54L15_XXAA)
    NRF_PIN_PORT_TO_PIN_NUMBER(4U, 1),
    NRF_PIN_PORT_TO_PIN_NUMBER(5U, 1),
    NRF_PIN_PORT_TO_PIN_NUMBER(6U, 1),
    NRF_PIN_PORT_TO_PIN_NUMBER(7U, 1),
    NRF_PIN_PORT_TO_PIN_NUMBER(11U, 1),
    NRF_PIN_PORT_TO_PIN_NUMBER(12U, 1),
    NRF_PIN_PORT_TO_PIN_NUMBER(13U, 1),
    NRF_PIN_PORT_TO_PIN_NUMBER(14U, 1),
#elif defined(NRF54LM20A_ENGA_XXAA)
    NRF_PIN_PORT_TO_PIN_NUMBER(0U, 1),
    NRF_PIN_PORT_TO_PIN_NUMBER(31U, 1),
    NRF_PIN_PORT_TO_PIN_NUMBER(30U, 1),
    NRF_PIN_PORT_TO_PIN_NUMBER(29U, 1),
    NRF_PIN_PORT_TO_PIN_NUMBER(6U, 1),
    NRF_PIN_PORT_TO_PIN_NUMBER(5U, 1),
    NRF_PIN_PORT_TO_PIN_NUMBER(4U, 1),
    NRF_PIN_PORT_TO_PIN_NUMBER(3U, 1),
#elif defined(NRF54LV10A_ENGA_XXAA)
    NRF_PIN_PORT_TO_PIN_NUMBER(0U, 1),
    NRF_PIN_PORT_TO_PIN_NUMBER(1U, 1),
    NRF_PIN_PORT_TO_PIN_NUMBER(2U, 1),
    NRF_PIN_PORT_TO_PIN_NUMBER(3U, 1),
    NRF_PIN_PORT_TO_PIN_NUMBER(7U, 1),
    NRF_PIN_PORT_TO_PIN_NUMBER(10U, 1),
    NRF_PIN_PORT_TO_PIN_NUMBER(11U, 1),
    NRF_PIN_PORT_TO_PIN_NUMBER(12U, 1),
#else
#error "Unsupported device for LPCOMP AIN pins."
#endif
};

/**
 * @brief Get the LPCOMP input configuration for given analog input.
 *
 * @param[in] input  The analog input to get the configuration for.
 *
 * @return  The LPCOMP input configuration for the given analog input.
 */
NRFX_STATIC_INLINE nrf_lpcomp_input_t nrfx_analog_lpcomp_ain_config_get(nrfx_analog_input_t input) {
    if (input < NRFX_ARRAY_SIZE(nrfx_analog_lpcomp_psels)) {
        return nrfx_analog_lpcomp_psels[input];
    }

    return 0;
}
#endif /* (NRF_LPCOMP_HAS_AIN_AS_PIN) */

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* NRFX_ANALOG_LPCOMP_COMMON_H__ */
