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
#include <nrfx_spim.h>

#define NRFX_LOG_MODULE                 EXAMPLE
#define NRFX_EXAMPLE_CONFIG_LOG_ENABLED 1
#define NRFX_EXAMPLE_CONFIG_LOG_LEVEL   3
#include <nrfx_log.h>

/**
 * @defgroup nrfx_spim_blocking_example Blocking SPIM example
 * @{
 * @ingroup nrfx_spim_examples
 *
 * @brief Example showing the basic functionality of nrfx_spim driver operating in the blocking mode.
 *
 * @details Application initializes nrfx_spim driver and starts operating in the blocking mode.
 *          Specified message ( @ref MSG_TO_SEND ) from @ref m_tx_buffer is transmitted and when
 *          the transfer is finished received message is read from @ref m_rx_buffer.
 */

/** @brief Symbol specifying SPIM instance to be used. */
#define SPIM_INST_IDX 1

/** @brief Symbol specifying pin number for MOSI. */
#define MOSI_PIN LOOPBACK_PIN_1A

/** @brief Symbol specifying pin number for MISO. */
#define MISO_PIN LOOPBACK_PIN_1B

/** @brief Symbol specifying pin number for SCK. */
#define SCK_PIN LOOPBACK_PIN_2A

/** @brief Symbol specifying message to be sent via SPIM data transfer. */
#define MSG_TO_SEND "Nordic Semiconductor"

/** @brief Transmit buffer initialized with the specified message ( @ref MSG_TO_SEND ). */
static uint8_t m_tx_buffer[] = MSG_TO_SEND;

/** @brief Receive buffer defined with the size to store specified message ( @ref MSG_TO_SEND ). */
static uint8_t m_rx_buffer[sizeof(MSG_TO_SEND)];

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

    NRFX_LOG_INFO("Starting nrfx_spim basic blocking example.");
    NRFX_EXAMPLE_LOG_PROCESS();

    nrfx_spim_t spim_inst = NRFX_SPIM_INSTANCE(SPIM_INST_IDX);

    nrfx_spim_config_t spim_config = NRFX_SPIM_DEFAULT_CONFIG(SCK_PIN,
                                                              MOSI_PIN,
                                                              MISO_PIN,
                                                              NRF_SPIM_PIN_NOT_CONNECTED);

    status = nrfx_spim_init(&spim_inst, &spim_config, NULL, NULL);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    nrfx_spim_xfer_desc_t spim_xfer_desc = NRFX_SPIM_XFER_TRX(m_tx_buffer, sizeof(m_tx_buffer),
                                                              m_rx_buffer, sizeof(m_rx_buffer));

    status = nrfx_spim_xfer(&spim_inst, &spim_xfer_desc, 0);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    NRFX_LOG_INFO("Message received: %s", m_rx_buffer);
    NRFX_EXAMPLE_LOG_PROCESS();

    while (1)
    {
        NRFX_EXAMPLE_LOG_PROCESS();
    }
}
/** @} */
