/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IRONSIDE_SE_UICR_H_
#define IRONSIDE_SE_UICR_H_

#include <stdint.h>

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup ironside_se_uicr UICR
 * @ingroup ironside_se
 * @{
 */

/** UICR structure defined by IronSide SE. */
#define IRONSIDE_SE_UICR ((struct UICR *)NRF_APPLICATION_UICR_NS_BASE)

/**
 * @defgroup ironside_se_uicr_version UICR format versions
 * @{
 */

/** UICR format version 2.0.
 *  @since Supported from IronSide SE v23.0.2+17 (@ref IRONSIDE_SE_V23_0_2_17)
 */
#define UICR_VERSION_2_0 0x00020000UL
/** UICR format version 2.1.
 *  @since Supported from IronSide SE v23.3.0+26 (@ref IRONSIDE_SE_V23_3_0_26)
 */
#define UICR_VERSION_2_1 0x00020001UL
/** UICR format version 2.2.
 *  @since Supported from IronSide SE v23.4.0+27 (@ref IRONSIDE_SE_V23_4_0_27)
 */
#define UICR_VERSION_2_2 0x00020002UL
/** UICR version 2.3
 *  @since Supported from IronSide SE v23.5.0+28 (@ref IRONSIDE_SE_V23_5_0_28)
 */
#define UICR_VERSION_2_3 0x00020003UL
/** Maximum UICR version supported by this header. */
#define UICR_VERSION_MAX UICR_VERSION_2_3

/** @} */

/**
 * @defgroup ironside_se_uicr_common Common values
 * @{
 */

/** Default erased value for all UICR fields. */
#define UICR_MAGIC_ERASE_VALUE (0xBD2328A8UL)
/** Common disabled value. */
#define UICR_DISABLED          UICR_MAGIC_ERASE_VALUE
/** Common enabled value. Any value other than UICR_DISABLED is interpreted as enabled. */
#define UICR_ENABLED           (0xFFFFFFFFUL)
/** Common unprotected value. */
#define UICR_UNPROTECTED       UICR_MAGIC_ERASE_VALUE
/** Common protected value. Any value other than UICR_UNPROTECTED is interpreted as protected. */
#define UICR_PROTECTED         UICR_ENABLED

/** First enumeration choice (typically "option 0" or disabled/default). */
#define UICR_ENUM_CHOICE_0 UICR_MAGIC_ERASE_VALUE
/** Second enumeration choice (typically "option 1" or enabled). */
#define UICR_ENUM_CHOICE_1 (0x1730C77FUL)

/** @} */

/**
 * @brief Access port protection.
 */
struct UICR_APPROTECT {
	/** APPLICATION access port protection (see @ref ironside_se_uicr_approtect_application). */
	volatile uint32_t APPLICATION;
	/** RADIOCORE access port protection (see @ref ironside_se_uicr_approtect_radiocore). */
	volatile uint32_t RADIOCORE;
	volatile const uint32_t RESERVED;
	/** CoreSight access port protection (see @ref ironside_se_uicr_approtect_coresight). */
	volatile uint32_t CORESIGHT;
};

/**
 * @defgroup ironside_se_uicr_approtect_application UICR_APPROTECT APPLICATION values
 * @{
 */

/** APPLICATION access port is not protected. */
#define UICR_APPROTECT_APPLICATION_UNPROTECTED UICR_MAGIC_ERASE_VALUE
/** APPLICATION access port is protected. */
#define UICR_APPROTECT_APPLICATION_PROTECTED   UICR_PROTECTED

/** @} */

/**
 * @defgroup ironside_se_uicr_approtect_radiocore UICR_APPROTECT RADIOCORE values
 * @{
 */

/** RADIOCORE access port is not protected. */
#define UICR_APPROTECT_RADIOCORE_UNPROTECTED UICR_MAGIC_ERASE_VALUE
/** RADIOCORE access port is protected. */
#define UICR_APPROTECT_RADIOCORE_PROTECTED   UICR_PROTECTED

/** @} */

/**
 * @defgroup ironside_se_uicr_approtect_coresight UICR_APPROTECT CORESIGHT values
 * @{
 */

/** CoreSight access port is not protected. */
#define UICR_APPROTECT_CORESIGHT_UNPROTECTED UICR_MAGIC_ERASE_VALUE
/** CoreSight access port is protected. */
#define UICR_APPROTECT_CORESIGHT_PROTECTED   UICR_PROTECTED

/** @} */

/**
 * @brief Protected Memory region configuration
 */
struct UICR_PROTECTEDMEM {
	/** Enable the Protected Memory region (see @ref ironside_se_uicr_common). */
	volatile uint32_t ENABLE;
	/** Protected Memory region size in 4 kiB blocks. */
	volatile uint32_t SIZE4KB;
};

/**
 * @brief Start a local watchdog timer ahead of the CPU boot.
 */
struct UICR_WDTSTART {
	/** Enable watchdog timer start (see @ref ironside_se_uicr_common). */
	volatile uint32_t ENABLE;
	/** Watchdog timer instance (see @ref ironside_se_uicr_wdtstart_instance). */
	volatile uint32_t INSTANCE;
	/** Initial value of CRV (Counter Reload Value) (see @ref ironside_se_uicr_wdtstart_crv). */
	volatile uint32_t CRV;
};

/**
 * @defgroup ironside_se_uicr_wdtstart_instance UICR_WDTSTART INSTANCE values
 * @{
 */

/** Start WDT0 in the domain of the processor being booted. */
#define UICR_WDTSTART_INSTANCE_WDT0 UICR_ENUM_CHOICE_0
/** Start WDT1 in the domain of the processor being booted. */
#define UICR_WDTSTART_INSTANCE_WDT1 UICR_ENUM_CHOICE_1

/** @} */

/**
 * @defgroup ironside_se_uicr_wdtstart_crv UICR_WDTSTART CRV limits
 * @{
 */

/** Minimum CRV value. */
#define UICR_WDTSTART_CRV_CRV_MIN (0xFUL)
/** Maximum CRV value. */
#define UICR_WDTSTART_CRV_CRV_MAX (0xFFFFFFFFUL)

/** @} */

/**
 * @brief Secure Storage partition sizes
 */
struct UICR_SECURESTORAGE_SIZES {
	/** Size of the APPLICATION partition in 1 kiB blocks. */
	volatile uint32_t APPLICATIONSIZE1KB;
	/** Size of the RADIOCORE partition in 1 kiB blocks. */
	volatile uint32_t RADIOCORESIZE1KB;
};

/**
 * @brief Secure Storage configuration
 */
struct UICR_SECURESTORAGE {
	/** Enable the Secure Storage (see @ref ironside_se_uicr_common). */
	volatile uint32_t ENABLE;
	/** Start address of the Secure Storage region. */
	volatile uint32_t ADDRESS;
	/** Secure Storage partitions for the Cryptographic service. */
	volatile struct UICR_SECURESTORAGE_SIZES CRYPTO;
	/** Secure Storage partitions for the Internal Trusted Storage service. */
	volatile struct UICR_SECURESTORAGE_SIZES ITS;
};

/**
 * @brief Global domain peripheral configuration
 */
struct UICR_PERIPHCONF {
	/** Enable the global domain peripheral configuration (see @ref ironside_se_uicr_common). */
	volatile uint32_t ENABLE;
	/** Start address of the array of peripheral configuration entries. */
	volatile uint32_t ADDRESS;
	/** Maximum number of peripheral configuration entries. */
	volatile uint32_t MAXCOUNT;
};

/**
 * @brief Global domain MPC configuration
 */
struct UICR_MPCCONF {
	/** Enable the global domain MPC configuration (see @ref ironside_se_uicr_common). */
	volatile uint32_t ENABLE;
	/** Start address of the array of MPC configuration entries. */
	volatile uint32_t ADDRESS;
	/** Maximum number of MPC configuration entries. */
	volatile uint32_t MAXCOUNT;
};

/**
 * @brief Automatic triggers for reset into secondary firmware
 */
struct UICR_SECONDARY_TRIGGER {
	/** Automatic reset triggers for secondary firmware (see @ref ironside_se_uicr_common). */
	volatile uint32_t ENABLE;
	/**
	 * Reset reasons that trigger automatic reset into secondary firmware
	 * (see @ref ironside_se_uicr_secondary_trigger_resetreas).
	 */
	volatile uint32_t RESETREAS;
	volatile const uint32_t RESERVED;
};

/**
 * @defgroup ironside_se_uicr_secondary_trigger_resetreas UICR_SECONDARY_TRIGGER RESETREAS bits
 * @{
 */

/** Bit position for application WDT0 reset reason. */
#define UICR_SECONDARY_TRIGGER_RESETREAS_APPLICATIONWDT0_Pos (0UL)
#define UICR_SECONDARY_TRIGGER_RESETREAS_APPLICATIONWDT0_Msk                                       \
	(0x1UL << UICR_SECONDARY_TRIGGER_RESETREAS_APPLICATIONWDT0_Pos)
/** Bit position for application WDT1 reset reason. */
#define UICR_SECONDARY_TRIGGER_RESETREAS_APPLICATIONWDT1_Pos (1UL)
#define UICR_SECONDARY_TRIGGER_RESETREAS_APPLICATIONWDT1_Msk                                       \
	(0x1UL << UICR_SECONDARY_TRIGGER_RESETREAS_APPLICATIONWDT1_Pos)
/** Bit position for application lockup reset reason. */
#define UICR_SECONDARY_TRIGGER_RESETREAS_APPLICATIONLOCKUP_Pos (3UL)
#define UICR_SECONDARY_TRIGGER_RESETREAS_APPLICATIONLOCKUP_Msk                                     \
	(0x1UL << UICR_SECONDARY_TRIGGER_RESETREAS_APPLICATIONLOCKUP_Pos)
/** Bit position for radio core WDT0 reset reason. */
#define UICR_SECONDARY_TRIGGER_RESETREAS_RADIOCOREWDT0_Pos (5UL)
#define UICR_SECONDARY_TRIGGER_RESETREAS_RADIOCOREWDT0_Msk                                         \
	(0x1UL << UICR_SECONDARY_TRIGGER_RESETREAS_RADIOCOREWDT0_Pos)
/** Bit position for radio core WDT1 reset reason. */
#define UICR_SECONDARY_TRIGGER_RESETREAS_RADIOCOREWDT1_Pos (6UL)
#define UICR_SECONDARY_TRIGGER_RESETREAS_RADIOCOREWDT1_Msk                                         \
	(0x1UL << UICR_SECONDARY_TRIGGER_RESETREAS_RADIOCOREWDT1_Pos)
/** Bit position for radio core lockup reset reason. */
#define UICR_SECONDARY_TRIGGER_RESETREAS_RADIOCORELOCKUP_Pos (8UL)
#define UICR_SECONDARY_TRIGGER_RESETREAS_RADIOCORELOCKUP_Msk                                       \
	(0x1UL << UICR_SECONDARY_TRIGGER_RESETREAS_RADIOCORELOCKUP_Pos)

/** @} */

/**
 * @brief Secondary firmware configuration
 */
struct UICR_SECONDARY {
	/** Enable booting of secondary firmware (see @ref ironside_se_uicr_common). */
	volatile uint32_t ENABLE;
	/** Processor for the secondary firmware (see @ref ironside_se_uicr_secondary_processor). */
	volatile uint32_t PROCESSOR;
	/** Automatic triggers for reset into secondary firmware. */
	volatile struct UICR_SECONDARY_TRIGGER TRIGGER;
	/**
	 * Start address of the secondary firmware (initial secure VTOR after CPU reset);
	 * address bits in @ref ironside_se_uicr_secondary_address.
	 */
	volatile uint32_t ADDRESS;
	/** Protected Memory region for the secondary firmware. */
	volatile struct UICR_PROTECTEDMEM PROTECTEDMEM;
	/** Start a local watchdog timer ahead of the CPU boot. */
	volatile struct UICR_WDTSTART WDTSTART;
	/** Global domain peripheral configuration used when booting the secondary firmware. */
	volatile struct UICR_PERIPHCONF PERIPHCONF;
	/** Global domain MPC configuration used when booting the secondary firmware. */
	volatile struct UICR_MPCCONF MPCCONF;
};

/**
 * @defgroup ironside_se_uicr_secondary_processor UICR_SECONDARY PROCESSOR values
 * @{
 */

/** Boot the application core. */
#define UICR_SECONDARY_PROCESSOR_APPLICATION UICR_ENUM_CHOICE_0
/** Boot the radio core. */
#define UICR_SECONDARY_PROCESSOR_RADIOCORE   UICR_ENUM_CHOICE_1

/** @} */

/**
 * @defgroup ironside_se_uicr_secondary_address UICR_SECONDARY ADDRESS field mask
 * @{
 */

/** Bit mask for SECONDARY ADDRESS field (address bits, 4 kB aligned). */
#ifndef UICR_SECONDARY_ADDRESS_ADDRESS_Msk
#define UICR_SECONDARY_ADDRESS_ADDRESS_Msk (0xFFFFF000UL)
#endif

/** @} */

/**
 * @defgroup ironside_se_uicr_lock UICR LOCK values
 * @{
 */

/** NVR page 0 can be written; not integrity checked by IronSide SE. */
#define UICR_LOCK_PALL_UNLOCKED UICR_MAGIC_ERASE_VALUE
/** NVR page 0 is read-only; integrity checked by IronSide SE on boot. */
#define UICR_LOCK_PALL_LOCKED   (0xFFFFFFFFUL)

/** @} */

/**
 * @defgroup ironside_se_uicr_eraseprotect UICR ERASEPROTECT values
 * @{
 */

/** Erase protection disabled. */
#define UICR_ERASEPROTECT_PALL_UNPROTECTED UICR_MAGIC_ERASE_VALUE
/** Erase protection enabled. */
#define UICR_ERASEPROTECT_PALL_PROTECTED   UICR_PROTECTED

/** @} */

/**
 * @defgroup ironside_se_uicr_policy_mpcconfstage UICR POLICY_MPCCONFSTAGE values
 * @{
 */

/** MPCCONF API stage is set to initialization stage at application boot. */
#define UICR_POLICY_MPCCONFSTAGE_INIT   UICR_ENUM_CHOICE_0
/** MPCCONF API stage is set to normal stage at application boot. */
#define UICR_POLICY_MPCCONFSTAGE_NORMAL UICR_ENUM_CHOICE_1

/** @} */

/**
 * @defgroup ironside_se_uicr_policy_periphconfstage UICR POLICY_PERIPHCONFSTAGE values
 * @{
 */

/** PERIPHCONF API stage is set to initialization stage at application boot. */
#define UICR_POLICY_PERIPHCONFSTAGE_INIT   UICR_ENUM_CHOICE_0
/** PERIPHCONF API stage is set to normal stage at application boot. */
#define UICR_POLICY_PERIPHCONFSTAGE_NORMAL UICR_ENUM_CHOICE_1

/** @} */

/**
 * @defgroup ironside_se_uicr_snapshot_regions UICR snapshot region count
 * @{
 */

/** Maximum configurable snapshot regions. */
#define UICR_SNAPSHOT_REGIONS_MAX_REGIONS (7UL)

/** @} */

/**
 * @defgroup ironside_se_uicr_snapshot_region_fields UICR_SNAPSHOT_REGIONS REGION[] field encoding
 * @{
 */

/** Bit position for the snapshot region size field (in kiB). */
#define UICR_SNAPSHOT_REGIONS_REGION_SIZEKB_Pos  (0UL)
/** Bit mask for the snapshot region size field (in kiB). */
#define UICR_SNAPSHOT_REGIONS_REGION_SIZEKB_Msk  (0xFFFUL << UICR_SNAPSHOT_REGIONS_REGION_SIZEKB_Pos)
/** Bit position for the snapshot region start address field. */
#define UICR_SNAPSHOT_REGIONS_REGION_ADDRESS_Pos (12UL)
/** Bit mask for the snapshot region start address field. */
#define UICR_SNAPSHOT_REGIONS_REGION_ADDRESS_Msk                                                   \
	(0xFFFFFUL << UICR_SNAPSHOT_REGIONS_REGION_ADDRESS_Pos)

/** @} */

/**
 * @brief Snapshot configuration.
 */
struct UICR_SNAPSHOT_REGIONS {
	/** Enable parsing of snapshot regions (see @ref ironside_se_uicr_common). */
	volatile uint32_t ENABLE;
	/** Number of snapshot region entries (at most @ref UICR_SNAPSHOT_REGIONS_MAX_REGIONS). */
	volatile uint32_t COUNT;
	/** Snapshot region entries (see @ref ironside_se_uicr_snapshot_region_fields). */
	volatile uint32_t REGION[UICR_SNAPSHOT_REGIONS_MAX_REGIONS];
};

/**
 * @brief User information configuration region.
 *
 * @note Certain fields do not take effect unless @ref UICR::VERSION is set to a specific version
 * or higher. The minimum version requirement is described per-field.
 *
 * Any fields named "RESERVED" or similar are reserved for future extensions
 * by the IronSide SE and should not be used for other data.
 */
struct UICR {
	/** Version of the UICR format (see @ref ironside_se_uicr_version).
	 *
	 * @note If set to @ref UICR_MAGIC_ERASE_VALUE, it is interpreted as the highest version
	 * supported by the installed IronSide SE.
	 */
	volatile uint32_t VERSION;
	volatile const uint32_t RESERVED;
	/** Lock the UICR from modification (see @ref ironside_se_uicr_lock). */
	volatile uint32_t LOCK;
	volatile const uint32_t RESERVED1;
	/** AP protection. */
	volatile struct UICR_APPROTECT APPROTECT;
	/** ERASEALL protection (see @ref ironside_se_uicr_eraseprotect). */
	volatile uint32_t ERASEPROTECT;
	/** Protected memory region. */
	volatile struct UICR_PROTECTEDMEM PROTECTEDMEM;
	/** Start a local watchdog timer ahead of the CPU boot. */
	volatile struct UICR_WDTSTART WDTSTART;
	volatile const uint32_t RESERVED2;
	/** Secure Storage configuration. */
	volatile struct UICR_SECURESTORAGE SECURESTORAGE;
	volatile const uint32_t RESERVED3[5];
	/** Global domain peripheral configuration. */
	volatile struct UICR_PERIPHCONF PERIPHCONF;
	/** Global domain MPC configuration. */
	volatile struct UICR_MPCCONF MPCCONF;
	/** Secondary firmware configuration. */
	volatile struct UICR_SECONDARY SECONDARY;
	volatile const uint32_t RESERVED4[8];
	/** Snapshot region configuration.
	 *
	 * @since Effective if @ref UICR::VERSION is @ref UICR_VERSION_2_3 or higher.
	 */
	volatile struct UICR_SNAPSHOT_REGIONS SNAPSHOT_REGIONS;
	volatile const uint32_t RESERVED5[60];
	/** MPCCONF API stage at application boot
	 *  (see @ref ironside_se_uicr_policy_mpcconfstage).
	 *
	 * @since Effective if @ref UICR::VERSION is @ref UICR_VERSION_2_2 or higher.
	 */
	volatile uint32_t POLICY_MPCCONFSTAGE;
	/** PERIPHCONF API stage at application boot
	 *  (see @ref ironside_se_uicr_policy_periphconfstage).
	 *
	 * @since Effective if @ref UICR::VERSION is @ref UICR_VERSION_2_1 or higher.
	 */
	volatile uint32_t POLICY_PERIPHCONFSTAGE;
#if !defined(UICR_DEF_OMIT_CUSTOMER)
	/** Reserved for user defined data.
	 *
	 *  This area is also write protected and integrity checked when @ref UICR::LOCK is enabled.
	 */
	volatile uint32_t CUSTOMER[320];
	volatile const uint32_t RESERVED6[44];
#endif
};

/** @} */

#ifdef __cplusplus
}
#endif
#endif /* IRONSIDE_SE_UICR_H_ */
