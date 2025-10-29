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

#if defined(NRF54LV10A_ENGA_XXAA) || defined(__NRFX_DOXYGEN__)
/** @brief Symbol specifying internal reference voltage. */
#define NRFX_ANALOG_REF_INTERNAL_VAL 1300
#elif defined(HALTIUM_XXAA) || defined(NRF7120_ENGA_XXAA) || defined(NRF54LS05B_ENGA_XXAA)
#define NRFX_ANALOG_REF_INTERNAL_VAL 1024
#elif defined(LUMOS_XXAA)
#define NRFX_ANALOG_REF_INTERNAL_VAL 900
#else
#define NRFX_ANALOG_REF_INTERNAL_VAL 600
#endif

/** @brief Symbol specifying internal inputs offset. */
#define NRFX_ANALOG_AIN_INTERNAL_OFFSET 128

/** @brief Generic analog input types. */
typedef enum {
    NRFX_ANALOG_EXTERNAL_AIN0 = 0,                              ///< External analog input 0
    NRFX_ANALOG_EXTERNAL_AIN1,                                  ///< External analog input 1
    NRFX_ANALOG_EXTERNAL_AIN2,                                  ///< External analog input 2
    NRFX_ANALOG_EXTERNAL_AIN3,                                  ///< External analog input 3
    NRFX_ANALOG_EXTERNAL_AIN4,                                  ///< External analog input 4
    NRFX_ANALOG_EXTERNAL_AIN5,                                  ///< External analog input 5
    NRFX_ANALOG_EXTERNAL_AIN6,                                  ///< External analog input 6
    NRFX_ANALOG_EXTERNAL_AIN7,                                  ///< External analog input 7
    NRFX_ANALOG_EXTERNAL_AIN8,                                  ///< External analog input 8
    NRFX_ANALOG_EXTERNAL_AIN9,                                  ///< External analog input 9
    NRFX_ANALOG_EXTERNAL_AIN10,                                 ///< External analog input 10
    NRFX_ANALOG_EXTERNAL_AIN11,                                 ///< External analog input 11
    NRFX_ANALOG_EXTERNAL_AIN12,                                 ///< External analog input 12
    NRFX_ANALOG_EXTERNAL_AIN13,                                 ///< External analog input 13
    NRFX_ANALOG_INTERNAL_VDD = NRFX_ANALOG_AIN_INTERNAL_OFFSET, ///< Internal VDD
    NRFX_ANALOG_INTERNAL_VDDDIV2,                               ///< Internal VDD/2
    NRFX_ANALOG_INTERNAL_AVDD,                                  ///< Internal AVDD
    NRFX_ANALOG_INTERNAL_DVDD,                                  ///< Internal DVDD
    NRFX_ANALOG_INTERNAL_VDDHDIV5,                              ///< Internal VDDH/5
    NRFX_ANALOG_INTERNAL_VDDL,                                  ///< Internal VDDL
    NRFX_ANALOG_INTERNAL_DECB,                                  ///< Internal DECB
    NRFX_ANALOG_INTERNAL_VSS,                                   ///< Internal VSS
    NRFX_ANALOG_INTERNAL_VDDAO3V0,                              ///< Internal VDDA - 3.0V
    NRFX_ANALOG_INTERNAL0,                                      ///< Internal input 0
    NRFX_ANALOG_INTERNAL1,                                      ///< Internal input 1
    NRFX_ANALOG_INTERNAL2,                                      ///< Internal input 2
    NRFX_ANALOG_INTERNAL3,                                      ///< Internal input 3
    NRFX_ANALOG_INPUT_DISABLED = UINT8_MAX,                     ///< Analog input disabled
} nrfx_analog_input_t;

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* NRFX_ANALOG_COMMON_H__ */
