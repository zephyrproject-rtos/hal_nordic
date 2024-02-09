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
#include <nrfx_twim.h>
#include <nrfx_twis.h>

#define NRFX_LOG_MODULE                 EXAMPLE
#define NRFX_EXAMPLE_CONFIG_LOG_ENABLED 1
#define NRFX_EXAMPLE_CONFIG_LOG_LEVEL   3
#include <nrfx_log.h>

/**
 * @defgroup nrfx_twim_twis_non_blocking_example Non-blocking TWIM with TWIS example
 * @{
 * @ingroup nrfx_twim_twis_examples
 *
 * @brief Example showing the functionality of nrfx_twim and nrfx_twis drivers operating in the
 *        non-blocking mode.
 *
 * @details Application initializes nrfx_twim and nrfx_twis drivers, then sends the specified message
 *          ( @ref MSG_TO_SEND ) from master to slave and back using TWIM and TWIS peripherals.
 *          To perform this operation different buffers are used and data is transferred as follows:
 *          @ref m_tx_buffer_master -> @ref m_rx_buffer_slave -> @ref m_rx_buffer_master
 *          The @ref twim_handler() and @ref twis_handler() are executed with relevant log messages.
 */

/** @brief Symbol specifying pin number of master SCL. */
#define MASTER_SCL_PIN LOOPBACK_PIN_1A

/** @brief Symbol specifying pin number of master SDA. */
#define MASTER_SDA_PIN LOOPBACK_PIN_2A

/** @brief Symbol specifying pin number of slave SCL. */
#define SLAVE_SCL_PIN LOOPBACK_PIN_1B

/** @brief Symbol specifying pin number of slave SDA. */
#define SLAVE_SDA_PIN LOOPBACK_PIN_2B

#if defined(NRF52_SERIES) || defined(__NRFX_DOXYGEN__)
/** @brief Symbol specifying TWIM instance to be used. */
#define TWIM_INST_IDX 0

/** @brief Symbol specifying TWIS instance to be used. */
#define TWIS_INST_IDX 1
#else
#define TWIM_INST_IDX 1
#define TWIS_INST_IDX 2
#endif

/** @brief Symbol specifying slave address on TWI bus. */
#define SLAVE_ADDR 0x0B

/** @brief Symbol specifying message to be sent via TWIM data transfer.*/
#define MSG_TO_SEND "Nordic Semiconductor"

/** @brief Structure containing TWIS driver instance. */
static nrfx_twis_t twis_inst = NRFX_TWIS_INSTANCE(TWIS_INST_IDX);

/** @brief Structure containing TWIM driver instance. */
static nrfx_twim_t twim_inst = NRFX_TWIM_INSTANCE(TWIM_INST_IDX);

/** @brief TWIM transmit buffer initialized with @ref MSG_TO_SEND. */
static uint8_t m_tx_buffer_master[] = MSG_TO_SEND;

/** @brief TWIS recieve buffer for a message from TWIM. */
static uint8_t m_rx_buffer_slave[sizeof(MSG_TO_SEND)];

/** @brief TWIM recieve buffer for a message from TWIS. */
static uint8_t m_rx_buffer_master[sizeof(MSG_TO_SEND)];

/**
 * @brief Function for handling TWIM driver events.
 *
 * @param[in] p_event   Event information structure.
 * @param[in] p_context General purpose parameter set during initialization of the TWIM.
 *                      This parameter can be used to pass additional information to the
 *                      handler function. In this example @p p_context is used to pass address
 *                      of TWI transfer descriptor structure.
 */
static void twim_handler(nrfx_twim_evt_t const * p_event, void * p_context)
{
    nrfx_err_t status;
    (void)status;

    nrfx_twim_xfer_desc_t * twim_desc = p_context;
    static bool m_master_done = false;

    if (p_event->type == NRFX_TWIM_EVT_DONE)
    {
        NRFX_LOG_INFO("--> Master event: done - transfer completed");
        twim_desc->type = NRFX_TWIM_XFER_RX;
        twim_desc->p_primary_buf = m_rx_buffer_master;
        twim_desc->primary_length = sizeof(m_rx_buffer_master);

        if (!m_master_done)
        {
            status = nrfx_twim_xfer(&twim_inst, twim_desc, 0);
            NRFX_ASSERT(status == NRFX_SUCCESS);
            m_master_done = true;
        }
        else
        {
            NRFX_LOG_INFO("Content of master RX buffer: %s", m_rx_buffer_master);
        }
    }
    else
    {
        NRFX_LOG_INFO("--> MASTER handler, event: %d.", p_event->type);
    }
}

/**
 * @brief Function for handling TWIS driver events.
 *
 * @param[in] p_event Event information structure.
 */
static void twis_handler(nrfx_twis_evt_t const * p_event)
{
    nrfx_err_t status;
    (void)status;

    switch (p_event->type)
    {
        case NRFX_TWIS_EVT_WRITE_DONE:
            NRFX_LOG_INFO("--> Slave event: write done.");
            break;

        case NRFX_TWIS_EVT_READ_DONE:
            NRFX_LOG_INFO("--> Slave event: read done.");
            break;

        case NRFX_TWIS_EVT_WRITE_REQ:
            status = nrfx_twis_rx_prepare(&twis_inst, m_rx_buffer_slave, sizeof(m_rx_buffer_slave));
            NRFX_ASSERT(status == NRFX_SUCCESS);
            NRFX_LOG_INFO("--> Slave event: write request");
            break;

        case NRFX_TWIS_EVT_READ_REQ:
            status = nrfx_twis_tx_prepare(&twis_inst, m_rx_buffer_slave, sizeof(m_rx_buffer_slave));
            NRFX_ASSERT(status == NRFX_SUCCESS);
            NRFX_LOG_INFO("--> Slave event: read request");
            break;

        default:
            NRFX_LOG_INFO("--> SLAVE event: %d.", p_event->type);
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
    IRQ_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_TWIM_INST_GET(TWIM_INST_IDX)), IRQ_PRIO_LOWEST,
                NRFX_TWIM_INST_HANDLER_GET(TWIM_INST_IDX), 0, 0);

    IRQ_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_TWIS_INST_GET(TWIS_INST_IDX)), IRQ_PRIO_LOWEST,
                NRFX_TWIS_INST_HANDLER_GET(TWIS_INST_IDX), 0, 0);
#endif

    NRFX_EXAMPLE_LOG_INIT();

    NRFX_LOG_INFO("Starting nrfx_twim_twis non-blocking example.");
    NRFX_EXAMPLE_LOG_PROCESS();

    nrfx_twim_xfer_desc_t twim_xfer_desc = NRFX_TWIM_XFER_DESC_TX(SLAVE_ADDR,
                                                                  m_tx_buffer_master,
                                                                  sizeof(m_tx_buffer_master));
    nrfx_twim_config_t twim_config = NRFX_TWIM_DEFAULT_CONFIG(MASTER_SCL_PIN, MASTER_SDA_PIN);
    status = nrfx_twim_init(&twim_inst, &twim_config, twim_handler, &twim_xfer_desc);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    nrfx_twis_config_t twis_config = NRFX_TWIS_DEFAULT_CONFIG(SLAVE_SCL_PIN,
                                                              SLAVE_SDA_PIN,
                                                              SLAVE_ADDR);
    status = nrfx_twis_init(&twis_inst, &twis_config, twis_handler);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    NRFX_LOG_INFO("Message to send: %s", m_tx_buffer_master);
    NRFX_LOG_INFO("Content of master RX buffer: %s", m_rx_buffer_master);
    NRFX_EXAMPLE_LOG_PROCESS();

    nrfx_twis_enable(&twis_inst);
    nrfx_twim_enable(&twim_inst);

    status = nrfx_twim_xfer(&twim_inst, &twim_xfer_desc, 0);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    while (1)
    {
        NRFX_EXAMPLE_LOG_PROCESS();
    }
}

/** @} */
