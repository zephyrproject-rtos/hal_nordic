/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <internal/nrfs_backend.h>
#include <internal/nrfs_callbacks.h>
#include <nrfs_reset.h>

typedef struct {
	nrfs_reset_evt_handler_t handler;
	bool is_initialized;
} nrfs_reset_cb_t;
static nrfs_reset_cb_t m_cb;

void nrfs_reset_service_notify(void *p_notification, size_t size)
{
	if (!m_cb.handler || !m_cb.is_initialized) {
		return;
	}

	nrfs_reset_evt_t evt;
	nrfs_generic_t *p_data = (nrfs_generic_t *)p_notification;
	if (NRFS_HDR_FILTER_ERR_GET(&p_data->hdr)) {
		evt.type = NRFS_RESET_EVT_REJECT;
		m_cb.handler(&evt);
		return;
	}

	switch (p_data->hdr.req) {
	case NRFS_RESET_REQ:
		evt.type = NRFS_RESET_EVT_DONE;
		m_cb.handler(&evt);
		break;

	default:
		break;
	}
}

nrfs_err_t nrfs_reset_init(nrfs_reset_evt_handler_t handler)
{
	if (m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	m_cb.handler	    = handler;
	m_cb.is_initialized = true;
	return NRFS_SUCCESS;
}

void nrfs_reset_uninit(void)
{
	m_cb.is_initialized = false;
}

nrfs_err_t nrfs_request_reset(void)
{
	nrfs_reset_req_t req;

	NRFS_SERVICE_HDR_FILL(&req, NRFS_RESET_REQ);
	req.ctx.ctx = (uint32_t)0;

	return nrfs_backend_send(&req, sizeof(req));
}
