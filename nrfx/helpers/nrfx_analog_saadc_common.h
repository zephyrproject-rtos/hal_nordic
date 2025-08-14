/*$$$LICENCE_NORDIC_STANDARD<2025>$$$*/

#ifndef NRFX_ANALOG_SAADC_COMMON_H__
#define NRFX_ANALOG_SAADC_COMMON_H__

#include <helpers/nrfx_analog_common.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrfx_analog_saadc_common Generic SAADC input layer.
 * @{
 * @ingroup nrfx
 * @ingroup nrf_saadc
 *
 * @brief Helper layer that provides the common functionality of SAADC driver.
 */

#if defined(NRF54LV10A_ENGA_XXAA) || defined(__NRFX_DOXYGEN__)
/** @brief Symbol specifying internal reference voltage. */
#define NRFX_ANALOG_SAADC_REF_INTERNAL_VALUE 1300
#elif defined(HALTIUM_XXAA) || defined(NRF7120_ENGA_XXAA) || defined(NRF54LS05B_ENGA_XXAA)
#define NRFX_ANALOG_SAADC_REF_INTERNAL_VALUE 1024
#elif defined(LUMOS_XXAA)
#define NRFX_ANALOG_SAADC_REF_INTERNAL_VALUE 900
#else
#define NRFX_ANALOG_SAADC_REF_INTERNAL_VALUE 600
#endif

#if (NRF_SAADC_HAS_AIN_AS_PIN)
static const uint32_t nrfx_analog_saadc_external_ain_psels[] = {
#if defined(HALTIUM_XXAA)
    NRF_PIN_PORT_TO_PIN_NUMBER(0U, 1),
    NRF_PIN_PORT_TO_PIN_NUMBER(1U, 1),
    NRF_PIN_PORT_TO_PIN_NUMBER(2U, 1),
    NRF_PIN_PORT_TO_PIN_NUMBER(3U, 1),
    NRF_PIN_PORT_TO_PIN_NUMBER(4U, 1),
    NRF_PIN_PORT_TO_PIN_NUMBER(5U, 1),
    NRF_PIN_PORT_TO_PIN_NUMBER(6U, 1),
    NRF_PIN_PORT_TO_PIN_NUMBER(7U, 1),
    NRF_PIN_PORT_TO_PIN_NUMBER(0U, 9),
    NRF_PIN_PORT_TO_PIN_NUMBER(1U, 9),
    NRF_PIN_PORT_TO_PIN_NUMBER(2U, 9),
    NRF_PIN_PORT_TO_PIN_NUMBER(3U, 9),
    NRF_PIN_PORT_TO_PIN_NUMBER(4U, 9),
    NRF_PIN_PORT_TO_PIN_NUMBER(5U, 9),
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
#elif defined(NRF54LS05B_ENGA_XXAA)
    NRF_PIN_PORT_TO_PIN_NUMBER(4U, 1),
    NRF_PIN_PORT_TO_PIN_NUMBER(5U, 1),
    NRF_PIN_PORT_TO_PIN_NUMBER(6U, 1),
    NRF_PIN_PORT_TO_PIN_NUMBER(7U, 1),
#elif defined(NRF7120_ENGA_XXAA)
    NRF_PIN_PORT_TO_PIN_NUMBER(0U, 0),
    NRF_PIN_PORT_TO_PIN_NUMBER(1U, 0),
    NRF_PIN_PORT_TO_PIN_NUMBER(2U, 0),
    NRF_PIN_PORT_TO_PIN_NUMBER(3U, 0),
    NRF_PIN_PORT_TO_PIN_NUMBER(4U, 0),
    NRF_PIN_PORT_TO_PIN_NUMBER(6U, 0),
    NRF_PIN_PORT_TO_PIN_NUMBER(7U, 0),
    NRF_PIN_PORT_TO_PIN_NUMBER(8U, 0),
    NRF_SAADC_INPUT_DISABLED,
    NRF_PIN_PORT_TO_PIN_NUMBER(0U, 4),
    NRF_PIN_PORT_TO_PIN_NUMBER(1U, 4),
    NRF_PIN_PORT_TO_PIN_NUMBER(2U, 4),
    NRF_PIN_PORT_TO_PIN_NUMBER(3U, 4),
    NRF_PIN_PORT_TO_PIN_NUMBER(4U, 4),
    NRF_PIN_PORT_TO_PIN_NUMBER(5U, 4),
#else
#error "Unsupported device for SAADC AIN pins."
#endif
};

static const uint32_t nrfx_analog_saadc_internal_ain_psels[] = {
#if defined(NRF54L05_XXAA) || defined(NRF54L10_XXAA) || defined(NRF54L15_XXAA)
    NRF_SAADC_INPUT_VDD,
    NRF_SAADC_INPUT_AVDD,
    NRF_SAADC_INPUT_DVDD,
#elif defined(NRF54LM20A_ENGA_XXAA)
    NRF_SAADC_INPUT_VDD,
    NRF_SAADC_INPUT_AVDD,
    NRF_SAADC_INPUT_DVDD,
#elif defined(NRF54LV10A_ENGA_XXAA)
    NRF_SAADC_INPUT_DECB,
    NRF_SAADC_INPUT_DISABLED,
    NRF_SAADC_INPUT_DVDD,
    NRF_SAADC_INPUT_DISABLED,
    NRF_SAADC_INPUT_VDDL,
    NRF_SAADC_INPUT_DECB,
    NRF_SAADC_INPUT_VSS,
#elif defined(NRF54LS05B_ENGA_XXAA)
    NRF_SAADC_INPUT_DECB,
    NRF_SAADC_INPUT_DISABLED,
    NRF_SAADC_INPUT_DVDD,
    NRF_SAADC_INPUT_DISABLED,
    NRF_SAADC_INPUT_DISABLED,
    NRF_SAADC_INPUT_DECB,
    NRF_SAADC_INPUT_VSS,
#elif defined(NRF7120_ENGA_XXAA)
    NRF_SAADC_INPUT_DISABLED,
    NRF_SAADC_INPUT_DISABLED,
    NRF_SAADC_INPUT_DISABLED,
    NRF_SAADC_INPUT_DISABLED,
    NRF_SAADC_INPUT_DISABLED,
    NRF_SAADC_INPUT_DISABLED,
    NRF_SAADC_INPUT_DISABLED,
    NRF_SAADC_INPUT_DISABLED,
    NRF_SAADC_INPUT_VDDAO1V8,
    NRF_SAADC_INPUT_VDDAO0V8,
    NRF_SAADC_INPUT_VDDRF,
    NRF_SAADC_INPUT_VBAT,
#else
    NRF_SAADC_INPUT_DISABLED,
#endif
};

/**
 * @brief Get the SAADC input configuration for given analog input.
 *
 * @param[in] input  The analog input to get the configuration for.
 *
 * @return  The SAADC input configuration for the given analog input.
 *
 * @note    The function takes into account the internal and external analog inputs.
 *          The function returns NRF_SAADC_INPUT_DISABLED for analog inputs that are not
 *          supported by the given device.
 */
NRFX_STATIC_INLINE nrf_saadc_input_t nrfx_analog_saadc_ain_config_get(nrfx_analog_input_t input) {
    if ((input >= NRFX_ANALOG_AIN_INTERNAL_OFFSET) &&
        ((uint8_t)(input - NRFX_ANALOG_AIN_INTERNAL_OFFSET)
            < NRFX_ARRAY_SIZE(nrfx_analog_saadc_internal_ain_psels))) {
        return nrfx_analog_saadc_internal_ain_psels[(input - NRFX_ANALOG_AIN_INTERNAL_OFFSET)];
    } else if ((uint8_t)input < NRFX_ARRAY_SIZE(nrfx_analog_saadc_external_ain_psels)) {
        return nrfx_analog_saadc_external_ain_psels[input];
    }

    return NRF_SAADC_INPUT_DISABLED;
}
#else
NRFX_STATIC_INLINE nrf_saadc_input_t nrfx_analog_saadc_ain_config_get(nrfx_analog_input_t input) {
    switch (input) {
        case NRFX_ANALOG_EXTERNAL_AIN0:
            return NRF_SAADC_INPUT_AIN0;
        case NRFX_ANALOG_EXTERNAL_AIN1:
            return NRF_SAADC_INPUT_AIN1;
        case NRFX_ANALOG_EXTERNAL_AIN2:
            return NRF_SAADC_INPUT_AIN2;
        case NRFX_ANALOG_EXTERNAL_AIN3:
            return NRF_SAADC_INPUT_AIN3;
        case NRFX_ANALOG_EXTERNAL_AIN4:
            return NRF_SAADC_INPUT_AIN4;
        case NRFX_ANALOG_EXTERNAL_AIN5:
            return NRF_SAADC_INPUT_AIN5;
        case NRFX_ANALOG_EXTERNAL_AIN6:
            return NRF_SAADC_INPUT_AIN6;
        case NRFX_ANALOG_EXTERNAL_AIN7:
            return NRF_SAADC_INPUT_AIN7;
#if defined(SAADC_CH_PSELP_PSELP_VDD)
        case NRFX_ANALOG_INTERNAL_VDD:
            return NRF_SAADC_INPUT_VDD;
#endif
#if defined(SAADC_CH_PSELP_PSELP_VDDHDIV5)
        case NRFX_ANALOG_INTERNAL_VDDHDIV5:
            return NRF_SAADC_INPUT_VDDHDIV5;
#endif
#if defined(SAADC_CH_PSELP_PSELP_AnalogInput8)
        case NRFX_ANALOG_EXTERNAL_AIN8:
            return NRF_SAADC_INPUT_AIN8;
#endif
#if defined(SAADC_CH_PSELP_PSELP_AnalogInput9)
        case NRFX_ANALOG_EXTERNAL_AIN9:
            return NRF_SAADC_INPUT_AIN9;
#endif
#if defined(SAADC_CH_PSELP_PSELP_AnalogInput10)
        case NRFX_ANALOG_EXTERNAL_AIN10:
            return NRF_SAADC_INPUT_AIN10;
#endif
#if defined(SAADC_CH_PSELP_PSELP_AnalogInput11)
        case NRFX_ANALOG_EXTERNAL_AIN11:
            return NRF_SAADC_INPUT_AIN11;
#endif
#if defined(SAADC_CH_PSELP_PSELP_AnalogInput12)
        case NRFX_ANALOG_EXTERNAL_AIN12:
            return NRF_SAADC_INPUT_AIN12;
#endif
#if defined(SAADC_CH_PSELP_PSELP_AnalogInput13)
        case NRFX_ANALOG_EXTERNAL_AIN13:
            return NRF_SAADC_INPUT_AIN13;
#endif
        default:
            return NRF_SAADC_INPUT_DISABLED;
    }
}
#endif /* NRF_SAADC_HAS_AIN_AS_PIN */

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* NRFX_ANALOG_SAADC_COMMON_H__ */
