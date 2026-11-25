/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide Integration Kit - hello_world.
 *
 * The smallest possible RTOS/SDK-agnostic sample: a bare-metal nrfx application
 * that prints over UART, with no Zephyr/NCS involved. It is built, flashed and
 * observed entirely through the IronSide Integration Kit's own scripts (see ../../scripts) - which is
 * the point: it proves the agnostic build -> flash -> UART flow before any
 * IronSide secure service exists.
 *
 * It is also the simplest member of the test suite: getting its greeting onto
 * the console *is* the thing under test, so it prints the greeting once and then
 * the INTEGRATION_KIT_TEST_SUCCESS / INTEGRATION_KIT_TEST_FINISHED markers (see
 * ../lib/integration_kit_test.h). With the console flow proven, it then
 * lights the DK's four LEDs one by one, forever - a visible sign of life that
 * still needs no IronSide service: the secure side hands every GPIO pin to the
 * non-secure App before it jumps, so the App drives them itself.
 *
 * Uses the shared console helper (integration_kit_uart_init/integration_kit_uart_write,
 * lib/integration_kit_uart.c) over the nrfx UARTE *driver* rather than poking the HAL
 * directly. integration_kit_uart_write() takes an explicit length rather than
 * integration_kit_uart_puts()'s NUL-terminated-and-copied string, so a flash/.rodata
 * literal streams straight through EasyDMA with no RAM copy. The console is
 * UARTE20 at 115200 8N1 on both DKs; the helper picks the TX pin per part.
 */

#include <hal/nrf_gpio.h>
#include <integration_kit_uart.h>
#include <integration_kit_test.h>
#include <stddef.h>

/*
 * The DK's four green LEDs, active high - per board, keyed on the MDK device
 * macro exactly as ../lib/integration_kit_uart.c keys the console TX pin. There is no
 * devicetree in a bare-metal build, so the pins the DK routes to its LEDs are
 * stated here, in led0..led3 order:
 *
 *   nRF54LM20 DK (PCA10184): P1.22, P1.25, P1.27, P1.28
 *   nRF54LV10 DK (PCA10188): P1.24, P1.10, P1.25, P1.22
 *   nRF54L15 DK (PCA10156): P2.9, P1.10, P2.7, P1.14
 *
 * The nRF54LV10's port 1 has 26 pins, so the nRF54LM20 table is not merely in
 * the wrong order there - two of its pins do not exist on that part at all.
 * The nRF54L15 DK's own gpio-leds node (nrf54l15dk_common.dtsi) spreads its four
 * LEDs across ports 1 and 2, unlike the other two DKs' port-1-only layout.
 */
#if defined(NRF54LM20A_XXAA) || defined(NRF54LM20B_XXAA)
static const uint32_t leds[] = {
	NRF_GPIO_PIN_MAP(1, 22),
	NRF_GPIO_PIN_MAP(1, 25),
	NRF_GPIO_PIN_MAP(1, 27),
	NRF_GPIO_PIN_MAP(1, 28),
};
#elif defined(NRF54LV10A_XXAA)
static const uint32_t leds[] = {
	NRF_GPIO_PIN_MAP(1, 24),
	NRF_GPIO_PIN_MAP(1, 10),
	NRF_GPIO_PIN_MAP(1, 25),
	NRF_GPIO_PIN_MAP(1, 22),
};
#elif defined(NRF54L15_XXAA)
static const uint32_t leds[] = {
	NRF_GPIO_PIN_MAP(2, 9),
	NRF_GPIO_PIN_MAP(1, 10),
	NRF_GPIO_PIN_MAP(2, 7),
	NRF_GPIO_PIN_MAP(1, 14),
};
#else
#error "hello_world: unknown SoC -- define the MDK device macro and the DK's LED pins"
#endif

#define LED_ON_US 200000U

int main(void)
{
	/* Flash/.rodata literals - the driver's TX cache bounces them through RAM
	 * for EasyDMA, so no RAM copy is needed here. */
	static const char msg[] = "Hello world from the IronSide Integration Kit\r\n";
	static const char ok[] = INTEGRATION_KIT_TEST_SUCCESS_LINE("hello_world");
	static const char done[] = INTEGRATION_KIT_TEST_FINISHED_LINE("hello_world");

	integration_kit_uart_init();

	integration_kit_uart_write(msg, sizeof(msg) - 1U);
	integration_kit_uart_write(ok, sizeof(ok) - 1U);
	integration_kit_uart_write(done, sizeof(done) - 1U);

	for (size_t i = 0U; i < NRFX_ARRAY_SIZE(leds); i++) {
		nrf_gpio_cfg_output(leds[i]);
		nrf_gpio_pin_clear(leds[i]);
	}

	for (size_t i = 0U;; i = (i + 1U) % NRFX_ARRAY_SIZE(leds)) {
		nrf_gpio_pin_set(leds[i]);
		NRFX_DELAY_US(LED_ON_US);
		nrf_gpio_pin_clear(leds[i]);
	}
}
