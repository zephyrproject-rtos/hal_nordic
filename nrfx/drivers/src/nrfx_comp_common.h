/*$$$LICENCE_NORDIC_STANDARD<2025>$$$*/

#ifndef NRFX_COMP_COMMON_H__
#define NRFX_COMP_COMMON_H__

#include <helpers/nrfx_analog_common.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrfx_comp_common Generic COMP inputs layer.
 * @{
 * @ingroup nrfx
 * @ingroup nrf_comp
 *
 * @brief Helper layer that provides the common functionality of COMP driver.
 */

 /** @brief Symbol specifying not existing analog input. */
#define NRFX_COMP_INPUT_NOT_PRESENT UINT32_MAX

/** @brief COMP external analog input pins mapping table. */
static const nrf_comp_input_t nrfx_comp_external_ain_psels[] =
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
    (nrf_comp_input_t)NRFX_COMP_INPUT_NOT_PRESENT,
    NRF_PIN_PORT_TO_PIN_NUMBER(0U, 4),
    NRF_PIN_PORT_TO_PIN_NUMBER(1U, 4),
    NRF_PIN_PORT_TO_PIN_NUMBER(2U, 4),
    NRF_PIN_PORT_TO_PIN_NUMBER(3U, 4),
    NRF_PIN_PORT_TO_PIN_NUMBER(4U, 4),
    NRF_PIN_PORT_TO_PIN_NUMBER(5U, 4),
#else /* legacy platforms */
    NRF_COMP_INPUT_0,
    NRF_COMP_INPUT_1,
    NRF_COMP_INPUT_2,
    NRF_COMP_INPUT_3,
#if defined(COMP_PSEL_PSEL_AnalogInput4)
    NRF_COMP_INPUT_4,
#endif
#if defined(COMP_PSEL_PSEL_AnalogInput5)
    NRF_COMP_INPUT_5,
#endif
#if defined(COMP_PSEL_PSEL_AnalogInput6)
    NRF_COMP_INPUT_6,
#endif
#if defined(COMP_PSEL_PSEL_AnalogInput7)
    NRF_COMP_INPUT_7,
#endif
#endif
};

/** @brief COMP internal inputs mapping table. */
static const nrf_comp_input_t nrfx_comp_internal_ain_psels[] =
{
    (nrf_comp_input_t)NRFX_COMP_INPUT_NOT_PRESENT,
#if defined(COMP_PSEL_PSEL_VddDiv2)
    NRF_COMP_VDD_DIV2,
#else
    (nrf_comp_input_t)NRFX_COMP_INPUT_NOT_PRESENT,
#endif
    (nrf_comp_input_t)NRFX_COMP_INPUT_NOT_PRESENT,
    (nrf_comp_input_t)NRFX_COMP_INPUT_NOT_PRESENT,
#if defined(COMP_PSEL_PSEL_VddhDiv5)
    NRF_COMP_VDDH_DIV5,
#endif
};

/** @brief COMP external analog reference pins mapping table. */
static const nrf_comp_ext_ref_t nrfx_comp_external_ref_psels[] =
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
    (nrf_comp_ext_ref_t)NRFX_COMP_INPUT_NOT_PRESENT,
    NRF_PIN_PORT_TO_PIN_NUMBER(0U, 4),
    NRF_PIN_PORT_TO_PIN_NUMBER(1U, 4),
    NRF_PIN_PORT_TO_PIN_NUMBER(2U, 4),
    NRF_PIN_PORT_TO_PIN_NUMBER(3U, 4),
    NRF_PIN_PORT_TO_PIN_NUMBER(4U, 4),
    NRF_PIN_PORT_TO_PIN_NUMBER(5U, 4),
#else /* legacy platforms */
    NRF_COMP_EXT_REF_0,
    NRF_COMP_EXT_REF_1,
    NRF_COMP_EXT_REF_2,
    NRF_COMP_EXT_REF_3,
#if defined(COMP_EXTREFSEL_EXTREFSEL_AnalogReference4)
    NRF_COMP_EXT_REF_4,
#endif
#if defined(COMP_EXTREFSEL_EXTREFSEL_AnalogReference5)
    NRF_COMP_EXT_REF_5,
#endif
#if defined(COMP_EXTREFSEL_EXTREFSEL_AnalogReference6)
    NRF_COMP_EXT_REF_6,
#endif
#if defined(COMP_EXTREFSEL_EXTREFSEL_AnalogReference7)
    NRF_COMP_EXT_REF_7,
#endif
#endif
};

/**
 * @brief Function for getting the COMP input configuration for given generic analog input.
 *
 * @param[in] input The generic analog input to get the configuration for.
 *
 * @return The COMP input configuration for the given analog input.
 *
 * @note The function takes into account the internal and external analog inputs.
 *       The function returns NRFX_COMP_INPUT_NOT_PRESENT for analog inputs that are not
 *       supported by the given device.
 */
NRFX_STATIC_INLINE nrf_comp_input_t nrfx_comp_ain_get(nrfx_analog_input_t input)
{
    if ((input >= NRFX_ANALOG_AIN_INTERNAL_OFFSET) &&
        ((uint8_t)(input - NRFX_ANALOG_AIN_INTERNAL_OFFSET)
            < NRFX_ARRAY_SIZE(nrfx_comp_internal_ain_psels)))
    {
        return nrfx_comp_internal_ain_psels[(input - NRFX_ANALOG_AIN_INTERNAL_OFFSET)];
    }
    else if ((uint8_t)input < NRFX_ARRAY_SIZE(nrfx_comp_external_ain_psels))
    {
        return nrfx_comp_external_ain_psels[input];
    }

    return (nrf_comp_input_t)NRFX_COMP_INPUT_NOT_PRESENT;
}

/**
 * @brief Function for getting the COMP external reference input configuration
 *        for given generic analog input.
 *
 * @param[in] input The generic analog input to get the configuration for.
 *
 * @return The COMP external reference input configuration for the given analog input.
 *
 * @note The function returns NRFX_COMP_INPUT_NOT_PRESENT for reference inputs that are not
 *       supported by the given device.
 */
NRFX_STATIC_INLINE nrf_comp_ext_ref_t nrfx_comp_ext_ref_get(nrfx_analog_input_t input)
{
    if ((uint8_t)input < NRFX_ARRAY_SIZE(nrfx_comp_external_ref_psels))
    {
        return nrfx_comp_external_ref_psels[input];
    }

    return (nrf_comp_ext_ref_t)NRFX_COMP_INPUT_NOT_PRESENT;
}

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* NRFX_COMP_COMMON_H__ */
