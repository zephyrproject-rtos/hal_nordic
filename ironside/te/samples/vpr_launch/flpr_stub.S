/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * FLPR (VPR, RISC-V) side of the vpr_launch sample: drive one pad through the
 * VPR's real-time VIO interface and report progress to the App in a RAM
 * mailbox.
 *
 * Assembled by the sample build with the optional RISC-V toolchain
 * (integration_kit_vpr_add_stub() in ../cmake/riscv-vpr.cmake) and included by main.c
 * as the generated flpr_stub.inc. Building this sample therefore needs a
 * RISC-V GCC installed; nothing else in the IronSide Integration Kit does.
 *
 * Nothing about the pad or the mailbox is spelled twice: the VIO index comes
 * from vpr_pad.h, which the App reads too, and the mailbox address from the
 * client's layout.h, which is where every address in this system comes from.
 *
 * Position-independent as CODE: every branch is PC-relative and the only
 * absolute addresses are DATA (the mailbox and the CSR numbers), so the same
 * bytes run wherever the App places them -- which is why the App can execute
 * them straight out of its own NVM slot without a relocation step.
 *
 * Uses no stack and no memory it was not given: x2 (sp) is never touched, so
 * nothing has to be set up before INITPC/CPURUN.
 *
 * Registers are the RV32E subset (x0..x15): a0..a4 only.
 */

#include <ironside_client/layout.h>

#include "vpr_pad.h"

/* VPR CSRs (MDK: VPRCSR_NORDIC_*). */
#define CSR_VPRNORDICCTRL 0x7C0
#define CSR_VIO_OUT       0xBC0
#define CSR_VIO_DIR       0xBC1

/* NORDICKEY (0x507D) | ENABLERTPERIPH: unlocks the real-time peripherals, of
 * which VIO -- the VPR's own pin interface -- is the one used here. */
#define VPRNORDICCTRL_RTPERIPH_ON 0x507D0001

/* The VIO bit carrying this part's pad (vpr_pad.h). */
#define VIO_MASK (1 << PAD_VIO)

/*
 * Mailbox in non-secure RAM, shared with the App. The App's linker always
 * reserves the APP-XRAM window at the top of non-secure RAM and this image
 * never defines an APP-XRAM region, so those bytes are ordinary writable
 * APP-RAM at an address both sides derive from the same header.
 *
 * Word 0: what the FLPR is doing. Word 1: what the App asks for next.
 */
#define MBOX       IRONSIDE_RAM_XRAM_ADDR
#define MBOX_HIGH  0x5650524C /* "VPRL" - VIO driving the pad high  */
#define MBOX_LOW   0x564C4F57 /* "VLOW" - VIO driving the pad low   */
#define ACK_GO_LOW 1          /* App -> FLPR: now drive it low      */

	.option norvc
	.section .text
	.globl _start
_start:
	li	a0, VPRNORDICCTRL_RTPERIPH_ON
	csrw	CSR_VPRNORDICCTRL, a0

	li	a0, VIO_MASK
	csrw	CSR_VIO_DIR, a0		/* VIO drives the pad as an output */
	csrw	CSR_VIO_OUT, a0		/* ... high */

	li	a1, MBOX
	li	a2, MBOX_HIGH
	sw	a2, 0(a1)

wait_ack:
	lw	a3, 4(a1)
	li	a4, ACK_GO_LOW
	bne	a3, a4, wait_ack

	csrw	CSR_VIO_OUT, x0		/* ... and low */
	li	a2, MBOX_LOW
	sw	a2, 0(a1)

/* Sleep rather than spin, so VPRSTATUS leaves RUNNING and the App can reset
 * the core between instructions. */
park:
	wfi
	j	park
