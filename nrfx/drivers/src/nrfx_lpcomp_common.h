/*$$$LICENCE_NORDIC_STANDARD<2025>$$$*/

#ifndef NRFX_LPCOMP_COMMON_H__
#define NRFX_LPCOMP_COMMON_H__

#include <helpers/nrfx_analog_common.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrfx_lpcomp_common Generic LPCOMP inputs layer.
 * @{
 * @ingroup nrfx
 * @ingroup nrf_lpcomp
 *
 * @brief Helper layer that provides the common functionality of LPCOMP driver.
 */

/** @brief Symbol specifying not existing analog input. */
#define NRFX_LPCOMP_INPUT_NOT_PRESENT UINT32_MAX

 /** @brief LPCOMP external analog input pins mapping table. */
static const nrf_lpcomp_input_t nrfx_lpcomp_psels[] =
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
    (nrf_lpcomp_input_t)NRFX_LPCOMP_INPUT_NOT_PRESENT,
    NRF_PIN_PORT_TO_PIN_NUMBER(0U, 4),
    NRF_PIN_PORT_TO_PIN_NUMBER(1U, 4),
    NRF_PIN_PORT_TO_PIN_NUMBER(2U, 4),
    NRF_PIN_PORT_TO_PIN_NUMBER(3U, 4),
    NRF_PIN_PORT_TO_PIN_NUMBER(4U, 4),
    NRF_PIN_PORT_TO_PIN_NUMBER(5U, 4),
#else /* legacy platforms */
    NRF_LPCOMP_INPUT_0,
    NRF_LPCOMP_INPUT_1,
    NRF_LPCOMP_INPUT_2,
    NRF_LPCOMP_INPUT_3,
    NRF_LPCOMP_INPUT_4,
    NRF_LPCOMP_INPUT_5,
    NRF_LPCOMP_INPUT_6,
    NRF_LPCOMP_INPUT_7,
#endif
};

/** @brief LPCOMP external analog reference pins mapping table. */
static const nrf_lpcomp_ext_ref_t nrfx_lpcomp_external_ref_psels[] =
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
    (nrf_lpcomp_ext_ref_t)NRFX_LPCOMP_INPUT_NOT_PRESENT,
    NRF_PIN_PORT_TO_PIN_NUMBER(0U, 4),
    NRF_PIN_PORT_TO_PIN_NUMBER(1U, 4),
    NRF_PIN_PORT_TO_PIN_NUMBER(2U, 4),
    NRF_PIN_PORT_TO_PIN_NUMBER(3U, 4),
    NRF_PIN_PORT_TO_PIN_NUMBER(4U, 4),
    NRF_PIN_PORT_TO_PIN_NUMBER(5U, 4),
#else /* legacy platforms */
    NRF_LPCOMP_EXT_REF_REF0,
    NRF_LPCOMP_EXT_REF_REF1,
#endif
};

/**
 * @brief Function for getting the LPCOMP input configuration for given generic analog input.
 *
 * @param[in] input The generic analog input to get the configuration for.
 *
 * @return The LPCOMP input configuration for the given analog input.
 *
 * @note The function returns NRFX_LPCOMP_INPUT_NOT_PRESENT for analog inputs that are not
 *       supported by the given device.
 */
NRFX_STATIC_INLINE nrf_lpcomp_input_t nrfx_lpcomp_ain_get(nrfx_analog_input_t input)
{
    if (input < NRFX_ARRAY_SIZE(nrfx_lpcomp_psels))
    {
        return nrfx_lpcomp_psels[input];
    }

    return (nrf_lpcomp_input_t)NRFX_LPCOMP_INPUT_NOT_PRESENT;
}

/**
 * @brief Function for getting the LPCOMP external reference input configuration
 *        for given generic analog input.
 *
 * @param[in] input The generic analog input to get the configuration for.
 *
 * @return The LPCOMP external reference input configuration for the given analog input.
 *
 * @note The function returns NRFX_LPCOMP_INPUT_NOT_PRESENT for reference inputs that are not
 *       supported by the given device.
 */
NRFX_STATIC_INLINE nrf_lpcomp_ext_ref_t nrfx_lpcomp_ext_ref_get(nrfx_analog_input_t input)
{
    if ((uint8_t)input < NRFX_ARRAY_SIZE(nrfx_lpcomp_external_ref_psels))
    {
        return nrfx_lpcomp_external_ref_psels[input];
    }

    return (nrf_lpcomp_ext_ref_t)NRFX_LPCOMP_INPUT_NOT_PRESENT;
}

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* NRFX_LPCOMP_COMMON_H__ */
