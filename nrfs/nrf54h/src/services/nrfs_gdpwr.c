/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <internal/nrfs_backend.h>
#include <internal/nrfs_callbacks.h>
#include <nrfs_gdpwr.h>

typedef struct {
	nrfs_gdpwr_evt_handler_t handler;
	bool is_initialized;
} nrfs_gdpwr_cb_t;
static nrfs_gdpwr_cb_t m_cb;

nrfs_err_t nrfs_gdpwr_init(nrfs_gdpwr_evt_handler_t handler)
{
	if (m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	m_cb.handler = handler;
	m_cb.is_initialized = true;
	return NRFS_SUCCESS;
}

void nrfs_gdpwr_uninit(void)
{
	m_cb.is_initialized = false;
}

void nrfs_gdpwr_service_notify(void *p_notification, size_t size)
{
	if (!m_cb.handler || !m_cb.is_initialized) {
		return;
	}

	nrfs_gdpwr_evt_t evt;
	nrfs_generic_t *p_data = (nrfs_generic_t *)p_notification;

	if (NRFS_HDR_FILTER_ERR_GET(&p_data->hdr)) {
		evt.type = NRFS_GDPWR_REQ_REJECTED;
		m_cb.handler(&evt, (void *)p_data->ctx.ctx);
		return;
	}

	evt.type = NRFS_GDPWR_REQ_APPLIED;
	m_cb.handler(&evt, (void *)p_data->ctx.ctx);
}

nrfs_err_t nrfs_gdpwr_power_request(gdpwr_power_domain_t power_domain,
				    gdpwr_request_type_t req_type,
				    void *p_context)
{
	if (!m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	nrfs_gdpwr_req_t req;

	NRFS_SERVICE_HDR_FILL(&req, NRFS_GDPWR_REQ_SET_POWER_REQUEST);
	req.ctx.ctx = (uint32_t)p_context;
	req.data.power_domain = power_domain;
	req.data.request_type = req_type;
	return nrfs_backend_send(&req, sizeof(req));
}
