/*
 * Copyright (c) 2019 - 2025, Nordic Semiconductor ASA
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

#include <nrfx.h>

#if NRFX_CHECK(NRFX_EGU_ENABLED)

#if !NRFX_FEATURE_PRESENT(NRFX_EGU, _ENABLED)
#error "No enabled EGU instances. Check <nrfx_config.h>."
#endif

#if NRFX_CHECK(NRFX_EGU0_ENABLED) && ((1 << 0) & NRFX_EGUS_USED)
    #error "EGU instance 0 is reserved for use outside of nrfx."
#endif
#if NRFX_CHECK(NRFX_EGU1_ENABLED) && ((1 << 1) & NRFX_EGUS_USED)
    #error "EGU instance 1 is reserved for use outside of nrfx."
#endif
#if NRFX_CHECK(NRFX_EGU2_ENABLED) && ((1 << 2) & NRFX_EGUS_USED)
    #error "EGU instance 2 is reserved for use outside of nrfx."
#endif
#if NRFX_CHECK(NRFX_EGU3_ENABLED) && ((1 << 3) & NRFX_EGUS_USED)
    #error "EGU instance 3 is reserved for use outside of nrfx."
#endif
#if NRFX_CHECK(NRFX_EGU4_ENABLED) && ((1 << 4) & NRFX_EGUS_USED)
    #error "EGU instance 4 is reserved for use outside of nrfx."
#endif
#if NRFX_CHECK(NRFX_EGU5_ENABLED) && ((1 << 5) & NRFX_EGUS_USED)
    #error "EGU instance 5 is reserved for use outside of nrfx."
#endif

#include <nrfx_egu.h>

typedef struct
{
    nrfx_egu_event_handler_t handler;
    void *                   p_context;
    nrfx_drv_state_t         state;
} egu_control_block_t;

static egu_control_block_t m_cb[NRFX_EGU_ENABLED_COUNT];

/*
 * `-Warray-bounds` warning is disabled for the `egu_event_mask_get_and_clear`
 * function because GCC 12 and above may report a false positive due to accessing
 * event registers.
 */
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#endif

static uint32_t egu_event_mask_get_and_clear(NRF_EGU_Type * p_reg, uint32_t int_mask)
{
    uint32_t event_mask = 0;
    while (int_mask)
    {
        uint8_t event_idx = (uint8_t)NRF_CTZ(int_mask);
        int_mask &= ~(1UL << event_idx);

        nrf_egu_event_t event = nrf_egu_triggered_event_get(event_idx);
        if (nrf_egu_event_check(p_reg, event))
        {
            nrf_egu_event_clear(p_reg, event);
            event_mask |= (1UL << event_idx);
        }
    }
    return event_mask;
}

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

nrfx_err_t nrfx_egu_init(nrfx_egu_t const *       p_instance,
                         uint8_t                  interrupt_priority,
                         nrfx_egu_event_handler_t event_handler,
                         void *                   p_context)
{
    egu_control_block_t * p_cb = &m_cb[p_instance->drv_inst_idx];

    if (p_cb->state != NRFX_DRV_STATE_UNINITIALIZED)
    {
#if NRFX_API_VER_AT_LEAST(3, 2, 0)
        return NRFX_ERROR_ALREADY;
#else
        return NRFX_ERROR_INVALID_STATE;
#endif
    }

    p_cb->state     = NRFX_DRV_STATE_INITIALIZED;
    p_cb->p_context = p_context;
    p_cb->handler   = event_handler;
    if (event_handler)
    {
        NRFX_IRQ_ENABLE(nrfx_get_irq_number(p_instance->p_reg));
        NRFX_IRQ_PRIORITY_SET(nrfx_get_irq_number(p_instance->p_reg), interrupt_priority);
    }

    return NRFX_SUCCESS;
}

void nrfx_egu_int_enable(nrfx_egu_t const * p_instance, uint32_t mask)
{
    NRFX_ASSERT(m_cb[p_instance->drv_inst_idx].state == NRFX_DRV_STATE_INITIALIZED);
    NRFX_ASSERT(m_cb[p_instance->drv_inst_idx].handler);

    (void)egu_event_mask_get_and_clear(p_instance->p_reg, mask);
    nrf_egu_int_enable(p_instance->p_reg, mask);
}

void nrfx_egu_int_disable(nrfx_egu_t const * p_instance, uint32_t mask)
{
    NRFX_ASSERT(m_cb[p_instance->drv_inst_idx].state == NRFX_DRV_STATE_INITIALIZED);

    nrf_egu_int_disable(p_instance->p_reg, mask);
}

void nrfx_egu_trigger(nrfx_egu_t const * p_instance, uint8_t event_idx)
{
    NRFX_ASSERT(m_cb[p_instance->drv_inst_idx].state == NRFX_DRV_STATE_INITIALIZED);
    NRFX_ASSERT(event_idx < nrf_egu_channel_count(p_instance->p_reg));

    nrf_egu_task_trigger(p_instance->p_reg, nrf_egu_trigger_task_get(event_idx));
}

void nrfx_egu_uninit(nrfx_egu_t const * p_instance)
{
    egu_control_block_t * p_cb = &m_cb[p_instance->drv_inst_idx];

    nrf_egu_int_disable(p_instance->p_reg, ~0UL);
    NRFX_IRQ_DISABLE(nrfx_get_irq_number(p_instance->p_reg));

    p_cb->state = NRFX_DRV_STATE_UNINITIALIZED;
}

bool nrfx_egu_init_check(nrfx_egu_t const * p_instance)
{
    egu_control_block_t * p_cb = &m_cb[p_instance->drv_inst_idx];

    return (p_cb->state != NRFX_DRV_STATE_UNINITIALIZED);
}

static void irq_handler(NRF_EGU_Type * p_reg, egu_control_block_t * p_cb)
{
    uint32_t int_mask = nrf_egu_int_enable_check(p_reg, ~0UL);

    /* Check (and clear) only the events that are set to generate interrupts.
       Leave the other ones untouched. */
    uint32_t event_mask = egu_event_mask_get_and_clear(p_reg, int_mask);
    while (event_mask)
    {
        uint8_t event_idx = (uint8_t)NRF_CTZ(event_mask);
        event_mask &= ~(1UL << event_idx);
        p_cb->handler(event_idx, p_cb->p_context);
    }
}

NRFX_INSTANCE_IRQ_HANDLERS(EGU, egu)

#endif // NRFX_CHECK(NRFX_EGU_ENABLED)
