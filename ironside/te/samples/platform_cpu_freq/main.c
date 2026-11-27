/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide Integration Kit - platform_cpu_freq.
 *
 * How an App reads a register on a peripheral IronSide keeps secure, using the
 * platform mem-read service (ironside_client/services/platform.h).
 *
 * Some peripherals are not attributed to the non-secure side, so a plain load from
 * one bus-faults - there is no MPU setting an App can change to fix that, because
 * the decision is IronSide's and it is made for a reason. Where IronSide is willing
 * to act on your behalf, this call is how you ask.
 *
 * The register used here is OSCILLATORS.PLL.CURRENTFREQ, which tells an App the
 * clock speed it is actually running at. It is worth reading because a non-secure
 * image is otherwise told its frequency at build time and has no way to check: the
 * oscillator block is one IronSide keeps, since it carries the per-die trims and the
 * analogue setup that decide whether the part runs inside its rated envelope at all.
 *
 * Note the address is spelled as the secure alias, 0x5xxxxxxx. A non-secure build
 * has the NRF_* symbols expanding to the non-secure aliases, and the service takes
 * addresses verbatim rather than translating between the two - so the non-secure
 * spelling is refused. Name the register the way the secure side sees it.
 *
 * Reads only. The service has a write request too, but nothing IronSide still keeps
 * secure on this SoC is a register a sample should be storing to - the remaining
 * ones are live hardware, not scratch space. Most peripherals, including the whole
 * CLOCK/POWER/RESET block, are the App's own and are reached directly rather than
 * through here.
 *
 * What it prints:
 *
 *   - the CPU frequency IronSide configured,
 *   - and what a refused request looks like: the same call against an address
 *     IronSide does not hand out.
 *
 * Console: UARTE20, TX = P1.16, 115200 8N1 (same as the other samples).
 */

#include <integration_kit_uart.h>
#include <integration_kit_test.h>
#include <ironside_client/services/platform.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* The oscillator block's secure alias (see above). */
#define OSCILLATORS_PLL_CURRENTFREQ 0x50120804u
#define PLL_FREQ_MASK               0x3u
#define PLL_FREQ_CK128M             1u
#define PLL_FREQ_CK64M              3u

/* IronSide's own key region: a secure address the service will not hand out, used
 * here to show what a refusal looks like. */
#define UICR_IRONSIDE_ADDR 0x00FFD200u

int main(void)
{
	bool ok = true;
	uint32_t value = 0u;
	int32_t status = -1;

	integration_kit_uart_init();
	integration_kit_uart_puts("platform_cpu_freq: reading a secure register\r\n");

	/* 1. Read the CPU frequency IronSide configured. An App is told this at build
	 *    time and cannot otherwise confirm it. */
	status = ironside_platform_reg_read(OSCILLATORS_PLL_CURRENTFREQ, &value);
	integration_kit_uart_puts("  CPU frequency         = ");
	if (status == 0) {
		switch (value & PLL_FREQ_MASK) {
		case PLL_FREQ_CK128M:
			integration_kit_uart_puts("128 MHz");
			break;
		case PLL_FREQ_CK64M:
			integration_kit_uart_puts("64 MHz");
			break;
		default:
			integration_kit_uart_printf("unrecognised 0x%08x", (unsigned int)value);
			break;
		}
	}
	integration_kit_uart_printf("  (status %d)\r\n", (int)status);
	ok &= (status == 0);

	/*
	 * 2. What a refusal looks like. Which registers IronSide hands out is its
	 *    decision and it varies by device, so treat a refusal as a normal runtime
	 *    outcome to handle - not as a bug in your build. -133 is
	 *    PSA_ERROR_NOT_PERMITTED.
	 */
	status = ironside_platform_reg_read(UICR_IRONSIDE_ADDR, &value);
	integration_kit_uart_printf("  a register IronSide keeps to itself -> status %d"
				    "  (expected -133)\r\n",
				    (int)status);
	ok &= (status != 0);

	if (ok) {
		integration_kit_uart_puts(INTEGRATION_KIT_TEST_SUCCESS_LINE("platform_cpu_freq"));
	} else {
		integration_kit_uart_puts(INTEGRATION_KIT_TEST_FAIL_LINE);
	}
	integration_kit_uart_puts(INTEGRATION_KIT_TEST_FINISHED_LINE("platform_cpu_freq"));

	for (;;) {
		__asm__ volatile("wfi");
	}
}
