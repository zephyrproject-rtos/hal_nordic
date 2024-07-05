/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <internal/nrfs_backend.h>
#include <internal/nrfs_callbacks.h>
#include <nrfs_mram.h>

typedef struct {
	nrfs_mram_latency_evt_handler_t handler;
	bool is_initialized;
} nrfs_mram_cb_t;
static nrfs_mram_cb_t m_cb;

nrfs_err_t nrfs_mram_init(nrfs_mram_latency_evt_handler_t handler)
{
	if (m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	m_cb.handler	    = handler;
	m_cb.is_initialized = true;
	return NRFS_SUCCESS;
}

void nrfs_mram_uninit(void)
{
	m_cb.is_initialized = false;
}

nrfs_err_t nrfs_mram_set_latency(mram_latency_request_t mram_latency_request, void *p_context)
{
	if (!m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	nrfs_mram_set_latency_t req;

	NRFS_SERVICE_HDR_FILL(&req, NRFS_MRAM_REQ_SET_LATENCY);
	req.ctx.ctx		      = (uint32_t)p_context;
	req.data.mram_latency_request = mram_latency_request;
	return nrfs_backend_send(&req, sizeof(req));
}

void nrfs_mram_service_notify(void *p_notification, size_t size)
{
	if (!m_cb.handler || !m_cb.is_initialized) {
		return;
	}

	nrfs_mram_latency_evt_t evt;
	nrfs_generic_t *p_data = (nrfs_generic_t *)p_notification;
	if (NRFS_HDR_FILTER_ERR_GET(&p_data->hdr)) {
		evt.type = NRFS_MRAM_LATENCY_REQ_REJECTED;
		m_cb.handler(&evt, (void *)p_data->ctx.ctx);
		return;
	}

	switch (p_data->hdr.req) {
	case NRFS_MRAM_REQ_SET_LATENCY: /*  sent only as a response to
					the request MRAM_LATENCY_NOT_ALLOWED */
		evt.type = NRFS_MRAM_LATENCY_REQ_APPLIED;
		m_cb.handler(&evt, (void *)p_data->ctx.ctx);
		break;

	default:
		break;
	}
}
