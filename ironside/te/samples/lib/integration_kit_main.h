/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Declare main() noreturn, so that the compiler warns when an application's
 * main() can return.
 *
 * The startup code calls main() and has no code after the call, so a return
 * executes data, faults, and IronSide resets the device.
 */
#ifndef INTEGRATION_KIT_SAMPLES_LIB_INTEGRATION_KIT_MAIN_H_
#define INTEGRATION_KIT_SAMPLES_LIB_INTEGRATION_KIT_MAIN_H_

__attribute__((noreturn)) int main(void);

#endif /* INTEGRATION_KIT_SAMPLES_LIB_INTEGRATION_KIT_MAIN_H_ */
