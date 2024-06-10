/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef EPERIPHS_BACKEND_H
#define EPERIPHS_BACKEND_H

#include "eperiph_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief FLPR EGPIO opcodes. */
typedef enum
{
    NRF_ICMG_EGPIO_PIN_CLEAR    = 0, ///< Clear eGPIO pin.
    NRF_ICMG_EGPIO_PIN_SET      = 1, ///< Set eGPIO pin.
    NRF_ICMG_EGPIO_PIN_TOGGLE   = 2, ///< Toggle eGPIO pin.
} nrf_icmg_egpio_opcode_t;

/** @brief EGPIO data packet. */
struct data_packet {
	unsigned int opcode;	/* Config type (nrf_icmg_config_t) in configuration step, operation to perform (nrf_icmg_egpio_opcode_t) in non-configure step*/
	unsigned int pin;		/* Pin number. */
	unsigned long flags;	/* Configuration flags in configure step (gpio_flags_t), not used in other cases. */
};

K_SEM_DEFINE(bound_sem, 0, 1);


static void ep_bound(void *priv)
{
	k_sem_give(&bound_sem);
	LOG_INF("Ep bounded");
}

static void ep_recv(const void *data, size_t len, void *priv)
{
}

static struct ipc_ept_cfg ep_cfg = {
	.cb = {
		.bound    = ep_bound,
		.received = ep_recv,
	},
};

struct ipc_ept ep;

int eperiphs_init(void)
{
	int ret;

	const struct device *ipc0_instance = DEVICE_DT_GET(DT_NODELABEL(ipc0));

	ret = ipc_service_open_instance(ipc0_instance);
	if ((ret < 0) && (ret != -EALREADY)) {
		LOG_ERR("ipc_service_open_instance() failure");
		return ret;
	}

	ret = ipc_service_register_endpoint(ipc0_instance, &ep, &ep_cfg);
	if (ret < 0) {
		LOG_ERR("ipc_service_register_endpoint() failure");
		return ret;
	}

	k_sem_take(&bound_sem, K_FOREVER);

	return 0;
}

int eperiphs_send(struct data_packet *message, size_t size)
{
	return ipc_service_send(ep, (void*)msg, size);
}

#ifdef __cplusplus
}
#endif

#endif /* EPERIPHS_BACKEND_H */