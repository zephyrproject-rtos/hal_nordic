/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <internal/nrfs_dispatcher.h>
#include <internal/nrfs_callbacks.h>
#include <internal/services/nrfs_generic.h>
#include <nrfs_config.h>

static const nrfs_service_cb_t services_callbacks[] = {
#ifdef NRFS_TEMP_SERVICE_ENABLED
	[NRFS_SERVICE_ID_TEMP] = nrfs_temp_service_notify,
#else
	[NRFS_SERVICE_ID_TEMP]	= NULL,
#endif
#ifdef NRFS_MRAM_SERVICE_ENABLED
	[NRFS_SERVICE_ID_MRAM] = nrfs_mram_service_notify,
#else
	[NRFS_SERVICE_ID_MRAM]	= NULL,
#endif
#ifdef NRFS_RESET_SERVICE_ENABLED
	[NRFS_SERVICE_ID_RESET] = nrfs_reset_service_notify,
#else
	[NRFS_SERVICE_ID_RESET] = NULL,
#endif
#ifdef NRFS_VBUS_DETECTOR_SERVICE_ENABLED
	[NRFS_SERVICE_ID_USB] = nrfs_usb_service_notify,
#else
	[NRFS_SERVICE_ID_USB]	= NULL,
#endif
#ifdef NRFS_PMIC_SERVICE_ENABLED
	[NRFS_SERVICE_ID_PMIC] = nrfs_pmic_service_notify,
#else
	[NRFS_SERVICE_ID_PMIC]	= NULL,
#endif
#ifdef NRFS_DVFS_SERVICE_ENABLED
	[NRFS_SERVICE_ID_DVFS] = nrfs_dvfs_service_notify,
#else
	[NRFS_SERVICE_ID_DVFS]	= NULL,
#endif
#ifdef NRFS_DIAG_SERVICE_ENABLED
	[NRFS_SERVICE_ID_DIAG] = nrfs_diag_service_notify,
#else
	[NRFS_SERVICE_ID_DIAG]	= NULL,
#endif
#ifdef NRFS_CLOCK_SERVICE_ENABLED
	[NRFS_SERVICE_ID_CLOCK] = nrfs_clock_service_notify,
#else
	[NRFS_SERVICE_ID_CLOCK]	= NULL,
#endif
#ifdef NRFS_GDPWR_SERVICE_ENABLED
	[NRFS_SERVICE_ID_GDPWR] = nrfs_gdpwr_service_notify,
#else
	[NRFS_SERVICE_ID_GDPWR]	= NULL,
#endif
#ifdef NRFS_GDFS_SERVICE_ENABLED
	[NRFS_SERVICE_ID_GDFS] = nrfs_gdfs_service_notify,
#else
	[NRFS_SERVICE_ID_GDFS]	= NULL,
#endif
#ifdef NRFS_SWEXT_SERVICE_ENABLED
	[NRFS_SERVICE_ID_SWEXT] = nrfs_swext_service_notify,
#else
	[NRFS_SERVICE_ID_SWEXT]	= NULL,
#endif
#ifdef NRFS_AUDIOPLL_SERVICE_ENABLED
	[NRFS_SERVICE_ID_AUDIOPLL] = nrfs_audiopll_service_notify,
#else
	[NRFS_SERVICE_ID_AUDIOPLL] = NULL,
#endif
};

/* Warning! All "UNSOLICITED" features are not supported. This is intended for possible future use. */
void __attribute__((weak)) nrfs_unsolicited_handler(void *p_buffer, size_t size)
{
	(void)p_buffer;
	(void)size;
}

void nrfs_dispatcher_notify(void *p_notification, size_t size)
{
	nrfs_hdr_t *p_hdr = (nrfs_hdr_t *)p_notification;

	if (NRFS_HDR_UNSOLICITED_GET(p_hdr)) {
		void *p_buffer = ((nrfs_generic_t *)p_notification)->payload;

		nrfs_unsolicited_handler(p_buffer, size - sizeof(nrfs_generic_t));
	} else {
		uint8_t srv_id = NRFS_SERVICE_ID_GET(p_hdr->req);

		if ((srv_id < NRFS_ARRAY_SIZE(services_callbacks)) &&
		    (services_callbacks[srv_id] != NULL)) {
			services_callbacks[srv_id](p_notification, size);
		} else {
			/* TODO: error! unknown service */
		}
	}
}
