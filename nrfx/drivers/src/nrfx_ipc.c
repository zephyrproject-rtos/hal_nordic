/*
 * Copyright (c) 2019, Nordic Semiconductor ASA
 * All rights reserved.
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

#include <nrfx.h>

#if NRFX_CHECK(NRFX_IPC_ENABLED)

#include <nrfx_ipc.h>

// Control block - driver instance local data.
typedef struct
{
    nrfx_ipc_handler_t handler;
    nrfx_drv_state_t   state;
    void *             p_context;
} ipc_control_block_t;

static ipc_control_block_t m_ipc_cb;

nrfx_err_t nrfx_ipc_init(nrfx_ipc_handler_t handler, uint8_t irq_priority, void * p_context)
{
    NRFX_ASSERT(handler);
    if (m_ipc_cb.state != NRFX_DRV_STATE_UNINITIALIZED)
    {
        return NRFX_ERROR_ALREADY_INITIALIZED;
    }

    NRFX_IRQ_PRIORITY_SET(IPC_IRQn, irq_priority);
    NRFX_IRQ_ENABLE(IPC_IRQn);

    m_ipc_cb.state = NRFX_DRV_STATE_INITIALIZED;
    m_ipc_cb.handler = handler;
    m_ipc_cb.p_context = p_context;

    return NRFX_SUCCESS;
}

nrfx_err_t nrfx_ipc_config_load(const nrfx_ipc_config_t *p_config)
{
    if (m_ipc_cb.state == NRFX_DRV_STATE_UNINITIALIZED)
    {
        return NRFX_ERROR_INVALID_STATE;
    }

    if (!p_config)
    {
        return NRFX_ERROR_INVALID_PARAM;
    }

    uint32_t i;
    for (i = 0; i < IPC_CONF_NUM; ++i)
    {
        nrf_ipc_send_config(NRF_IPC,
                            nrf_ipc_send_task_get(i),
                            p_config->tx_signals_channels_cfg[i]);
    }

    for (i = 0; i < IPC_CONF_NUM; ++i)
    {
        nrf_ipc_receive_config(NRF_IPC,
                               nrf_ipc_receive_event_get(i),
                               p_config->rx_events_channels_cfg[i]);
    }

    nrf_ipc_int_enable(NRF_IPC, p_config->rx_events_enable_cfg);

    return NRFX_SUCCESS;
}

void nrfx_ipc_uninit(void)
{
    NRFX_ASSERT(m_ipc_cb.state == NRFX_DRV_STATE_INITIALIZED);

    uint32_t i;
    for (i = 0; i < IPC_CONF_NUM; ++i)
    {
        nrf_ipc_send_config(NRF_IPC, nrf_ipc_send_task_get(i), 0);
    }

    for (i = 0; i < IPC_CONF_NUM; ++i)
    {
        nrf_ipc_receive_config(NRF_IPC, nrf_ipc_receive_event_get(i), 0);
    }

    nrf_ipc_int_disable(NRF_IPC, 0xFFFFFFFF);
    m_ipc_cb.state = NRFX_DRV_STATE_UNINITIALIZED;
}

void nrfx_ipc_receive_event_enable(uint8_t event_index)
{
    NRFX_ASSERT(m_ipc_cb.state == NRFX_DRV_STATE_INITIALIZED);
    nrf_ipc_int_enable(NRF_IPC, (1 << event_index));
}

void nrfx_ipc_receive_event_disable(uint8_t event_index)
{
    NRFX_ASSERT(m_ipc_cb.state == NRFX_DRV_STATE_INITIALIZED);
    nrf_ipc_int_disable(NRF_IPC, (1 << event_index));
}

void nrfx_ipc_receive_events_set_enable(uint32_t events_bitmask)
{
    NRFX_ASSERT(m_ipc_cb.state == NRFX_DRV_STATE_INITIALIZED);
    nrf_ipc_int_enable(NRF_IPC, events_bitmask);
}

void nrfx_ipc_receive_events_set_disable(uint32_t events_bitmask)
{
    NRFX_ASSERT(m_ipc_cb.state == NRFX_DRV_STATE_INITIALIZED);
    nrf_ipc_int_disable(NRF_IPC, events_bitmask);
}

void nrfx_ipc_receive_event_channel_assign(uint8_t event_index, uint8_t channel_index)
{
    nrf_ipc_receive_channel_config(NRF_IPC,
                                   nrf_ipc_receive_event_get(event_index),
                                   (nrf_ipc_channel_t)(1 << channel_index));
}

void nrfx_ipc_send_task_channel_assign(uint8_t send_index, uint8_t channel_index)
{
    nrf_ipc_send_channel_config(NRF_IPC,
                                nrf_ipc_send_task_get(send_index),
                                (nrf_ipc_channel_t)(1 << channel_index));
}

void nrfx_ipc_receive_event_channels_config(uint8_t  event_index, uint32_t channels_bitmask)
{
    nrf_ipc_receive_config(NRF_IPC,
                           nrf_ipc_receive_event_get(event_index),
                           channels_bitmask);
}

void nrfx_ipc_send_task_channels_config(uint8_t  send_index, uint32_t channels_bitmask)
{
    nrf_ipc_send_config(NRF_IPC, nrf_ipc_send_task_get(send_index), channels_bitmask);
}

void nrfx_ipc_irq_handler(void)
{
    // Get the information about events that fire this interrupt
    uint32_t events_map = nrf_ipc_int_pending_get(NRF_IPC);

    // Clear these events
    uint32_t bitmask = events_map;
    uint32_t bitpos = 0;
    while (bitmask) {
        if (bitmask & 0x01)
        {
            nrf_ipc_event_clear(NRF_IPC, nrfx_bitpos_to_event(bitpos));
            // Execute interrupt handler to provide information about events to app
            m_ipc_cb.handler(bitpos, m_ipc_cb.p_context);
        }
        bitmask >>= 1;
        bitpos++;
    }

}

#endif // NRFX_CHECK(NRFX_IPC_ENABLED)
