/*$$$LICENCE_NORDIC_STANDARD<2025>$$$*/

#ifndef NRFX_ANALOG_COMMON_H__
#define NRFX_ANALOG_COMMON_H__

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrfx_analog_common Generic analog input layer.
 * @{
 * @ingroup nrfx
 * @ingroup nrf_saadc
 * @ingroup nrf_comp
 * @ingroup nrf_lpcomp
 *
 * @brief Helper layer that provides the common functionality of SAADC, COMP and LPCOMP drivers.
 */

/** @brief Symbol specifying internal inputs offset. */
#define NRFX_ANALOG_AIN_INTERNAL_OFFSET 128

typedef enum {
    NRFX_ANALOG_EXTERNAL_AIN0 = 0,
    NRFX_ANALOG_EXTERNAL_AIN1,
    NRFX_ANALOG_EXTERNAL_AIN2,
    NRFX_ANALOG_EXTERNAL_AIN3,
    NRFX_ANALOG_EXTERNAL_AIN4,
    NRFX_ANALOG_EXTERNAL_AIN5,
    NRFX_ANALOG_EXTERNAL_AIN6,
    NRFX_ANALOG_EXTERNAL_AIN7,
    NRFX_ANALOG_EXTERNAL_AIN8,
    NRFX_ANALOG_EXTERNAL_AIN9,
    NRFX_ANALOG_EXTERNAL_AIN10,
    NRFX_ANALOG_EXTERNAL_AIN11,
    NRFX_ANALOG_EXTERNAL_AIN12,
    NRFX_ANALOG_EXTERNAL_AIN13,
    NRFX_ANALOG_INTERNAL_VDD = NRFX_ANALOG_AIN_INTERNAL_OFFSET,
    NRFX_ANALOG_INTERNAL_AVDD,
    NRFX_ANALOG_INTERNAL_DVDD,
    NRFX_ANALOG_INTERNAL_VDDHDIV5,
    NRFX_ANALOG_INTERNAL_VDDL,
    NRFX_ANALOG_INTERNAL_DECB,
    NRFX_ANALOG_INTERNAL_VSS,
    NRFX_ANALOG_INTERNAL_VDDAO3V0,
    NRFX_ANALOG_INTERNAL_VDDAO1V8,
    NRFX_ANALOG_INTERNAL_VDDAO0V8,
    NRFX_ANALOG_INTERNAL_VDDRF,
    NRFX_ANALOG_INTERNAL_VBAT,
} nrfx_analog_input_t;

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* NRFX_ANALOG_COMMON_H__ */
