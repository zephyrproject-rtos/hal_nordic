/*
 * Copyright (c) 2022 - 2023, Nordic Semiconductor ASA
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
 * @defgroup nrfx_twim_twis_blocking_example Blocking TWIM with TWIS example
 * @{
 * @ingroup nrfx_twim_twis_examples
 *
 * @brief Example showing the functionality of nrfx_twim and nrfx_twis drivers operating in the
 *        blocking mode.
 *
 * @details Application initializes nrfx_twim and nrfx_twis drivers, then sends the specified message
 *          ( @ref MSG_TO_SEND ) from master to slave and back using TWIM and TWIS peripherals.
 *          To perform this operation different buffers are used and data is transferred as follows:
 *          @ref m_tx_buffer_master -> @ref m_rx_buffer_slave -> @ref m_rx_buffer_master
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

/** @brief TWIM transmit buffer initialized with @ref MSG_TO_SEND. */
static uint8_t m_tx_buffer_master[] = MSG_TO_SEND;

/** @brief TWIS recieve buffer for a message from TWIM. */
static uint8_t m_rx_buffer_slave[sizeof(MSG_TO_SEND)];

/** @brief TWIM recieve buffer for a message from TWIS. */
static uint8_t m_rx_buffer_master[sizeof(MSG_TO_SEND)];

/**
 * @brief Function for application main entry.
 *
 * @return Nothing.
 */
int main(void)
{
    nrfx_err_t status;
    (void)status;

    NRFX_EXAMPLE_LOG_INIT();

    NRFX_LOG_INFO("Starting nrfx_twim_twis blocking example.");
    NRFX_EXAMPLE_LOG_PROCESS();

    nrfx_twim_t twim_inst = NRFX_TWIM_INSTANCE(TWIM_INST_IDX);
    nrfx_twis_t twis_inst = NRFX_TWIS_INSTANCE(TWIS_INST_IDX);

    nrfx_twim_xfer_desc_t twim_xfer_desc = NRFX_TWIM_XFER_DESC_TX(SLAVE_ADDR,
                                                                  m_tx_buffer_master,
                                                                  sizeof(m_tx_buffer_master));
    nrfx_twim_config_t twim_config = NRFX_TWIM_DEFAULT_CONFIG(MASTER_SCL_PIN, MASTER_SDA_PIN);
    status = nrfx_twim_init(&twim_inst, &twim_config, NULL, &twim_xfer_desc);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    nrfx_twis_config_t twis_config = NRFX_TWIS_DEFAULT_CONFIG(SLAVE_SCL_PIN,
                                                              SLAVE_SDA_PIN,
                                                              SLAVE_ADDR);
    status = nrfx_twis_init(&twis_inst, &twis_config, NULL);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    NRFX_LOG_INFO("Message to send: %s", m_tx_buffer_master);
    NRFX_LOG_INFO("Content of slave RX buffer:  %s", m_rx_buffer_slave);
    NRFX_LOG_INFO("Content of master RX buffer: %s", m_rx_buffer_master);
    NRFX_EXAMPLE_LOG_PROCESS();

    nrfx_twis_enable(&twis_inst);
    nrfx_twim_enable(&twim_inst);

    status = nrfx_twis_rx_prepare(&twis_inst, m_rx_buffer_slave, sizeof(m_rx_buffer_slave));
    NRFX_ASSERT(status == NRFX_SUCCESS);

    status = nrfx_twim_xfer(&twim_inst, &twim_xfer_desc, 0);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    twim_xfer_desc.type = NRFX_TWIM_XFER_RX;
    twim_xfer_desc.p_primary_buf = m_rx_buffer_master;
    twim_xfer_desc.primary_length = sizeof(m_rx_buffer_master);

    status = nrfx_twis_tx_prepare(&twis_inst, m_rx_buffer_slave, sizeof(m_rx_buffer_slave));
    NRFX_ASSERT(status == NRFX_SUCCESS);

    status = nrfx_twim_xfer(&twim_inst, &twim_xfer_desc, 0);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    NRFX_LOG_INFO("m_tx_buffer_master -> m_rx_buffer_slave -> m_rx_buffer_master");
    NRFX_LOG_INFO("Content of slave RX buffer:  %s", m_rx_buffer_slave);
    NRFX_LOG_INFO("Content of master RX buffer: %s", m_rx_buffer_master);

    while (1)
    {
        NRFX_EXAMPLE_LOG_PROCESS();
    }
}

/** @} */
