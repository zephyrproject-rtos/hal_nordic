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

#ifndef NRFX_UART_H__
#define NRFX_UART_H__

#include <nrfx.h>
#include <hal/nrf_uart.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrfx_uart UART driver
 * @{
 * @ingroup nrf_uart
 * @brief   UART peripheral driver.
 */

/** @brief Data structure of the UART driver instance. */
typedef struct
{
    NRF_UART_Type * p_reg;        ///< Pointer to a structure with UART registers.
    uint8_t         drv_inst_idx; ///< Index of the driver instance. For internal use only.
} nrfx_uart_t;

#ifndef __NRFX_DOXYGEN__
enum {
    /* List all enabled driver instances (in the format NRFX_\<instance_name\>_INST_IDX). */
    NRFX_INSTANCE_ENUM_LIST(UART)
    NRFX_UART_ENABLED_COUNT
};
#endif

/** @brief Macro for creating a UART driver instance. */
#define NRFX_UART_INSTANCE(id)                               \
{                                                            \
    .p_reg        = NRFX_CONCAT(NRF_, UART, id),             \
    .drv_inst_idx = NRFX_CONCAT(NRFX_UART, id, _INST_IDX),   \
}

/** @brief Types of UART driver events. */
typedef enum
{
    NRFX_UART_EVT_TX_DONE, ///< Requested TX transfer completed.
    NRFX_UART_EVT_RX_DONE, ///< Requested RX transfer completed.
    NRFX_UART_EVT_ERROR,   ///< Error reported by UART peripheral.
} nrfx_uart_evt_type_t;

/** @brief Structure for the UART configuration. */
typedef struct
{
    uint32_t            pseltxd;            ///< TXD pin number.
    uint32_t            pselrxd;            ///< RXD pin number.
    uint32_t            pselcts;            ///< CTS pin number.
    uint32_t            pselrts;            ///< RTS pin number.
    void *              p_context;          ///< Context passed to interrupt handler.
    nrf_uart_baudrate_t baudrate;           ///< Baud rate.
    uint8_t             interrupt_priority; ///< Interrupt priority.
    nrf_uart_config_t   hal_cfg;            ///< Parity, flow control and stop bits settings.
    bool                skip_gpio_cfg;      ///< Skip GPIO configuration of pins.
                                            /**< When set to true, the driver does not modify
                                             *   any GPIO parameters of the used pins. Those
                                             *   parameters are supposed to be configured
                                             *   externally before the driver is initialized. */
    bool                skip_psel_cfg;      ///< Skip pin selection configuration.
                                            /**< When set to true, the driver does not modify
                                             *   pin select registers in the peripheral.
                                             *   Those registers are supposed to be set up
                                             *   externally before the driver is initialized.
                                             *   @note When both GPIO configuration and pin
                                             *   selection are to be skipped, the structure
                                             *   fields that specify pins can be omitted,
                                             *   as they are ignored anyway. */
} nrfx_uart_config_t;

#if NRF_UART_HAS_STOP_BITS || defined(__NRFX_DOXYGEN__)
    /** @brief UART additional stop bits configuration. */
    #define NRFX_UART_DEFAULT_EXTENDED_STOP_CONFIG   \
        .stop = NRF_UART_STOP_ONE,
#else
    #define NRFX_UART_DEFAULT_EXTENDED_STOP_CONFIG
#endif

#if NRF_UART_HAS_PARITY_BIT || defined(__NRFX_DOXYGEN__)
    /**  @brief UART additional parity type configuration. */
    #define NRFX_UART_DEFAULT_EXTENDED_PARITYTYPE_CONFIG   \
        .paritytype = NRF_UART_PARITYTYPE_EVEN,
#else
    #define NRFX_UART_DEFAULT_EXTENDED_PARITYTYPE_CONFIG
#endif

/**
 * @brief UART driver default configuration.
 *
 * This configuration sets up UART with the following options:
 * - hardware flow control disabled
 * - no parity bit
 * - one stop bit
 * - baudrate: 115200
 *
 * @param[in] _pin_tx TX pin.
 * @param[in] _pin_rx RX pin.
 */
#define NRFX_UART_DEFAULT_CONFIG(_pin_tx, _pin_rx)                                \
{                                                                                 \
    .pseltxd            = _pin_tx,                                                \
    .pselrxd            = _pin_rx,                                                \
    .pselcts            = NRF_UART_PSEL_DISCONNECTED,                             \
    .pselrts            = NRF_UART_PSEL_DISCONNECTED,                             \
    .p_context          = NULL,                                                   \
    .baudrate           = NRF_UART_BAUDRATE_115200,                               \
    .interrupt_priority = NRFX_UART_DEFAULT_CONFIG_IRQ_PRIORITY,                  \
    .hal_cfg            = {                                                       \
        .hwfc           = NRF_UART_HWFC_DISABLED,                                 \
        .parity         = NRF_UART_PARITY_EXCLUDED,                               \
        NRFX_UART_DEFAULT_EXTENDED_STOP_CONFIG                                    \
        NRFX_UART_DEFAULT_EXTENDED_PARITYTYPE_CONFIG                              \
    }                                                                             \
}

/** @brief Structure for the UART transfer completion event. */
typedef struct
{
    uint8_t * p_data; ///< Pointer to memory used for transfer.
    uint32_t  bytes;  ///< Number of bytes transfered.
} nrfx_uart_xfer_evt_t;

/** @brief Structure for the UART error event. */
typedef struct
{
    nrfx_uart_xfer_evt_t rxtx;       ///< Transfer details, including number of bytes transferred.
    uint32_t             error_mask; ///< Mask of error flags that generated the event.
} nrfx_uart_error_evt_t;

/** @brief Structure for the UART event. */
typedef struct
{
    nrfx_uart_evt_type_t type; ///< Event type.
    union
    {
        nrfx_uart_xfer_evt_t  rxtx;  ///< Data provided for transfer completion events.
        nrfx_uart_error_evt_t error; ///< Data provided for error event.
    } data;                          ///< Union to store event data.
} nrfx_uart_event_t;

/**
 * @brief UART interrupt event handler.
 *
 * @param[in] p_event   Pointer to event structure. Event is allocated on the stack so it is available
 *                      only within the context of the event handler.
 * @param[in] p_context Context passed to the interrupt handler, set on initialization.
 */
typedef void (*nrfx_uart_event_handler_t)(nrfx_uart_event_t const * p_event,
                                          void *                    p_context);

/**
 * @brief Function for initializing the UART driver.
 *
 * This function configures and enables UART. After this function GPIO pins are controlled by UART.
 *
 * @param[in] p_instance    Pointer to the driver instance structure.
 * @param[in] p_config      Pointer to the structure with the initial configuration.
 * @param[in] event_handler Event handler provided by the user. If not provided, the driver works in
 *                          blocking mode.
 *
 * @retval NRFX_SUCCESS             Initialization is successful.
 * @retval NRFX_ERROR_ALREADY       The driver is already initialized.
 * @retval NRFX_ERROR_INVALID_STATE The driver is already initialized.
 *                                  Deprecated - use @ref NRFX_ERROR_ALREADY instead.
 * @retval NRFX_ERROR_BUSY          Some other peripheral with the same
 *                                  instance ID is already in use. This is
 *                                  possible only if @ref nrfx_prs module
 *                                  is enabled.
 */
nrfx_err_t nrfx_uart_init(nrfx_uart_t const *        p_instance,
                          nrfx_uart_config_t const * p_config,
                          nrfx_uart_event_handler_t  event_handler);

/**
 * @brief Function for reconfiguring the UART driver.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 * @param[in] p_config   Pointer to the structure with the configuration.
 *
 * @retval NRFX_SUCCESS             Reconfiguration was successful.
 * @retval NRFX_ERROR_BUSY          The driver is during transfer.
 * @retval NRFX_ERROR_INVALID_STATE The driver is uninitialized.
 */
nrfx_err_t nrfx_uart_reconfigure(nrfx_uart_t const *        p_instance,
                                 nrfx_uart_config_t const * p_config);

/**
 * @brief Function for uninitializing the UART driver.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 */
void nrfx_uart_uninit(nrfx_uart_t const * p_instance);

/**
 * @brief Function for checking if the UART driver instance is initialized.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 *
 * @retval true  Instance is already initialized.
 * @retval false Instance is not initialized.
 */
bool nrfx_uart_init_check(nrfx_uart_t const * p_instance);

/**
 * @brief Function for getting the address of the specified UART task.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 * @param[in] task       Task.
 *
 * @return Task address.
 */
NRFX_STATIC_INLINE uint32_t nrfx_uart_task_address_get(nrfx_uart_t const * p_instance,
                                                       nrf_uart_task_t     task);

/**
 * @brief Function for getting the address of the specified UART event.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 * @param[in] event      Event.
 *
 * @return Event address.
 */
NRFX_STATIC_INLINE uint32_t nrfx_uart_event_address_get(nrfx_uart_t const * p_instance,
                                                        nrf_uart_event_t    event);

/**
 * @brief Function for sending data over UART.
 *
 * If an event handler was provided in nrfx_uart_init() call, this function
 * returns immediately and the handler is called when the transfer is done.
 * Otherwise, the transfer is performed in blocking mode, that is this function
 * returns when the transfer is finished. Blocking mode is not using interrupt
 * so there is no context switching inside the function.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 * @param[in] p_data     Pointer to data.
 * @param[in] length     Number of bytes to send.
 *
 * @retval NRFX_SUCCESS         Initialization was successful.
 * @retval NRFX_ERROR_BUSY      Driver is already transferring.
 * @retval NRFX_ERROR_FORBIDDEN The transfer was aborted from a different context
 *                              (blocking mode only).
 */
nrfx_err_t nrfx_uart_tx(nrfx_uart_t const * p_instance,
                        uint8_t const *     p_data,
                        size_t              length);

/**
 * @brief Function for checking if UART is currently transmitting.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 *
 * @retval true  The UART is transmitting.
 * @retval false The UART is not transmitting.
 */
bool nrfx_uart_tx_in_progress(nrfx_uart_t const * p_instance);

/**
 * @brief Function for aborting any ongoing transmission.
 * @note @ref NRFX_UART_EVT_TX_DONE event will be generated in non-blocking mode.
 *       It will contain number of bytes sent until the abort was called. The event
 *       handler will be called from the function context.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 */
void nrfx_uart_tx_abort(nrfx_uart_t const * p_instance);

/**
 * @brief Function for receiving data over UART.
 *
 * If an event handler is provided in the nrfx_uart_init() call, this function
 * returns immediately and the handler is called when the transfer is done.
 * Otherwise, the transfer is performed in blocking mode, that is this function
 * returns when the transfer is finished. Blocking mode is not using interrupt so
 * there is no context switching inside the function.
 * The receive buffer pointer is double-buffered in non-blocking mode. The secondary
 * buffer can be set immediately after starting the transfer and will be filled
 * when the primary buffer is full. The double-buffering feature allows
 * receiving data continuously.
 *
 * If this function is used without a previous call to @ref nrfx_uart_rx_enable, the reception
 * will be stopped on error or when the supplied buffer fills up. In both cases,
 * RX FIFO gets disabled. This means that, in case of error, the bytes that follow are lost.
 * If this nrfx_uart_rx() function is used with the previous call to @ref nrfx_uart_rx_enable,
 * the reception is stopped in case of error, but FIFO is still ongoing. The receiver is still
 * working, so after handling the error, an immediate repeated call to this nrfx_uart_rx()
 * function with fresh data buffer will re-establish reception. To disable the receiver,
 * you must call @ref nrfx_uart_rx_disable explicitly.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 * @param[in] p_data     Pointer to data.
 * @param[in] length     Number of bytes to receive.
 *
 * @retval    NRFX_SUCCESS         Reception is complete (in case of blocking mode) or it is
 *                                 successfully started (in case of non-blocking mode).
 * @retval    NRFX_ERROR_BUSY      The driver is already receiving
 *                                 (and the secondary buffer has already been set
 *                                 in non-blocking mode).
 * @retval    NRFX_ERROR_FORBIDDEN The transfer was aborted from a different context
 *                                 (blocking mode only, also see @ref nrfx_uart_rx_disable).
 * @retval    NRFX_ERROR_INTERNAL  The UART peripheral reported an error.
 */
nrfx_err_t nrfx_uart_rx(nrfx_uart_t const * p_instance,
                        uint8_t *           p_data,
                        size_t              length);

/**
 * @brief Function for testing the receiver state in blocking mode.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 *
 * @retval true  The receiver has at least one byte of data to get.
 * @retval false The receiver is empty.
 */
bool nrfx_uart_rx_ready(nrfx_uart_t const * p_instance);

/**
 * @brief Function for enabling the receiver.
 *
 * UART has a 6-byte-long RX FIFO and it is used to store incoming data. If a user does not call the
 * UART receive function before the FIFO is filled, an overrun error will appear. The receiver must be
 * explicitly closed by the user @sa nrfx_uart_rx_disable.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 */
void nrfx_uart_rx_enable(nrfx_uart_t const * p_instance);

/**
 * @brief Function for disabling the receiver.
 *
 * This function must be called to close the receiver after it has been explicitly enabled by
 * @sa nrfx_uart_rx_enable.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 */
void nrfx_uart_rx_disable(nrfx_uart_t const * p_instance);

/**
 * @brief Function for aborting any ongoing reception.
 * @note @ref NRFX_UART_EVT_TX_DONE event will be generated in non-blocking mode.
 *       It will contain number of bytes received until the abort was called. The event
 *       handler will be called from the UART interrupt context.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 */
void nrfx_uart_rx_abort(nrfx_uart_t const * p_instance);

/**
 * @brief Function for reading error source mask. Mask contains values from @ref nrf_uart_error_mask_t.
 * @note Function must be used in blocking mode only. In case of non-blocking mode, an error event is
 *       generated. Function clears error sources after reading.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 *
 * @return Mask of reported errors.
 */
uint32_t nrfx_uart_errorsrc_get(nrfx_uart_t const * p_instance);


#ifndef NRFX_DECLARE_ONLY
NRFX_STATIC_INLINE uint32_t nrfx_uart_task_address_get(nrfx_uart_t const * p_instance,
                                                       nrf_uart_task_t     task)
{
    return nrf_uart_task_address_get(p_instance->p_reg, task);
}

NRFX_STATIC_INLINE uint32_t nrfx_uart_event_address_get(nrfx_uart_t const * p_instance,
                                                        nrf_uart_event_t    event)
{
    return nrf_uart_event_address_get(p_instance->p_reg, event);
}
#endif // NRFX_DECLARE_ONLY

/**
 * @brief Macro returning UART interrupt handler.
 *
 * param[in] idx UART index.
 *
 * @return Interrupt handler.
 */
#define NRFX_UART_INST_HANDLER_GET(idx) NRFX_CONCAT_3(nrfx_uart_, idx, _irq_handler)

/** @} */

/*
 * Declare interrupt handlers for all enabled driver instances in the following format:
 * nrfx_\<periph_name\>_\<idx\>_irq_handler (for example, nrfx_uart_0_irq_handler).
 *
 * A specific interrupt handler for the driver instance can be retrieved by using
 * the NRFX_UART_INST_HANDLER_GET macro.
 *
 * Here is a sample of using the NRFX_UART_INST_HANDLER_GET macro to map an interrupt handler
 * in a Zephyr application:
 *
 * IRQ_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_UART_INST_GET(\<instance_index\>)), \<priority\>,
 *             NRFX_UART_INST_HANDLER_GET(\<instance_index\>), 0, 0);
 */
NRFX_INSTANCE_IRQ_HANDLERS_DECLARE(UART, uart)

#ifdef __cplusplus
}
#endif

#endif // NRFX_UART_H__
