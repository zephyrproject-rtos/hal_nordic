/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <internal/nrfs_backend.h>
#include <internal/nrfs_callbacks.h>
#include <nrfs_clock.h>

typedef struct {
	nrfs_clock_evt_handler_t handler;
	bool is_initialized;
} nrfs_clock_cb_t;
static nrfs_clock_cb_t m_cb;

void nrfs_clock_service_notify(void *p_notification, size_t size)
{
	if (!m_cb.handler || !m_cb.is_initialized) {
		return;
	}

	nrfs_clock_evt_t evt;
	nrfs_generic_t *p_data = (nrfs_generic_t *)p_notification;

	if (NRFS_HDR_FILTER_ERR_GET(&p_data->hdr)) {
		evt.type = NRFS_CLOCK_EVT_REJECT;
		m_cb.handler(&evt, (void *)p_data->ctx.ctx);
		return;
	}

	nrfs_clock_rsp_t *p_rsp = (nrfs_clock_rsp_t *)p_notification;

	evt.data		= p_rsp->data;
	switch (p_data->hdr.req) {
	case NRFS_CLOCK_REQ_SUBSCRIBE:
		evt.type = NRFS_CLOCK_EVT_CHANGE;
		m_cb.handler(&evt, (void *)p_rsp->ctx.ctx);
		break;

	case NRFS_CLOCK_REQ_LFCLK_SRC:
	case NRFS_CLOCK_REQ_HSFLL_MODE:
		evt.type = NRFS_CLOCK_EVT_APPLIED;
		m_cb.handler(&evt, (void *)p_rsp->ctx.ctx);
		break;

	default:
		break;
	}
}

nrfs_err_t nrfs_clock_init(nrfs_clock_evt_handler_t handler)
{
	if (m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	m_cb.handler	    = handler;
	m_cb.is_initialized = true;
	return NRFS_SUCCESS;
}

void nrfs_clock_uninit(void)
{
	m_cb.is_initialized = false;
}

nrfs_err_t nrfs_clock_subscribe(uint8_t event_mask, void *p_context)
{
	if (!m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	nrfs_clock_subscribe_t req;

	NRFS_SERVICE_HDR_FILL(&req, NRFS_CLOCK_REQ_SUBSCRIBE);
	req.ctx.ctx    = (uint32_t)p_context;
	req.event_mask = event_mask;

	return nrfs_backend_send(&req, sizeof(req));
}

nrfs_err_t nrfs_clock_unsubscribe(void)
{
	if (!m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	nrfs_clock_subscribe_t req;

	NRFS_SERVICE_HDR_FILL(&req, NRFS_CLOCK_REQ_UNSUBSCRIBE);

	return nrfs_backend_send(&req, sizeof(req));
}

nrfs_err_t nrfs_clock_lfclk_src_set(nrfs_clock_src_t src, void *p_context)
{
	if (!m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	nrfs_clock_lfclk_src_req_t req;

	NRFS_SERVICE_HDR_FILL(&req, NRFS_CLOCK_REQ_LFCLK_SRC);
	req.src	    = src;
	req.ctx.ctx = (uint32_t)p_context;

	return nrfs_backend_send(&req, sizeof(req));
}

nrfs_err_t nrfs_clock_lfclk_src_set_no_rsp(nrfs_clock_src_t src, void *p_context)
{
	if (!m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	nrfs_clock_lfclk_src_req_t req;

	NRFS_SERVICE_HDR_FILL(&req, NRFS_CLOCK_REQ_LFCLK_SRC);
	NRFS_HDR_NO_RSP_SET(&req.hdr);
	req.src	    = src;
	req.ctx.ctx = (uint32_t)p_context;

	return nrfs_backend_send(&req, sizeof(req));
}

nrfs_err_t nrfs_clock_hsfll_mode_set(const nrfs_clock_hsfll_mode_t mode, void * p_context)
{
	if (!m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	nrfs_clock_hsfll_mode_req_t req;

	NRFS_SERVICE_HDR_FILL(&req, NRFS_CLOCK_REQ_HSFLL_MODE);
	req.mode = mode;
	req.ctx.ctx = (uint32_t)p_context;

	return nrfs_backend_send(&req, sizeof(req));
}

nrfs_err_t nrfs_clock_hsfll_mode_set_no_rsp(const nrfs_clock_hsfll_mode_t mode, void * p_context)
{
	if (!m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	nrfs_clock_hsfll_mode_req_t req;

	NRFS_SERVICE_HDR_FILL(&req, NRFS_CLOCK_REQ_HSFLL_MODE);
	NRFS_HDR_NO_RSP_SET(&req.hdr);
	req.mode = mode;
	req.ctx.ctx = (uint32_t)p_context;

	return nrfs_backend_send(&req, sizeof(req));
}
