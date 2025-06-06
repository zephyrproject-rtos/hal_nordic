/*
 * Copyright (c) 2015 - 2025, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef NRF_I2S_H__
#define NRF_I2S_H__

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(HALTIUM_XXAA)
#define NRF_I2S_CLOCKPIN_SCK_NEEDED  1
#define NRF_I2S_CLOCKPIN_LRCK_NEEDED 1
#define NRF_I2S_CLOCKPIN_MCK_NEEDED  1
#endif

#if !defined(NRF_I2S0) && defined(NRF_I2S)
#define NRF_I2S0 NRF_I2S
#endif

/**
 * @defgroup nrf_i2s_hal I2S HAL
 * @{
 * @ingroup nrf_i2s
 * @brief   Hardware access layer for managing the Inter-IC Sound (I2S) peripheral.
 */

#if defined(I2S_CONFIG_CLKCONFIG_CLKSRC_Msk) || defined(__NRFX_DOXYGEN__)
/** @brief Symbol indicating whether clock source configuration is available. */
#define NRF_I2S_HAS_CLKCONFIG 1
#else
#define NRF_I2S_HAS_CLKCONFIG 0
#endif

#if defined(I2S_INTENSET_FRAMESTART_Msk) || defined(__NRFX_DOXYGEN__)
/** @brief Symbol indicating whether frame start event is available. */
#define NRF_I2S_HAS_FRAMESTART 1
#else
#define NRF_I2S_HAS_FRAMESTART 0
#endif

#if defined(I2S_PSEL_SCK_PORT_Msk) || defined(__NRFX_DOXYGEN__)
/** @brief Symbol indicating whether GPIO port selection for pins is available. */
#define NRF_I2S_HAS_GPIO_PORT_SELECTION 1
#else
#define NRF_I2S_HAS_GPIO_PORT_SELECTION 0
#endif

#if defined(I2S_CONFIG_SWIDTH_SWIDTH_32Bit) || defined(__NRFX_DOXYGEN__)
/** @brief Symbol indicating whether 32-bit sample width is available. */
#define NRF_I2S_HAS_SWIDTH_32BIT 1
#else
#define NRF_I2S_HAS_SWIDTH_32BIT 0
#endif

/**
 * @brief This value can be provided as a parameter for the @ref nrf_i2s_pins_set
 *        function call to specify that the given I2S signal (SDOUT, SDIN, or MCK)
 *        shall not be connected to a physical pin.
 */
#define NRF_I2S_PIN_NOT_CONNECTED  0xFFFFFFFF

/** @brief I2S SCK pin selection mask. */
#define NRF_I2S_PSEL_SCK_PIN_MASK  I2S_PSEL_SCK_PIN_Msk

/** @brief I2S SCK port selection mask. */
#define NRF_I2S_PSEL_SCK_PORT_MASK I2S_PSEL_SCK_PORT_Msk

/** @brief I2S tasks. */
typedef enum
{
    NRF_I2S_TASK_START = offsetof(NRF_I2S_Type, TASKS_START), ///< Starts continuous I2S transfer. Also starts the MCK generator if this is enabled.
    NRF_I2S_TASK_STOP  = offsetof(NRF_I2S_Type, TASKS_STOP)   ///< Stops I2S transfer. Also stops the MCK generator.
} nrf_i2s_task_t;

/** @brief I2S events. */
typedef enum
{
    NRF_I2S_EVENT_RXPTRUPD   = offsetof(NRF_I2S_Type, EVENTS_RXPTRUPD),  ///< The RXD.PTR register has been copied to internal double buffers.
    NRF_I2S_EVENT_TXPTRUPD   = offsetof(NRF_I2S_Type, EVENTS_TXPTRUPD),  ///< The TXD.PTR register has been copied to internal double buffers.
    NRF_I2S_EVENT_STOPPED    = offsetof(NRF_I2S_Type, EVENTS_STOPPED),   ///< I2S transfer stopped.
#if NRF_I2S_HAS_FRAMESTART
    NRF_I2S_EVENT_FRAMESTART = offsetof(NRF_I2S_Type, EVENTS_FRAMESTART) ///< Frame start event, generated on the active edge of LRCK.
#endif
} nrf_i2s_event_t;

/** @brief I2S interrupts. */
typedef enum
{
    NRF_I2S_INT_RXPTRUPD_MASK   = I2S_INTENSET_RXPTRUPD_Msk,  ///< Interrupt on RXPTRUPD event.
    NRF_I2S_INT_TXPTRUPD_MASK   = I2S_INTENSET_TXPTRUPD_Msk,  ///< Interrupt on TXPTRUPD event.
    NRF_I2S_INT_STOPPED_MASK    = I2S_INTENSET_STOPPED_Msk,   ///< Interrupt on STOPPED event.
#if NRF_I2S_HAS_FRAMESTART
    NRF_I2S_INT_FRAMESTART_MASK = I2S_INTENCLR_FRAMESTART_Msk ///< Interrupt on FRAMESTART event.
#endif
} nrf_i2s_int_mask_t;

/** @brief I2S modes of operation. */
typedef enum
{
    NRF_I2S_MODE_MASTER = I2S_CONFIG_MODE_MODE_Master, ///< Master mode.
    NRF_I2S_MODE_SLAVE  = I2S_CONFIG_MODE_MODE_Slave   ///< Slave mode.
} nrf_i2s_mode_t;

/** @brief I2S master clock generator settings. */
typedef enum
{
    NRF_I2S_MCK_DISABLED  = 0,                                       ///< MCK disabled.
#if defined(I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV2) || defined(__NRFX_DOXYGEN__)
    // [conversion to 'int' needed to prevent compilers from complaining
    //  that the provided value (0x80000000UL) is out of range of "int"]
    NRF_I2S_MCK_32MDIV2   = (int)I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV2, ///< 32 MHz / 2 = 16.0 MHz.
#endif
#if defined(I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV3) || defined(__NRFX_DOXYGEN__)
    NRF_I2S_MCK_32MDIV3   = I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV3,      ///< 32 MHz / 3 = 10.6666667 MHz.
#endif
#if defined(I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV4) || defined(__NRFX_DOXYGEN__)
    NRF_I2S_MCK_32MDIV4   = I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV4,      ///< 32 MHz / 4 = 8.0 MHz.
#endif
#if defined(I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV5) || defined(__NRFX_DOXYGEN__)
    NRF_I2S_MCK_32MDIV5   = I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV5,      ///< 32 MHz / 5 = 6.4 MHz.
#endif
#if defined(I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV6) || defined(__NRFX_DOXYGEN__)
    NRF_I2S_MCK_32MDIV6   = I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV6,      ///< 32 MHz / 6 = 5.3333333 MHz.
#endif
    NRF_I2S_MCK_32MDIV8   = I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV8,      ///< 32 MHz / 8 = 4.0 MHz.
    NRF_I2S_MCK_32MDIV10  = I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV10,     ///< 32 MHz / 10 = 3.2 MHz.
    NRF_I2S_MCK_32MDIV11  = I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV11,     ///< 32 MHz / 11 = 2.9090909 MHz.
    NRF_I2S_MCK_32MDIV15  = I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV15,     ///< 32 MHz / 15 = 2.1333333 MHz.
    NRF_I2S_MCK_32MDIV16  = I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV16,     ///< 32 MHz / 16 = 2.0 MHz.
    NRF_I2S_MCK_32MDIV21  = I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV21,     ///< 32 MHz / 21 = 1.5238095 MHz.
    NRF_I2S_MCK_32MDIV23  = I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV23,     ///< 32 MHz / 23 = 1.3913043 MHz.
    NRF_I2S_MCK_32MDIV30  = I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV30,     ///< 32 MHz / 30 = 1.0666667 MHz.
    NRF_I2S_MCK_32MDIV31  = I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV31,     ///< 32 MHz / 31 = 1.0322581 MHz.
    NRF_I2S_MCK_32MDIV32  = I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV32,     ///< 32 MHz / 32 = 1.0 MHz.
    NRF_I2S_MCK_32MDIV42  = I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV42,     ///< 32 MHz / 42 = 0.7619048 MHz.
    NRF_I2S_MCK_32MDIV63  = I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV63,     ///< 32 MHz / 63 = 0.5079365 MHz.
    NRF_I2S_MCK_32MDIV125 = I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV125     ///< 32 MHz / 125 = 0.256 MHz.
} nrf_i2s_mck_t;

/** @brief I2S MCK/LRCK ratios. */
typedef enum
{
    NRF_I2S_RATIO_32X  = I2S_CONFIG_RATIO_RATIO_32X,  ///< LRCK = MCK / 32.
    NRF_I2S_RATIO_48X  = I2S_CONFIG_RATIO_RATIO_48X,  ///< LRCK = MCK / 48.
    NRF_I2S_RATIO_64X  = I2S_CONFIG_RATIO_RATIO_64X,  ///< LRCK = MCK / 64.
    NRF_I2S_RATIO_96X  = I2S_CONFIG_RATIO_RATIO_96X,  ///< LRCK = MCK / 96.
    NRF_I2S_RATIO_128X = I2S_CONFIG_RATIO_RATIO_128X, ///< LRCK = MCK / 128.
    NRF_I2S_RATIO_192X = I2S_CONFIG_RATIO_RATIO_192X, ///< LRCK = MCK / 192.
    NRF_I2S_RATIO_256X = I2S_CONFIG_RATIO_RATIO_256X, ///< LRCK = MCK / 256.
    NRF_I2S_RATIO_384X = I2S_CONFIG_RATIO_RATIO_384X, ///< LRCK = MCK / 384.
    NRF_I2S_RATIO_512X = I2S_CONFIG_RATIO_RATIO_512X  ///< LRCK = MCK / 512.
} nrf_i2s_ratio_t;

/** @brief I2S sample widths. */
typedef enum
{
    NRF_I2S_SWIDTH_8BIT          = I2S_CONFIG_SWIDTH_SWIDTH_8Bit,      ///< 8 bit.
    NRF_I2S_SWIDTH_16BIT         = I2S_CONFIG_SWIDTH_SWIDTH_16Bit,     ///< 16 bit.
    NRF_I2S_SWIDTH_24BIT         = I2S_CONFIG_SWIDTH_SWIDTH_24Bit,     ///< 24 bit.
#if NRF_I2S_HAS_SWIDTH_32BIT
    NRF_I2S_SWIDTH_32BIT         = I2S_CONFIG_SWIDTH_SWIDTH_32Bit,     ///< 32 bit.
#endif
#if defined(I2S_CONFIG_SWIDTH_SWIDTH_8BitIn16) || defined(__NRFX_DOXYGEN__)
    NRF_I2S_SWIDTH_8BIT_IN16BIT  = I2S_CONFIG_SWIDTH_SWIDTH_8BitIn16,  ///< 8 bit sample in a 16-bit half-frame.
#endif
#if defined(I2S_CONFIG_SWIDTH_SWIDTH_8BitIn32) || defined(__NRFX_DOXYGEN__)
    NRF_I2S_SWIDTH_8BIT_IN32BIT  = I2S_CONFIG_SWIDTH_SWIDTH_8BitIn32,  ///< 8 bit sample in a 32-bit half-frame.
#endif
#if defined(I2S_CONFIG_SWIDTH_SWIDTH_16BitIn32) || defined(__NRFX_DOXYGEN__)
    NRF_I2S_SWIDTH_16BIT_IN32BIT = I2S_CONFIG_SWIDTH_SWIDTH_16BitIn32, ///< 16 bit sample in a 32-bit half-frame.
#endif
#if defined(I2S_CONFIG_SWIDTH_SWIDTH_24BitIn32) || defined(__NRFX_DOXYGEN__)
    NRF_I2S_SWIDTH_24BIT_IN32BIT = I2S_CONFIG_SWIDTH_SWIDTH_24BitIn32, ///< 24 bit sample in a 32-bit half-frame.
#endif
} nrf_i2s_swidth_t;

/** @brief I2S alignments of sample within a frame. */
typedef enum
{
    NRF_I2S_ALIGN_LEFT  = I2S_CONFIG_ALIGN_ALIGN_Left, ///< Left-aligned.
    NRF_I2S_ALIGN_RIGHT = I2S_CONFIG_ALIGN_ALIGN_Right ///< Right-aligned.
} nrf_i2s_align_t;

/** @brief I2S frame formats. */
typedef enum
{
    NRF_I2S_FORMAT_I2S     = I2S_CONFIG_FORMAT_FORMAT_I2S,    ///< Original I2S format.
    NRF_I2S_FORMAT_ALIGNED = I2S_CONFIG_FORMAT_FORMAT_Aligned ///< Alternate (left-aligned or right-aligned) format.
} nrf_i2s_format_t;

/** @brief I2S enabled channels. */
typedef enum
{
    NRF_I2S_CHANNELS_STEREO = I2S_CONFIG_CHANNELS_CHANNELS_Stereo, ///< Stereo.
    NRF_I2S_CHANNELS_LEFT   = I2S_CONFIG_CHANNELS_CHANNELS_Left,   ///< Left only.
    NRF_I2S_CHANNELS_RIGHT  = I2S_CONFIG_CHANNELS_CHANNELS_Right   ///< Right only.
} nrf_i2s_channels_t;

#if NRF_I2S_HAS_CLKCONFIG
/** @brief I2S Clock source selection. */
typedef enum
{
    NRF_I2S_CLKSRC_PCLK32M = I2S_CONFIG_CLKCONFIG_CLKSRC_PCLK32M, ///< 32MHz peripheral clock.
    NRF_I2S_CLKSRC_ACLK    = I2S_CONFIG_CLKCONFIG_CLKSRC_ACLK     ///< Audio PLL clock.
} nrf_i2s_clksrc_t;
#endif

/** @brief I2S configuration. */
typedef struct
{
    nrf_i2s_mode_t     mode;         /**< Mode of operation (master or slave). */
    nrf_i2s_format_t   format;       /**< I2S frame format. */
    nrf_i2s_align_t    alignment;    /**< Alignment of sample within a frame. */
    nrf_i2s_swidth_t   sample_width; /**< Sample width. */
    nrf_i2s_channels_t channels;     /**< Enabled channels. */
    nrf_i2s_mck_t      mck_setup;    /**< Master clock generator setup. */
    nrf_i2s_ratio_t    ratio;        /**< MCK/LRCK ratio. */
} nrf_i2s_config_t;

/** @brief I2S pins. */
typedef struct
{
    uint32_t sck_pin;   ///< SCK pin number.
    uint32_t lrck_pin;  ///< LRCK pin number.
    uint32_t mck_pin;   ///< MCK pin number.
                        /**< Optional. Use @ref NRF_I2S_PIN_NOT_CONNECTED
                         *   if this signal is not needed. */
    uint32_t sdout_pin; ///< SDOUT pin number.
                        /**< Optional. Use @ref NRF_I2S_PIN_NOT_CONNECTED
                         *   if this signal is not needed. */
    uint32_t sdin_pin;  ///< SDIN pin number.
                        /**< Optional. Use @ref NRF_I2S_PIN_NOT_CONNECTED
                         *   if this signal is not needed. */
} nrf_i2s_pins_t;

/**
 * @brief Function for activating the specified I2S task.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] task  Task to be activated.
 */
NRF_STATIC_INLINE void nrf_i2s_task_trigger(NRF_I2S_Type * p_reg,
                                            nrf_i2s_task_t task);

/**
 * @brief Function for getting the address of the specified I2S task register.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] task  Specified task.
 *
 * @return Address of the specified task register.
 */
NRF_STATIC_INLINE uint32_t nrf_i2s_task_address_get(NRF_I2S_Type const * p_reg,
                                                    nrf_i2s_task_t       task);

/**
 * @brief Function for clearing the specified I2S event.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Event to clear.
 */
NRF_STATIC_INLINE void nrf_i2s_event_clear(NRF_I2S_Type *  p_reg,
                                           nrf_i2s_event_t event);

/**
 * @brief Function for retrieving the state of the I2S event.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Event to be checked.
 *
 * @retval true  The event has been generated.
 * @retval false The event has not been generated.
 */
NRF_STATIC_INLINE bool nrf_i2s_event_check(NRF_I2S_Type const * p_reg,
                                           nrf_i2s_event_t      event);

/**
 * @brief Function for getting the address of the specified I2S event register.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Specified event.
 *
 * @return Address of the specified event register.
 */
NRF_STATIC_INLINE uint32_t nrf_i2s_event_address_get(NRF_I2S_Type const * p_reg,
                                                     nrf_i2s_event_t      event);

/**
 * @brief Function for enabling specified interrupts.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Mask of interrupts to be enabled.
 *                  Use @ref nrf_i2s_int_mask_t values for bit masking.
 */
NRF_STATIC_INLINE void nrf_i2s_int_enable(NRF_I2S_Type * p_reg, uint32_t mask);

/**
 * @brief Function for disabling specified interrupts.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Mask of interrupts to be disabled.
 *                  Use @ref nrf_i2s_int_mask_t values for bit masking.
 */
NRF_STATIC_INLINE void nrf_i2s_int_disable(NRF_I2S_Type * p_reg, uint32_t mask);

/**
 * @brief Function for checking if the specified interrupts are enabled.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Mask of interrupts to be checked.
 *                  Use @ref nrf_i2s_int_mask_t values for bit masking.
 *
 * @return Mask of enabled interrupts.
 */
NRF_STATIC_INLINE uint32_t nrf_i2s_int_enable_check(NRF_I2S_Type const * p_reg, uint32_t mask);

/**
 * @brief Function for enabling the I2S peripheral.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 */
NRF_STATIC_INLINE void nrf_i2s_enable(NRF_I2S_Type * p_reg);

/**
 * @brief Function for disabling the I2S peripheral.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 */
NRF_STATIC_INLINE void nrf_i2s_disable(NRF_I2S_Type * p_reg);

#if defined(DPPI_PRESENT) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for setting the subscribe configuration for a given
 *        I2S task.
 *
 * @param[in] p_reg   Pointer to the structure of registers of the peripheral.
 * @param[in] task    Task for which to set the configuration.
 * @param[in] channel Channel through which to subscribe events.
 */
NRF_STATIC_INLINE void nrf_i2s_subscribe_set(NRF_I2S_Type * p_reg,
                                             nrf_i2s_task_t task,
                                             uint8_t        channel);

/**
 * @brief Function for clearing the subscribe configuration for a given
 *        I2S task.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] task  Task for which to clear the configuration.
 */
NRF_STATIC_INLINE void nrf_i2s_subscribe_clear(NRF_I2S_Type * p_reg,
                                               nrf_i2s_task_t task);

/**
 * @brief Function for getting the subscribe configuration for a given
 *        I2S task.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] task  Task for which to read the configuration.
 *
 * @return I2S subscribe configuration.
 */
NRF_STATIC_INLINE uint32_t nrf_i2s_subscribe_get(NRF_I2S_Type const * p_reg,
                                                 nrf_i2s_task_t       task);

/**
 * @brief Function for setting the publish configuration for a given
 *        I2S event.
 *
 * @param[in] p_reg   Pointer to the structure of registers of the peripheral.
 * @param[in] event   Event for which to set the configuration.
 * @param[in] channel Channel through which to publish the event.
 */
NRF_STATIC_INLINE void nrf_i2s_publish_set(NRF_I2S_Type *  p_reg,
                                           nrf_i2s_event_t event,
                                           uint8_t         channel);

/**
 * @brief Function for clearing the publish configuration for a given
 *        I2S event.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Event for which to clear the configuration.
 */
NRF_STATIC_INLINE void nrf_i2s_publish_clear(NRF_I2S_Type *  p_reg,
                                             nrf_i2s_event_t event);

/**
 * @brief Function for getting the publish configuration for a given
 *        I2S event.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Event for which to read the configuration.
 *
 * @return I2S publish configuration.
 */
NRF_STATIC_INLINE uint32_t nrf_i2s_publish_get(NRF_I2S_Type const * p_reg,
                                               nrf_i2s_event_t      event);
#endif // defined(DPPI_PRESENT) || defined(__NRFX_DOXYGEN__)

/**
 * @brief Function for configuring I2S pins.
 *
 * Usage of the SDOUT, SDIN, and MCK signals is optional.
 * If a given signal is not needed, pass the @ref NRF_I2S_PIN_NOT_CONNECTED
 * value instead of its pin number.
 *
 * @param[in] p_reg  Pointer to the structure of registers of the peripheral.
 * @param[in] p_pins Pointer to the structure with pins selection.
 */
NRF_STATIC_INLINE void nrf_i2s_pins_set(NRF_I2S_Type * p_reg, nrf_i2s_pins_t const * p_pins);

/**
 * @brief Function for setting the SCK pin.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] pin   SCK pin number.
 */
NRF_STATIC_INLINE void nrf_i2s_sck_pin_set(NRF_I2S_Type * p_reg, uint32_t pin);

/**
 * @brief Function for setting the LRCK pin.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] pin   LRCK pin number.
 */
NRF_STATIC_INLINE void nrf_i2s_lrck_pin_set(NRF_I2S_Type * p_reg, uint32_t pin);

/**
 * @brief Function for setting the MCK pin.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] pin   MCK pin number.
 */
NRF_STATIC_INLINE void nrf_i2s_mck_pin_set(NRF_I2S_Type * p_reg, uint32_t pin);

/**
 * @brief Function for setting the SDOUT pin.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] pin   SDOUT pin number.
 */
NRF_STATIC_INLINE void nrf_i2s_sdout_pin_set(NRF_I2S_Type * p_reg, uint32_t pin);

/**
 * @brief Function for setting the SDIN pin.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] pin   SDIN pin number.
 */
NRF_STATIC_INLINE void nrf_i2s_sdin_pin_set(NRF_I2S_Type * p_reg, uint32_t pin);

/**
 * @brief Function for getting the SCK pin selection.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return SCK pin selection.
 */
NRF_STATIC_INLINE uint32_t nrf_i2s_sck_pin_get(NRF_I2S_Type const * p_reg);

/**
 * @brief Function for getting the LRCK pin selection.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return LRCK pin selection.
 */
NRF_STATIC_INLINE uint32_t nrf_i2s_lrck_pin_get(NRF_I2S_Type const * p_reg);

/**
 * @brief Function for getting the MCK pin selection.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return MCK pin selection.
 */
NRF_STATIC_INLINE uint32_t nrf_i2s_mck_pin_get(NRF_I2S_Type const * p_reg);

/**
 * @brief Function for getting the SDOUT pin selection.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return SDOUT pin selection.
 */
NRF_STATIC_INLINE uint32_t nrf_i2s_sdout_pin_get(NRF_I2S_Type const * p_reg);

/**
 * @brief Function for getting the SDIN pin selection.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return SDIN pin selection.
 */
NRF_STATIC_INLINE uint32_t nrf_i2s_sdin_pin_get(NRF_I2S_Type const * p_reg);

/**
 * @brief Function for setting the I2S peripheral configuration.
 *
 * @param[in] p_reg    Pointer to the structure of registers of the peripheral.
 * @param[in] p_config Pointer to the structure with configuration.
 */
NRF_STATIC_INLINE void nrf_i2s_configure(NRF_I2S_Type * p_reg, nrf_i2s_config_t const * p_config);

/**
 * @brief Function for setting up the I2S transfer.
 *
 * This function sets up the RX and TX buffers and enables reception or
 * transmission (or both) accordingly. If the transfer in a given direction is not
 * required, pass NULL instead of the pointer to the corresponding buffer.
 *
 * @param[in] p_reg       Pointer to the structure of registers of the peripheral.
 * @param[in] size        Size of the buffers (in 32-bit words).
 * @param[in] p_rx_buffer Pointer to the receive buffer.
 *                        Pass NULL to disable reception.
 * @param[in] p_tx_buffer Pointer to the transmit buffer.
 *                        Pass NULL to disable transmission.
 */
NRF_STATIC_INLINE void nrf_i2s_transfer_set(NRF_I2S_Type *   p_reg,
                                            uint16_t         size,
                                            uint32_t *       p_rx_buffer,
                                            uint32_t const * p_tx_buffer);

/**
 * @brief Function for setting the pointer to the receive buffer.
 *
 * @note The size of the buffer can be set only by calling
 *       @ref nrf_i2s_transfer_set.
 *
 * @param[in] p_reg    Pointer to the structure of registers of the peripheral.
 * @param[in] p_buffer Pointer to the receive buffer.
 */
NRF_STATIC_INLINE void nrf_i2s_rx_buffer_set(NRF_I2S_Type * p_reg,
                                             uint32_t *     p_buffer);

/**
 * @brief Function for getting the pointer to the receive buffer.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return Pointer to the receive buffer.
 */
NRF_STATIC_INLINE uint32_t * nrf_i2s_rx_buffer_get(NRF_I2S_Type const * p_reg);

/**
 * @brief Function for setting the pointer to the transmit buffer.
 *
 * @note The size of the buffer can be set only by calling
 *       @ref nrf_i2s_transfer_set.
 *
 * @param[in] p_reg    Pointer to the structure of registers of the peripheral.
 * @param[in] p_buffer Pointer to the transmit buffer.
 */
NRF_STATIC_INLINE void nrf_i2s_tx_buffer_set(NRF_I2S_Type *   p_reg,
                                             uint32_t const * p_buffer);

/**
 * @brief Function for getting the pointer to the transmit buffer.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return Pointer to the transmit buffer.
 */
NRF_STATIC_INLINE uint32_t * nrf_i2s_tx_buffer_get(NRF_I2S_Type const * p_reg);

#if NRF_I2S_HAS_CLKCONFIG
/**
 * @brief Function for configuring I2S Clock.
 *
 * @param[in] p_reg         Pointer to the structure of registers of the peripheral.
 * @param[in] clksrc        I2S Clock source selection.
 * @param[in] enable_bypass Bypass clock generator. MCK will be equal to source input.
 *                          If bypass is enabled the MCKFREQ setting has no effect.
 */
NRF_STATIC_INLINE void nrf_i2s_clk_configure(NRF_I2S_Type *   p_reg,
                                             nrf_i2s_clksrc_t clksrc,
                                             bool             enable_bypass);
#endif

#ifndef NRF_DECLARE_ONLY

NRF_STATIC_INLINE void nrf_i2s_task_trigger(NRF_I2S_Type * p_reg,
                                            nrf_i2s_task_t task)
{
    *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)task)) = 0x1UL;
}

NRF_STATIC_INLINE uint32_t nrf_i2s_task_address_get(NRF_I2S_Type const * p_reg,
                                                    nrf_i2s_task_t       task)
{
    return ((uint32_t)p_reg + (uint32_t)task);
}

NRF_STATIC_INLINE void nrf_i2s_event_clear(NRF_I2S_Type *  p_reg,
                                           nrf_i2s_event_t event)
{
    *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)event)) = 0x0UL;
    nrf_event_readback((uint8_t *)p_reg + (uint32_t)event);
}

NRF_STATIC_INLINE bool nrf_i2s_event_check(NRF_I2S_Type const * p_reg,
                                           nrf_i2s_event_t      event)
{
    return nrf_event_check(p_reg, event);
}

NRF_STATIC_INLINE uint32_t nrf_i2s_event_address_get(NRF_I2S_Type const * p_reg,
                                                     nrf_i2s_event_t      event)
{
    return ((uint32_t)p_reg + (uint32_t)event);
}

NRF_STATIC_INLINE void nrf_i2s_int_enable(NRF_I2S_Type * p_reg, uint32_t mask)
{
    p_reg->INTENSET = mask;
}

NRF_STATIC_INLINE void nrf_i2s_int_disable(NRF_I2S_Type * p_reg, uint32_t mask)
{
    p_reg->INTENCLR = mask;
}

NRF_STATIC_INLINE uint32_t nrf_i2s_int_enable_check(NRF_I2S_Type const * p_reg, uint32_t mask)
{
    return p_reg->INTENSET & mask;
}

NRF_STATIC_INLINE void nrf_i2s_enable(NRF_I2S_Type * p_reg)
{
    p_reg->ENABLE = (I2S_ENABLE_ENABLE_Enabled << I2S_ENABLE_ENABLE_Pos);
}

NRF_STATIC_INLINE void nrf_i2s_disable(NRF_I2S_Type * p_reg)
{
    p_reg->ENABLE = (I2S_ENABLE_ENABLE_Disabled << I2S_ENABLE_ENABLE_Pos);
}

#if defined(DPPI_PRESENT)
NRF_STATIC_INLINE void nrf_i2s_subscribe_set(NRF_I2S_Type * p_reg,
                                             nrf_i2s_task_t task,
                                             uint8_t        channel)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) task + 0x80uL)) =
            ((uint32_t)channel | NRF_SUBSCRIBE_PUBLISH_ENABLE);
}

NRF_STATIC_INLINE void nrf_i2s_subscribe_clear(NRF_I2S_Type * p_reg,
                                               nrf_i2s_task_t task)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) task + 0x80uL)) = 0;
}

NRF_STATIC_INLINE uint32_t nrf_i2s_subscribe_get(NRF_I2S_Type const * p_reg,
                                                 nrf_i2s_task_t       task)
{
    return *((volatile uint32_t const *) ((uint8_t const *) p_reg + (uint32_t) task + 0x80uL));
}

NRF_STATIC_INLINE void nrf_i2s_publish_set(NRF_I2S_Type *  p_reg,
                                           nrf_i2s_event_t event,
                                           uint8_t         channel)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) event + 0x80uL)) =
            ((uint32_t)channel | NRF_SUBSCRIBE_PUBLISH_ENABLE);
}

NRF_STATIC_INLINE void nrf_i2s_publish_clear(NRF_I2S_Type *  p_reg,
                                             nrf_i2s_event_t event)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) event + 0x80uL)) = 0;
}

NRF_STATIC_INLINE uint32_t nrf_i2s_publish_get(NRF_I2S_Type const * p_reg,
                                               nrf_i2s_event_t      event)
{
    return *((volatile uint32_t const *) ((uint8_t const *) p_reg + (uint32_t) event + 0x80uL));
}
#endif // defined(DPPI_PRESENT)

NRF_STATIC_INLINE void nrf_i2s_pins_set(NRF_I2S_Type * p_reg, nrf_i2s_pins_t const * p_pins)
{
    p_reg->PSEL.SCK   = p_pins->sck_pin;
    p_reg->PSEL.LRCK  = p_pins->lrck_pin;
    p_reg->PSEL.MCK   = p_pins->mck_pin;
    p_reg->PSEL.SDOUT = p_pins->sdout_pin;
    p_reg->PSEL.SDIN  = p_pins->sdin_pin;
}

NRF_STATIC_INLINE void nrf_i2s_sck_pin_set(NRF_I2S_Type * p_reg, uint32_t pin)
{
    p_reg->PSEL.SCK = pin;
}

NRF_STATIC_INLINE void nrf_i2s_lrck_pin_set(NRF_I2S_Type * p_reg, uint32_t pin)
{
    p_reg->PSEL.LRCK = pin;
}

NRF_STATIC_INLINE void nrf_i2s_mck_pin_set(NRF_I2S_Type * p_reg, uint32_t pin)
{
    p_reg->PSEL.MCK = pin;
}

NRF_STATIC_INLINE void nrf_i2s_sdout_pin_set(NRF_I2S_Type * p_reg, uint32_t pin)
{
    p_reg->PSEL.SDOUT = pin;
}

NRF_STATIC_INLINE void nrf_i2s_sdin_pin_set(NRF_I2S_Type * p_reg, uint32_t pin)
{
    p_reg->PSEL.SDIN = pin;
}

NRF_STATIC_INLINE uint32_t nrf_i2s_sck_pin_get(NRF_I2S_Type const * p_reg)
{
    return p_reg->PSEL.SCK;
}

NRF_STATIC_INLINE uint32_t nrf_i2s_lrck_pin_get(NRF_I2S_Type const * p_reg)
{
    return p_reg->PSEL.LRCK;
}

NRF_STATIC_INLINE uint32_t nrf_i2s_mck_pin_get(NRF_I2S_Type const * p_reg)
{
    return p_reg->PSEL.MCK;
}

NRF_STATIC_INLINE uint32_t nrf_i2s_sdout_pin_get(NRF_I2S_Type const * p_reg)
{
    return p_reg->PSEL.SDOUT;
}

NRF_STATIC_INLINE uint32_t nrf_i2s_sdin_pin_get(NRF_I2S_Type const * p_reg)
{
    return p_reg->PSEL.SDIN;
}

NRF_STATIC_INLINE void nrf_i2s_configure(NRF_I2S_Type * p_reg, nrf_i2s_config_t const * p_config)
{
    p_reg->CONFIG.MODE     = p_config->mode;
    p_reg->CONFIG.FORMAT   = p_config->format;
    p_reg->CONFIG.ALIGN    = p_config->alignment;
    p_reg->CONFIG.SWIDTH   = p_config->sample_width;
    p_reg->CONFIG.CHANNELS = p_config->channels;
    p_reg->CONFIG.RATIO    = p_config->ratio;

    if (p_config->mck_setup == NRF_I2S_MCK_DISABLED)
    {
        p_reg->CONFIG.MCKEN =
            (I2S_CONFIG_MCKEN_MCKEN_Disabled << I2S_CONFIG_MCKEN_MCKEN_Pos);
    }
    else
    {
        p_reg->CONFIG.MCKFREQ = p_config->mck_setup;
        p_reg->CONFIG.MCKEN =
            (I2S_CONFIG_MCKEN_MCKEN_Enabled << I2S_CONFIG_MCKEN_MCKEN_Pos);
    }
}

NRF_STATIC_INLINE void nrf_i2s_transfer_set(NRF_I2S_Type *   p_reg,
                                            uint16_t         size,
                                            uint32_t *       p_buffer_rx,
                                            uint32_t const * p_buffer_tx)
{
#if defined(DMA_BUFFER_UNIFIED_BYTE_ACCESS)
    p_reg->RXTXD.MAXCNT = size * sizeof(uint32_t);
#else
    p_reg->RXTXD.MAXCNT = size;
#endif

    nrf_i2s_rx_buffer_set(p_reg, p_buffer_rx);
    p_reg->CONFIG.RXEN = (p_buffer_rx != NULL) ? 1 : 0;

    nrf_i2s_tx_buffer_set(p_reg, p_buffer_tx);
    p_reg->CONFIG.TXEN = (p_buffer_tx != NULL) ? 1 : 0;
}

NRF_STATIC_INLINE void nrf_i2s_rx_buffer_set(NRF_I2S_Type * p_reg,
                                             uint32_t * p_buffer)
{
    p_reg->RXD.PTR = (uint32_t)p_buffer;
}

NRF_STATIC_INLINE uint32_t * nrf_i2s_rx_buffer_get(NRF_I2S_Type const * p_reg)
{
    return (uint32_t *)(p_reg->RXD.PTR);
}

NRF_STATIC_INLINE void nrf_i2s_tx_buffer_set(NRF_I2S_Type *   p_reg,
                                             uint32_t const * p_buffer)
{
    p_reg->TXD.PTR = (uint32_t)p_buffer;
}

NRF_STATIC_INLINE uint32_t * nrf_i2s_tx_buffer_get(NRF_I2S_Type const * p_reg)
{
    return (uint32_t *)(p_reg->TXD.PTR);
}

#if NRF_I2S_HAS_CLKCONFIG
NRF_STATIC_INLINE void nrf_i2s_clk_configure(NRF_I2S_Type *   p_reg,
                                             nrf_i2s_clksrc_t clksrc,
                                             bool             enable_bypass)
{
    p_reg->CONFIG.CLKCONFIG = ((uint32_t) clksrc << I2S_CONFIG_CLKCONFIG_CLKSRC_Pos) |
                              ((uint32_t) enable_bypass << I2S_CONFIG_CLKCONFIG_BYPASS_Pos);
}
#endif

#endif // NRF_DECLARE_ONLY

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRF_I2S_H__
