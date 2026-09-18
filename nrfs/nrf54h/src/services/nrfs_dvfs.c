/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <internal/nrfs_backend.h>
#include <internal/nrfs_callbacks.h>
#include <nrfs_dvfs.h>

typedef struct {
	nrfs_dvfs_evt_handler_t handler;
	bool is_initialized;
} nrfs_dvfs_cb_t;
static nrfs_dvfs_cb_t m_cb;

void nrfs_dvfs_service_notify(void *p_notification, size_t size)
{
	if (!m_cb.handler || !m_cb.is_initialized) {
		return;
	}

	nrfs_dvfs_evt_t evt;
	nrfs_generic_t *p_data = (nrfs_generic_t *)p_notification;
	if (NRFS_HDR_FILTER_ERR_GET(&p_data->hdr)) {
		evt.type = NRFS_DVFS_EVT_REJECT;
		m_cb.handler(&evt, (void *)p_data->ctx.ctx);
		return;
	}

	nrfs_dvfs_rsp_t *p_rsp = (nrfs_dvfs_rsp_t *)p_notification;
	evt.freq	       = p_rsp->data.freq;
	switch (p_data->hdr.req) {
	case NRFS_DVFS_REQ_INIT_PREPARE:
		evt.type = NRFS_DVFS_EVT_INIT_PREPARATION;
		m_cb.handler(&evt, (void *)p_rsp->ctx.ctx);
		break;

	case NRFS_DVFS_REQ_INIT_COMPLETE:
		evt.type = NRFS_DVFS_EVT_INIT_DONE;
		m_cb.handler(&evt, (void *)p_rsp->ctx.ctx);
		break;

	case NRFS_DVFS_REQ_OPPOINT:
		if (p_rsp->data.scaling_prepare == true) {
			evt.type = NRFS_DVFS_EVT_OPPOINT_SCALING_PREPARE;
		} else {
			evt.type = NRFS_DVFS_EVT_OPPOINT_REQ_CONFIRMED;
		}
		m_cb.handler(&evt, (void *)p_rsp->ctx.ctx);
		break;

	case NRFS_DVFS_REQ_READY_TO_SCALE:
		evt.type = NRFS_DVFS_EVT_OPPOINT_SCALING_DONE;
		m_cb.handler(&evt, (void *)p_rsp->ctx.ctx);
		break;

	default:
		break;
	}
}

nrfs_err_t nrfs_dvfs_init(nrfs_dvfs_evt_handler_t handler)
{
	if (m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	m_cb.handler	    = handler;
	m_cb.is_initialized = true;
	return NRFS_SUCCESS;
}

void nrfs_dvfs_uninit(void)
{
	m_cb.is_initialized = false;
}

nrfs_err_t nrfs_dvfs_init_prepare_request(void *p_context)
{
	if (!m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	nrfs_dvfs_req_t req;

	NRFS_SERVICE_HDR_FILL(&req, NRFS_DVFS_REQ_INIT_PREPARE);
	req.ctx.ctx = (uint32_t)p_context;

	return nrfs_backend_send(&req, sizeof(req));
}

nrfs_err_t nrfs_dvfs_init_complete_request(void *p_context)
{
	if (!m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	nrfs_dvfs_req_t req;

	NRFS_SERVICE_HDR_FILL(&req, NRFS_DVFS_REQ_INIT_COMPLETE);
	req.ctx.ctx = (uint32_t)p_context;

	return nrfs_backend_send(&req, sizeof(req));
}

nrfs_err_t nrfs_dvfs_oppoint_request(enum dvfs_frequency_setting target_freq, void *p_context)
{
	if (!m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	nrfs_dvfs_opp_req_t req;

	NRFS_SERVICE_HDR_FILL(&req, NRFS_DVFS_REQ_OPPOINT);
	req.ctx.ctx	     = (uint32_t)p_context;
	req.data.target_freq = target_freq;

	return nrfs_backend_send(&req, sizeof(req));
}

nrfs_err_t nrfs_dvfs_oppoint_request_no_rsp(enum dvfs_frequency_setting target_freq,
					    void *p_context)
{
	if (!m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	nrfs_dvfs_opp_req_t req;

	NRFS_SERVICE_HDR_FILL(&req, NRFS_DVFS_REQ_OPPOINT);
	NRFS_HDR_NO_RSP_SET(&req.hdr);
	req.ctx.ctx	     = (uint32_t)p_context;
	req.data.target_freq = target_freq;

	return nrfs_backend_send(&req, sizeof(req));
}

nrfs_err_t nrfs_dvfs_ready_to_scale(void *p_context)
{
	if (!m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	nrfs_dvfs_req_t req;

	NRFS_SERVICE_HDR_FILL(&req, NRFS_DVFS_REQ_READY_TO_SCALE);
	req.ctx.ctx = (uint32_t)p_context;

	return nrfs_backend_send(&req, sizeof(req));
}
