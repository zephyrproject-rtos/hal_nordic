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
 * @defgroup nrfx_spim_spis_non_blocking Non-blocking SPIM with SPIS example
 * @{
 * @ingroup nrfx_spim_spis_examples
 *
 * @brief Example showing the basic functionality of nrfx_spim and nrfx_spis drivers operating in the non-blocking mode.
 *
 * @details Application initializes nrfx_spim and nrfx_spis drivers. Then starts operating in the non-blocking mode.
 *          Message @ref MSG_TO_SEND_MASTER from @ref m_tx_buffer_master is transmitted to @ref m_rx_buffer_slave.
 *          Simultaneously message @ref MSG_TO_SEND_SLAVE from @ref m_tx_buffer_slave is transmitted to @ref m_rx_buffer_master.
 *          When the transactions are completed @ref spim_handler() and @ref spis_handler() are executed.
 *          Then received messages are read from @ref m_rx_buffer_master and @ref m_rx_buffer_slave.
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

/** @brief Symbol specifying master's pin number for SS. */
#define SS_PIN_MASTER LOOPBACK_PIN_4A

/** @brief Symbol specifying slave's pin number for CSN. */
#define CSN_PIN_SLAVE LOOPBACK_PIN_4B

/** @brief Symbol specifying message to be sent via SPIM data transfer. */
#define MSG_TO_SEND_MASTER "Semiconductor"

/** @brief Symbol specifying message to be sent via SPIS data transfer. */
#define MSG_TO_SEND_SLAVE "Nordic"

/** @brief Transmit buffer initialized with the specified message ( @ref MSG_TO_SEND_MASTER ). */
static uint8_t m_tx_buffer_master[] = MSG_TO_SEND_MASTER;

/** @brief Transmit buffer initialized with the specified message ( @ref MSG_TO_SEND_SLAVE ). */
static uint8_t m_tx_buffer_slave[] = MSG_TO_SEND_SLAVE;

/** @brief Receive buffer defined with the size to store specified message ( @ref MSG_TO_SEND_SLAVE ). */
static uint8_t m_rx_buffer_master[sizeof(MSG_TO_SEND_SLAVE)];

/** @brief Receive buffer defined with the size to store specified message ( @ref MSG_TO_SEND_MASTER ). */
static uint8_t m_rx_buffer_slave[sizeof(MSG_TO_SEND_MASTER)];

/**
 * @brief Function for handling SPIM driver events.
 *
 * @param[in] p_event   Pointer to the SPIM driver event.
 * @param[in] p_context Pointer to the context passed from the driver.
 */
static void spim_handler(nrfx_spim_evt_t const * p_event, void * p_context)
{
    if (p_event->type == NRFX_SPIM_EVENT_DONE)
    {
        char * p_msg = p_context;
        NRFX_LOG_INFO("SPIM finished. Context passed to the handler: >%s<", p_msg);
        NRFX_LOG_INFO("SPIM rx buffer: %s", m_rx_buffer_master);
    }
}

/**
 * @brief Function for handling SPIS driver events.
 *
 * @param[in] p_event   Pointer to the SPIS driver event.
 * @param[in] p_context Pointer to the context passed from the driver.
 */
static void spis_handler(nrfx_spis_evt_t const * p_event, void * p_context)
{
    if (p_event->evt_type == NRFX_SPIS_XFER_DONE)
    {
        char * p_msg = p_context;
        NRFX_LOG_INFO("SPIS finished. Context passed to the handler: >%s<", p_msg);
        NRFX_LOG_INFO("SPIS rx buffer: %s", m_rx_buffer_slave);
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

    NRFX_LOG_INFO("Starting nrfx_spim_spis basic non-blocking example.");
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

    status = nrfx_spis_init(&spis_inst, &spis_config, spis_handler, p_context);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    status = nrfx_spis_buffers_set(&spis_inst,
                                   m_tx_buffer_slave, sizeof(m_tx_buffer_slave),
                                   m_rx_buffer_slave, sizeof(m_rx_buffer_slave));
    NRFX_ASSERT(status == NRFX_SUCCESS);

    nrfx_spim_xfer_desc_t spim_xfer_desc = NRFX_SPIM_XFER_TRX(m_tx_buffer_master,
                                                              sizeof(m_tx_buffer_master),
                                                              m_rx_buffer_master,
                                                              sizeof(m_rx_buffer_master));

    status = nrfx_spim_xfer(&spim_inst, &spim_xfer_desc, 0);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    while (1)
    {
        NRFX_EXAMPLE_LOG_PROCESS();
    }
}

/** @} */
