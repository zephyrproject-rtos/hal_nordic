/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFS_INTERNAL_PMIC_H
#define NRFS_INTERNAL_PMIC_H

#include <internal/services/nrfs_generic.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief PMIC SIM selection. */
typedef enum __NRFS_PACKED {
	PMIC_SIM1 = 1, /** SIM1 */
	PMIC_SIM2 = 2  /** SIM2 */
} pmic_sim_t;

/** @brief PMIC BLE RADIO TX power. */
typedef enum __NRFS_PACKED {
	PMIC_BLE_RADIO_TXPOWER_POS10DBM, /** +10 dBm */
	PMIC_BLE_RADIO_TXPOWER_POS9DBM,	 /** +9 dBm */
	PMIC_BLE_RADIO_TXPOWER_POS8DBM,	 /** +8 dBm */
	PMIC_BLE_RADIO_TXPOWER_POS7DBM,	 /** +7 dBm */
	PMIC_BLE_RADIO_TXPOWER_POS6DBM,	 /** +6 dBm */
	PMIC_BLE_RADIO_TXPOWER_POS5DBM,	 /** +5 dBm */
	PMIC_BLE_RADIO_TXPOWER_POS4DBM,	 /** +4 dBm */
	PMIC_BLE_RADIO_TXPOWER_POS3DBM,	 /** +3 dBm */
	PMIC_BLE_RADIO_TXPOWER_POS2DBM,	 /** +2 dBm */
	PMIC_BLE_RADIO_TXPOWER_POS1DBM,	 /** +1 dBm */
	PMIC_BLE_RADIO_TXPOWER_0DBM,	 /** 0 dBm */
	PMIC_BLE_RADIO_TXPOWER_NEG1DBM,	 /** -1 dBm */
	PMIC_BLE_RADIO_TXPOWER_NEG2DBM,	 /** -2 dBm */
	PMIC_BLE_RADIO_TXPOWER_NEG4DBM,	 /** -4 dBm */
	PMIC_BLE_RADIO_TXPOWER_NEG8DBM,	 /** -8 dBm */
	PMIC_BLE_RADIO_TXPOWER_NEG12DBM, /** -12 dBm */
	PMIC_BLE_RADIO_TXPOWER_NEG16DBM, /** -16 dBm */
	PMIC_BLE_RADIO_TXPOWER_NEG20DBM, /** -20 dBm */
	PMIC_BLE_RADIO_TXPOWER_NEG30DBM, /** -30 dBm */
	PMIC_BLE_RADIO_TXPOWER_NEG40DBM, /** -40 dBm */
	PMIC_BLE_RADIO_TXPOWER_NEG70DBM	 /** -70 dBm */
} pmic_ble_radio_txpower_t;

/** @brief PMIC TEST IF register access type. */
typedef enum __NRFS_PACKED {
	PMIC_REG_READ,	  /** Register read */
	PMIC_REG_WRITE,	  /** Register write */
	PMIC_REG_INVALID, /** Register access invalid */
} pmic_reg_access_type_t;

/** @brief PMIC TEST IF register access. */
typedef struct __NRFS_PACKED {
	pmic_reg_access_type_t access_type; /** Register access type */
	uint16_t addr;			    /** Register address */
	uint8_t val;			    /** Register value for writes */
} pmic_reg_access_t;

/** @brief PMIC service notification data structure. */
typedef struct __NRFS_PACKED {
	pmic_reg_access_type_t access_type; /** Register access type */
	uint8_t val;			    /** Register value of 8-bit register. */
} nrfs_pmic_rsp_data_t;

/** @brief PMIC RFFE ON request structure. */
typedef struct __NRFS_PACKED {
	nrfs_hdr_t hdr; /**< Header of the message. */
	nrfs_ctx_t ctx; /**< Context of the message. */
} nrfs_pmic_rffe_on_req_t;

/** @brief PMIC RFFE OFF request structure. */
typedef struct __NRFS_PACKED {
	nrfs_hdr_t hdr; /**< Header of the message. */
	nrfs_ctx_t ctx; /**< Context of the message. */
} nrfs_pmic_rffe_off_req_t;

/** @brief PMIC SIM ON request structure. */
typedef struct __NRFS_PACKED {
	nrfs_hdr_t hdr; /**< Header of the message. */
	nrfs_ctx_t ctx; /**< Context of the message. */
	pmic_sim_t sim; /**< SIM to enable. */
} nrfs_pmic_sim_on_req_t;

/** @brief PMIC SIM OFF request structure. */
typedef struct __NRFS_PACKED {
	nrfs_hdr_t hdr; /**< Header of the message. */
	nrfs_ctx_t ctx; /**< Context of the message. */
	pmic_sim_t sim; /**< SIM to disable. */
} nrfs_pmic_sim_off_req_t;

/** @brief PMIC BLE RADIO ON request structure. */
typedef struct __NRFS_PACKED {
	nrfs_hdr_t hdr;			  /**< Header of the message. */
	nrfs_ctx_t ctx;			  /**< Context of the message. */
	pmic_ble_radio_txpower_t txpower; /**< Needed BLE Radio TX Power. */
} nrfs_pmic_ble_radio_on_req_t;

/** @brief PMIC BLE RADIO OFF request structure. */
typedef struct __NRFS_PACKED {
	nrfs_hdr_t hdr; /**< Header of the message. */
	nrfs_ctx_t ctx; /**< Context of the message. */
} nrfs_pmic_ble_radio_off_req_t;

/** @brief PMIC PWM default set request structure. */
typedef struct __NRFS_PACKED {
	nrfs_hdr_t hdr; /**< Header of the message. */
	nrfs_ctx_t ctx; /**< Context of the message. */
} nrfs_pmic_pwm_default_req_t;

/** @brief PMIC PWM ghost avoid set request structure. */
typedef struct __NRFS_PACKED {
	nrfs_hdr_t hdr; /**< Header of the message. */
	nrfs_ctx_t ctx; /**< Context of the message. */
} nrfs_pmic_pwm_ghost_avoid_req_t;

/** @brief PMIC TEST IF request structure. */
typedef struct __NRFS_PACKED {
	nrfs_hdr_t hdr;	       /**< Header of the message. */
	nrfs_ctx_t ctx;	       /**< Context of the message. */
	pmic_reg_access_t reg; /**< PMIC register access. */
} nrfs_pmic_test_if_req_t;

/** @brief PMIC INFO request structure. */
typedef struct __NRFS_PACKED {
	nrfs_hdr_t hdr;	       /**< Header of the message. */
	nrfs_ctx_t ctx;	       /**< Context of the message. */
} nrfs_pmic_info_req_t;

/** @brief PMIC available info. */
typedef enum __NRFS_PACKED {
	PMIC_NOT_AVAILABLE,
	PMIC_AVAILABLE
} pmic_available_t;

/** @brief PMIC info data structure. */
typedef struct __NRFS_PACKED {
	pmic_available_t  pmic_available;
} nrfs_pmic_info_rsp_data_t;

/** @brief PMIC service notification structure. */
typedef struct __NRFS_PACKED {
	nrfs_hdr_t hdr;		   /**< Header of the message. */
	nrfs_ctx_t ctx;		   /**< Context of the message. */
	nrfs_pmic_info_rsp_data_t data; /**< Data of the notification. */
} nrfs_pmic_info_rsp_t;

/** @brief PMIC service notification structure. */
typedef struct __NRFS_PACKED {
	nrfs_hdr_t hdr;		   /**< Header of the message. */
	nrfs_ctx_t ctx;		   /**< Context of the message. */
	nrfs_pmic_rsp_data_t data; /**< Data of the notification. */
} nrfs_pmic_rsp_t;

#ifdef __cplusplus
}
#endif

#endif /* NRFS_INTERNAL_PMIC_H */
