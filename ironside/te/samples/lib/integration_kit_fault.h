/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Report the fault IronSide recorded for the previous boot, and stop.
 *
 * Every CPU fault on this system is taken to the secure side, so none of them
 * reach a handler this image installs: the application stops executing and
 * IronSide resets the device. A fault is therefore indistinguishable from a
 * spontaneous reboot unless the application reads IronSide's record of it, which
 * arrives in the next boot's boot report as `struct ironside_fault_report`
 * (ironside_client/services/boot_report.h documents every field and its limits).
 *
 * Two entry points, both taking a record the caller has already fetched with
 * ironside_boot_report_get():
 *
 *   integration_kit_fault_print(fault)    prints the record and returns
 *   integration_kit_fault_handle(fault)   prints nothing and returns on a boot that did
 *                                 not follow a fault; prints the record and
 *                                 parks the CPU on one that did
 *
 * Call integration_kit_fault_handle() once at the top of main(), after
 * integration_kit_uart_init(): the record describes the boot before this one and
 * IronSide delivers it exactly once, so there is nothing to poll. This header
 * never initializes the console itself -- that is the application's peripheral,
 * and a different instance, pin or baud rate here would silence the output that
 * matters.
 */
#ifndef INTEGRATION_KIT_FAULT_H
#define INTEGRATION_KIT_FAULT_H

#include <integration_kit_uart.h>
#include <ironside_client/services/boot_report.h>
#include <stddef.h>

/*
 * Print @fault as text, whether or not it holds a fault, and return.
 *
 * The lines are indented, because a fault report is always a detail of
 * something: the handler below prints a line saying the device is stopping, and
 * an application printing the whole boot report prints its other lines around
 * this one.
 *
 * Three shapes, so that no line carries fields that do not apply to it:
 *
 *   fault: none                        nothing recorded, or nothing IronSide
 *                                      will describe
 *   fault: mem addr=... access=...     a refused bus transaction (typically a
 *                                      DMA engine): no exception number, no
 *                                      fault status and no frame
 *   fault: exc=... + 4 detail lines    a CPU exception: the architectural fault
 *                                      status and the non-secure frame
 *
 * @fault may be NULL, which prints the same "none" as an absent record.
 */
static inline void integration_kit_fault_print(const struct ironside_fault_report *fault)
{
	if (fault == NULL || fault->present == 0U) {
		integration_kit_uart_puts("  fault: none\r\n");
		return;
	}

	/* A refused bus transaction is not an exception, so none of the
	 * processor-side fields below apply to it: folding it into the line after
	 * this one would print an exception number of 0 and five empty registers. */
	if (fault->source == IRONSIDE_FAULT_SOURCE_MPC_MEMACCERR) {
		integration_kit_uart_printf(
			"  fault: mem addr=0x%x access=%s%s%s count=%u\r\n",
			(unsigned int)fault->mpc_address,
			(fault->mpc_access & IRONSIDE_FAULT_MPC_ACCESS_READ) ? "r" : "-",
			(fault->mpc_access & IRONSIDE_FAULT_MPC_ACCESS_WRITE) ? "w" : "-",
			(fault->mpc_access & IRONSIDE_FAULT_MPC_ACCESS_EXECUTE) ? "x" : "-",
			(unsigned int)fault->count);
		return;
	}

	integration_kit_uart_printf("  fault: exc=%u flags=0x%x count=%u\r\n",
				    (unsigned int)fault->exception, (unsigned int)fault->flags,
				    (unsigned int)fault->count);
	integration_kit_uart_printf("    cfsr=0x%x bfar=0x%x mmfar=0x%x sfsr=0x%x sfar=0x%x\r\n",
				    (unsigned int)fault->cfsr, (unsigned int)fault->bfar,
				    (unsigned int)fault->mmfar, (unsigned int)fault->sfsr,
				    (unsigned int)fault->sfar);
	integration_kit_uart_printf("    cfsr_ns=0x%x mmfar_ns=0x%x\r\n",
				    (unsigned int)fault->cfsr_ns, (unsigned int)fault->mmfar_ns);

	/* The frame is the faulting instruction's own, so its PC names that
	 * instruction -- unless stacking failed, where there is no frame to name it
	 * with and printing zeros would read as an address. */
	if ((fault->flags & IRONSIDE_FAULT_FLAG_FRAME_UNAVAILABLE) != 0U) {
		integration_kit_uart_puts("    frame: unavailable (stacking failed)\r\n");
		return;
	}

	integration_kit_uart_printf("    frame: pc=0x%x lr=0x%x xpsr=0x%x sp=0x%x\r\n",
				    (unsigned int)fault->frame[6], (unsigned int)fault->frame[5],
				    (unsigned int)fault->frame[7], (unsigned int)fault->frame_sp);
	integration_kit_uart_printf("    r0=0x%x r1=0x%x r2=0x%x r3=0x%x r12=0x%x\r\n",
				    (unsigned int)fault->frame[0], (unsigned int)fault->frame[1],
				    (unsigned int)fault->frame[2], (unsigned int)fault->frame[3],
				    (unsigned int)fault->frame[4]);
}

/*
 * Print @fault and park the CPU if it holds a fault; return, having printed
 * nothing, if it does not.
 *
 * Parking keeps the cause on the console and the device still, which is what a
 * starting point wants rather than what a product wants: continuing runs the
 * code that just faulted, and the record is consumed by the boot that read it,
 * so a second fault leaves the device rebooting with the first one's evidence
 * gone. An application that must stay available counts the faults instead
 * (`count` saturates over the device's life), degrades, or asks to be booted
 * into IronSide DFU -- see the dfu_enter sample.
 */
static inline void integration_kit_fault_handle(const struct ironside_fault_report *fault)
{
	if (fault == NULL || fault->present == 0U) {
		return;
	}

	/* Not spelled "fault: ...": that prefix belongs to the record below, and a
	 * reader should be able to tell this line -- a decision this application
	 * made -- from the record it made it on. */
	integration_kit_uart_puts(
		"ironside: the previous boot ended in a fault; stopping here\r\n");
	integration_kit_fault_print(fault);

	for (;;) {
		__asm__ volatile("wfi");
	}
}

#endif /* INTEGRATION_KIT_FAULT_H */
