/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <internal/nrfs_backend.h>
#include <internal/nrfs_callbacks.h>
#include <nrfs_pmic.h>

typedef struct {
	nrfs_pmic_evt_handler_t handler;
	bool is_initialized;
} nrfs_pmic_cb_t;
static nrfs_pmic_cb_t m_cb;

nrfs_err_t nrfs_pmic_init(nrfs_pmic_evt_handler_t handler)
{
	if (m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	m_cb.handler	    = handler;
	m_cb.is_initialized = true;
	return NRFS_SUCCESS;
}

void nrfs_pmic_uninit(void)
{
	m_cb.is_initialized = false;
}

nrfs_err_t nrfs_pmic_rffe_on(void *p_context)
{
	if (!m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	nrfs_pmic_rffe_on_req_t req;

	NRFS_SERVICE_HDR_FILL(&req, NRFS_PMIC_RFFE_ON);
	req.ctx.ctx = (uint32_t)p_context;

	return nrfs_backend_send(&req, sizeof(req));
}

nrfs_err_t nrfs_pmic_rffe_on_no_rsp(void)
{
	if (!m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	nrfs_pmic_rffe_on_req_t req;

	NRFS_SERVICE_HDR_FILL(&req, NRFS_PMIC_RFFE_ON);
	NRFS_HDR_NO_RSP_SET(&req.hdr);

	return nrfs_backend_send(&req, sizeof(req));
}

nrfs_err_t nrfs_pmic_rffe_off(void *p_context)
{
	if (!m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	nrfs_pmic_rffe_off_req_t req;

	NRFS_SERVICE_HDR_FILL(&req, NRFS_PMIC_RFFE_OFF);
	req.ctx.ctx = (uint32_t)p_context;

	return nrfs_backend_send(&req, sizeof(req));
}

nrfs_err_t nrfs_pmic_rffe_off_no_rsp(void)
{
	if (!m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	nrfs_pmic_rffe_off_req_t req;

	NRFS_SERVICE_HDR_FILL(&req, NRFS_PMIC_RFFE_OFF);
	NRFS_HDR_NO_RSP_SET(&req.hdr);

	return nrfs_backend_send(&req, sizeof(req));
}

nrfs_err_t nrfs_pmic_sim_on(pmic_sim_t sim, void *p_context)
{
	if (!m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	nrfs_pmic_sim_on_req_t req;

	NRFS_SERVICE_HDR_FILL(&req, NRFS_PMIC_SIM_ON);
	req.ctx.ctx = (uint32_t)p_context;
	req.sim	    = sim;

	return nrfs_backend_send(&req, sizeof(req));
}

nrfs_err_t nrfs_pmic_sim_on_no_rsp(pmic_sim_t sim)
{
	if (!m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	nrfs_pmic_sim_on_req_t req;

	NRFS_SERVICE_HDR_FILL(&req, NRFS_PMIC_SIM_ON);
	NRFS_HDR_NO_RSP_SET(&req.hdr);
	req.sim = sim;

	return nrfs_backend_send(&req, sizeof(req));
}

nrfs_err_t nrfs_pmic_sim_off(pmic_sim_t sim, void *p_context)
{
	if (!m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	nrfs_pmic_sim_off_req_t req;

	NRFS_SERVICE_HDR_FILL(&req, NRFS_PMIC_SIM_OFF);
	req.ctx.ctx = (uint32_t)p_context;
	req.sim	    = sim;

	return nrfs_backend_send(&req, sizeof(req));
}

nrfs_err_t nrfs_pmic_sim_off_no_rsp(pmic_sim_t sim)
{
	if (!m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	nrfs_pmic_sim_off_req_t req;

	NRFS_SERVICE_HDR_FILL(&req, NRFS_PMIC_SIM_OFF);
	NRFS_HDR_NO_RSP_SET(&req.hdr);
	req.sim = sim;

	return nrfs_backend_send(&req, sizeof(req));
}

nrfs_err_t nrfs_pmic_ble_radio_on(pmic_ble_radio_txpower_t txpower, void *p_context)
{
	if (!m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	nrfs_pmic_ble_radio_on_req_t req;

	NRFS_SERVICE_HDR_FILL(&req, NRFS_PMIC_BLE_RADIO_ON);
	req.ctx.ctx = (uint32_t)p_context;
	req.txpower = txpower;

	return nrfs_backend_send(&req, sizeof(req));
}

nrfs_err_t nrfs_pmic_ble_radio_on_no_rsp(pmic_ble_radio_txpower_t txpower)
{
	if (!m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	nrfs_pmic_ble_radio_on_req_t req;

	NRFS_SERVICE_HDR_FILL(&req, NRFS_PMIC_BLE_RADIO_ON);
	NRFS_HDR_NO_RSP_SET(&req.hdr);
	req.txpower = txpower;

	return nrfs_backend_send(&req, sizeof(req));
}

nrfs_err_t nrfs_pmic_ble_radio_off(void *p_context)
{
	if (!m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	nrfs_pmic_ble_radio_off_req_t req;

	NRFS_SERVICE_HDR_FILL(&req, NRFS_PMIC_BLE_RADIO_OFF);
	req.ctx.ctx = (uint32_t)p_context;

	return nrfs_backend_send(&req, sizeof(req));
}

nrfs_err_t nrfs_pmic_ble_radio_off_no_rsp(void)
{
	if (!m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	nrfs_pmic_ble_radio_off_req_t req;

	NRFS_SERVICE_HDR_FILL(&req, NRFS_PMIC_BLE_RADIO_OFF);
	NRFS_HDR_NO_RSP_SET(&req.hdr);

	return nrfs_backend_send(&req, sizeof(req));
}

nrfs_err_t nrfs_pmic_pwm_default_set(void *p_context)
{
	if (!m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	nrfs_pmic_pwm_default_req_t req;

	NRFS_SERVICE_HDR_FILL(&req, NRFS_PMIC_PWM_DEFAULT);
	req.ctx.ctx = (uint32_t)p_context;

	return nrfs_backend_send(&req, sizeof(req));
}

nrfs_err_t nrfs_pmic_pwm_default_set_no_rsp(void)
{
	if (!m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	nrfs_pmic_pwm_default_req_t req;

	NRFS_SERVICE_HDR_FILL(&req, NRFS_PMIC_PWM_DEFAULT);
	NRFS_HDR_NO_RSP_SET(&req.hdr);

	return nrfs_backend_send(&req, sizeof(req));
}

nrfs_err_t nrfs_pmic_pwm_ghost_avoid_set(void *p_context)
{
	if (!m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	nrfs_pmic_pwm_ghost_avoid_req_t req;

	NRFS_SERVICE_HDR_FILL(&req, NRFS_PMIC_PWM_GHOST_AVOID);
	req.ctx.ctx = (uint32_t)p_context;

	return nrfs_backend_send(&req, sizeof(req));
}

nrfs_err_t nrfs_pmic_pwm_ghost_avoid_set_no_rsp(void)
{
	if (!m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	nrfs_pmic_pwm_ghost_avoid_req_t req;

	NRFS_SERVICE_HDR_FILL(&req, NRFS_PMIC_PWM_GHOST_AVOID);
	NRFS_HDR_NO_RSP_SET(&req.hdr);

	return nrfs_backend_send(&req, sizeof(req));
}

nrfs_err_t nrfs_pmic_test_if_read(uint16_t addr, void *p_context)
{
	if (!m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	nrfs_pmic_test_if_req_t req;

	NRFS_SERVICE_HDR_FILL(&req, NRFS_PMIC_TEST_IF);
	req.ctx.ctx	    = (uint32_t)p_context;
	req.reg.access_type = PMIC_REG_READ;
	req.reg.addr	    = addr;
	req.reg.val	    = 0;

	return nrfs_backend_send(&req, sizeof(req));
}

nrfs_err_t nrfs_pmic_test_if_write(uint16_t addr, uint8_t val, void *p_context)
{
	if (!m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	nrfs_pmic_test_if_req_t req;

	NRFS_SERVICE_HDR_FILL(&req, NRFS_PMIC_TEST_IF);
	req.ctx.ctx	    = (uint32_t)p_context;
	req.reg.access_type = PMIC_REG_WRITE;
	req.reg.addr	    = addr;
	req.reg.val	    = val;

	return nrfs_backend_send(&req, sizeof(req));
}

nrfs_err_t nrfs_pmic_info_read(void *p_context)
{
	if (!m_cb.is_initialized) {
		return NRFS_ERR_INVALID_STATE;
	}

	nrfs_pmic_info_req_t req;

	NRFS_SERVICE_HDR_FILL(&req, NRFS_PMIC_INFO);
	req.ctx.ctx = (uint32_t)p_context;

	return nrfs_backend_send(&req, sizeof(req));
}

void nrfs_pmic_service_notify(void *p_notification, size_t size)
{
	if (!m_cb.handler || !m_cb.is_initialized) {
		return;
	}

	nrfs_pmic_evt_t evt;
	nrfs_generic_t *p_data = (nrfs_generic_t *)p_notification;

	if (NRFS_HDR_FILTER_ERR_GET(&p_data->hdr)) {
		evt.type = NRFS_PMIC_EVT_REJECT;
		m_cb.handler(&evt, (void *)p_data->ctx.ctx);
		return;
	}

	switch (p_data->hdr.req) {
	case NRFS_PMIC_RFFE_ON:
	case NRFS_PMIC_RFFE_OFF:
	case NRFS_PMIC_SIM_ON:
	case NRFS_PMIC_SIM_OFF:
	case NRFS_PMIC_BLE_RADIO_ON:
	case NRFS_PMIC_BLE_RADIO_OFF:
	case NRFS_PMIC_PWM_DEFAULT:
	case NRFS_PMIC_PWM_GHOST_AVOID:
		evt.type = NRFS_PMIC_EVT_APPLIED;
		m_cb.handler(&evt, (void *)p_data->ctx.ctx);
		break;
	case NRFS_PMIC_TEST_IF:
		nrfs_pmic_rsp_t *p_rsp = (nrfs_pmic_rsp_t *)p_notification;

		evt.type	= NRFS_PMIC_EVT_TEST_IF_RSP;
		evt.val		= p_rsp->data.val;
		evt.access_type = p_rsp->data.access_type;
		m_cb.handler(&evt, (void *)p_rsp->ctx.ctx);
		break;
	case NRFS_PMIC_INFO:
		nrfs_pmic_info_rsp_t *p_rsp_info = (nrfs_pmic_info_rsp_t *)p_notification;
		nrfs_pmic_info_evt_t info_evt;

		info_evt.type = NRFS_PMIC_EVT_INFO_RSP;
		info_evt.info = p_rsp_info->data;
		m_cb.handler(&info_evt, (void *)p_rsp_info->ctx.ctx);
		break;
	default:
		break;
	}
}
