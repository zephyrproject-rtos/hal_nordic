/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Shared console-UART helper for the IronSide Integration Kit samples/tests: one nrfx UARTE20
 * instance (the DK's virtual serial port 1 console, 115200 8N1, TX pin per part),
 * blocking TX only (no RX -- every sample only prints). Every sample already links the nrfx
 * UARTE driver for this console; integration_kit_uart.c gives them one shared
 * implementation instead of each copying its own init/puts.
 */
#ifndef INTEGRATION_KIT_UART_H
#define INTEGRATION_KIT_UART_H

#include <stddef.h>

/* GCC checks the argument list against the format string, exactly as it does
 * for libc printf. */
#define INTEGRATION_KIT_UART_PRINTF_FMT(fmt_idx, arg_idx) \
	__attribute__((format(printf, fmt_idx, arg_idx)))

/* Initialize the console UART (UARTE20, 115200 8N1; TX pin chosen per part). */
void integration_kit_uart_init(void);

/* Blocking-write a NUL-terminated string to the console. A string longer than
 * the internal RAM scratch buffer is silently truncated -- keep individual
 * prints reasonably short. */
void integration_kit_uart_puts(const char *s);

/* Blocking-write exactly @len bytes to the console, no truncation and no RAM
 * copy of the whole buffer: the driver's own TX cache bounces a non-RAM
 * (flash/.rodata) buffer through RAM internally. Prefer this over
 * integration_kit_uart_puts() when the caller already has a fixed-length flash
 * literal, as hello_world does. */
void integration_kit_uart_write(const char *buf, size_t len);

/* Blocking-write a printf-formatted string to the console.
 *
 * Self-contained: the bare-metal sample build links no libc printf, so this is
 * a small formatter of its own rather than a wrapper around vsnprintf. Output
 * is handed to the UART in chunks as it is produced, so -- unlike
 * integration_kit_uart_puts() -- a call is not capped by any scratch-buffer size and a
 * long line is printed whole rather than truncated.
 *
 * Supported conversions, and nothing else:
 *
 *   %s      NUL-terminated string
 *   %c      single character
 *   %d %i   signed decimal
 *   %u      unsigned decimal
 *   %x %X   unsigned hexadecimal, lower / upper case
 *   %%      a literal '%'
 *
 * Each numeric conversion takes an optional minimum field width -- zero-padded
 * when written with a leading zero, space-padded otherwise ("%02x", "%8u") --
 * and an optional l / ll / z length modifier. No precision, no flags beyond
 * that leading zero, and no floating point. An unrecognized conversion is
 * printed verbatim and consumes no argument, so a typo shows up in the output
 * instead of desynchronizing the remaining arguments. */
void integration_kit_uart_printf(const char *fmt, ...) INTEGRATION_KIT_UART_PRINTF_FMT(1, 2);

/* Blocking-write @len bytes of @data as contiguous lowercase hex -- two digits
 * per byte, no separators, no prefix and no trailing newline. Composable by
 * design: the caller supplies whatever surrounds it, which is what lets one
 * helper serve a whole-line digest dump, a mid-line "bad_digest=" field and a
 * chunked, per-line-prefixed token dump alike. Every caller printing a buffer
 * had grown its own copy of this loop plus a digits[] table. */
void integration_kit_uart_hexdump(const void *data, size_t len);

#endif /* INTEGRATION_KIT_UART_H */
