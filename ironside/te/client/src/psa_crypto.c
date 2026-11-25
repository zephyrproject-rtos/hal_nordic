/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide TE Client - PSA Crypto dispatch.
 */

#include <ironside_client/psa/crypto.h>
#include <ironside_client/psa/crypto_abi.h>

#include <ironside_client/internal/client.h>
#include <ironside_client/internal/psa/crypto_wire.h>

_Static_assert(sizeof(psa_status_t) == IRONSIDE_CLIENT_PSA_ABI_STATUS_SIZE, "status width");
_Static_assert(sizeof(psa_key_id_t) == IRONSIDE_CLIENT_PSA_ABI_KEY_ID_SIZE, "key id width");
_Static_assert(sizeof(psa_key_type_t) == IRONSIDE_CLIENT_PSA_ABI_KEY_TYPE_SIZE, "key type width");
_Static_assert(sizeof(psa_key_lifetime_t) == IRONSIDE_CLIENT_PSA_ABI_KEY_LIFETIME_SIZE,
	       "key lifetime width");
_Static_assert(sizeof(psa_key_usage_t) == IRONSIDE_CLIENT_PSA_ABI_KEY_USAGE_SIZE,
	       "key usage width");
_Static_assert(sizeof(psa_algorithm_t) == IRONSIDE_CLIENT_PSA_ABI_ALGORITHM_SIZE,
	       "algorithm width");
_Static_assert(sizeof(psa_key_derivation_step_t) == IRONSIDE_CLIENT_PSA_ABI_DERIVATION_STEP_SIZE,
	       "key derivation step width");

#define ASSERT_KEY_ATTRIBUTE(member, offset, size)                                    \
	_Static_assert(offsetof(psa_key_attributes_t, member) == (offset) &&          \
			       sizeof(((psa_key_attributes_t *)0)->member) == (size), \
		       "key attributes " #member " layout")

_Static_assert(sizeof(psa_key_attributes_t) == IRONSIDE_CLIENT_PSA_ABI_KEY_ATTRIBUTES_SIZE,
	       "key attributes size");
_Static_assert(_Alignof(psa_key_attributes_t) == IRONSIDE_CLIENT_PSA_ABI_KEY_ATTRIBUTES_ALIGN,
	       "key attributes alignment");
ASSERT_KEY_ATTRIBUTE(type, IRONSIDE_CLIENT_PSA_ABI_KEY_ATTRIBUTES_TYPE_OFFSET,
		     IRONSIDE_CLIENT_PSA_ABI_KEY_TYPE_SIZE);
ASSERT_KEY_ATTRIBUTE(bits, IRONSIDE_CLIENT_PSA_ABI_KEY_ATTRIBUTES_BITS_OFFSET,
		     IRONSIDE_CLIENT_PSA_ABI_KEY_BITS_SIZE);
ASSERT_KEY_ATTRIBUTE(lifetime, IRONSIDE_CLIENT_PSA_ABI_KEY_ATTRIBUTES_LIFETIME_OFFSET,
		     IRONSIDE_CLIENT_PSA_ABI_KEY_LIFETIME_SIZE);
ASSERT_KEY_ATTRIBUTE(usage, IRONSIDE_CLIENT_PSA_ABI_KEY_ATTRIBUTES_USAGE_OFFSET,
		     IRONSIDE_CLIENT_PSA_ABI_KEY_USAGE_SIZE);
ASSERT_KEY_ATTRIBUTE(alg, IRONSIDE_CLIENT_PSA_ABI_KEY_ATTRIBUTES_ALG_OFFSET,
		     IRONSIDE_CLIENT_PSA_ABI_ALGORITHM_SIZE);
ASSERT_KEY_ATTRIBUTE(alg2, IRONSIDE_CLIENT_PSA_ABI_KEY_ATTRIBUTES_ALG2_OFFSET,
		     IRONSIDE_CLIENT_PSA_ABI_ALGORITHM_SIZE);
ASSERT_KEY_ATTRIBUTE(id, IRONSIDE_CLIENT_PSA_ABI_KEY_ATTRIBUTES_ID_OFFSET,
		     IRONSIDE_CLIENT_PSA_ABI_KEY_ID_SIZE);

#define ASSERT_OPERATION_LAYOUT(type)                                                     \
	_Static_assert(sizeof(type) == IRONSIDE_CLIENT_PSA_ABI_OPERATION_SIZE &&          \
			       _Alignof(type) == IRONSIDE_CLIENT_PSA_ABI_OPERATION_ALIGN, \
		       #type " layout")

ASSERT_OPERATION_LAYOUT(psa_hash_operation_t);
ASSERT_OPERATION_LAYOUT(psa_mac_operation_t);
ASSERT_OPERATION_LAYOUT(psa_cipher_operation_t);
ASSERT_OPERATION_LAYOUT(psa_aead_operation_t);
ASSERT_OPERATION_LAYOUT(psa_key_derivation_operation_t);
ASSERT_OPERATION_LAYOUT(psa_pake_operation_t);

/* On failure the secure side leaves an output vector's length at its capacity. */
static size_t produced_output_length(psa_status_t status,
				     const struct ironside_client_iovec *out_vec)
{
	if (status != PSA_SUCCESS) {
		return 0U;
	}
	return out_vec->len;
}

psa_status_t psa_crypto_init(void)
{
	return PSA_SUCCESS;
}

psa_status_t psa_get_key_attributes(psa_key_id_t key, psa_key_attributes_t *attributes)
{
	struct ironside_client_pack_iovec iov = {
		.key_id = key,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_GET_KEY_ATTRIBUTES_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
	};
	struct ironside_client_iovec out_vec[] = {
		{ .base = attributes, .len = sizeof(*attributes) },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 1,
		.out_vec = out_vec,
		.out_len = 1,
	};

	psa_status_t status = ironside_client_call(&params);
	if (status != PSA_SUCCESS) {
		psa_reset_key_attributes(attributes);
	}
	return status;
}

__attribute__((weak)) void psa_reset_key_attributes(psa_key_attributes_t *attributes)
{
	*attributes = psa_key_attributes_init();
}

static void set_key_id_on_error(psa_key_id_t *key)
{
	if (key != NULL) {
		*key = PSA_KEY_ID_NULL;
	}
}

psa_status_t psa_import_key(const psa_key_attributes_t *attributes, const uint8_t *data,
			    size_t data_length, psa_key_id_t *key)
{
	struct ironside_client_pack_iovec iov = {
		.key_id = psa_get_key_id(attributes),
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_IMPORT_KEY_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
		{ .base = attributes, .len = sizeof(*attributes) },
		{ .base = data, .len = data_length },
	};
	struct ironside_client_iovec out_vec[] = {
		{ .base = key, .len = sizeof(*key) },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 3,
		.out_vec = out_vec,
		.out_len = 1,
	};
	psa_status_t status = ironside_client_call(&params);

	if (status != PSA_SUCCESS) {
		set_key_id_on_error(key);
	}

	return status;
}

psa_status_t psa_generate_key(const psa_key_attributes_t *attributes, psa_key_id_t *key)
{
	struct ironside_client_pack_iovec iov = {
		.key_id = psa_get_key_id(attributes),
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_GENERATE_KEY_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
		{ .base = attributes, .len = sizeof(*attributes) },
	};
	struct ironside_client_iovec out_vec[] = {
		{ .base = key, .len = sizeof(*key) },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 2,
		.out_vec = out_vec,
		.out_len = 1,
	};
	psa_status_t status = ironside_client_call(&params);

	if (status != PSA_SUCCESS) {
		set_key_id_on_error(key);
	}

	return status;
}

psa_status_t psa_copy_key(psa_key_id_t source_key, const psa_key_attributes_t *attributes,
			  psa_key_id_t *target_key)
{
	struct ironside_client_pack_iovec iov = {
		.key_id = source_key,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_COPY_KEY_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
		{ .base = attributes, .len = sizeof(*attributes) },
	};
	struct ironside_client_iovec out_vec[] = {
		{ .base = target_key, .len = sizeof(*target_key) },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 2,
		.out_vec = out_vec,
		.out_len = 1,
	};

	return ironside_client_call(&params);
}

psa_status_t psa_destroy_key(psa_key_id_t key)
{
	struct ironside_client_pack_iovec iov = {
		.key_id = key,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_DESTROY_KEY_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 1,
		.out_vec = NULL,
		.out_len = 0,
	};

	return ironside_client_call(&params);
}

psa_status_t psa_export_key(psa_key_id_t key, uint8_t *data, size_t data_size, size_t *data_length)
{
	struct ironside_client_pack_iovec iov = {
		.key_id = key,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_EXPORT_KEY_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
	};
	struct ironside_client_iovec out_vec[] = {
		{ .base = data, .len = data_size },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 1,
		.out_vec = out_vec,
		.out_len = 1,
	};

	psa_status_t status = ironside_client_call(&params);

	if (data_length != NULL) {
		*data_length = produced_output_length(status, &out_vec[0]);
	}
	return status;
}

psa_status_t psa_export_public_key(psa_key_id_t key, uint8_t *data, size_t data_size,
				   size_t *data_length)
{
	struct ironside_client_pack_iovec iov = {
		.key_id = key,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_EXPORT_PUBLIC_KEY_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
	};
	struct ironside_client_iovec out_vec[] = {
		{ .base = data, .len = data_size },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 1,
		.out_vec = out_vec,
		.out_len = 1,
	};

	psa_status_t status = ironside_client_call(&params);

	if (data_length != NULL) {
		*data_length = produced_output_length(status, &out_vec[0]);
	}
	return status;
}

psa_status_t psa_hash_compute(psa_algorithm_t alg, const uint8_t *input, size_t input_length,
			      uint8_t *hash, size_t hash_size, size_t *hash_length)
{
	struct ironside_client_pack_iovec iov = {
		.alg = alg,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_HASH_COMPUTE_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
		{ .base = input, .len = input_length },
	};
	struct ironside_client_iovec out_vec[] = {
		{ .base = hash, .len = hash_size },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 2,
		.out_vec = out_vec,
		.out_len = 1,
	};

	psa_status_t status = ironside_client_call(&params);

	if (hash_length != NULL) {
		*hash_length = produced_output_length(status, &out_vec[0]);
	}
	return status;
}

psa_status_t psa_hash_compare(psa_algorithm_t alg, const uint8_t *input, size_t input_length,
			      const uint8_t *hash, size_t hash_length)
{
	struct ironside_client_pack_iovec iov = {
		.alg = alg,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_HASH_COMPARE_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
		{ .base = input, .len = input_length },
		{ .base = hash, .len = hash_length },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 3,
		.out_vec = NULL,
		.out_len = 0,
	};

	return ironside_client_call(&params);
}

psa_status_t psa_hash_setup(psa_hash_operation_t *operation, psa_algorithm_t alg)
{
	struct ironside_client_pack_iovec iov = {
		.alg = alg,
		.op_handle = operation->handle,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_HASH_SETUP_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
	};
	struct ironside_client_iovec out_vec[] = {
		{ .base = &operation->handle, .len = sizeof(operation->handle) },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 1,
		.out_vec = out_vec,
		.out_len = 1,
	};

	return ironside_client_call(&params);
}

psa_status_t psa_hash_update(psa_hash_operation_t *operation, const uint8_t *input,
			     size_t input_length)
{
	struct ironside_client_pack_iovec iov = {
		.op_handle = operation->handle,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_HASH_UPDATE_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
		{ .base = input, .len = input_length },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 2,
		.out_vec = NULL,
		.out_len = 0,
	};

	return ironside_client_call(&params);
}

psa_status_t psa_hash_finish(psa_hash_operation_t *operation, uint8_t *hash, size_t hash_size,
			     size_t *hash_length)
{
	struct ironside_client_pack_iovec iov = {
		.op_handle = operation->handle,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_HASH_FINISH_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
	};
	struct ironside_client_iovec out_vec[] = {
		{ .base = &operation->handle, .len = sizeof(operation->handle) },
		{ .base = hash, .len = hash_size },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 1,
		.out_vec = out_vec,
		.out_len = 2,
	};

	psa_status_t status = ironside_client_call(&params);

	if (hash_length != NULL) {
		*hash_length = produced_output_length(status, &out_vec[1]);
	}
	return status;
}

psa_status_t psa_hash_verify(psa_hash_operation_t *operation, const uint8_t *hash,
			     size_t hash_length)
{
	struct ironside_client_pack_iovec iov = {
		.op_handle = operation->handle,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_HASH_VERIFY_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
		{ .base = hash, .len = hash_length },
	};
	struct ironside_client_iovec out_vec[] = {
		{ .base = &operation->handle, .len = sizeof(operation->handle) },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 2,
		.out_vec = out_vec,
		.out_len = 1,
	};

	return ironside_client_call(&params);
}

psa_status_t psa_hash_abort(psa_hash_operation_t *operation)
{
	struct ironside_client_pack_iovec iov = {
		.op_handle = operation->handle,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_HASH_ABORT_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
	};
	struct ironside_client_iovec out_vec[] = {
		{ .base = &operation->handle, .len = sizeof(operation->handle) },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 1,
		.out_vec = out_vec,
		.out_len = 1,
	};

	return ironside_client_call(&params);
}

psa_status_t psa_hash_clone(const psa_hash_operation_t *source_operation,
			    psa_hash_operation_t *target_operation)
{
	struct ironside_client_pack_iovec iov = {
		.op_handle = source_operation->handle,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_HASH_CLONE_SID,
	};

	if (target_operation->handle != 0) {
		return PSA_ERROR_BAD_STATE;
	}

	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
		{ .base = &target_operation->handle, .len = sizeof(target_operation->handle) },
	};
	struct ironside_client_iovec out_vec[] = {
		{ .base = &target_operation->handle, .len = sizeof(target_operation->handle) },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 2,
		.out_vec = out_vec,
		.out_len = 1,
	};

	return ironside_client_call(&params);
}

psa_status_t psa_mac_compute(psa_key_id_t key, psa_algorithm_t alg, const uint8_t *input,
			     size_t input_length, uint8_t *mac, size_t mac_size, size_t *mac_length)
{
	struct ironside_client_pack_iovec iov = {
		.key_id = key,
		.alg = alg,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_MAC_COMPUTE_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
		{ .base = input, .len = input_length },
	};
	struct ironside_client_iovec out_vec[] = {
		{ .base = mac, .len = mac_size },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 2,
		.out_vec = out_vec,
		.out_len = 1,
	};

	psa_status_t status = ironside_client_call(&params);

	if (mac_length != NULL) {
		*mac_length = produced_output_length(status, &out_vec[0]);
	}
	return status;
}

psa_status_t psa_mac_verify(psa_key_id_t key, psa_algorithm_t alg, const uint8_t *input,
			    size_t input_length, const uint8_t *mac, size_t mac_length)
{
	struct ironside_client_pack_iovec iov = {
		.key_id = key,
		.alg = alg,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_MAC_VERIFY_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
		{ .base = input, .len = input_length },
		{ .base = mac, .len = mac_length },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 3,
		.out_vec = NULL,
		.out_len = 0,
	};

	return ironside_client_call(&params);
}

psa_status_t psa_mac_sign_setup(psa_mac_operation_t *operation, psa_key_id_t key,
				psa_algorithm_t alg)
{
	struct ironside_client_pack_iovec iov = {
		.key_id = key,
		.alg = alg,
		.op_handle = operation->handle,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_MAC_SIGN_SETUP_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
	};
	struct ironside_client_iovec out_vec[] = {
		{ .base = &operation->handle, .len = sizeof(operation->handle) },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 1,
		.out_vec = out_vec,
		.out_len = 1,
	};

	return ironside_client_call(&params);
}

psa_status_t psa_mac_verify_setup(psa_mac_operation_t *operation, psa_key_id_t key,
				  psa_algorithm_t alg)
{
	struct ironside_client_pack_iovec iov = {
		.key_id = key,
		.alg = alg,
		.op_handle = operation->handle,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_MAC_VERIFY_SETUP_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
	};
	struct ironside_client_iovec out_vec[] = {
		{ .base = &operation->handle, .len = sizeof(operation->handle) },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 1,
		.out_vec = out_vec,
		.out_len = 1,
	};

	return ironside_client_call(&params);
}

psa_status_t psa_mac_update(psa_mac_operation_t *operation, const uint8_t *input,
			    size_t input_length)
{
	struct ironside_client_pack_iovec iov = {
		.op_handle = operation->handle,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_MAC_UPDATE_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
		{ .base = input, .len = input_length },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 2,
		.out_vec = NULL,
		.out_len = 0,
	};

	return ironside_client_call(&params);
}

psa_status_t psa_mac_sign_finish(psa_mac_operation_t *operation, uint8_t *mac, size_t mac_size,
				 size_t *mac_length)
{
	struct ironside_client_pack_iovec iov = {
		.op_handle = operation->handle,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_MAC_SIGN_FINISH_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
	};
	struct ironside_client_iovec out_vec[] = {
		{ .base = &operation->handle, .len = sizeof(operation->handle) },
		{ .base = mac, .len = mac_size },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 1,
		.out_vec = out_vec,
		.out_len = 2,
	};

	psa_status_t status = ironside_client_call(&params);

	if (mac_length != NULL) {
		*mac_length = produced_output_length(status, &out_vec[1]);
	}
	return status;
}

psa_status_t psa_mac_verify_finish(psa_mac_operation_t *operation, const uint8_t *mac,
				   size_t mac_length)
{
	struct ironside_client_pack_iovec iov = {
		.op_handle = operation->handle,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_MAC_VERIFY_FINISH_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
		{ .base = mac, .len = mac_length },
	};
	struct ironside_client_iovec out_vec[] = {
		{ .base = &operation->handle, .len = sizeof(operation->handle) },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 2,
		.out_vec = out_vec,
		.out_len = 1,
	};

	return ironside_client_call(&params);
}

psa_status_t psa_mac_abort(psa_mac_operation_t *operation)
{
	struct ironside_client_pack_iovec iov = {
		.op_handle = operation->handle,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_MAC_ABORT_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
	};
	struct ironside_client_iovec out_vec[] = {
		{ .base = &operation->handle, .len = sizeof(operation->handle) },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 1,
		.out_vec = out_vec,
		.out_len = 1,
	};

	return ironside_client_call(&params);
}

psa_status_t psa_cipher_encrypt(psa_key_id_t key, psa_algorithm_t alg, const uint8_t *input,
				size_t input_length, uint8_t *output, size_t output_size,
				size_t *output_length)
{
	struct ironside_client_pack_iovec iov = {
		.key_id = key,
		.alg = alg,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_CIPHER_ENCRYPT_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
		{ .base = input, .len = input_length },
	};
	struct ironside_client_iovec out_vec[] = {
		{ .base = output, .len = output_size },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 2,
		.out_vec = out_vec,
		.out_len = 1,
	};

	psa_status_t status = ironside_client_call(&params);

	if (output_length != NULL) {
		*output_length = produced_output_length(status, &out_vec[0]);
	}
	return status;
}

psa_status_t psa_cipher_decrypt(psa_key_id_t key, psa_algorithm_t alg, const uint8_t *input,
				size_t input_length, uint8_t *output, size_t output_size,
				size_t *output_length)
{
	struct ironside_client_pack_iovec iov = {
		.key_id = key,
		.alg = alg,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_CIPHER_DECRYPT_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
		{ .base = input, .len = input_length },
	};
	struct ironside_client_iovec out_vec[] = {
		{ .base = output, .len = output_size },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 2,
		.out_vec = out_vec,
		.out_len = 1,
	};

	psa_status_t status = ironside_client_call(&params);

	if (output_length != NULL) {
		*output_length = produced_output_length(status, &out_vec[0]);
	}
	return status;
}

psa_status_t psa_cipher_encrypt_setup(psa_cipher_operation_t *operation, psa_key_id_t key,
				      psa_algorithm_t alg)
{
	struct ironside_client_pack_iovec iov = {
		.key_id = key,
		.alg = alg,
		.op_handle = operation->handle,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_CIPHER_ENCRYPT_SETUP_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
	};
	struct ironside_client_iovec out_vec[] = {
		{ .base = &operation->handle, .len = sizeof(operation->handle) },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 1,
		.out_vec = out_vec,
		.out_len = 1,
	};

	return ironside_client_call(&params);
}

psa_status_t psa_cipher_decrypt_setup(psa_cipher_operation_t *operation, psa_key_id_t key,
				      psa_algorithm_t alg)
{
	struct ironside_client_pack_iovec iov = {
		.key_id = key,
		.alg = alg,
		.op_handle = operation->handle,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_CIPHER_DECRYPT_SETUP_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
	};
	struct ironside_client_iovec out_vec[] = {
		{ .base = &operation->handle, .len = sizeof(operation->handle) },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 1,
		.out_vec = out_vec,
		.out_len = 1,
	};

	return ironside_client_call(&params);
}

psa_status_t psa_cipher_generate_iv(psa_cipher_operation_t *operation, uint8_t *iv, size_t iv_size,
				    size_t *iv_length)
{
	struct ironside_client_pack_iovec iov = {
		.op_handle = operation->handle,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_CIPHER_GENERATE_IV_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
	};
	struct ironside_client_iovec out_vec[] = {
		{ .base = iv, .len = iv_size },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 1,
		.out_vec = out_vec,
		.out_len = 1,
	};

	psa_status_t status = ironside_client_call(&params);

	if (iv_length != NULL) {
		*iv_length = produced_output_length(status, &out_vec[0]);
	}
	return status;
}

psa_status_t psa_cipher_set_iv(psa_cipher_operation_t *operation, const uint8_t *iv,
			       size_t iv_length)
{
	struct ironside_client_pack_iovec iov = {
		.op_handle = operation->handle,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_CIPHER_SET_IV_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
		{ .base = iv, .len = iv_length },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 2,
		.out_vec = NULL,
		.out_len = 0,
	};

	return ironside_client_call(&params);
}

psa_status_t psa_cipher_update(psa_cipher_operation_t *operation, const uint8_t *input,
			       size_t input_length, uint8_t *output, size_t output_size,
			       size_t *output_length)
{
	struct ironside_client_pack_iovec iov = {
		.op_handle = operation->handle,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_CIPHER_UPDATE_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
		{ .base = input, .len = input_length },
	};
	struct ironside_client_iovec out_vec[] = {
		{ .base = output, .len = output_size },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 2,
		.out_vec = out_vec,
		.out_len = 1,
	};

	psa_status_t status = ironside_client_call(&params);

	if (output_length != NULL) {
		*output_length = produced_output_length(status, &out_vec[0]);
	}
	return status;
}

psa_status_t psa_cipher_finish(psa_cipher_operation_t *operation, uint8_t *output,
			       size_t output_size, size_t *output_length)
{
	struct ironside_client_pack_iovec iov = {
		.op_handle = operation->handle,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_CIPHER_FINISH_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
	};
	struct ironside_client_iovec out_vec[] = {
		{ .base = &operation->handle, .len = sizeof(operation->handle) },
		{ .base = output, .len = output_size },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 1,
		.out_vec = out_vec,
		.out_len = 2,
	};

	psa_status_t status = ironside_client_call(&params);

	if (output_length != NULL) {
		*output_length = produced_output_length(status, &out_vec[1]);
	}
	return status;
}

psa_status_t psa_cipher_abort(psa_cipher_operation_t *operation)
{
	struct ironside_client_pack_iovec iov = {
		.op_handle = operation->handle,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_CIPHER_ABORT_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
	};
	struct ironside_client_iovec out_vec[] = {
		{ .base = &operation->handle, .len = sizeof(operation->handle) },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 1,
		.out_vec = out_vec,
		.out_len = 1,
	};

	return ironside_client_call(&params);
}

psa_status_t psa_aead_encrypt(psa_key_id_t key, psa_algorithm_t alg, const uint8_t *nonce,
			      size_t nonce_length, const uint8_t *additional_data,
			      size_t additional_data_length, const uint8_t *plaintext,
			      size_t plaintext_length, uint8_t *ciphertext, size_t ciphertext_size,
			      size_t *ciphertext_length)
{
	struct ironside_client_pack_iovec iov = {
		.key_id = key,
		.alg = alg,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_AEAD_ENCRYPT_SID,
		.nonce_length = (uint32_t)nonce_length,
	};
	if (nonce_length > sizeof(iov.nonce)) {
		if (ciphertext_length != NULL) {
			*ciphertext_length = 0U;
		}
		return PSA_ERROR_INVALID_ARGUMENT;
	}
	for (size_t i = 0; i < nonce_length; i++) {
		iov.nonce[i] = nonce[i];
	}
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
		{ .base = plaintext, .len = plaintext_length },
		{ .base = additional_data, .len = additional_data_length },
	};
	struct ironside_client_iovec out_vec[] = {
		{ .base = ciphertext, .len = ciphertext_size },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 3,
		.out_vec = out_vec,
		.out_len = 1,
	};

	psa_status_t status = ironside_client_call(&params);

	if (ciphertext_length != NULL) {
		*ciphertext_length = produced_output_length(status, &out_vec[0]);
	}
	return status;
}

psa_status_t psa_aead_decrypt(psa_key_id_t key, psa_algorithm_t alg, const uint8_t *nonce,
			      size_t nonce_length, const uint8_t *additional_data,
			      size_t additional_data_length, const uint8_t *ciphertext,
			      size_t ciphertext_length, uint8_t *plaintext, size_t plaintext_size,
			      size_t *plaintext_length)
{
	struct ironside_client_pack_iovec iov = {
		.key_id = key,
		.alg = alg,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_AEAD_DECRYPT_SID,
		.nonce_length = (uint32_t)nonce_length,
	};
	if (nonce_length > sizeof(iov.nonce)) {
		if (plaintext_length != NULL) {
			*plaintext_length = 0U;
		}
		return PSA_ERROR_INVALID_ARGUMENT;
	}
	for (size_t i = 0; i < nonce_length; i++) {
		iov.nonce[i] = nonce[i];
	}
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
		{ .base = ciphertext, .len = ciphertext_length },
		{ .base = additional_data, .len = additional_data_length },
	};
	struct ironside_client_iovec out_vec[] = {
		{ .base = plaintext, .len = plaintext_size },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 3,
		.out_vec = out_vec,
		.out_len = 1,
	};

	psa_status_t status = ironside_client_call(&params);

	if (plaintext_length != NULL) {
		*plaintext_length = produced_output_length(status, &out_vec[0]);
	}
	return status;
}

psa_status_t psa_aead_encrypt_setup(psa_aead_operation_t *operation, psa_key_id_t key,
				    psa_algorithm_t alg)
{
	struct ironside_client_pack_iovec iov = {
		.key_id = key,
		.alg = alg,
		.op_handle = operation->handle,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_AEAD_ENCRYPT_SETUP_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
	};
	struct ironside_client_iovec out_vec[] = {
		{ .base = &operation->handle, .len = sizeof(operation->handle) },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 1,
		.out_vec = out_vec,
		.out_len = 1,
	};

	return ironside_client_call(&params);
}

psa_status_t psa_aead_decrypt_setup(psa_aead_operation_t *operation, psa_key_id_t key,
				    psa_algorithm_t alg)
{
	struct ironside_client_pack_iovec iov = {
		.key_id = key,
		.alg = alg,
		.op_handle = operation->handle,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_AEAD_DECRYPT_SETUP_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
	};
	struct ironside_client_iovec out_vec[] = {
		{ .base = &operation->handle, .len = sizeof(operation->handle) },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 1,
		.out_vec = out_vec,
		.out_len = 1,
	};

	return ironside_client_call(&params);
}

psa_status_t psa_aead_set_lengths(psa_aead_operation_t *operation, size_t ad_length,
				  size_t plaintext_length)
{
	struct ironside_client_pack_iovec iov = {
		.op_handle = operation->handle,
		.ad_length = (uint32_t)ad_length,
		.plaintext_length = (uint32_t)plaintext_length,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_AEAD_SET_LENGTHS_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 1,
		.out_vec = NULL,
		.out_len = 0,
	};

	return ironside_client_call(&params);
}

psa_status_t psa_aead_generate_nonce(psa_aead_operation_t *operation, uint8_t *nonce,
				     size_t nonce_size, size_t *nonce_length)
{
	struct ironside_client_pack_iovec iov = {
		.op_handle = operation->handle,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_AEAD_GENERATE_NONCE_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
	};
	struct ironside_client_iovec out_vec[] = {
		{ .base = nonce, .len = nonce_size },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 1,
		.out_vec = out_vec,
		.out_len = 1,
	};

	psa_status_t status = ironside_client_call(&params);

	if (nonce_length != NULL) {
		*nonce_length = produced_output_length(status, &out_vec[0]);
	}
	return status;
}

psa_status_t psa_aead_set_nonce(psa_aead_operation_t *operation, const uint8_t *nonce,
				size_t nonce_length)
{
	struct ironside_client_pack_iovec iov = {
		.op_handle = operation->handle,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_AEAD_SET_NONCE_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
		{ .base = nonce, .len = nonce_length },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 2,
		.out_vec = NULL,
		.out_len = 0,
	};

	return ironside_client_call(&params);
}

psa_status_t psa_aead_update_ad(psa_aead_operation_t *operation, const uint8_t *input,
				size_t input_length)
{
	struct ironside_client_pack_iovec iov = {
		.op_handle = operation->handle,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_AEAD_UPDATE_AD_SID,
	};

	if (input == NULL && input_length != 0) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
		{ .base = input, .len = input_length },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = (input == NULL) ? 1u : 2u,
		.out_vec = NULL,
		.out_len = 0,
	};

	return ironside_client_call(&params);
}

psa_status_t psa_aead_update(psa_aead_operation_t *operation, const uint8_t *input,
			     size_t input_length, uint8_t *output, size_t output_size,
			     size_t *output_length)
{
	struct ironside_client_pack_iovec iov = {
		.op_handle = operation->handle,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_AEAD_UPDATE_SID,
	};

	if (input == NULL && input_length != 0) {
		if (output_length != NULL) {
			*output_length = 0U;
		}
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
		{ .base = input, .len = input_length },
	};
	struct ironside_client_iovec out_vec[] = {
		{ .base = output, .len = output_size },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = (input == NULL) ? 1u : 2u,
		.out_vec = out_vec,
		.out_len = 1,
	};

	psa_status_t status = ironside_client_call(&params);

	if (output_length != NULL) {
		*output_length = produced_output_length(status, &out_vec[0]);
	}
	return status;
}

psa_status_t psa_aead_finish(psa_aead_operation_t *operation, uint8_t *ciphertext,
			     size_t ciphertext_size, size_t *ciphertext_length, uint8_t *tag,
			     size_t tag_size, size_t *tag_length)
{
	struct ironside_client_pack_iovec iov = {
		.op_handle = operation->handle,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_AEAD_FINISH_SID,
	};

	if (ciphertext == NULL && ciphertext_size != 0) {
		if (ciphertext_length != NULL) {
			*ciphertext_length = 0U;
		}
		if (tag_length != NULL) {
			*tag_length = 0U;
		}
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
	};
	struct ironside_client_iovec out_vec[] = {
		{ .base = &operation->handle, .len = sizeof(operation->handle) },
		{ .base = tag, .len = tag_size },
		{ .base = ciphertext, .len = ciphertext_size },
	};
	size_t out_len = 3;

	if (ciphertext == NULL || ciphertext_size == 0) {
		out_len = 2;
	}
	if (out_len == 3 && ciphertext_length == NULL) {
		if (tag_length != NULL) {
			*tag_length = 0U;
		}
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 1,
		.out_vec = out_vec,
		.out_len = out_len,
	};

	psa_status_t status = ironside_client_call(&params);

	if (out_len == 3 && ciphertext_length != NULL) {
		*ciphertext_length = produced_output_length(status, &out_vec[2]);
	} else if (ciphertext_length != NULL) {
		*ciphertext_length = 0;
	}
	if (tag_length != NULL) {
		*tag_length = produced_output_length(status, &out_vec[1]);
	}
	return status;
}

psa_status_t psa_aead_verify(psa_aead_operation_t *operation, uint8_t *plaintext,
			     size_t plaintext_size, size_t *plaintext_length, const uint8_t *tag,
			     size_t tag_length)
{
	struct ironside_client_pack_iovec iov = {
		.op_handle = operation->handle,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_AEAD_VERIFY_SID,
	};

	if (plaintext == NULL && plaintext_size != 0) {
		if (plaintext_length != NULL) {
			*plaintext_length = 0U;
		}
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
		{ .base = tag, .len = tag_length },
	};
	struct ironside_client_iovec out_vec[] = {
		{ .base = &operation->handle, .len = sizeof(operation->handle) },
		{ .base = plaintext, .len = plaintext_size },
	};
	size_t out_len = 2;

	if (plaintext == NULL || plaintext_size == 0) {
		out_len = 1;
	}
	if (out_len == 2 && plaintext_length == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 2,
		.out_vec = out_vec,
		.out_len = out_len,
	};

	psa_status_t status = ironside_client_call(&params);

	if (out_len == 2 && plaintext_length != NULL) {
		*plaintext_length = produced_output_length(status, &out_vec[1]);
	} else if (plaintext_length != NULL) {
		*plaintext_length = 0;
	}
	return status;
}

psa_status_t psa_aead_abort(psa_aead_operation_t *operation)
{
	struct ironside_client_pack_iovec iov = {
		.op_handle = operation->handle,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_AEAD_ABORT_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
	};
	struct ironside_client_iovec out_vec[] = {
		{ .base = &operation->handle, .len = sizeof(operation->handle) },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 1,
		.out_vec = out_vec,
		.out_len = 1,
	};

	return ironside_client_call(&params);
}

psa_status_t psa_unwrap_key(const psa_key_attributes_t *attributes, psa_key_id_t wrapping_key,
			    psa_algorithm_t alg, const uint8_t *data, size_t data_length,
			    psa_key_id_t *key)
{
	struct ironside_client_pack_iovec iov = {
		.key_id = wrapping_key,
		.alg = alg,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_KEY_WRAPPING_UNWRAP_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
		{ .base = attributes, .len = sizeof(*attributes) },
		{ .base = data, .len = data_length },
	};
	struct ironside_client_iovec out_vec[] = {
		{ .base = key, .len = sizeof(*key) },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 3,
		.out_vec = out_vec,
		.out_len = 1,
	};
	psa_status_t status = ironside_client_call(&params);
	if (status != PSA_SUCCESS) {
		set_key_id_on_error(key);
	}

	return status;
}

psa_status_t psa_key_derivation_setup(psa_key_derivation_operation_t *operation,
				      psa_algorithm_t alg)
{
	struct ironside_client_pack_iovec iov = {
		.alg = alg,
		.op_handle = operation->handle,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_KEY_DERIVATION_SETUP_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
	};
	struct ironside_client_iovec out_vec[] = {
		{ .base = &operation->handle, .len = sizeof(operation->handle) },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 1,
		.out_vec = out_vec,
		.out_len = 1,
	};

	return ironside_client_call(&params);
}

psa_status_t psa_key_derivation_get_capacity(const psa_key_derivation_operation_t *operation,
					     size_t *capacity)
{
	struct ironside_client_pack_iovec iov = {
		.op_handle = operation->handle,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_KEY_DERIVATION_GET_CAPACITY_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
	};
	struct ironside_client_iovec out_vec[] = {
		{ .base = capacity, .len = sizeof(*capacity) },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 1,
		.out_vec = out_vec,
		.out_len = 1,
	};

	return ironside_client_call(&params);
}

psa_status_t psa_key_derivation_set_capacity(psa_key_derivation_operation_t *operation,
					     size_t capacity)
{
	struct ironside_client_pack_iovec iov = {
		.op_handle = operation->handle,
		.u.capacity = capacity,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_KEY_DERIVATION_SET_CAPACITY_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 1,
		.out_vec = NULL,
		.out_len = 0,
	};

	return ironside_client_call(&params);
}

psa_status_t psa_key_derivation_input_bytes(psa_key_derivation_operation_t *operation,
					    psa_key_derivation_step_t step, const uint8_t *data,
					    size_t data_length)
{
	struct ironside_client_pack_iovec iov = {
		.op_handle = operation->handle,
		.step = step,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_KEY_DERIVATION_INPUT_BYTES_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
		{ .base = data, .len = data_length },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 2,
		.out_vec = NULL,
		.out_len = 0,
	};

	return ironside_client_call(&params);
}

psa_status_t psa_key_derivation_input_integer(psa_key_derivation_operation_t *operation,
					      psa_key_derivation_step_t step, uint64_t value)
{
	struct ironside_client_pack_iovec iov = {
		.op_handle = operation->handle,
		.step = step,
		.u.value = value,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_KEY_DERIVATION_INPUT_INTEGER_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 1,
		.out_vec = NULL,
		.out_len = 0,
	};

	return ironside_client_call(&params);
}

psa_status_t psa_key_derivation_input_key(psa_key_derivation_operation_t *operation,
					  psa_key_derivation_step_t step, psa_key_id_t key)
{
	struct ironside_client_pack_iovec iov = {
		.key_id = key,
		.op_handle = operation->handle,
		.step = step,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_KEY_DERIVATION_INPUT_KEY_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 1,
		.out_vec = NULL,
		.out_len = 0,
	};

	return ironside_client_call(&params);
}

psa_status_t psa_key_derivation_output_bytes(psa_key_derivation_operation_t *operation,
					     uint8_t *output, size_t output_length)
{
	struct ironside_client_pack_iovec iov = {
		.op_handle = operation->handle,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_KEY_DERIVATION_OUTPUT_BYTES_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
	};
	struct ironside_client_iovec out_vec[] = {
		{ .base = output, .len = output_length },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 1,
		.out_vec = out_vec,
		.out_len = 1,
	};

	return ironside_client_call(&params);
}

psa_status_t psa_key_derivation_output_key(const psa_key_attributes_t *attributes,
					   psa_key_derivation_operation_t *operation,
					   psa_key_id_t *key)
{
	struct ironside_client_pack_iovec iov = {
		.op_handle = operation->handle,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_KEY_DERIVATION_OUTPUT_KEY_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
		{ .base = attributes, .len = sizeof(*attributes) },
	};
	struct ironside_client_iovec out_vec[] = {
		{ .base = key, .len = sizeof(*key) },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 2,
		.out_vec = out_vec,
		.out_len = 1,
	};
	psa_status_t status = ironside_client_call(&params);
	if (status != PSA_SUCCESS) {
		set_key_id_on_error(key);
	}

	return status;
}

psa_status_t psa_key_derivation_abort(psa_key_derivation_operation_t *operation)
{
	struct ironside_client_pack_iovec iov = {
		.op_handle = operation->handle,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_KEY_DERIVATION_ABORT_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
	};
	struct ironside_client_iovec out_vec[] = {
		{ .base = &operation->handle, .len = sizeof(operation->handle) },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 1,
		.out_vec = out_vec,
		.out_len = 1,
	};

	return ironside_client_call(&params);
}

psa_status_t psa_sign_message(psa_key_id_t key, psa_algorithm_t alg, const uint8_t *input,
			      size_t input_length, uint8_t *signature, size_t signature_size,
			      size_t *signature_length)
{
	struct ironside_client_pack_iovec iov = {
		.key_id = key,
		.alg = alg,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_SIGN_MESSAGE_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
		{ .base = input, .len = input_length },
	};
	struct ironside_client_iovec out_vec[] = {
		{ .base = signature, .len = signature_size },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 2,
		.out_vec = out_vec,
		.out_len = 1,
	};

	psa_status_t status = ironside_client_call(&params);

	if (signature_length != NULL) {
		*signature_length = produced_output_length(status, &out_vec[0]);
	}
	return status;
}

psa_status_t psa_verify_message(psa_key_id_t key, psa_algorithm_t alg, const uint8_t *input,
				size_t input_length, const uint8_t *signature,
				size_t signature_length)
{
	struct ironside_client_pack_iovec iov = {
		.key_id = key,
		.alg = alg,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_VERIFY_MESSAGE_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
		{ .base = input, .len = input_length },
		{ .base = signature, .len = signature_length },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 3,
		.out_vec = NULL,
		.out_len = 0,
	};

	return ironside_client_call(&params);
}

psa_status_t psa_sign_hash(psa_key_id_t key, psa_algorithm_t alg, const uint8_t *hash,
			   size_t hash_length, uint8_t *signature, size_t signature_size,
			   size_t *signature_length)
{
	struct ironside_client_pack_iovec iov = {
		.key_id = key,
		.alg = alg,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_SIGN_HASH_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
		{ .base = hash, .len = hash_length },
	};
	struct ironside_client_iovec out_vec[] = {
		{ .base = signature, .len = signature_size },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 2,
		.out_vec = out_vec,
		.out_len = 1,
	};

	psa_status_t status = ironside_client_call(&params);

	if (signature_length != NULL) {
		*signature_length = produced_output_length(status, &out_vec[0]);
	}
	return status;
}

psa_status_t psa_verify_hash(psa_key_id_t key, psa_algorithm_t alg, const uint8_t *hash,
			     size_t hash_length, const uint8_t *signature, size_t signature_length)
{
	struct ironside_client_pack_iovec iov = {
		.key_id = key,
		.alg = alg,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_VERIFY_HASH_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
		{ .base = hash, .len = hash_length },
		{ .base = signature, .len = signature_length },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 3,
		.out_vec = NULL,
		.out_len = 0,
	};

	return ironside_client_call(&params);
}

_Static_assert(sizeof(psa_pake_cipher_suite_t) == IRONSIDE_CLIENT_PSA_ABI_PAKE_CIPHER_SUITE_SIZE &&
		       _Alignof(psa_pake_cipher_suite_t) ==
			       IRONSIDE_CLIENT_PSA_ABI_PAKE_CIPHER_SUITE_ALIGN &&
		       offsetof(psa_pake_cipher_suite_t, algorithm) ==
			       IRONSIDE_CLIENT_PSA_ABI_PAKE_CIPHER_SUITE_ALGORITHM_OFFSET &&
		       offsetof(psa_pake_cipher_suite_t, type) ==
			       IRONSIDE_CLIENT_PSA_ABI_PAKE_CIPHER_SUITE_TYPE_OFFSET &&
		       offsetof(psa_pake_cipher_suite_t, family) ==
			       IRONSIDE_CLIENT_PSA_ABI_PAKE_CIPHER_SUITE_FAMILY_OFFSET &&
		       offsetof(psa_pake_cipher_suite_t, bits) ==
			       IRONSIDE_CLIENT_PSA_ABI_PAKE_CIPHER_SUITE_BITS_OFFSET &&
		       offsetof(psa_pake_cipher_suite_t, key_confirmation) ==
			       IRONSIDE_CLIENT_PSA_ABI_PAKE_CIPHER_SUITE_KEY_CONFIRMATION_OFFSET,
	       "PAKE cipher suite layout");

psa_status_t psa_pake_setup(psa_pake_operation_t *operation, psa_key_id_t password_key,
			    const psa_pake_cipher_suite_t *cipher_suite)
{
	struct ironside_client_pack_iovec iov = {
		.key_id = password_key,
		.alg = cipher_suite->algorithm,
		.op_handle = operation->handle,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_PAKE_SETUP_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
		{ .base = cipher_suite, .len = sizeof(*cipher_suite) },
	};
	struct ironside_client_iovec out_vec[] = {
		{ .base = &operation->handle, .len = sizeof(operation->handle) },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 2,
		.out_vec = out_vec,
		.out_len = 1,
	};

	return ironside_client_call(&params);
}

psa_status_t psa_pake_set_role(psa_pake_operation_t *operation, psa_pake_role_t role)
{
	struct ironside_client_pack_iovec iov = {
		.op_handle = operation->handle,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_PAKE_SET_ROLE_SID,
		.role = role,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 1,
	};

	return ironside_client_call(&params);
}

psa_status_t psa_pake_set_user(psa_pake_operation_t *operation, const uint8_t *user_id,
			       size_t user_id_len)
{
	struct ironside_client_pack_iovec iov = {
		.op_handle = operation->handle,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_PAKE_SET_USER_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
		{ .base = user_id, .len = user_id_len },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 2,
	};

	return ironside_client_call(&params);
}

psa_status_t psa_pake_set_peer(psa_pake_operation_t *operation, const uint8_t *peer_id,
			       size_t peer_id_len)
{
	struct ironside_client_pack_iovec iov = {
		.op_handle = operation->handle,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_PAKE_SET_PEER_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
		{ .base = peer_id, .len = peer_id_len },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 2,
	};

	return ironside_client_call(&params);
}

psa_status_t psa_pake_set_context(psa_pake_operation_t *operation, const uint8_t *context,
				  size_t context_len)
{
	struct ironside_client_pack_iovec iov = {
		.op_handle = operation->handle,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_PAKE_SET_CONTEXT_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
		{ .base = context, .len = context_len },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 2,
	};

	return ironside_client_call(&params);
}

psa_status_t psa_pake_input(psa_pake_operation_t *operation, psa_pake_step_t step,
			    const uint8_t *input, size_t input_length)
{
	struct ironside_client_pack_iovec iov = {
		.op_handle = operation->handle,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_PAKE_INPUT_SID,
		.step = step,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
		{ .base = input, .len = input_length },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 2,
	};

	return ironside_client_call(&params);
}

psa_status_t psa_pake_output(psa_pake_operation_t *operation, psa_pake_step_t step, uint8_t *output,
			     size_t output_size, size_t *output_length)
{
	struct ironside_client_pack_iovec iov = {
		.op_handle = operation->handle,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_PAKE_OUTPUT_SID,
		.step = step,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
	};
	struct ironside_client_iovec out_vec[] = {
		{ .base = output, .len = output_size },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 1,
		.out_vec = out_vec,
		.out_len = 1,
	};

	psa_status_t status = ironside_client_call(&params);

	if (output_length != NULL) {
		*output_length = produced_output_length(status, &out_vec[0]);
	}
	return status;
}

psa_status_t psa_pake_get_shared_key(psa_pake_operation_t *operation,
				     const psa_key_attributes_t *attributes, psa_key_id_t *key)
{
	struct ironside_client_pack_iovec iov = {
		.op_handle = operation->handle,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_PAKE_GET_SHARED_KEY_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
		{ .base = attributes, .len = sizeof(*attributes) },
	};
	struct ironside_client_iovec out_vec[] = {
		{ .base = &operation->handle, .len = sizeof(operation->handle) },
		{ .base = key, .len = sizeof(*key) },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 2,
		.out_vec = out_vec,
		.out_len = 2,
	};

	return ironside_client_call(&params);
}

psa_status_t psa_pake_abort(psa_pake_operation_t *operation)
{
	struct ironside_client_pack_iovec iov = {
		.op_handle = operation->handle,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_PAKE_ABORT_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
	};
	struct ironside_client_iovec out_vec[] = {
		{ .base = &operation->handle, .len = sizeof(operation->handle) },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 1,
		.out_vec = out_vec,
		.out_len = 1,
	};

	return ironside_client_call(&params);
}

psa_status_t psa_raw_key_agreement(psa_algorithm_t alg, psa_key_id_t private_key,
				   const uint8_t *peer_key, size_t peer_key_length, uint8_t *output,
				   size_t output_size, size_t *output_length)
{
	struct ironside_client_pack_iovec iov = {
		.key_id = private_key,
		.alg = alg,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_RAW_KEY_AGREEMENT_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
		{ .base = peer_key, .len = peer_key_length },
	};
	struct ironside_client_iovec out_vec[] = {
		{ .base = output, .len = output_size },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 2,
		.out_vec = out_vec,
		.out_len = 1,
	};

	psa_status_t status = ironside_client_call(&params);

	if (output_length != NULL) {
		*output_length = produced_output_length(status, &out_vec[0]);
	}
	return status;
}

psa_status_t psa_key_derivation_key_agreement(psa_key_derivation_operation_t *operation,
					      psa_key_derivation_step_t step,
					      psa_key_id_t private_key, const uint8_t *peer_key,
					      size_t peer_key_length)
{
	struct ironside_client_pack_iovec iov = {
		.key_id = private_key,
		.op_handle = operation->handle,
		.step = step,
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_KEY_DERIVATION_KEY_AGREEMENT_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
		{ .base = peer_key, .len = peer_key_length },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 2,
		.out_vec = NULL,
		.out_len = 0,
	};

	return ironside_client_call(&params);
}

psa_status_t psa_generate_random(uint8_t *output, size_t output_size)
{
	struct ironside_client_pack_iovec iov = {
		.function_id = IRONSIDE_CLIENT_PSA_CRYPTO_GENERATE_RANDOM_SID,
	};
	struct ironside_client_iovec in_vec[] = {
		{ .base = &iov, .len = sizeof(iov) },
	};
	struct ironside_client_iovec out_vec[] = {
		{ .base = output, .len = output_size },
	};
	struct ironside_client_call_params params = {
		.handle = IRONSIDE_CLIENT_PSA_CRYPTO_HANDLE,
		.in_vec = in_vec,
		.in_len = 1,
		.out_vec = out_vec,
		.out_len = 1,
	};

	return ironside_client_call(&params);
}
