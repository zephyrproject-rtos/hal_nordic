/*
 * Copyright (c) 2020, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>

#include "nrf_802154.h"
#include "nrf_802154_serialization.h"

static int serialization_init(const struct device *dev)
{
	/* On NET core we don't use Zephyr's shim layer so we have to call inits manually */
	nrf_802154_init();

	nrf_802154_serialization_init();

	return 0;
}

/* 
 * TODO: We should set it to CONFIG_IEEE802154_NRF5_INIT_PRIO but we need to compile net app with
 * real radio driver for this kconfig entry to be provided.
 * And we don't have working RD for Yoda right now.
 */
SYS_INIT(serialization_init, POST_KERNEL, CONFIG_NRF_802154_SER_RADIO_INIT_PRIO);
