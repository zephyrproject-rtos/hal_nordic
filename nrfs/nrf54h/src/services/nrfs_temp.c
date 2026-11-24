/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <internal/nrfs_backend.h>
#include <internal/nrfs_callbacks.h>
#include <nrfs_temp.h>

typedef struct {
	nrfs_temp_evt_handler_t handler;
	bool is_initialized;
} nrfs_temp_cb_t;
static nrfs_temp_cb_t m_cb;

void nrfs_temp_service_notify(void *p_notification, size_t size)
{
	if (!m_cb.handler || !m_cb.is_initialized) {
		return;
	}

	nrfs_temp_evt_t evt;
	nrfs_generic_t *p_data = (nrfs_generic_t *)p_notification;
	if (NRFS_HDR_FILTER_ERR_GET(&p_data->hdr)) {
		evt.type = NRFS_TEMP_EVT_REJECT;
		m_cb.handler(&evt, (void *)p_data->ctx.ctx);
		return;
	}

	nrfs_temp_rsp_t *p_rsp = (nrfs_temp_rsp_t *)p_notification;
	evt.raw_temp	       = p_rsp->data.raw_temp;
	switch (p_data->hdr.req) {
	case NRFS_TEMP_REQ_MEASURE:
		evt.type = NRFS_TEMP_EVT_MEASURE_DONE;
		m_cb.handler(&evt, (void *)p_rsp->ctx.ctx);
		break;
#ifdef NRFS_TEMP_SERVICE_SUBSCRIPTION_ENABLED
	case NRFS_TEMP_REQ_SUBSCRIBE:
		evt.type = NRFS_TEMP_EVT_CHANGE;
		m_cb.handler(&evt, (void *)p_rsp->ctx.ctx);
		break;
#endif
	default:
		break;
	}
}

nrfs_err_t nrfs_temp_init(nrfs_temp_evt_handler_t handler)
{
	if (m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	m_cb.handler	    = handler;
	m_cb.is_initialized = true;
	return NRFS_SUCCESS;
}

void nrfs_temp_uninit(void)
{
	m_cb.is_initialized = false;
}

nrfs_err_t nrfs_temp_measure_request(void *p_context)
{
	if (!m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	nrfs_temp_measure_t req;

	NRFS_SERVICE_HDR_FILL(&req, NRFS_TEMP_REQ_MEASURE);
	req.ctx.ctx = (uint32_t)p_context;

	return nrfs_backend_send(&req, sizeof(req));
}
#ifdef NRFS_TEMP_SERVICE_SUBSCRIPTION_ENABLED
nrfs_err_t nrfs_temp_subscribe(uint16_t measure_rate_ms,
			       int32_t lower_threshold,
			       int32_t upper_threshold,
			       void * p_context)
{
	if (!m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	nrfs_temp_subscribe_t req;

	NRFS_SERVICE_HDR_FILL(&req, NRFS_TEMP_REQ_SUBSCRIBE);
	req.ctx.ctx		 = (uint32_t)p_context;
	req.data.measure_rate_ms = measure_rate_ms;
	req.data.lower_threshold = lower_threshold;
	req.data.upper_threshold = upper_threshold;

	return nrfs_backend_send(&req, sizeof(req));
}

nrfs_err_t nrfs_temp_unsubscribe(void)
{
	if (!m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	nrfs_temp_subscribe_t req;

	NRFS_SERVICE_HDR_FILL(&req, NRFS_TEMP_REQ_UNSUBSCRIBE);

	return nrfs_backend_send(&req, sizeof(req));
}
#endif  /* NRFS_TEMP_SERVICE_SUBSCRIPTION_ENABLED */
