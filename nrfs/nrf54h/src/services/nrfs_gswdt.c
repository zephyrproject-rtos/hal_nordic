/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <internal/nrfs_backend.h>
#include <internal/nrfs_callbacks.h>
#include <nrfs_gswdt.h>

typedef struct {
	nrfs_gswdt_evt_handler_t handler;
	bool is_initialized;
} nrfs_gswdt_cb_t;
static nrfs_gswdt_cb_t m_cb;

void nrfs_gswdt_service_notify(void *p_notification, size_t size)
{
	if (!m_cb.handler || !m_cb.is_initialized) {
		return;
	}

	nrfs_gswdt_evt_t evt;
	nrfs_generic_t *p_data = (nrfs_generic_t *)p_notification;

	if (NRFS_HDR_FILTER_ERR_GET(&p_data->hdr)) {
		evt.type = NRFS_GSWDT_EVT_REJECT;
		m_cb.handler(&evt, (void *)p_data->ctx.ctx);
		return;
	}

	nrfs_gswdt_rsp_t *p_rsp = (nrfs_gswdt_rsp_t *)p_notification;

	switch (p_data->hdr.req) {
	case NRFS_GSWDT_TIMEOUT_SET_REQ:
		evt.type = NRFS_GSWDT_EVT_TIMEOUT_SET;
		m_cb.handler(&evt, (void *)p_rsp->ctx.ctx);
		break;
	case NRFS_GSWDT_STOP_REQ:
		evt.type = NRFS_GSWDT_EVT_STOP_DONE;
		m_cb.handler(&evt, (void *)p_rsp->ctx.ctx);
		break;
	default:
		break;
	}
}

nrfs_err_t nrfs_gswdt_init(nrfs_gswdt_evt_handler_t handler)
{
	if (m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	m_cb.handler = handler;
	m_cb.is_initialized = true;
	return NRFS_SUCCESS;
}

void nrfs_gswdt_uninit(void)
{
	m_cb.is_initialized = false;
}

nrfs_err_t nrfs_gswdt_timeout_set(uint32_t timeout_ms, void *p_context)
{
	if (!m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	nrfs_gswdt_timeout_set_req_t req;

	NRFS_SERVICE_HDR_FILL(&req, NRFS_GSWDT_TIMEOUT_SET_REQ);
	req.ctx.ctx = (uint32_t)p_context;
	req.timeout_ms = timeout_ms;

	return nrfs_backend_send(&req, sizeof(req));
}

nrfs_err_t nrfs_gswdt_stop(void *p_context)
{
	if (!m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	nrfs_gswdt_stop_req_t req;

	NRFS_SERVICE_HDR_FILL(&req, NRFS_GSWDT_STOP_REQ);
	req.ctx.ctx = (uint32_t)p_context;

	return nrfs_backend_send(&req, sizeof(req));
}
