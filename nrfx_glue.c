/**
 * Copyright (c) 2018, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <nrfx.h>
#include <kernel.h>

void nrfx_isr(void *irq_handler)
{
	((nrfx_irq_handler_t)irq_handler)();
}

void nrfx_busy_wait(u32_t usec_to_wait)
{
	k_busy_wait(usec_to_wait);
}
