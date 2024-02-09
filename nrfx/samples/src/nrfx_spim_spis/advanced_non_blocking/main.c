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
#include <nrfx_spim.h>
#include <nrfx_spis.h>

#define NRFX_LOG_MODULE                 EXAMPLE
#define NRFX_EXAMPLE_CONFIG_LOG_ENABLED 1
#define NRFX_EXAMPLE_CONFIG_LOG_LEVEL   3
#include <nrfx_log.h>

/**
 * @defgroup nrfx_spim_spis_advanced_non_blocking_example Advanced non-blocking SPIM with SPIS example
 * @{
 * @ingroup nrfx_spim_spis_examples
 *
 * @brief Example showing the advanced functionality of nrfx_spim and nrfx_spis drivers operating in the non-blocking mode.
 *
 * @details Application initializes nrfx_spim and nrfx_spis drivers. Then starts operating in the non-blocking mode.
 *          Communication between SPIM and SPIS proceeds as follows:
 *              1. The first message from @ref m_master_messages is copied to @ref m_tx_buffer_master
 *              2. SPIM sends the message from @ref m_tx_buffer_master to @ref m_rx_buffer_slave
 *              3. SPIS receives and compares this message if the message:
 *                  - is equal to @ref NORDIC_CMD then a message @ref NORDIC_RESP is copied to @ref m_tx_buffer_slave
 *                  - is equal to @ref UNINIT_CMD then SPIS instance is uninitialized
 *                  - is not equal to any of the above then a message @ref ERROR_RESP is copied to @ref m_tx_buffer_slave
 *              4. SPIS (if initialized) sends messages from @ref m_tx_buffer_slave to @ref m_rx_buffer_master
 *              5. Consecutive message from @ref m_master_messages is copied to @ref m_tx_buffer_master.
 *              Steps 2 to 5 are repeated until SPIS instance is uninitialized or respond to the last message
 *              from @ref m_master_messages is sent by SPIS.
 */

/** @brief Symbol specifying SPIM instance to be used. */
#define SPIM_INST_IDX 1

/** @brief Symbol specifying SPIS instance to be used. */
#define SPIS_INST_IDX 2

/** @brief Symbol specifying master's pin number for MOSI. */
#define MOSI_PIN_MASTER LOOPBACK_PIN_1A

/** @brief Symbol specifying slave's pin number for MOSI. */
#define MOSI_PIN_SLAVE LOOPBACK_PIN_1B

/** @brief Symbol specifying master's pin number for MISO. */
#define MISO_PIN_MASTER LOOPBACK_PIN_2A

/** @brief Symbol specifying slave's pin number for MISO. */
#define MISO_PIN_SLAVE LOOPBACK_PIN_2B

/** @brief Symbol specifying master's pin number for SCK. */
#define SCK_PIN_MASTER LOOPBACK_PIN_3A

/** @brief Symbol specifying slave's pin number for SCK. */
#define SCK_PIN_SLAVE LOOPBACK_PIN_3B

/** @brief Symbol specifying master's pin number for CS. */
#define SS_PIN_MASTER LOOPBACK_PIN_4A

/** @brief Symbol specifying slave's pin number for CS. */
#define CSN_PIN_SLAVE LOOPBACK_PIN_4B

/** @brief Symbol specifying the master's command. */
#define NORDIC_CMD "Nordic"

/** @brief Symbol specifying the slave's response to the @ref NORDIC_CMD command. */
#define NORDIC_RESP "Semi"

/** @brief Symbol specifying some unrecognized by the slave command to test slave's @ref ERROR_RESP response. */
#define UNKNOWN_CMD "Unknown"

/** @brief Symbol specifying the slave's response to some unknown command. */
#define ERROR_RESP "ERROR"

/** @brief Symbol specifying the master's command to uninitialize SPIS instance. */
#define UNINIT_CMD "Uninit"

/** @brief Union used to calculate a value of the @ref BUFF_SIZE symbol. */
union messages
{
    uint8_t cmd1[sizeof(NORDIC_CMD)];   ///< Array with size to store @ref NORDIC_CMD command.
    uint8_t cmd2[sizeof(UNKNOWN_CMD)];  ///< Array with size to store @ref UNKNOWN_CMD command.
    uint8_t cmd3[sizeof(UNINIT_CMD)];   ///< Array with size to store @ref UNINIT_CMD command.
    uint8_t resp1[sizeof(NORDIC_RESP)]; ///< Array with size to store @ref NORDIC_RESP respond.
    uint8_t resp2[sizeof(ERROR_RESP)];  ///< Array with size to store @ref ERROR_RESP respond.
};

/** @brief Symbol specifying the length of the buffers. */
#define BUFF_SIZE sizeof(union messages)

/** @brief 2D Array containing consecutive messages to be sent via SPIM data transfer. */
static uint8_t m_master_messages[][BUFF_SIZE] = {NORDIC_CMD, UNKNOWN_CMD, UNINIT_CMD};

/** @brief Symbol specifying the number of messages to be sent via SPIM data transfer. */
#define MSG_COUNT NRFX_ARRAY_SIZE(m_master_messages)

/** @brief Master's transmit buffer defined with the size of @ref BUFF_SIZE symbol. */
static uint8_t m_tx_buffer_master[BUFF_SIZE];

/** @brief Slave's transmit buffer defined with the size of @ref BUFF_SIZE symbol. */
static uint8_t m_tx_buffer_slave[BUFF_SIZE];

/** @brief Master's receive buffer defined with the size of @ref BUFF_SIZE symbol. */
static uint8_t m_rx_buffer_master[BUFF_SIZE];

/** @brief Slave's receive buffer defined with the size of @ref BUFF_SIZE symbol. */
static uint8_t m_rx_buffer_slave[BUFF_SIZE];

/** @brief Flag indicating that a message was transmitted by SPIM driver. */
static bool m_spim_transmitted;

/** @brief Flag indicating that a message was received by SPIM driver. */
static bool m_spim_received;

/** @brief Flag indicating that @ref UNINIT_CMD message was received by SPIS. */
static bool m_spis_uninit_cmd_received;

/** @brief Flag indicating that SPIS instance was uninitialized. */
static bool m_spis_uninitialized;

/**
 * @brief Function for handling SPIM driver events.
 *
 * @param[in] p_event   Pointer to the SPIM driver event.
 * @param[in] p_context Pointer to the context passed from the driver.
 */
static void spim_handler(nrfx_spim_evt_t const * p_event, void * p_context)
{
    static uint8_t i = 1;
    if (p_event->type == NRFX_SPIM_EVENT_DONE)
    {
        char * p_msg = p_context;

        size_t tx_length = p_event->xfer_desc.tx_length;
        size_t rx_length = p_event->xfer_desc.rx_length;

        NRFX_LOG_INFO("SPIM finished. >%s<", p_msg);

        if (tx_length > 0)
        {
            m_spim_transmitted = true;
            NRFX_LOG_INFO("SPIM: Message transmitted: %s", m_tx_buffer_master);
        }

        if (rx_length > 0 && i < MSG_COUNT)
        {
            m_spim_received = true;
            NRFX_LOG_INFO("SPIM: Message received: %s", m_rx_buffer_master);
            memcpy(m_tx_buffer_master, m_master_messages[i], sizeof(m_master_messages[i]));
            i++;
        }
    }
}

/**
 * @brief Function for handling SPIS driver events.
 *
 * @param[in] p_event   Pointer to the SPIM driver event.
 * @param[in] p_context Pointer to the context passed from the driver.
 */
static void spis_handler(nrfx_spis_evt_t const * p_event, void * p_context)
{
    if (p_event->evt_type == NRFX_SPIS_XFER_DONE)
    {
        nrfx_err_t status;
        (void)status;

        nrfx_spis_t * p_spis_inst = p_context;

        size_t tx_length = p_event->tx_amount;
        size_t rx_length = p_event->rx_amount;

        NRFX_LOG_INFO("SPIS finished.");

        if (tx_length > 0)
        {
            NRFX_LOG_INFO("SPIS: Message transmitted: %s", m_tx_buffer_slave);
            status = nrfx_spis_buffers_set(p_spis_inst, NULL, 0, m_rx_buffer_slave, BUFF_SIZE);
            NRFX_ASSERT(status == NRFX_SUCCESS);
        }

        if (rx_length > 0)
        {
            NRFX_LOG_INFO("SPIS: Message received: %s", m_rx_buffer_slave);
            memset(m_tx_buffer_slave, 0, BUFF_SIZE);

            if (!memcmp(m_rx_buffer_slave, NORDIC_CMD, sizeof(NORDIC_CMD)))
            {
                memcpy(m_tx_buffer_slave, NORDIC_RESP, sizeof(NORDIC_RESP));
            }
            else if (!memcmp(m_rx_buffer_slave, UNINIT_CMD, sizeof(UNINIT_CMD)))
            {
                m_spis_uninit_cmd_received = true;
                return;
            }
            else
            {
                memcpy(m_tx_buffer_slave, ERROR_RESP, BUFF_SIZE);
            }

            status = nrfx_spis_buffers_set(p_spis_inst, m_tx_buffer_slave, BUFF_SIZE, NULL, 0);
            NRFX_ASSERT(status == NRFX_SUCCESS);
        }
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
    IRQ_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_SPIM_INST_GET(SPIM_INST_IDX)), IRQ_PRIO_LOWEST,
                NRFX_SPIM_INST_HANDLER_GET(SPIM_INST_IDX), 0, 0);
    IRQ_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_SPIS_INST_GET(SPIS_INST_IDX)), IRQ_PRIO_LOWEST,
                NRFX_SPIS_INST_HANDLER_GET(SPIS_INST_IDX), 0, 0);
#endif

    NRFX_EXAMPLE_LOG_INIT();

    NRFX_LOG_INFO("Starting nrfx_spim_spis advanced non-blocking example.");
    NRFX_EXAMPLE_LOG_PROCESS();

    nrfx_spim_t spim_inst = NRFX_SPIM_INSTANCE(SPIM_INST_IDX);

    nrfx_spim_config_t spim_config = NRFX_SPIM_DEFAULT_CONFIG(SCK_PIN_MASTER,
                                                              MOSI_PIN_MASTER,
                                                              MISO_PIN_MASTER,
                                                              SS_PIN_MASTER);

    void * p_context = "Some context";
    status = nrfx_spim_init(&spim_inst, &spim_config, spim_handler, p_context);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    nrfx_spis_t spis_inst = NRFX_SPIS_INSTANCE(SPIS_INST_IDX);
    nrfx_spis_config_t spis_config = NRFX_SPIS_DEFAULT_CONFIG(SCK_PIN_SLAVE,
                                                              MOSI_PIN_SLAVE,
                                                              MISO_PIN_SLAVE,
                                                              CSN_PIN_SLAVE);

    status = nrfx_spis_init(&spis_inst, &spis_config, spis_handler, &spis_inst);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    status = nrfx_spis_buffers_set(&spis_inst, NULL, 0, m_rx_buffer_slave, BUFF_SIZE);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    memcpy(m_tx_buffer_master, m_master_messages[0], BUFF_SIZE);
    nrfx_spim_xfer_desc_t spim_xfer_desc_tx = NRFX_SPIM_XFER_TX(m_tx_buffer_master, BUFF_SIZE);

    status = nrfx_spim_xfer(&spim_inst, &spim_xfer_desc_tx, 0);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    while (1)
    {
        if (!m_spis_uninit_cmd_received)
        {
            if (m_spim_transmitted)
            {
                NRFX_EXAMPLE_LOG_PROCESS();
                nrfx_spim_xfer_desc_t spim_xfer_desc_rx = NRFX_SPIM_XFER_RX(m_rx_buffer_master,
                                                                            BUFF_SIZE);

                status = nrfx_spim_xfer(&spim_inst, &spim_xfer_desc_rx, 0);
                NRFX_ASSERT(status == NRFX_SUCCESS);

                m_spim_transmitted = false;
            }

            if (m_spim_received)
            {
                NRFX_EXAMPLE_LOG_PROCESS();
                status = nrfx_spim_xfer(&spim_inst, &spim_xfer_desc_tx, 0);
                NRFX_ASSERT(status == NRFX_SUCCESS);

                m_spim_received = false;
            }
        }
        else if (m_spis_uninit_cmd_received && !m_spis_uninitialized)
        {
            nrfx_spis_uninit(&spis_inst);
            m_spis_uninitialized = true;
        }
        NRFX_EXAMPLE_LOG_PROCESS();
    }
}

/** @} */
