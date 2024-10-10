/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @brief File containing promiscuous mode
 * support functions for the FMAC IF Layer
 * of the Wi-Fi driver.
 */

#include "osal_api.h"
#include "fmac_structs.h"
#include "fmac_promisc.h"

bool nrf_wifi_util_check_filt_setting(struct nrf_wifi_fmac_vif_ctx *vif,
				      unsigned short *frame_control)
{
	bool filter_check = false;
	struct nrf_wifi_fmac_frame_ctrl *frm_ctrl =
	(struct nrf_wifi_fmac_frame_ctrl *)frame_control;

	/**
	 * The different filter settings for 802.11 packets within the driver
	 * is a bit map. The description of the different valid values for
	 * the case statements are as follows:
	 * 0x2 - Enable management packets only
	 * 0x4 - Enable data packets only
	 * 0x8 - Enable control packets only
	 * 0x6 - Enable data and management packets
	 * 0xa - Enable control and management packets
	 * 0xc - Enable data and control packets
	 * The bit map setting is checked against the packet type field and
	 * the relevant filtered packet is sent up the stack.  if bit 0 is set
	 * in the driver bitmap, all packet types are allowed to be sent upstack.
	 **/
	switch (vif->packet_filter) {
		case 0x2:
			if (frm_ctrl->type == NRF_WIFI_MGMT_PKT_TYPE) {
				filter_check = true;
			}
			break;
		case 0x4:
			if (frm_ctrl->type == NRF_WIFI_DATA_PKT_TYPE) {
				filter_check = true;
			}
			break;
		case 0x6:
			if ((frm_ctrl->type == NRF_WIFI_DATA_PKT_TYPE) ||
			    (frm_ctrl->type == NRF_WIFI_MGMT_PKT_TYPE)) {
				filter_check = true;
			}
			break;
		case 0x8:
			if (frm_ctrl->type == NRF_WIFI_CTRL_PKT_TYPE) {
				filter_check = true;
			}
			break;
		case 0xa:
			if ((frm_ctrl->type == NRF_WIFI_CTRL_PKT_TYPE) ||
			    (frm_ctrl->type == NRF_WIFI_MGMT_PKT_TYPE)) {
				filter_check = true;
			}
			break;
		case 0xc:
			if ((frm_ctrl->type == NRF_WIFI_DATA_PKT_TYPE) ||
			    (frm_ctrl->type == NRF_WIFI_CTRL_PKT_TYPE)) {
				filter_check = true;
			}
			break;
		/* all other packet_filter cases - bit 0 is set and hence all
		 * packet types are allowed or in the case of 0xE - all packet
		 * type bits are enabled */
		default:
			filter_check = true;
			break;
	}
	return filter_check;
}
