/*
 * Copyright (c) 2020, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef NRF_FEM_21540_SPI_CONFIG_H_
#define NRF_FEM_21540_SPI_CONFIG_H_

#include <stdint.h>
#include "nrf.h"
#include "nrf_fem_common_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief The ID of "nrf21540spi" FEM type (e.g. for run-time selection)  */
#define NRF_FEM_IF_TYPE_NRF21540_SPI 3

typedef struct
{
    uint8_t mosi_pin;
    uint8_t sck_pin;
    uint8_t csn_pin;
    uint8_t pdn_pin;
} nrf_fem_spi_pins_config_t;

/**
 * @brief Configuration parameters for nRF21540 SPI interface.
 */
typedef struct
{
    nrf_fem_spi_pins_config_t pins_config; /* Pins configuration. */
    NRF_SPIM_Type           * p_spi;       /* SPIM instance to be used with the nRF21540. */

    uint8_t                   ppi_channels[4];

} nrf_fem_nrf21540_spi_interface_config_t;

/**
 * @brief Configures the PA and LNA device interface.
 *
 * This function sets device interface parameters for the PA/LNA module.
 * The module can then be used to control a power amplifier or a low noise amplifier (or both) through the given interface and resources.
 *
 * The function also sets the PPI and GPIOTE channels to be configured for the PA/LNA interface.
 *
 * @param[in] p_config Pointer to the interface parameters for the PA/LNA device.
 *
 * @retval   ::NRFX_SUCCESS                 PA/LNA control successfully configured.
 * @retval   ::NRFX_ERROR_NOT_SUPPORTED     PA/LNA is not available.
 *
 */
int32_t nrf_fem_nrf21540spi_interface_config_set(
    nrf_fem_nrf21540_spi_interface_config_t const * const p_config);

/**
 * @brief Retrieves the configuration of PA and LNA device interface.
 *
 * This function gets device interface parameters for the PA/LNA module.
 * The module can then be used to control a power amplifier or a low noise amplifier (or both) through the given interface and resources.
 *
 *
 * @param[in] p_config Pointer to the interface parameters for the PA/LNA device to be populated.
 *
 * @retval   ::NRFX_SUCCESS                 PA/LNA control successfully configured.
 * @retval   ::NRFX_ERROR_NOT_SUPPORTED     PA/LNA is not available.
 * @retval   ::NRFX_ERROR_FORBIDDEN         The "nrf21540spi" is not currently selected as the FEM type.
 *
 */
int32_t nrf_fem_nrf21540spi_interface_config_get(
    nrf_fem_nrf21540_spi_interface_config_t * p_config);

#if 0 /* TODO these definitions should be moved into the implementation part (*.c) */
      /* or (some of them) removed completely                                      */
/**
 * @section Timings.
 */

/** Time in microseconds when PA GPIO is activated before the radio is ready for transmission. */
#define NRF_FEM_PA_TIME_IN_ADVANCE_US  13

/** Time in microseconds when LNA GPIO is activated before the radio is ready for reception. */
#define NRF_FEM_LNA_TIME_IN_ADVANCE_US 13

/** The time between activating the PDN and asserting the RX_EN/TX_EN. */
#define NRF_FEM_PDN_SETTLE_US          18

/** The time between deasserting the RX_EN/TX_EN and deactivating PDN. */
#define NRF_FEM_TRX_HOLD_US            5

#ifdef NRF52811_XXAA
/** Default SPI MOSI pin. */
#define NRF_FEM_CONTROL_DEFAULT_MOSI_PIN 19

/** Default SPI SCK pin. */
#define NRF_FEM_CONTROL_DEFAULT_SCK_PIN  20

/** Default SPI CSN pin. */
#define NRF_FEM_CONTROL_DEFAULT_CSN_PIN  21

#else

/** Default Power Amplifier pin. */
#define NRF_FEM_CONTROL_DEFAULT_MOSI_PIN 15

/** Default Low Noise Amplifier pin. */
#define NRF_FEM_CONTROL_DEFAULT_SCK_PIN  16

/** Default SPI CSN pin. */
#define NRF_FEM_CONTROL_DEFAULT_CSN_PIN  17

#endif

/** Default nRF21540 PDN pin. */
#define NRF_FEM_CONTROL_DEFAULT_PDN_PIN            13

/** Default PPI channel for setting. */
#define NRF_FEM_CONTROL_DEFAULT_SET_PPI_CHANNEL    15

/** Default PPI channel for clearing. */
#define NRF_FEM_CONTROL_DEFAULT_CLR_PPI_CHANNEL    16

/** Default PPI channel for SPI continiuty. */
#define NRF_FEM_CONTROL_DEFAULT_SPI_PPI_CHANNEL    17

/** Default PPI channel for PDN pin handling. */
#define NRF_FEM_CONTROL_DEFAULT_PDN_PPI_CHANNEL    5

/** Default GPIOTE channel for PDN control. */
#define NRF_FEM_CONTROL_DEFAULT_PDN_GPIOTE_CHANNEL 5

/** Mask of GPIO pins used for FEM control. */
#define NRF_802154_FEM_PINS_USED_MASK              ((1 << NRF_FEM_CONTROL_DEFAULT_PA_PIN) |  \
                                                    (1 << NRF_FEM_CONTROL_DEFAULT_LNA_PIN) | \
                                                    (1 << NRF_FEM_CONTROL_DEFAULT_PDN_PIN))

/** Mask of PPI channels used for FEM control. */
#define NRF_802154_FEM_PPI_CHANNELS_USED_MASK      ((1 << NRF_FEM_CONTROL_DEFAULT_SET_PPI_CHANNEL) | \
                                                    (1 << NRF_FEM_CONTROL_DEFAULT_CLR_PPI_CHANNEL) | \
                                                    (1 << NRF_FEM_CONTROL_DEFAULT_SPI_PPI_CHANNEL) | \
                                                    (1 << NRF_FEM_CONTROL_DEFAULT_PDN_PPI_CHANNEL))

/** Mask of GPIOTE channels used for FEM control. */
#define NRF_802154_FEM_GPIOTE_CHANNELS_USED_MASK   ( \
        (1 << NRF_FEM_CONTROL_DEFAULT_PDN_GPIOTE_CHANNEL))

#endif // #if 0 /* <-- these definitions should be moved ..  */

#ifdef __cplusplus
}
#endif

#endif /* NRF_FEM_21540_SPI_CONFIG_H_ */
