/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide Integration Kit on-device test markers.
 *
 * A sample becomes a HW test by
 * printing these recognizable markers on the cpuapp console:
 *
 *   - INTEGRATION_KIT_TEST_SUCCESS <name>  - the test's checks all passed. The <name> is
 *                          the sample's own name (its samples/<name>/ directory).
 *                          The runner waits for the *specific* name it flashed,
 *                          so a stale or wrongly-flashed image printing some
 *                          other test's success can't be miscredited as a pass.
 *   - INTEGRATION_KIT_TEST_FAIL     - the test detected a failure. Optional: it lets a test
 *                          report failure explicitly instead of relying on a
 *                          timeout / missing success line.
 *   - INTEGRATION_KIT_TEST_FINISHED <name> - the test is done. The runner stops listening
 *                          the instant it sees this, so a passing test costs only
 *                          as long as it actually runs - no fixed UART wait window.
 *                          Name-tagged for the same reason as the success marker:
 *                          an untagged terminator still sitting in the console
 *                          buffer from a previous image would end the next test's
 *                          capture before that test printed its own verdict.
 *   - INTEGRATION_KIT_TEST_FAULT_EXPECTED <name> - printed right before an access that must
 *                          raise an uncatchable (secure) fault, so a capture says
 *                          the right image ran and reached the probe. If the access
 *                          does not fault, the test reports INTEGRATION_KIT_TEST_FAIL
 *                          instead (a regression). IronSide records such a fault
 *                          and carries it into the next boot's boot report, so a
 *                          test normally checks its own probe there and judges
 *                          itself in the ordinary way - which says which access was
 *                          refused rather than only that something stopped the
 *                          device. The `expect_fault = true` form in test.toml -
 *                          PASS := this line appeared and then nothing else -
 *                          remains for a fault that leaves no record to read.
 *   - INTEGRATION_KIT_TEST_READY_FOR_INSPECTION <name> - for a test whose verdict is decided
 *                          by a debugger, not the App: printed right before the App
 *                          parks in an idle loop, signaling that whatever state the
 *                          test wants inspected (e.g. a destroyed key's slot) is now
 *                          quiescent and safe to read over SWD without a race. No
 *                          test.toml (see dfu_trigger's "not self-judging" pattern) -
 *                          a dedicated runner script waits for this marker, does its
 *                          own debugger-side check, and is the sole judge of PASS/FAIL.
 *                          Used by psa_volatile_wipe.
 *
 * Print order for a passing test: ... INTEGRATION_KIT_TEST_SUCCESS <name>, then
 * INTEGRATION_KIT_TEST_FINISHED <name> as the very last line. The _LINE forms carry a
 * trailing CRLF for direct UART output; the bare tokens are what the runner
 * greps for.
 *
 * Every _LINE form that takes a name takes the sample's own name, e.g.
 *   uart_puts(INTEGRATION_KIT_TEST_SUCCESS_LINE("hello_world"));
 *   uart_puts(INTEGRATION_KIT_TEST_FINISHED_LINE("hello_world"));
 */

#ifndef INTEGRATION_KIT_SAMPLES_LIB_INTEGRATION_KIT_TEST_H_
#define INTEGRATION_KIT_SAMPLES_LIB_INTEGRATION_KIT_TEST_H_

#define INTEGRATION_KIT_TEST_SUCCESS_TOKEN              "INTEGRATION_KIT_TEST_SUCCESS"
#define INTEGRATION_KIT_TEST_FAIL_TOKEN                 "INTEGRATION_KIT_TEST_FAIL"
#define INTEGRATION_KIT_TEST_FINISHED_TOKEN             "INTEGRATION_KIT_TEST_FINISHED"
#define INTEGRATION_KIT_TEST_FAULT_EXPECTED_TOKEN       "INTEGRATION_KIT_TEST_FAULT_EXPECTED"
#define INTEGRATION_KIT_TEST_READY_FOR_INSPECTION_TOKEN "INTEGRATION_KIT_TEST_READY_FOR_INSPECTION"

/* name must be a string literal - the sample's own name. */
#define INTEGRATION_KIT_TEST_SUCCESS_LINE(name)  INTEGRATION_KIT_TEST_SUCCESS_TOKEN " " name "\r\n"
#define INTEGRATION_KIT_TEST_FAIL_LINE           INTEGRATION_KIT_TEST_FAIL_TOKEN "\r\n"
#define INTEGRATION_KIT_TEST_FINISHED_LINE(name) INTEGRATION_KIT_TEST_FINISHED_TOKEN " " name "\r\n"
#define INTEGRATION_KIT_TEST_FAULT_EXPECTED_LINE(name) \
	INTEGRATION_KIT_TEST_FAULT_EXPECTED_TOKEN " " name "\r\n"
#define INTEGRATION_KIT_TEST_READY_FOR_INSPECTION_LINE(name) \
	INTEGRATION_KIT_TEST_READY_FOR_INSPECTION_TOKEN " " name "\r\n"

#endif /* INTEGRATION_KIT_SAMPLES_LIB_INTEGRATION_KIT_TEST_H_ */
