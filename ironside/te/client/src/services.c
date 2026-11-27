/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide TE Client - secure service wrappers.
 *
 * Implements the service functions declared in the public headers. The wire
 * layouts they send to IronSide TE are private to this file.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <ironside_client/services/boot_report.h>
#include <ironside_client/services/devmgmt.h>
#include <ironside_client/services/dfu_request.h>
#include <ironside_client/services/platform.h>
#include <ironside_client/psa/initial_attestation.h>
#include <ironside_client/internal/client.h>

/* ---- Wire layouts --------------------------------------------------------- */

enum ironside_client_devmgmt_op {
	IRONSIDE_CLIENT_DEVMGMT_OP_SET_APPROTECT = 1,
	IRONSIDE_CLIENT_DEVMGMT_OP_DEFINE_XRAM = 3,
};

struct ironside_client_devmgmt_req {
	uint32_t op;
	uint32_t arg;
};

_Static_assert(sizeof(struct ironside_client_devmgmt_req) == 8,
	       "ironside_client_devmgmt_req ABI size");
_Static_assert(offsetof(struct ironside_client_devmgmt_req, arg) == 4,
	       "ironside_client_devmgmt_req arg offset");

enum ironside_client_dfu_req_op {
	IRONSIDE_CLIENT_DFU_REQ_OP_SET = 1,
	IRONSIDE_CLIENT_DFU_REQ_OP_CLEAR = 2,
	IRONSIDE_CLIENT_DFU_REQ_OP_GET = 3,
};

struct ironside_client_dfu_req {
	uint32_t op;
};

_Static_assert(sizeof(struct ironside_client_dfu_req) == 4, "ironside_client_dfu_req ABI size");
_Static_assert(offsetof(struct ironside_client_dfu_req, op) == 0,
	       "ironside_client_dfu_req op offset");

enum ironside_boot_report_op {
	IRONSIDE_BOOT_REPORT_OP_GET = 1,
};

struct ironside_boot_report_req {
	uint32_t op;
};

_Static_assert(sizeof(struct ironside_boot_report_req) == 4, "boot_report_req ABI size");

struct ironside_client_platform_nvm_write_args {
	uint32_t addr;
};

_Static_assert(sizeof(struct ironside_client_platform_nvm_write_args) == 4,
	       "ironside_client_platform_nvm_write_args ABI size");
_Static_assert(offsetof(struct ironside_client_platform_nvm_write_args, addr) == 0,
	       "ironside_client_platform_nvm_write_args addr offset");

_Static_assert(IRONSIDE_PLATFORM_RESET_SYSTEM == 0 && IRONSIDE_PLATFORM_RESET_CTRLAP_SOFT == 1 &&
		       IRONSIDE_PLATFORM_RESET_CTRLAP_PIN == 2,
	       "ironside_platform_reset_type wire values");

_Static_assert(IRONSIDE_PLATFORM_RRAM_POWER_MODE_DEFAULT == 0 &&
		       IRONSIDE_PLATFORM_RRAM_POWER_MODE_LOW_LATENCY == 1,
	       "ironside_platform_rram_power_mode wire values");

/* ---- Platform ------------------------------------------------------------- */

_Static_assert(sizeof(struct ironside_client_platform_mem_write_pair) == 8,
	       "ironside_client_platform_mem_write_pair ABI size");
_Static_assert(offsetof(struct ironside_client_platform_mem_write_pair, addr) == 0,
	       "ironside_client_platform_mem_write_pair addr offset");
_Static_assert(offsetof(struct ironside_client_platform_mem_write_pair, value) == 4,
	       "ironside_client_platform_mem_write_pair value offset");

int32_t ironside_platform_mem_read(const uint32_t *addrs, uint32_t *values, size_t count)
{
	if (addrs == NULL || values == NULL || count == 0U) {
		return -135;
	}

	struct ironside_client_iovec in_vec[1] = {
		{ .base = addrs, .len = count * sizeof(uint32_t) },
	};
	struct ironside_client_iovec out_vec[1] = {
		{ .base = values, .len = count * sizeof(uint32_t) },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PLATFORM_IOCTL_MEM_READ_HANDLE,
		.in_vec = in_vec,
		.in_len = 1,
		.out_vec = out_vec,
		.out_len = 1,
	};

	return ironside_client_call(&params);
}

int32_t ironside_platform_mem_write(const struct ironside_client_platform_mem_write_pair *pairs,
				    size_t count, uint32_t *readback, size_t readback_count)
{
	if (pairs == NULL || count == 0U) {
		return -135;
	}

	struct ironside_client_iovec in_vec[1] = {
		{ .base = pairs, .len = count * sizeof(pairs[0]) },
	};
	struct ironside_client_iovec out_vec[1] = {
		{ .base = readback, .len = readback_count * sizeof(uint32_t) },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PLATFORM_IOCTL_MEM_WRITE_HANDLE,
		.in_vec = in_vec,
		.in_len = 1,
		.out_vec = (readback != NULL && readback_count > 0U) ? out_vec : NULL,
		.out_len = (readback != NULL && readback_count > 0U) ? 1u : 0u,
	};

	return ironside_client_call(&params);
}

int32_t ironside_platform_reg_read(uint32_t addr, uint32_t *value)
{
	return ironside_platform_mem_read(&addr, value, 1U);
}

int32_t ironside_platform_reg_write(uint32_t addr, uint32_t value, uint32_t *readback)
{
	struct ironside_client_platform_mem_write_pair pair = {
		.addr = addr,
		.value = value,
	};

	return ironside_platform_mem_write(&pair, 1U, readback, readback != NULL ? 1U : 0U);
}

int32_t ironside_platform_rram_power_mode_set(enum ironside_platform_rram_power_mode mode)
{
	return ironside_client_call_values(IRONSIDE_CLIENT_PLATFORM_RRAMC_LOWPOWERCONFIG_HANDLE,
					   (uint32_t)mode, 0U, 0U);
}

int32_t ironside_platform_nvm_write(uint32_t addr, const void *src, uint32_t len)
{
	struct ironside_client_platform_nvm_write_args args = { .addr = addr };
	struct ironside_client_iovec in_vec[2] = {
		{ .base = &args, .len = sizeof(args) },
		{ .base = src, .len = len },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PLATFORM_IOCTL_NVM_WRITE_HANDLE,
		.in_vec = in_vec,
		.in_len = 2,
		.out_vec = NULL,
		.out_len = 0,
	};

	if (len == 0u) {
		return -135;
	}

	return ironside_client_call(&params);
}

int32_t ironside_platform_system_off(void)
{
	return ironside_client_call_values(IRONSIDE_CLIENT_PLATFORM_SYSTEM_OFF_HANDLE, 0U, 0U, 0U);
}

int32_t ironside_platform_system_reset(enum ironside_platform_reset_type type)
{
	return ironside_client_call_values(IRONSIDE_CLIENT_PLATFORM_SYSTEM_RESET_HANDLE,
					   (uint32_t)type, 0U, 0U);
}

/* ---- Device management ---------------------------------------------------- */

static int32_t ironside_devmgmt_call(uint32_t op, uint32_t arg)
{
	struct ironside_client_devmgmt_req req = { .op = op, .arg = arg };
	struct ironside_client_iovec in_vec[1] = {
		{ .base = &req, .len = sizeof(req) },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_DEVMGMT_HANDLE,
		.in_vec = in_vec,
		.in_len = 1,
		.out_vec = NULL,
		.out_len = 0,
	};

	return ironside_client_call(&params);
}

int32_t ironside_devmgmt_xram_region_define(uint32_t size)
{
	return ironside_devmgmt_call(IRONSIDE_CLIENT_DEVMGMT_OP_DEFINE_XRAM, size);
}

int32_t ironside_devmgmt_ap_protect(enum ironside_devmgmt_approtect_mode mode)
{
	/* The enum is the API surface only: it is widened to the wire's fixed-width
	 * uint32_t here, so no bare enum crosses the boundary. An out-of-range value
	 * is passed through and rejected by IronSide TE, which validates the mode as
	 * untrusted input anyway. */
	return ironside_devmgmt_call(IRONSIDE_CLIENT_DEVMGMT_OP_SET_APPROTECT, (uint32_t)mode);
}

/* ---- DFU-entry request ---------------------------------------------------- */

/** One DFU-request call with no response. */
static int32_t ironside_dfu_request_op(uint32_t op)
{
	struct ironside_client_dfu_req req = { .op = op };
	struct ironside_client_iovec in_vec[1] = {
		{ .base = &req, .len = sizeof(req) },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_DFU_REQ_HANDLE,
		.in_vec = in_vec,
		.in_len = 1,
		.out_vec = NULL,
		.out_len = 0,
	};

	return ironside_client_call(&params);
}

int32_t ironside_dfu_request_set(void)
{
	return ironside_dfu_request_op(IRONSIDE_CLIENT_DFU_REQ_OP_SET);
}

int32_t ironside_dfu_request_clear(void)
{
	return ironside_dfu_request_op(IRONSIDE_CLIENT_DFU_REQ_OP_CLEAR);
}

int32_t ironside_dfu_request_get(bool *pending)
{
	struct ironside_client_dfu_req req = { .op = IRONSIDE_CLIENT_DFU_REQ_OP_GET };
	uint32_t rsp = 0;
	struct ironside_client_iovec in_vec[1] = {
		{ .base = &req, .len = sizeof(req) },
	};
	struct ironside_client_iovec out_vec[1] = {
		{ .base = &rsp, .len = sizeof(rsp) },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_DFU_REQ_HANDLE,
		.in_vec = in_vec,
		.in_len = 1,
		.out_vec = out_vec,
		.out_len = 1,
	};
	int32_t status = ironside_client_call(&params);

	if (status == 0) {
		*pending = (rsp != 0u);
	}
	return status;
}

/* ---- Boot report ---------------------------------------------------------- */

_Static_assert(sizeof(struct ironside_image_version) == 4, "ironside_image_version ABI size");
_Static_assert(sizeof(struct ironside_boot_region) == 8, "ironside_boot_region ABI size");
_Static_assert(sizeof(struct ironside_boot_report) == 180, "ironside_boot_report ABI size");
_Static_assert(offsetof(struct ironside_boot_report, update_target) == 4,
	       "ironside_boot_report update_target offset");
_Static_assert(offsetof(struct ironside_boot_report, update_status) == 8,
	       "ironside_boot_report update_status offset");
_Static_assert(offsetof(struct ironside_boot_report, booted_image) == 12,
	       "ironside_boot_report booted_image offset");
_Static_assert(offsetof(struct ironside_boot_report, dfu_reason) == 16,
	       "ironside_boot_report dfu_reason offset");
_Static_assert(offsetof(struct ironside_boot_report, lifecycle) == 20,
	       "ironside_boot_report lifecycle offset");
_Static_assert(offsetof(struct ironside_boot_report, irot_version) == 24,
	       "ironside_boot_report irot_version offset");
_Static_assert(offsetof(struct ironside_boot_report, urot_version) == 28,
	       "ironside_boot_report urot_version offset");
_Static_assert(offsetof(struct ironside_boot_report, app_version) == 32,
	       "ironside_boot_report app_version offset");
_Static_assert(offsetof(struct ironside_boot_report, dfu_version) == 36,
	       "ironside_boot_report dfu_version offset");
_Static_assert(sizeof(struct ironside_fault_report) == 92, "ironside_fault_report ABI size");
_Static_assert(offsetof(struct ironside_boot_report, fault) == 40,
	       "ironside_boot_report fault offset");
_Static_assert(offsetof(struct ironside_boot_report, reset_reason) == 132,
	       "ironside_boot_report reset_reason offset");
_Static_assert(offsetof(struct ironside_boot_report, slot0) == 136,
	       "ironside_boot_report slot0 offset");
_Static_assert(offsetof(struct ironside_boot_report, slot1) == 144,
	       "ironside_boot_report slot1 offset");
_Static_assert(offsetof(struct ironside_boot_report, appstorage) == 152,
	       "ironside_boot_report appstorage offset");
_Static_assert(offsetof(struct ironside_boot_report, dfuimage) == 160,
	       "ironside_boot_report dfuimage offset");
_Static_assert(offsetof(struct ironside_boot_report, ram_ns) == 168,
	       "ironside_boot_report ram_ns offset");
_Static_assert(offsetof(struct ironside_boot_report, fault_image) == 176,
	       "ironside_boot_report fault_image offset");
_Static_assert(offsetof(struct ironside_fault_report, source) == 4,
	       "ironside_fault_report source offset");
_Static_assert(offsetof(struct ironside_fault_report, count) == 12,
	       "ironside_fault_report count offset");
_Static_assert(offsetof(struct ironside_fault_report, exception) == 16,
	       "ironside_fault_report exception offset");
_Static_assert(offsetof(struct ironside_fault_report, cfsr) == 20,
	       "ironside_fault_report cfsr offset");
_Static_assert(offsetof(struct ironside_fault_report, cfsr_ns) == 40,
	       "ironside_fault_report cfsr_ns offset");
_Static_assert(offsetof(struct ironside_fault_report, frame) == 48,
	       "ironside_fault_report frame offset");
_Static_assert(offsetof(struct ironside_fault_report, frame_sp) == 80,
	       "ironside_fault_report frame_sp offset");
_Static_assert(offsetof(struct ironside_fault_report, mpc_address) == 84,
	       "ironside_fault_report mpc_address offset");
_Static_assert(offsetof(struct ironside_fault_report, mpc_access) == 88,
	       "ironside_fault_report mpc_access offset");

int32_t ironside_boot_report_get(struct ironside_boot_report *report, size_t report_size,
				 size_t *report_length)
{
	struct ironside_boot_report_req req = { .op = IRONSIDE_BOOT_REPORT_OP_GET };

	if (report == NULL || report_size == 0 || report_length == NULL) {
		return -135;
	}

	*report_length = 0;

	struct ironside_client_iovec in_vec[1] = {
		{ .base = &req, .len = sizeof(req) },
	};
	struct ironside_client_iovec out_vec[1] = {
		{ .base = report, .len = report_size },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_BOOT_REPORT_HANDLE,
		.in_vec = in_vec,
		.in_len = 1,
		.out_vec = out_vec,
		.out_len = 1,
	};

	int32_t status = ironside_client_call(&params);

	if (status == 0) {
		/* out_vec[0].len is updated on return (client.h) - same idiom as
		 * psa_hash_compute()'s hash_length. */
		*report_length = out_vec[0].len;
	}

	return status;
}

/* ---- PSA Initial Attestation ---------------------------------------------- */

enum ironside_client_attest_op {
	IRONSIDE_CLIENT_ATTEST_OP_GET_TOKEN_SIZE = 1,
	IRONSIDE_CLIENT_ATTEST_OP_GET_TOKEN = 2,
};

#define IRONSIDE_CLIENT_ATTEST_CHALLENGE_LEN 64U

struct ironside_client_attest_req {
	uint32_t op;
	uint32_t challenge_size;
	uint8_t challenge[IRONSIDE_CLIENT_ATTEST_CHALLENGE_LEN];
};

_Static_assert(sizeof(struct ironside_client_attest_req) == 72,
	       "ironside_client_attest_req ABI size");
_Static_assert(offsetof(struct ironside_client_attest_req, challenge_size) == 4,
	       "ironside_client_attest_req challenge_size offset");
_Static_assert(offsetof(struct ironside_client_attest_req, challenge) == 8,
	       "ironside_client_attest_req challenge offset");

psa_status_t psa_initial_attest_get_token_size(size_t challenge_size, size_t *token_size)
{
	struct ironside_client_attest_req req = {
		.op = IRONSIDE_CLIENT_ATTEST_OP_GET_TOKEN_SIZE,
		.challenge_size = (uint32_t)challenge_size,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &req, .len = sizeof(req) },
	};
	size_t size_out = 0;
	struct ironside_client_iovec out_vec[] = {
		{ .base = &size_out, .len = sizeof(size_out) },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_INITIAL_ATTEST_HANDLE,
		.in_vec = in_vec,
		.in_len = 1,
		.out_vec = out_vec,
		.out_len = 1,
	};

	psa_status_t status = ironside_client_call(&params);

	if (status == PSA_SUCCESS && token_size != NULL) {
		*token_size = size_out;
	}
	return status;
}

psa_status_t psa_initial_attest_get_token(const uint8_t *auth_challenge, size_t challenge_size,
					  uint8_t *token_buf, size_t token_buf_size,
					  size_t *token_size)
{
	struct ironside_client_attest_req req = {
		.op = IRONSIDE_CLIENT_ATTEST_OP_GET_TOKEN,
		.challenge_size = (uint32_t)challenge_size,
	};
	/* Bounded regardless of what challenge_size claims - a value bigger than
	 * the 64-byte wire buffer (i.e. not one of the three legal sizes) simply
	 * doesn't get copied in full; IronSide TE rejects any size outside {32,48,64}
	 * before ever reading .challenge, so a partial copy here is harmless. */
	size_t copy_len = challenge_size < sizeof(req.challenge) ? challenge_size :
								   sizeof(req.challenge);

	for (size_t i = 0; i < copy_len; i++) {
		req.challenge[i] = auth_challenge[i];
	}

	struct ironside_client_iovec in_vec[] = {
		{ .base = &req, .len = sizeof(req) },
	};
	struct ironside_client_iovec out_vec[] = {
		{ .base = token_buf, .len = token_buf_size },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_INITIAL_ATTEST_HANDLE,
		.in_vec = in_vec,
		.in_len = 1,
		.out_vec = out_vec,
		.out_len = 1,
	};

	psa_status_t status = ironside_client_call(&params);

	if (status == PSA_SUCCESS && token_size != NULL) {
		*token_size = out_vec[0].len;
	}
	return status;
}
