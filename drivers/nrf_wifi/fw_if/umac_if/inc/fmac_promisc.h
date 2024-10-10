/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @brief Header containing code to support promiscuous mode
 * for the FMAC IF Layer of the Wi-Fi driver.
 */
#ifndef __FMAC_PROMISC_H__
#define __FMAC_PROMISC_H__
#include "fmac_structs.h"
#include "pack_def.h"

enum nrf_wifi_fmac_frame_type {
	/** 802.11 management packet type */
	NRF_WIFI_MGMT_PKT_TYPE = 0x0,
	/** 802.11 control packet type */
	NRF_WIFI_CTRL_PKT_TYPE,
	/** 802.11 data packet type */
	NRF_WIFI_DATA_PKT_TYPE,
};

/* Frame Control structure */
struct nrf_wifi_fmac_frame_ctrl {
	unsigned short protocolVersion : 2;
	unsigned short type            : 2;
	unsigned short subtype         : 4;
	unsigned short toDS            : 1;
	unsigned short fromDS          : 1;
	unsigned short moreFragments   : 1;
	unsigned short retry           : 1;
	unsigned short powerManagement : 1;
	unsigned short moreData        : 1;
	unsigned short protectedFrame  : 1;
	unsigned short order           : 1;
} __NRF_WIFI_PKD;

bool nrf_wifi_util_check_filt_setting(struct nrf_wifi_fmac_vif_ctx *vif,
				      unsigned short *frame_control);
#endif /* __FMAC_PROMISC_H__ */
