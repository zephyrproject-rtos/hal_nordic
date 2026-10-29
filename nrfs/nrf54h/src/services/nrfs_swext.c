/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <internal/nrfs_backend.h>
#include <internal/nrfs_callbacks.h>
#include <nrfs_swext.h>

typedef struct {
	nrfs_swext_evt_handler_t handler;
	bool is_initialized;
} nrfs_swext_cb_t;
static nrfs_swext_cb_t m_cb;

nrfs_err_t nrfs_swext_init(nrfs_swext_evt_handler_t handler)
{
	if (m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	m_cb.handler = handler;
	m_cb.is_initialized = true;
	return NRFS_SUCCESS;
}

void nrfs_swext_uninit(void)
{
	m_cb.is_initialized = false;
}

void nrfs_swext_service_notify(void *p_notification, size_t size)
{
	if (!m_cb.handler || !m_cb.is_initialized) {
		return;
	}

	nrfs_swext_evt_t evt;
	nrfs_generic_t *p_data = (nrfs_generic_t *)p_notification;

	if (NRFS_HDR_FILTER_ERR_GET(&p_data->hdr)) {
		evt.type = NRFS_SWEXT_EVT_REJECTED;
		m_cb.handler(&evt, (void *)p_data->ctx.ctx);
		return;
	}

	nrfs_swext_rsp_t *p_rsp = (nrfs_swext_rsp_t *)p_notification;

	switch(p_rsp->status){
	case SWEXT_OUTPUT_ENABLED:
		evt.type = NRFS_SWEXT_EVT_ENABLED;
		break;
	case SWEXT_OVERCURRENT:
		evt.type = NRFS_SWEXT_EVT_OVERCURRENT;
		break;
	}
	m_cb.handler(&evt, (void *)p_data->ctx.ctx);
}

nrfs_err_t nrfs_swext_power_up(uint8_t load_current, void *p_context)
{
	if (!m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	nrfs_swext_req_t req;

	NRFS_SERVICE_HDR_FILL(&req, NRFS_SWEXT_REQ_POWER_UP);
	req.ctx.ctx = (uint32_t)p_context;
	req.data.load_current = load_current;
	return nrfs_backend_send(&req, sizeof(req));
}

nrfs_err_t nrfs_swext_power_down(swext_pd_clamp_t pd_clamp, void *p_context)
{
	if (!m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	nrfs_swext_req_t req;

	NRFS_SERVICE_HDR_FILL(&req, NRFS_SWEXT_REQ_POWER_DOWN);
	req.ctx.ctx = (uint32_t)p_context;
	req.data.pd_clamp_control = pd_clamp;
	return nrfs_backend_send(&req, sizeof(req));
}
