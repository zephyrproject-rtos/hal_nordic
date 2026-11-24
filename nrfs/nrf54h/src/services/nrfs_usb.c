/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <internal/nrfs_backend.h>
#include <internal/nrfs_callbacks.h>
#include <nrfs_usb.h>

typedef struct {
	nrfs_usb_evt_handler_t handler;
	bool is_initialized;
} nrfs_usb_cb_t;
static nrfs_usb_cb_t m_cb;

void nrfs_usb_service_notify(void *p_notification, size_t size)
{
	if (!m_cb.handler || !m_cb.is_initialized) {
		return;
	}

	nrfs_usb_evt_t evt;
	nrfs_generic_t *p_data = (nrfs_generic_t *)p_notification;
	if (NRFS_HDR_FILTER_ERR_GET(&p_data->hdr)) {
		evt.type = NRFS_USB_EVT_REJECT;
		m_cb.handler(&evt, (void *)p_data->ctx.ctx);
		return;
	}

	nrfs_usb_rsp_t *p_rsp = (nrfs_usb_rsp_t *)p_notification;
	switch (p_data->hdr.req) {
	case NRFS_USB_REQ_ENABLE:
		evt.type	  = NRFS_USB_EVT_VBUS_STATUS_CHANGE;
		evt.usbhspll_ok	  = p_rsp->data.pll_ok;
		evt.vregusb_ok	  = p_rsp->data.vreg_ok;
		evt.vbus_detected = p_rsp->data.vbus_detected;
		m_cb.handler(&evt, (void *)p_rsp->ctx.ctx);
		break;

	default:
		break;
	}
}

nrfs_err_t nrfs_usb_init(nrfs_usb_evt_handler_t handler)
{
	if (m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	m_cb.handler	    = handler;
	m_cb.is_initialized = true;
	return NRFS_SUCCESS;
}

void nrfs_usb_uninit(void)
{
	m_cb.is_initialized = false;
}

nrfs_err_t nrfs_usb_enable_request(void *p_context)
{
	if (!m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	nrfs_usb_enable_req_t req;

	NRFS_SERVICE_HDR_FILL(&req, NRFS_USB_REQ_ENABLE);
	req.ctx.ctx = (uint32_t)p_context;

	return nrfs_backend_send(&req, sizeof(req));
}

nrfs_err_t nrfs_usb_disable_request(void *p_context)
{
	if (!m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	nrfs_usb_disable_req_t req;

	NRFS_SERVICE_HDR_FILL(&req, NRFS_USB_REQ_DISABLE);
	req.ctx.ctx = (uint32_t)p_context;

	return nrfs_backend_send(&req, sizeof(req));
}

nrfs_err_t nrfs_usb_dplus_pullup_enable(void *p_context)
{
	if (!m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	nrfs_usb_ld_dplus_pullup_enable_req_t req;

	NRFS_SERVICE_HDR_FILL(&req, NRFS_USB_REQ_DPLUS_PULLUP_ENABLE);
	NRFS_HDR_NO_RSP_SET(&req.hdr);
	req.ctx.ctx = (uint32_t)p_context;

	return nrfs_backend_send(&req, sizeof(req));
}

nrfs_err_t nrfs_usb_dplus_pullup_disable(void *p_context)
{
	if (!m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	nrfs_usb_ld_dplus_pullup_disable_req_t req;

	NRFS_SERVICE_HDR_FILL(&req, NRFS_USB_REQ_DPLUS_PULLUP_DISABLE);
	NRFS_HDR_NO_RSP_SET(&req.hdr);
	req.ctx.ctx = (uint32_t)p_context;

	return nrfs_backend_send(&req, sizeof(req));
}

