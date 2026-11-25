/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <integration_kit_uart.h>
#include <nrfx.h>
#include <nrfx_uarte.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/*
 * Console TX pin, per board, keyed on the MDK device macro.
 *
 * The samples drive UARTE20 directly (no devicetree, no pinctrl: these are
 * bare-metal builds), so the pin the DK routes to its debugger virtual serial
 * port has to be stated here. All three supported DKs put the console on UARTE20
 * and route UARTE20 to virtual serial port 1, which is why the instance is not
 * per-board -- but the pins are:
 *
 *   nRF54LM20 DK (PCA10184): uart20 TX = P1.16
 *   nRF54LV10 DK (PCA10188): uart20 TX = P1.4
 *   nRF54L15 DK (PCA10156): uart20 TX = P1.4
 *
 * (All from the boards' own uart20_default pinctrl. RX is
 * left disconnected -- the samples only ever print.)
 *
 * Getting this wrong does not fail to build or flash. The image boots, runs and
 * transmits into an unrouted pin, so the runner sees a silent board and reports a
 * timeout -- which reads exactly like a boot failure. That is how it presented the
 * first time a sample was run on the nRF54LV10 DK.
 */
#if defined(NRF54LM20A_XXAA) || defined(NRF54LM20B_XXAA)
#define UART_TX_PIN (32u + 16u) /* absolute pin: port 1, pin 16 */
#elif defined(NRF54LV10A_XXAA) || defined(NRF54L15_XXAA)
#define UART_TX_PIN (32u + 4u) /* absolute pin: port 1, pin 4 */
#else
#error "unknown SoC -- define the MDK device macro and the DK's console TX pin"
#endif

static nrfx_uarte_t uart = NRFX_UARTE_INSTANCE(NRF_UARTE20);

/* RAM scratch the driver uses to bounce non-RAM TX buffers for EasyDMA. In
 * blocking mode the driver caches one byte at a time, so this can be small. */
static uint8_t uart_tx_cache[64];

void integration_kit_uart_init(void)
{
	nrfx_uarte_config_t cfg =
		NRFX_UARTE_DEFAULT_CONFIG(UART_TX_PIN, NRF_UARTE_PSEL_DISCONNECTED);

	cfg.tx_cache.p_buffer = uart_tx_cache;
	cfg.tx_cache.length = sizeof(uart_tx_cache);
	(void)nrfx_uarte_init(&uart, &cfg, NULL);
}

void integration_kit_uart_puts(const char *s)
{
	/* Sized for the longest current caller (a concatenated ~120-byte
	 * psa_policy_guard line); a longer string is truncated, not overrun. */
	static char ram[128];
	size_t len = strlen(s);

	if (len > sizeof(ram)) {
		len = sizeof(ram);
	}
	memcpy(ram, s, len);
	(void)nrfx_uarte_tx(&uart, (const uint8_t *)ram, len, NRFX_UARTE_TX_BLOCKING);
}

void integration_kit_uart_write(const char *buf, size_t len)
{
	/* The TX cache handed to the driver in integration_kit_uart_init() lets EasyDMA
	 * send straight from flash or RAM -- no local copy needed here. */
	(void)nrfx_uarte_tx(&uart, (const uint8_t *)buf, len, NRFX_UARTE_TX_BLOCKING);
}

/*
 * integration_kit_uart_printf() and its helpers.
 *
 * Formatted output accumulates in a small stack buffer that is handed to the
 * driver whenever it fills, and once more at the end of the call. That is what
 * lets a single call print a line of any length: no caller-visible buffer
 * bounds the output, so a 200-character hex dump comes out whole where
 * integration_kit_uart_puts() would have cut it at its scratch size. The buffer is on
 * the stack and therefore in RAM, which EasyDMA can read directly.
 */
struct fmt_out {
	char buf[64];
	size_t len;
};

static void fmt_flush(struct fmt_out *o)
{
	if (o->len != 0U) {
		(void)nrfx_uarte_tx(&uart, (const uint8_t *)o->buf, o->len, NRFX_UARTE_TX_BLOCKING);
		o->len = 0U;
	}
}

static void fmt_ch(struct fmt_out *o, char c)
{
	if (o->len == sizeof(o->buf)) {
		fmt_flush(o);
	}
	o->buf[o->len++] = c;
}

static void fmt_str(struct fmt_out *o, const char *s)
{
	while (*s != '\0') {
		fmt_ch(o, *s++);
	}
}

/* Emit @uv in @base, prefixed with '-' when @neg, padded to @width. Zero
 * padding goes after the sign ("-0042") and space padding before it ("  -42"),
 * as printf does. */
static void fmt_num(struct fmt_out *o, unsigned long long uv, unsigned int base, bool upper,
		    bool neg, unsigned int width, bool zero_pad)
{
	static const char lower_digits[] = "0123456789abcdef";
	static const char upper_digits[] = "0123456789ABCDEF";
	const char *digits = upper ? upper_digits : lower_digits;
	/* Reversed digits. 20 is the longest a 64-bit value reaches in the two
	 * bases used here (decimal; hex needs 16). */
	char rev[20];
	size_t n = 0U;
	size_t emitted = 0U;

	do {
		rev[n++] = digits[uv % base];
		uv /= base;
	} while (uv != 0ULL);

	emitted = n + (neg ? 1U : 0U);

	if (zero_pad) {
		if (neg) {
			fmt_ch(o, '-');
		}
		for (; emitted < width; emitted++) {
			fmt_ch(o, '0');
		}
	} else {
		for (; emitted < width; emitted++) {
			fmt_ch(o, ' ');
		}
		if (neg) {
			fmt_ch(o, '-');
		}
	}
	while (n != 0U) {
		fmt_ch(o, rev[--n]);
	}
}

void integration_kit_uart_printf(const char *fmt, ...)
{
	struct fmt_out o = { .len = 0U };
	va_list ap;

	va_start(ap, fmt);

	while (*fmt != '\0') {
		/* Start of this conversion, for the unrecognized-spec path. */
		const char *spec = fmt;
		unsigned int width = 0U;
		unsigned int longness = 0U;
		bool zero_pad = false;

		if (*fmt != '%') {
			fmt_ch(&o, *fmt++);
			continue;
		}
		fmt++;

		if (*fmt == '0') {
			zero_pad = true;
			fmt++;
		}
		while (*fmt >= '0' && *fmt <= '9') {
			width = (width * 10U) + (unsigned int)(*fmt++ - '0');
		}
		while (*fmt == 'l') {
			longness++;
			fmt++;
		}
		if (*fmt == 'z') {
			/* size_t is 32-bit on this ABI, i.e. the same width as long. */
			longness = 1U;
			fmt++;
		}

		switch (*fmt) {
		case '%':
			fmt_ch(&o, '%');
			fmt++;
			break;
		case 'c':
			/* Promoted to int through the varargs. */
			fmt_ch(&o, (char)va_arg(ap, int));
			fmt++;
			break;
		case 's':
			fmt_str(&o, va_arg(ap, const char *));
			fmt++;
			break;
		case 'd':
		case 'i': {
			long long sv = 0;

			if (longness >= 2U) {
				sv = va_arg(ap, long long);
			} else if (longness == 1U) {
				sv = (long long)va_arg(ap, long);
			} else {
				sv = (long long)va_arg(ap, int);
			}
			bool neg = sv < 0;
			/* Negate in unsigned space -- -LLONG_MIN is undefined. */
			unsigned long long uv = neg ? (unsigned long long)(-(sv + 1)) + 1ULL :
						      (unsigned long long)sv;

			fmt_num(&o, uv, 10U, false, neg, width, zero_pad);
			fmt++;
			break;
		}
		case 'u':
		case 'x':
		case 'X': {
			unsigned int base = (*fmt == 'u') ? 10U : 16U;
			bool upper = (*fmt == 'X');
			unsigned long long uv = 0U;

			if (longness >= 2U) {
				uv = va_arg(ap, unsigned long long);
			} else if (longness == 1U) {
				uv = (unsigned long long)va_arg(ap, unsigned long);
			} else {
				uv = (unsigned long long)va_arg(ap, unsigned int);
			}

			fmt_num(&o, uv, base, upper, false, width, zero_pad);
			fmt++;
			break;
		}
		default:
			/* Unrecognized. Print what was parsed verbatim and consume no
			 * argument, so the rest of the line stays aligned with its
			 * arguments. */
			while (spec != fmt) {
				fmt_ch(&o, *spec++);
			}
			if (*fmt != '\0') {
				fmt_ch(&o, *fmt++);
			}
			break;
		}
	}

	va_end(ap);
	fmt_flush(&o);
}

void integration_kit_uart_hexdump(const void *data, size_t len)
{
	const uint8_t *b = (const uint8_t *)data;
	struct fmt_out o = { .len = 0U };

	/* Not a loop of integration_kit_uart_printf("%02x") calls: that would flush a TX
	 * per byte. One fmt_out for the whole dump means a 384-byte P-384 key
	 * costs 12 transfers rather than 384. */
	for (size_t i = 0; i < len; i++) {
		fmt_num(&o, b[i], 16U, false, false, 2U, true);
	}

	fmt_flush(&o);
}
