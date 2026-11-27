/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide TE Client - client core.
 *
 * The ABI every service header is built on: the secure-gateway entry, the service
 * handles, the call layout and the one call point. Internal to the Client - an App
 * includes a service header (<ironside_client/services/platform.h>, <ironside_client/psa/crypto.h>, ...),
 * never this one.
 */

#ifndef IRONSIDE_CLIENT_INTERNAL_CLIENT_H
#define IRONSIDE_CLIENT_INTERNAL_CLIENT_H

#include <stddef.h>
#include <stdint.h>
#include <ironside_client/error.h>
#include <ironside_client/integration.h>

/* ---- The secure-gateway entry ---------------------------------------------
 *
 * IronSide TE exposes exactly one CMSE secure-gateway entry; every service is
 * dispatched behind it by the handle in its first argument. The handle's
 * IRONSIDE_CLIENT_HANDLE_VALUES bit says how the other three are read: a
 * service that takes IO vectors (bit clear) gets the address of a struct
 * ironside_client_call_vectors in arg0 and zero in arg1 and arg2, and a service
 * whose arguments fit in registers (bit set) gets them as plain values.
 */
#define IRONSIDE_CLIENT_ENTRY_ADDR 0x00021000u

/** Signature of the single secure-gateway entry. */
typedef int32_t (*ironside_client_entry_fn)(int32_t handle, uint32_t arg0, uint32_t arg1,
					    uint32_t arg2);

/** The callable entry (Thumb bit set). */
#define IRONSIDE_CLIENT_ENTRY ((ironside_client_entry_fn)(IRONSIDE_CLIENT_ENTRY_ADDR | 1u))

/* ---- Service handles (stable, append-only) -------------------------------- */

/* Set in the handle of a service that takes its arguments as plain values. */
#define IRONSIDE_CLIENT_HANDLE_VALUES (0x00008000)

/* Selects the device-management service. */
#define IRONSIDE_DEVMGMT_HANDLE (0x40000200)

/* Selects the PSA Crypto service. */
#define IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE (0x40000100)

/* Selects the Initial Attestation service. */
#define IRONSIDE_CLIENT_PSA_INITIAL_ATTEST_HANDLE (0x40000300)

/* Selects the boot-report service. */
#define IRONSIDE_CLIENT_BOOT_REPORT_HANDLE (0x40000400)

/* 0x40000500 reserved (retired single-range READ handle). */
/* 0x40000501 reserved (retired WRITE32 handle). */

/* Selects the platform NVM write service. */
#define IRONSIDE_CLIENT_PLATFORM_IOCTL_NVM_WRITE_HANDLE (0x40000502)

/* 0x40000503 and 0x40000504 reserved (retired RRAMC low-power on and off handles). */

/* 0x40000505 reserved (retired GPIO_PIN_CTRLSEL handle). */

/* Batched platform register read and write. */
#define IRONSIDE_CLIENT_PLATFORM_IOCTL_MEM_READ_HANDLE  (0x40000506)
#define IRONSIDE_CLIENT_PLATFORM_IOCTL_MEM_WRITE_HANDLE (0x40000507)

/* Select the RRAM power mode: the first argument is an enum ironside_platform_rram_power_mode. */
#define IRONSIDE_CLIENT_PLATFORM_RRAMC_LOWPOWERCONFIG_HANDLE (0x40008500)

/* Enter System OFF. */
#define IRONSIDE_CLIENT_PLATFORM_SYSTEM_OFF_HANDLE (0x40008501)

/*
 * Reset the device through the secure side (SYSRESETREQ is secure-only): the first argument is the
 * reset type, enum ironside_platform_reset_type.
 */
#define IRONSIDE_CLIENT_PLATFORM_SYSTEM_RESET_HANDLE (0x40008502)

/* Selects the DFU-entry-request service. */
#define IRONSIDE_CLIENT_DFU_REQ_HANDLE (0x40000700)

/* ---- Wire layout (append-only) -------------------------------------------- */

struct ironside_client_iovec {
	const void *base;
	size_t len;
};

/** The IO vectors of a call; the entry's arg0 holds the address of one. */
struct ironside_client_call_vectors {
	const struct ironside_client_iovec *in_vec; /* inputs */
	size_t in_len;                              /* number of input vectors */
	struct ironside_client_iovec *out_vec;      /* outputs; .len is updated on return */
	size_t out_len;                             /* number of output vectors */
};

/** One call to a service that takes IO vectors, as ironside_client_call() takes it. */
struct ironside_client_call_params {
	int32_t handle; /* service selector (e.g. IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE) */
	const struct ironside_client_iovec *in_vec; /* inputs */
	size_t in_len;                              /* number of input vectors */
	struct ironside_client_iovec *out_vec;      /* outputs; .len is updated on return */
	size_t out_len;                             /* number of output vectors */
};

/**
 * @brief Call an IronSide TE secure service that takes IO vectors.
 *
 * @param params The service and its IO vectors.
 * @return A psa_status_t value (int32_t): 0 on success or a negative PSA error.
 */
int32_t ironside_client_call(const struct ironside_client_call_params *params);

/**
 * @brief Call an IronSide TE secure service that takes its arguments as plain values.
 *
 * Inline so that a caller passing constants, as every service wrapper does, keeps nothing in a
 * register across the lock hook: these calls sit on the App's latency path.
 *
 * @param handle The service.
 * @param arg0   First argument, 0 for a service that takes none.
 * @param arg1   Second argument, 0 for a service that takes fewer.
 * @param arg2   Third argument, 0 for a service that takes fewer.
 * @return A psa_status_t value (int32_t): 0 on success or a negative PSA error.
 */
__attribute__((always_inline)) static inline int32_t
ironside_client_call_values(int32_t handle, uint32_t arg0, uint32_t arg1, uint32_t arg2)
{
	int32_t status = PSA_ERROR_GENERIC_ERROR;

	ironside_client_lock_acquire();
	status = IRONSIDE_CLIENT_ENTRY(handle, arg0, arg1, arg2);
	ironside_client_lock_release();

	return status;
}

#endif /* IRONSIDE_CLIENT_INTERNAL_CLIENT_H */
