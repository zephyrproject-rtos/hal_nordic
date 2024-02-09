/*
 * Copyright (c) 2022 - 2024, Nordic Semiconductor ASA
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

#include <nrfx_example.h>
#include <nrfx_uarte.h>

#define NRFX_LOG_MODULE                 EXAMPLE
#define NRFX_EXAMPLE_CONFIG_LOG_ENABLED 1
#define NRFX_EXAMPLE_CONFIG_LOG_LEVEL   3
#include <nrfx_log.h>

/**
 * @defgroup nrfx_uarte_rx_double_buffered_example RX double-buffered UARTE example
 * @{
 * @ingroup nrfx_uarte_examples
 *
 * @brief Example showing the functionality of nrfx_uarte driver utilizing double-buffering feature.
 *
 * @details Application initializes nrfx_uarte driver, then sends the specified message, which
 *          is stored in @ref m_tx_buffer using UARTE instance. The same instance of UARTE is
 *          configured to receive this message. To perform this operation different buffers are used
 *          and data is transferred as follows:
 *          @ref m_tx_buffer -> @ref m_rx_buffers
 *          To show the double-buffering feature the data is written to the different buffers in the
 *          @ref m_rx_buffers structure.
 *          The @ref uarte_handler() is executed with relevant log messages.
 */

/** @brief Symbol specifying UARTE instance to be used. */
#define UARTE_INST_IDX 1

/** @brief Symbol specifying TX pin number of UARTE. */
#define UARTE_TX_PIN LOOPBACK_PIN_1A

/** @brief Symbol specifying RX pin number of UARTE. */
#define UARTE_RX_PIN LOOPBACK_PIN_1B

/** @brief Symbol specifying the first message to be sent via UARTE data transfer as part of @ref m_tx_buffer. */
#define MSG1 "Nordic"

/** @brief Symbol specifying the second message to be sent via UARTE data transfer as part of @ref m_tx_buffer. */
#define MSG2 " Semiconductor"

/** @brief Symbol specifying the third message to be sent via UARTE data transfer as part of @ref m_tx_buffer. */
#define MSG3 " nRF"

/**
 * @brief Structure containing receive buffers registers.
 *
 * @note To show the double-buffered functionality of data pointer register @p buff_2 is prepared to
 *       store @p MSG3 and @p buff_3 is prepared to store @p MSG2.
 */
typedef struct
{
    uint8_t buff_1[sizeof(MSG1)];   ///< RX buffer 1
    uint8_t buff_2[sizeof(MSG3)];   ///< RX buffer 2
    uint8_t buff_3[sizeof(MSG2)];   ///< RX buffer 3
} rx_buffers_t;

/**
 *  @brief UARTE transmit buffer, it is later filled with @ref MSG1, @ref MSG2 and @ref MSG3 to store
 *         following message: "Nordic Semiconductor nRF".
 */
static uint8_t m_tx_buffer[sizeof(MSG1) + sizeof(MSG2) + sizeof(MSG3)];

/** @brief Structure containing receive buffers. */
static rx_buffers_t m_rx_buffers;

/**
 * @brief Function for showing values of all receive buffers.
 *
 * @param[in] p_rx_buff Pointer to the structure with receive buffers.
 */
static void rx_buffers_print(rx_buffers_t * p_rx_buff)
{
    NRFX_LOG_INFO("................................");
    NRFX_LOG_INFO("RX buffer1 - addr: %p | content: %s", p_rx_buff->buff_1, p_rx_buff->buff_1);
    NRFX_LOG_INFO("RX buffer2 - addr: %p | content: %s", p_rx_buff->buff_2, p_rx_buff->buff_2);
    NRFX_LOG_INFO("RX buffer3 - addr: %p | content: %s", p_rx_buff->buff_3, p_rx_buff->buff_3);
    NRFX_LOG_INFO("................................");
    NRFX_EXAMPLE_LOG_PROCESS();
}

/**
 * @brief Function for handling UARTE driver events.
 *
 * @param[in] p_event   Pointer to event structure. Event is allocated on the stack so it is available
 *                      only within the context of the event handler.
 * @param[in] p_context Context passed to the interrupt handler, set on initialization. In this example
 *                      p_context is used to pass the address of the UARTE instance that calls this handler.
 */
static void uarte_handler(nrfx_uarte_event_t const * p_event, void * p_context)
{
    nrfx_err_t status;
    (void)status;

    nrfx_uarte_t * p_inst = p_context;
    static uint8_t num_handler_exec = 0;

    switch (num_handler_exec++)
    {
        case 0:
            status = nrfx_uarte_rx(p_inst,
                                   m_rx_buffers.buff_3,
                                   NRFX_ARRAY_SIZE(m_rx_buffers.buff_3) - 1);
            NRFX_ASSERT(status == NRFX_SUCCESS);
            break;

        case 1:
            status = nrfx_uarte_rx(p_inst,
                                   m_rx_buffers.buff_2,
                                   NRFX_ARRAY_SIZE(m_rx_buffers.buff_2) - 1);
            NRFX_ASSERT(status == NRFX_SUCCESS);
            break;
    }

    if (p_event->type == NRFX_UARTE_EVT_TX_DONE)
    {
        NRFX_LOG_INFO("--> TX done");
        NRFX_LOG_INFO("--> Bytes transfered: %u", p_event->data.tx.length);
        nrfx_uarte_uninit(p_inst);
    }
}

/**
 * @brief Function for application main entry.
 *
 * @return Nothing.
 */
int main(void)
{
    nrfx_err_t status;
    (void)status;

#if defined(__ZEPHYR__)
    IRQ_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_UARTE_INST_GET(UARTE_INST_IDX)), IRQ_PRIO_LOWEST,
                NRFX_UARTE_INST_HANDLER_GET(UARTE_INST_IDX), 0, 0);
#endif

    NRFX_EXAMPLE_LOG_INIT();

    NRFX_LOG_INFO("Starting nrfx_uarte RX double-buffered example.");
    NRFX_EXAMPLE_LOG_PROCESS();

    nrfx_uarte_t uarte_inst = NRFX_UARTE_INSTANCE(UARTE_INST_IDX);
    nrfx_uarte_config_t uarte_config = NRFX_UARTE_DEFAULT_CONFIG(UARTE_TX_PIN, UARTE_RX_PIN);
    uarte_config.p_context = &uarte_inst;
    status = nrfx_uarte_init(&uarte_inst, &uarte_config, uarte_handler);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    /* Declaration and filling m_tx_buffer to store desired msg ("Nordic Semiconductor nRF"). */
    memcpy(m_tx_buffer                              , MSG1, strlen(MSG1));
    memcpy(m_tx_buffer + strlen(MSG1)               , MSG2, strlen(MSG2));
    memcpy(m_tx_buffer + strlen(MSG1) + strlen(MSG2), MSG3, strlen(MSG3) + 1);
    NRFX_LOG_INFO("Content of TX buffer: %s", m_tx_buffer);

    rx_buffers_print(&m_rx_buffers);

    status = nrfx_uarte_rx(&uarte_inst,
                           m_rx_buffers.buff_1,
                           NRFX_ARRAY_SIZE(m_rx_buffers.buff_1) - 1);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    status = nrfx_uarte_tx(&uarte_inst, m_tx_buffer, NRFX_ARRAY_SIZE(m_tx_buffer), 0);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    while (nrfx_uarte_tx_in_progress(&uarte_inst))
    {}

    rx_buffers_print(&m_rx_buffers);

    while (1)
    {
        NRFX_EXAMPLE_LOG_PROCESS();
    }
}

/** @} */
