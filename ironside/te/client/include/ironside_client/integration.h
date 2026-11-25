/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide TE Client - calling contract and integration hooks.
 *
 * Each hook has a weak default in src/client.c that is sufficient for a single-threaded
 * bare-metal app. An RTOS should override it with a strong definition in its own code.
 */

#ifndef IRONSIDE_CLIENT_INTEGRATION_H
#define IRONSIDE_CLIENT_INTEGRATION_H

/* ---- Calling contract -----------------------------------------------------
 *
 * These rules apply to every function in this client that calls IronSide TE.
 * The helpers that make no call are exempt: psa_crypto_init(), the key-attribute
 * accessors, and image.h and layout.h.
 *
 * A call may be made from any non-secure context, thread or interrupt, but only
 * one call may be in flight. A second call returns IRONSIDE_ERROR_CALL_IN_FLIGHT
 * without running, and the call already in flight is unaffected. The exception is
 * ironside_platform_rram_power_mode_set(), which is never refused, so an interrupt
 * handler can call it while another call is in flight.
 *
 * The client takes the lock below around every call, so a thread that takes the
 * lock waits for the call to return and is never refused. A caller that bypasses
 * the lock, such as an interrupt handler, can be refused. It must defer the work
 * to a thread or retry later, but never retry before the handler returns.
 *
 * Every call returns unless the application keeps it from returning, except
 * ironside_platform_system_off() and ironside_platform_system_reset(), which do not
 * return on success. The application keeps a call from returning when a non-secure
 * handler that preempted it never returns, when the scheduler never resumes the
 * calling thread, or when code that preempted it leaves an interrupt mask set.
 * IronSide TE sets no time limit on a call.
 *
 * Rationale and the consequences of a call that does not return are in the user guide,
 * sections "Interrupts, concurrency and latency" and "Calls that the application keeps
 * from returning".
 */

/**
 * @brief Acquire the client lock.
 *
 * Override with a blocking mutex, and bypass it in both hooks alike in interrupt
 * context and before the scheduler starts, where the caller cannot block.
 *
 * The lock is mandatory if the RTOS can switch threads while a call is in flight.
 * The RTOS must also save and restore the exception return value (EXC_RETURN) of
 * a thread that is switched out inside a call; without that, the switch faults.
 * A thread that skips the lock and is switched out inside the refusal path corrupts
 * non-secure state if it and the thread that was switched out in the call are resumed out
 * of order.
 *
 * The lock holder should not be starved while it holds the lock, because every other
 * caller waits or is refused until it returns. A priority ceiling on the holder covers
 * every case; priority inheritance covers it only while another thread waits for the
 * lock. The recommended implementation is a mutex without a ceiling, with every
 * higher-priority thread kept short.
 *
 * Weak no-op by default, correct for a single-threaded app; override when more than
 * one non-secure caller makes secure calls.
 */
void ironside_client_lock_acquire(void);

/** @brief Release the client lock. Counterpart to @ref ironside_client_lock_acquire. */
void ironside_client_lock_release(void);

#endif /* IRONSIDE_CLIENT_INTEGRATION_H */
