/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <internal/nrfs_backend.h>
#include <internal/nrfs_callbacks.h>
#include <nrfs_audiopll.h>

typedef struct {
	nrfs_audiopll_evt_handler_t handler;
	bool is_initialized;
} nrfs_audiopll_cb_t;
static nrfs_audiopll_cb_t m_cb;

void nrfs_audiopll_service_notify(void *p_notification, size_t size)
{
	if (!m_cb.handler || !m_cb.is_initialized) {
		return;
	}

	nrfs_audiopll_evt_t evt;
	nrfs_generic_t *p_data = (nrfs_generic_t *)p_notification;

	if (NRFS_HDR_FILTER_ERR_GET(&p_data->hdr)) {
		evt.type = NRFS_AUDIOPLL_EVT_REJECT;
		m_cb.handler(&evt, (void *)p_data->ctx.ctx);
		return;
	}

	switch (p_data->hdr.req) {
	case NRFS_AUDIOPLL_REQ_ENABLE:
		evt.type	  = NRFS_AUDIOPLL_EVT_ENABLED;
		m_cb.handler(&evt, (void *)p_data->ctx.ctx);
		break;
	case NRFS_AUDIOPLL_REQ_DISABLE:
		evt.type	  = NRFS_AUDIOPLL_EVT_DISABLED;
		m_cb.handler(&evt, (void *)p_data->ctx.ctx);
		break;
	case NRFS_AUDIOPLL_REQ_FREQ:
		evt.type	  = NRFS_AUDIOPLL_EVT_FREQ_CONFIRMED;
		m_cb.handler(&evt, (void *)p_data->ctx.ctx);
		break;
	case NRFS_AUDIOPLL_REQ_PRESCALER:
		evt.type	  = NRFS_AUDIOPLL_EVT_PRESCALER_CONFIRMED;
		m_cb.handler(&evt, (void *)p_data->ctx.ctx);
		break;
	case NRFS_AUDIOPLL_REQ_FREQ_INC:
		evt.type	  = NRFS_AUDIOPLL_EVT_FREQ_INC_CONFIRMED;
		m_cb.handler(&evt, (void *)p_data->ctx.ctx);
		break;
	default:
		break;
	}
}

nrfs_err_t nrfs_audiopll_init(nrfs_audiopll_evt_handler_t handler)
{
	if (m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	m_cb.handler	    = handler;
	m_cb.is_initialized = true;
	return NRFS_SUCCESS;
}

void nrfs_audiopll_uninit(void)
{
	m_cb.is_initialized = false;
}

nrfs_err_t nrfs_audiopll_enable_request(void *p_context)
{
	if (!m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	nrfs_audiopll_enable_req_t req;

	NRFS_SERVICE_HDR_FILL(&req, NRFS_AUDIOPLL_REQ_ENABLE);
	req.ctx.ctx = (uint32_t)p_context;

	return nrfs_backend_send(&req, sizeof(req));
}

nrfs_err_t nrfs_audiopll_disable_request(void *p_context)
{
	if (!m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	nrfs_audiopll_disable_req_t req;

	NRFS_SERVICE_HDR_FILL(&req, NRFS_AUDIOPLL_REQ_DISABLE);
	req.ctx.ctx = (uint32_t)p_context;

	return nrfs_backend_send(&req, sizeof(req));
}

nrfs_err_t nrfs_audiopll_request_freq(uint16_t pll_freq_fraction, void *p_context)
{
	if (!m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	nrfs_audiopll_freq_req_t req;

	req.data.pll_freq_fraction = pll_freq_fraction;


	NRFS_SERVICE_HDR_FILL(&req, NRFS_AUDIOPLL_REQ_FREQ);
	req.ctx.ctx = (uint32_t)p_context;

	return nrfs_backend_send(&req, sizeof(req));
}

nrfs_err_t nrfs_audiopll_request_prescaler(enum audiopll_prescaler_div pll_div, void *p_context)
{
	if (!m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	nrfs_audiopll_prescaler_req_t req;

	req.data.pll_div = pll_div;


	NRFS_SERVICE_HDR_FILL(&req, NRFS_AUDIOPLL_REQ_PRESCALER);
	req.ctx.ctx = (uint32_t)p_context;

	return nrfs_backend_send(&req, sizeof(req));
}

nrfs_err_t nrfs_audiopll_request_freq_inc(int8_t inc_val, uint16_t inc_period,
	void *p_context)
{
	if (!m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	nrfs_audiopll_freq_inc_req_t req;

	req.data.freq_inc_period = inc_period;
	req.data.freq_inc_val = inc_val;

	NRFS_SERVICE_HDR_FILL(&req, NRFS_AUDIOPLL_REQ_FREQ_INC);
	req.ctx.ctx = (uint32_t)p_context;

	return nrfs_backend_send(&req, sizeof(req));
}
