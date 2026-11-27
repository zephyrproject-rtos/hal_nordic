/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide Integration Kit - vpr_launch.
 *
 * How an App runs code on the VPR (the FLPR) and gives it a pin to drive.
 *
 * Starting the FLPR is entirely the App's own business. VPR00 is a non-secure
 * peripheral and the FLPR is a non-secure bus master, matching this image, so
 * INITPC and CPURUN are ordinary register writes - no secure call is involved.
 * The FLPR executes its code straight out of the App's own NVM slot, which the
 * App is granted execute on, so there is no copy step and no separate image.
 *
 * The one thing an App cannot do for itself is hand a PAD to the VPR.
 * GPIO.PIN_CNF[n].CTRLSEL - the field that decides which peripheral drives a
 * pin - is writable only from a secure transaction and reads back as 0 from
 * here. A non-secure store to it does nothing and says nothing: no fault, no
 * error, and a readback that cannot tell you it was ignored. That is what
 * ironside_platform_reg_write() on GPIO.PIN_CNF is for.
 *
 * So the sample does it wrong first, on purpose, because the wrong way is the
 * one that looks like it worked:
 *
 *   1. The App writes CTRLSEL = VPR itself and starts the FLPR. The FLPR
 *      reports over a RAM mailbox that it has driven its VIO output high, so it
 *      really is running - and the pad stays low, because the pad is still the
 *      GPIO block's. Working core, dead pin, nothing anywhere saying why.
 *   2. The App asks IronSide for the same selection, then lets the FLPR drive
 *      the pad low and high again. Now the pad follows the FLPR's VIO writes.
 *
 * The pad is read back on the port's own IN register, so watching this work
 * needs no oscilloscope. Which pad it is, is per part - see vpr_pad.h.
 *
 * The FLPR's own code is flpr_stub.S, assembled for the VPR by the build (which
 * is why this sample needs the optional RISC-V toolchain - see
 * ../PREREQUISITES.md) and included below as the generated flpr_stub.inc.
 *
 * Console: UARTE20, TX = P1.16, 115200 8N1.
 */

#include <integration_kit_uart.h>
#include <integration_kit_test.h>
#include <ironside_client/layout.h>
#include <ironside_client/psa/error.h>
#include <ironside_client/services/platform.h>
#include <nrf.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static uint32_t pin_cnf_addr(uint32_t pin)
{
	uint32_t port = pin / 32U;
	uint32_t idx = pin % 32U;
	uint32_t base = 0U;

	switch (port) {
	case 0:
		base = NRF_P0_S_BASE;
		break;
	case 1:
		base = NRF_P1_S_BASE;
		break;
#if defined(NRF_P2_S_BASE)
	case 2:
		base = NRF_P2_S_BASE;
		break;
#endif
#if defined(NRF_P3_S_BASE)
	case 3:
		base = NRF_P3_S_BASE;
		break;
#endif
	default:
		return 0U;
	}

	return base + offsetof(NRF_GPIO_Type, PIN_CNF) + (idx * sizeof(uint32_t));
}

static int32_t pin_ctrlsel_set(uint32_t pin, uint32_t ctrlsel, uint32_t *applied)
{
	uint32_t addr = pin_cnf_addr(pin);
	uint32_t value = (ctrlsel << GPIO_PIN_CNF_CTRLSEL_Pos) & GPIO_PIN_CNF_CTRLSEL_Msk;
	uint32_t readback = 0U;
	int32_t rc = PSA_ERROR_GENERIC_ERROR;

	if (addr == 0U) {
		return -133;
	}

	rc = ironside_platform_reg_write(addr, value, &readback);
	if (rc == 0 && applied != NULL) {
		*applied = (readback & GPIO_PIN_CNF_CTRLSEL_Msk) >> GPIO_PIN_CNF_CTRLSEL_Pos;
	}

	return rc;
}

#define SAMPLE_NAME "vpr_launch"

/*
 * The pad this sample drives (PAD_PORT/PAD_PIN) and the VIO index that carries
 * it (PAD_VIO, used by the stub). Per part, and shared with flpr_stub.S so the
 * two sides cannot disagree about which pin they are talking about.
 */
#include "vpr_pad.h"

/* The port's register block, keyed off the shared table rather than the part a
 * second time. */
#if PAD_PORT == 2
#define PAD_REG NRF_P2
#else
#define PAD_REG NRF_P1
#endif

/* Absolute pin number, the numbering the MDK, nrfx and this request all use. */
#define PIN ((PAD_PORT * 32U) + PAD_PIN)

/*
 * The mailbox, at the address the stub derives from this same header. The App
 * linker always reserves the APP-XRAM window at the top of non-secure RAM, and
 * this image never defines an APP-XRAM region - so those bytes are ordinary
 * writable APP-RAM at an address both sides can name without a linker section
 * of their own. Word 0: what the FLPR is doing. Word 1: what the App asks next.
 */
#define MBOX       ((volatile uint32_t *)(uintptr_t)IRONSIDE_RAM_XRAM_ADDR)
#define MBOX_HIGH  0x5650524CUL /* "VPRL" - VIO driving the pad high */
#define MBOX_LOW   0x564C4F57UL /* "VLOW" - VIO driving the pad low  */
#define ACK_GO_LOW 1UL

/* Long enough that a stuck FLPR is a failure rather than a hang. The stub is
 * 19 instructions, so a running one arrives in microseconds. */
#define SPIN_LIMIT 2000000UL

/* flpr_stub[], assembled from flpr_stub.S for this part by the build. */
#include "flpr_stub.inc"

static bool wait_mbox(uint32_t want)
{
	for (uint32_t i = 0; i < SPIN_LIMIT; i++) {
		if (MBOX[0] == want) {
			return true;
		}
	}

	return false;
}

static unsigned pad_level(void)
{
	return (PAD_REG->IN >> PAD_PIN) & 1U;
}

#if defined(VPR_VPRSTATUS_CPUSTATUS_Msk)
static uint32_t flpr_cpu_status(void)
{
	return (NRF_VPR00->VPRSTATUS & VPR_VPRSTATUS_CPUSTATUS_Msk) >> VPR_VPRSTATUS_CPUSTATUS_Pos;
}

static bool flpr_is_running(void)
{
	return flpr_cpu_status() == VPR_VPRSTATUS_CPUSTATUS_RUNNING;
}

static bool flpr_is_waiting(void)
{
	return flpr_cpu_status() == VPR_VPRSTATUS_CPUSTATUS_WAITING;
}
#else
/*
 * The nRF54L15 MDK has no VPRSTATUS, so the FLPR's state cannot be read back.
 * The stub parks in WFI right after its last mailbox write; give it time to get
 * there, and report the state as unknown (0xFF) in a failure message.
 */
static uint32_t flpr_cpu_status(void)
{
	return 0xFFU;
}

static bool flpr_is_running(void)
{
	for (volatile uint32_t i = 0; i < SPIN_LIMIT; i++) {
	}

	return false;
}

static bool flpr_is_waiting(void)
{
	for (volatile uint32_t i = 0; i < SPIN_LIMIT; i++) {
	}

	return true;
}
#endif

/*
 * CPURUN only selects the state the FLPR takes after a core reset, so clearing
 * it leaves a running FLPR running. Clear it, wait for the stub to sleep, then
 * reset the core through the debug module's NDMRESET so it comes back stopped.
 * Returns false if the FLPR is not in WAITING afterwards.
 */
static bool stop_flpr(void)
{
	uint32_t i = 0U;

	NRF_VPR00->CPURUN = (VPR_CPURUN_EN_Stopped << VPR_CPURUN_EN_Pos);

	for (i = 0; i < SPIN_LIMIT; i++) {
		if (!flpr_is_running()) {
			break;
		}
	}

	if (i == SPIN_LIMIT) {
		return false;
	}

	NRF_VPR00->DEBUGIF.DMCONTROL =
		(VPR_DEBUGIF_DMCONTROL_NDMRESET_Active << VPR_DEBUGIF_DMCONTROL_NDMRESET_Pos) |
		(VPR_DEBUGIF_DMCONTROL_DMACTIVE_Enabled << VPR_DEBUGIF_DMCONTROL_DMACTIVE_Pos);
	NRF_VPR00->DEBUGIF.DMCONTROL =
		(VPR_DEBUGIF_DMCONTROL_NDMRESET_Inactive << VPR_DEBUGIF_DMCONTROL_NDMRESET_Pos) |
		(VPR_DEBUGIF_DMCONTROL_DMACTIVE_Disabled << VPR_DEBUGIF_DMCONTROL_DMACTIVE_Pos);

	for (i = 0; i < SPIN_LIMIT; i++) {
		if (flpr_is_waiting()) {
			return true;
		}
	}

	return false;
}

int main(void)
{
	bool ok = true;

	integration_kit_uart_init();
	integration_kit_uart_printf("vpr_launch: run code on the FLPR and give it P%u.%02u\r\n",
				    (unsigned)PAD_PORT, (unsigned)PAD_PIN);

	/*
	 * The pad, configured by the App as it always is: output, input buffer
	 * connected so IN reads back the level whoever is driving it. Driven low
	 * first, so "the FLPR raised it" cannot be confused with a leftover level.
	 */
	PAD_REG->PIN_CNF[PAD_PIN] = (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
				    (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos);
	PAD_REG->OUTCLR = (1UL << PAD_PIN);

	MBOX[0] = 0;
	MBOX[1] = 0;

	/* 1. Select the VPR from non-secure, the way an App would if the field were
	 *    its own. The store is dropped. */
	integration_kit_uart_puts("  1. the App selects the VPR on the pad itself\r\n");
	PAD_REG->PIN_CNF[PAD_PIN] |= (GPIO_PIN_CNF_CTRLSEL_VPR << GPIO_PIN_CNF_CTRLSEL_Pos);

	/* Start the FLPR on the stub, in place, out of the App's own NVM slot. */
	NRF_VPR00->INITPC = (uint32_t)(uintptr_t)flpr_stub;
	NRF_VPR00->CPURUN = (VPR_CPURUN_EN_Running << VPR_CPURUN_EN_Pos);

	if (!wait_mbox(MBOX_HIGH)) {
		integration_kit_uart_printf("     FAIL: the FLPR never reported in "
					    "(INITPC=0x%08x, mbox=0x%08x)\r\n",
					    (unsigned)NRF_VPR00->INITPC, (unsigned)MBOX[0]);
		ok = false;
	} else if (pad_level() != 0U) {
		integration_kit_uart_puts(
			"     FAIL: the pad followed the FLPR with no request made\r\n");
		ok = false;
	} else {
		integration_kit_uart_puts(
			"     the FLPR runs and drives its VIO high, and the pad "
			"stays low\r\n"
			"     -> the App's own CTRLSEL write did nothing, silently\r\n");
	}

	/* 2. Ask IronSide for the selection, then let the FLPR move the pad. */
	integration_kit_uart_puts("  2. ask IronSide for the same selection\r\n");
	uint32_t applied = 0xFFFFFFFFu;
	int32_t rc = pin_ctrlsel_set(PIN, GPIO_PIN_CNF_CTRLSEL_VPR, &applied);

	integration_kit_uart_printf("     ctrlsel_set(VPR) -> status %d, selection now %u\r\n",
				    (int)rc, (unsigned)applied);

	if (rc != 0 || applied != GPIO_PIN_CNF_CTRLSEL_VPR) {
		integration_kit_uart_puts("     FAIL: the request did not apply the selection\r\n");
		ok = false;
	} else if (pad_level() != 1U) {
		integration_kit_uart_puts(
			"     FAIL: pad did not go high when the VPR took the pad\r\n");
		ok = false;
	} else {
		integration_kit_uart_puts("     the pad is high: the VPR has it\r\n");

		/* And it tracks the FLPR from here: ask for low. */
		MBOX[1] = ACK_GO_LOW;
		if (!wait_mbox(MBOX_LOW)) {
			integration_kit_uart_puts("     FAIL: the FLPR did not acknowledge\r\n");
			ok = false;
		} else if (pad_level() != 0U) {
			integration_kit_uart_puts(
				"     FAIL: pad stayed high after the FLPR drove it "
				"low\r\n");
			ok = false;
		} else {
			integration_kit_uart_puts(
				"     and it went low again with the FLPR's next VIO "
				"write\r\n");
		}
	}

	/*
	 * Give the hardware back: stop the FLPR and return the pad to the GPIO
	 * block. An App that leaves a core running and a pad routed elsewhere hands
	 * whatever runs next a machine it did not configure.
	 */
	if (stop_flpr()) {
		integration_kit_uart_puts("  FLPR stopped\r\n");
	} else {
		integration_kit_uart_printf("  FAIL: the FLPR did not stop (CPUSTATUS=%u)\r\n",
					    (unsigned)flpr_cpu_status());
		ok = false;
	}
	(void)pin_ctrlsel_set(PIN, GPIO_PIN_CNF_CTRLSEL_GPIO, NULL);
	integration_kit_uart_puts("  pad returned to the GPIO block\r\n");

	if (ok) {
		integration_kit_uart_puts(INTEGRATION_KIT_TEST_SUCCESS_LINE(SAMPLE_NAME));
	} else {
		integration_kit_uart_puts(INTEGRATION_KIT_TEST_FAIL_LINE);
	}
	integration_kit_uart_puts(INTEGRATION_KIT_TEST_FINISHED_LINE(SAMPLE_NAME));

	for (;;) {
		__asm__ volatile("wfi");
	}
}
