/*$$$LICENCE_NORDIC_STANDARD<2025>$$$*/

#ifndef NRFX_SAADC_COMMON_H__
#define NRFX_SAADC_COMMON_H__

#include <helpers/nrfx_analog_common.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrfx_saadc_common Generic SAADC input layer.
 * @{
 * @ingroup nrfx
 * @ingroup nrf_saadc
 *
 * @brief Helper layer that provides the common functionality of SAADC driver.
 */

/** @brief Symbol specifying not existing analog input. */
#define NRFX_SAADC_INPUT_NOT_PRESENT NRF_SAADC_INPUT_DISABLED

/** @brief SAADC external analog input pins mapping table. */
static const nrf_saadc_input_t nrfx_saadc_external_ain_psels[] =
{
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
    NRFX_SAADC_INPUT_NOT_PRESENT,
    NRF_PIN_PORT_TO_PIN_NUMBER(0U, 4),
    NRF_PIN_PORT_TO_PIN_NUMBER(1U, 4),
    NRF_PIN_PORT_TO_PIN_NUMBER(2U, 4),
    NRF_PIN_PORT_TO_PIN_NUMBER(3U, 4),
    NRF_PIN_PORT_TO_PIN_NUMBER(4U, 4),
    NRF_PIN_PORT_TO_PIN_NUMBER(5U, 4),
#else /* legacy platforms */
    NRF_SAADC_INPUT_AIN0,
    NRF_SAADC_INPUT_AIN1,
    NRF_SAADC_INPUT_AIN2,
    NRF_SAADC_INPUT_AIN3,
    NRF_SAADC_INPUT_AIN4,
    NRF_SAADC_INPUT_AIN5,
    NRF_SAADC_INPUT_AIN6,
    NRF_SAADC_INPUT_AIN7,
#if defined(SAADC_CH_PSELP_PSELP_AnalogInput8)
    NRF_SAADC_INPUT_AIN8,
#endif
#if defined(SAADC_CH_PSELP_PSELP_AnalogInput9)
    NRF_SAADC_INPUT_AIN9,
#endif
#if defined(SAADC_CH_PSELP_PSELP_AnalogInput10)
    NRF_SAADC_INPUT_AIN10,
#endif
#if defined(SAADC_CH_PSELP_PSELP_AnalogInput11)
    NRF_SAADC_INPUT_AIN11,
#endif
#if defined(SAADC_CH_PSELP_PSELP_AnalogInput12)
    NRF_SAADC_INPUT_AIN12,
#endif
#if defined(SAADC_CH_PSELP_PSELP_AnalogInput13)
    NRF_SAADC_INPUT_AIN13,
#endif
#endif
};

/** @brief SAADC internal inputs mapping table. */
static const nrf_saadc_input_t nrfx_saadc_internal_ain_psels[] =
{
#if defined(NRF54L05_XXAA) || defined(NRF54L10_XXAA) || defined(NRF54L15_XXAA)
    NRF_SAADC_INPUT_VDD,
    NRFX_SAADC_INPUT_NOT_PRESENT,
    NRF_SAADC_INPUT_AVDD,
    NRF_SAADC_INPUT_DVDD,
#elif defined(NRF54LM20A_ENGA_XXAA)
    NRF_SAADC_INPUT_VDD,
    NRFX_SAADC_INPUT_NOT_PRESENT,
    NRF_SAADC_INPUT_AVDD,
    NRF_SAADC_INPUT_DVDD,
#elif defined(NRF54LV10A_ENGA_XXAA)
    NRFX_SAADC_INPUT_NOT_PRESENT,
    NRFX_SAADC_INPUT_NOT_PRESENT,
    NRFX_SAADC_INPUT_NOT_PRESENT,
    NRF_SAADC_INPUT_DVDD,
    NRFX_SAADC_INPUT_NOT_PRESENT,
    NRF_SAADC_INPUT_VDDL,
    NRFX_SAADC_INPUT_NOT_PRESENT,
    NRF_SAADC_INPUT_VSS,
#elif defined(NRF54LS05B_ENGA_XXAA)
    NRF_SAADC_INPUT_DECB,
    NRFX_SAADC_INPUT_NOT_PRESENT,
    NRFX_SAADC_INPUT_NOT_PRESENT,
    NRF_SAADC_INPUT_DVDD,
    NRFX_SAADC_INPUT_NOT_PRESENT,
    NRFX_SAADC_INPUT_NOT_PRESENT,
    NRFX_SAADC_INPUT_NOT_PRESENT,
    NRF_SAADC_INPUT_VSS,
#elif defined(NRF7120_ENGA_XXAA)
    NRFX_SAADC_INPUT_NOT_PRESENT,
    NRFX_SAADC_INPUT_NOT_PRESENT,
    NRFX_SAADC_INPUT_NOT_PRESENT,
    NRFX_SAADC_INPUT_NOT_PRESENT,
    NRFX_SAADC_INPUT_NOT_PRESENT,
    NRFX_SAADC_INPUT_NOT_PRESENT,
    NRFX_SAADC_INPUT_NOT_PRESENT,
    NRFX_SAADC_INPUT_NOT_PRESENT,
    NRFX_SAADC_INPUT_NOT_PRESENT,
    NRF_SAADC_INPUT_INTERNAL0,
    NRF_SAADC_INPUT_INTERNAL1,
    NRF_SAADC_INPUT_INTERNAL2,
    NRF_SAADC_INPUT_INTERNAL3,
#else  /* legacy platforms */
#if defined(SAADC_CH_PSELP_PSELP_VDD)
    NRF_SAADC_INPUT_VDD,
#else
    NRFX_SAADC_INPUT_NOT_PRESENT,
#endif
    NRFX_SAADC_INPUT_NOT_PRESENT,
    NRFX_SAADC_INPUT_NOT_PRESENT,
    NRFX_SAADC_INPUT_NOT_PRESENT,
#if defined(SAADC_CH_PSELP_PSELP_VDDHDIV5)
    NRF_SAADC_INPUT_VDDHDIV5,
#else
    NRFX_SAADC_INPUT_NOT_PRESENT,
#endif
#endif
};

/**
 * @brief Function for getting the SAADC input configuration for given generic analog input.
 *
 * @param[in] input The generic analog input to get the configuration for.
 *
 * @return The SAADC input configuration for the given analog input.
 *
 * @note The function takes into account the internal and external analog inputs.
 *       The function returns NRFX_SAADC_INPUT_NOT_PRESENT for analog inputs that are not
 *       supported by the given device.
 */
NRFX_STATIC_INLINE nrf_saadc_input_t nrfx_saadc_ain_get(nrfx_analog_input_t input)
{
    if ((input >= NRFX_ANALOG_AIN_INTERNAL_OFFSET) &&
        ((uint8_t)(input - NRFX_ANALOG_AIN_INTERNAL_OFFSET)
            < NRFX_ARRAY_SIZE(nrfx_saadc_internal_ain_psels)))
    {
        return nrfx_saadc_internal_ain_psels[(input - NRFX_ANALOG_AIN_INTERNAL_OFFSET)];
    }
    else if ((uint8_t)input < NRFX_ARRAY_SIZE(nrfx_saadc_external_ain_psels))
    {
        return nrfx_saadc_external_ain_psels[input];
    }

    return (nrf_saadc_input_t)NRFX_SAADC_INPUT_NOT_PRESENT;
}

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* NRFX_SAADC_COMMON_H__ */
