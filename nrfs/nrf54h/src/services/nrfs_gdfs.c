/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <internal/nrfs_backend.h>
#include <internal/nrfs_callbacks.h>
#include <nrfs_gdfs.h>

typedef struct {
	nrfs_gdfs_evt_handler_t handler;
	bool is_initialized;
} nrfs_gdfs_cb_t;
static nrfs_gdfs_cb_t m_cb;

void nrfs_gdfs_service_notify(void *p_notification, size_t size)
{
	if (!m_cb.handler || !m_cb.is_initialized) {
		return;
	}

	nrfs_gdfs_evt_t evt;
	nrfs_generic_t *p_data = (nrfs_generic_t *)p_notification;
	if (NRFS_HDR_FILTER_ERR_GET(&p_data->hdr)) {
		evt.type = NRFS_GDFS_EVT_REJECT;
		m_cb.handler(&evt, (void *)p_data->ctx.ctx);
		return;
	}

	nrfs_gdfs_rsp_t *p_rsp = (nrfs_gdfs_rsp_t *)p_notification;
	switch (p_data->hdr.req) {
	case NRFS_GDFS_REQ_FREQ:
		evt.type = NRFS_GDFS_EVT_FREQ_CONFIRMED;
		m_cb.handler(&evt, (void *)p_rsp->ctx.ctx);
		break;

	default:
		break;
	}
}

nrfs_err_t nrfs_gdfs_init(nrfs_gdfs_evt_handler_t handler)
{
	if (m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	m_cb.handler	    = handler;
	m_cb.is_initialized = true;
	return NRFS_SUCCESS;
}

void nrfs_gdfs_uninit(void)
{
	m_cb.is_initialized = false;
}

nrfs_err_t nrfs_gdfs_request_freq(enum gdfs_frequency_setting target_freq, void *p_context)
{
	if (!m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	nrfs_gdfs_req_t req;

	NRFS_SERVICE_HDR_FILL(&req, NRFS_GDFS_REQ_FREQ);
	req.ctx.ctx	     = (uint32_t)p_context;
	req.data.target_freq = target_freq;

	return nrfs_backend_send(&req, sizeof(req));
}

nrfs_err_t nrfs_gdfs_request_freq_no_rsp(enum gdfs_frequency_setting target_freq,
					    void *p_context)
{
	if (!m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	nrfs_gdfs_req_t req;

	NRFS_SERVICE_HDR_FILL(&req, NRFS_GDFS_REQ_FREQ);
	NRFS_HDR_NO_RSP_SET(&req.hdr);
	req.ctx.ctx	     = (uint32_t)p_context;
	req.data.target_freq = target_freq;

	return nrfs_backend_send(&req, sizeof(req));
}
