/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <internal/nrfs_backend.h>
#include <internal/nrfs_callbacks.h>
#include <nrfs_diag.h>

typedef struct {
	nrfs_diag_response_handler_t handler;
	bool is_initialized;
} nrfs_diag_cb_t;
static nrfs_diag_cb_t m_cb;

nrfs_err_t nrfs_diag_init(nrfs_diag_response_handler_t handler)
{
	if (m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	m_cb.handler	    = handler;
	m_cb.is_initialized = true;
	return NRFS_SUCCESS;
}

void nrfs_diag_uninit(void)
{
	m_cb.is_initialized = false;
}

nrfs_err_t nrfs_diag_reg_read(uint32_t addr, void *p_context)
{
	if (!m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	nrfs_diag_reg_req_t req;

	NRFS_SERVICE_HDR_FILL(&req, NRFS_DIAG_REG);
	req.ctx.ctx	    = (uint32_t)p_context;
	req.reg.access_type = DIAG_REG_READ;
	req.reg.addr	    = addr;
	req.reg.val	    = 0;

	return nrfs_backend_send(&req, sizeof(req));
}

nrfs_err_t nrfs_diag_reg_write(uint32_t addr, uint32_t val, void *p_context)
{
	if (!m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	nrfs_diag_reg_req_t req;

	NRFS_SERVICE_HDR_FILL(&req, NRFS_DIAG_REG);
	req.ctx.ctx	    = (uint32_t)p_context;
	req.reg.access_type = DIAG_REG_WRITE;
	req.reg.addr	    = addr;
	req.reg.val	    = val;

	return nrfs_backend_send(&req, sizeof(req));
}

void nrfs_diag_service_notify(void *p_notification, size_t size)
{
	if (!m_cb.handler || !m_cb.is_initialized) {
		return;
	}

	nrfs_diag_evt_t evt;
	nrfs_generic_t *p_data = (nrfs_generic_t *)p_notification;
	if (NRFS_HDR_FILTER_ERR_GET(&p_data->hdr)) {
		evt.type = NRFS_DIAG_EVT_REJECT;
		m_cb.handler(&evt, (void *)p_data->ctx.ctx);
		return;
	}

	switch (p_data->hdr.req) {
	case NRFS_DIAG_REG:
		nrfs_diag_rsp_t *p_rsp = (nrfs_diag_rsp_t *)p_notification;
		evt.type	       = NRFS_DIAG_EVT_REG_RSP;
		evt.reg.addr	       = p_rsp->data.addr;
		evt.reg.val	       = p_rsp->data.val;
		evt.reg.access_type    = p_rsp->data.access_type;
		m_cb.handler(&evt, (void *)p_data->ctx.ctx);

	default:
		break;
	}
}
