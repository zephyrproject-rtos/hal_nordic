/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IRONSIDE_SE_INTERNAL_CALL_MINIMAL_H_
#define IRONSIDE_SE_INTERNAL_CALL_MINIMAL_H_

#include <cmsis_compiler.h>

#include <ironside/se/call.h>
#include <ironside/se/glue.h>
#include <ironside/se/memory_map.h>
#include <ironside/se/peripheral_interface.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Busy-wait iterations between bell re-rings while waiting for a response.
 *
 * Workaround for IronSide SE versions that notify the client before dequeuing
 * the request: a request posted in that window is skipped and would otherwise
 * never be answered. Re-ringing the bell is harmless because the enqueue is
 * idempotent.
 *
 * The interval is a raw spin count, not wall-clock time (this bare-metal path
 * has no timer), so it varies with CPU clock and wait states. That is
 * acceptable: correctness only needs the re-ring to repeat, not to fire at a
 * precise time. Define to 0 to busy wait forever and disable the workaround on
 * IronSide SE versions that include the fix.
 */
#ifndef IRONSIDE_SE_CALL_MINIMAL_RERING_SPINS
#define IRONSIDE_SE_CALL_MINIMAL_RERING_SPINS 1000000UL
#endif

static IRONSIDE_SE_ALWAYS_INLINE struct ironside_se_call_buf *ironside_se_call_alloc(void)
{
	/* In this minimal implementation we only use the first buffer.
	 * We assume that there is no concurrent usage of the API,
	 * so the buffer is always available when this is called.
	 */
	return &IRONSIDE_SE_IPC_BUFFER[0];
}

static IRONSIDE_SE_ALWAYS_INLINE void ironside_se_call_dispatch(struct ironside_se_call_buf *buf)
{
	buf->status = IRONSIDE_SE_CALL_STATUS_REQ;
	__DMB();

	/* Cache handling omitted here as we assume that the IPC buffer is not cached. */

	/* Send the request and busy wait for a response, re-ringing every
	 * IRONSIDE_SE_CALL_MINIMAL_RERING_SPINS iterations; see the macro definition.
	 */
#if IRONSIDE_SE_CALL_MINIMAL_RERING_SPINS > 0
	/* Init to the limit so the first pass rings (sends the request). */
	uint32_t spins = IRONSIDE_SE_CALL_MINIMAL_RERING_SPINS;

	do {
		if (spins < IRONSIDE_SE_CALL_MINIMAL_RERING_SPINS) {
			spins++;
			continue;
		}

		spins = 0;
		NRF_SECDOMBELLBOARD->TASKS_TRIGGER[IRONSIDE_SE_BELLBOARD_IPC_TX_BELL_IDX] = 1;
	} while (!NRF_BELLBOARD->EVENTS_TRIGGERED[IRONSIDE_SE_BELLBOARD_IPC_RX_BELL_IDX]);
#else
	NRF_SECDOMBELLBOARD->TASKS_TRIGGER[IRONSIDE_SE_BELLBOARD_IPC_TX_BELL_IDX] = 1;

	while (!NRF_BELLBOARD->EVENTS_TRIGGERED[IRONSIDE_SE_BELLBOARD_IPC_RX_BELL_IDX]) {
	}
#endif

	NRF_BELLBOARD->EVENTS_TRIGGERED[IRONSIDE_SE_BELLBOARD_IPC_RX_BELL_IDX] = 0;
}

static IRONSIDE_SE_ALWAYS_INLINE void ironside_se_call_release(struct ironside_se_call_buf *buf)
{
	buf->status = IRONSIDE_SE_CALL_STATUS_IDLE;
	__DMB();

	/* Cache handling omitted here as we assume that the IPC buffer is not cached. */
}

#ifdef __cplusplus
}
#endif
#endif /* IRONSIDE_SE_INTERNAL_CALL_MINIMAL_H_ */
