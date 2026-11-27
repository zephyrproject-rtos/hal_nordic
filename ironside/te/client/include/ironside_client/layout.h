/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide TE - system memory layout (NVM + RAM) as seen by a non-secure App.
 *
 * NVM slot addresses/sizes and the secure/non-secure RAM boundary - the one
 * place a non-secure App looks up "where is my flash slot / my RAM / the secure
 * boundary".
 *
 * The layout depends on the IronSide bundle the App runs under: its partition
 * sizes and the secure RAM it reserves for PSA key slots. This header reads them from
 * ironside_bundle_config.h, which `ironside_bundle.py export-config --app-header` writes
 * from the bundle, and fails to compile without it. Put the directory that holds
 * it on the include path, and regenerate it when the bundle's configuration
 * changes.
 *
 * Shared with the App linker script (samples/lib/app.ld derives its FLASH/RAM
 * regions from here, C-preprocessed by the build), so this header and the
 * generated one stay linker-friendly: no C integer suffixes (U/UL) - ld can't
 * parse them. Every value fits in a 32-bit signed range, so a bare literal is
 * unambiguous for both the C client and ld.
 */

#ifndef IRONSIDE_CLIENT_LAYOUT_H
#define IRONSIDE_CLIENT_LAYOUT_H

#include <ironside_client/internal/layout_abi.h>

#if !__has_include(<ironside_bundle_config.h>)
#error "ironside_client/layout.h needs ironside_bundle_config.h for the IronSide bundle the App runs under: write it with ironside_bundle.py export-config BUNDLE --app-header DIR/ironside_bundle_config.h and add DIR to the include path"
#endif
#include <ironside_bundle_config.h>

#if IRONSIDE_BASECONF_KEYSLOTSTORAGESIZE < IRONSIDE_KEY_SLOT_STORAGE_SIZE_MIN || \
	IRONSIDE_BASECONF_KEYSLOTSTORAGESIZE > IRONSIDE_KEY_SLOT_STORAGE_SIZE_MAX
#error "IRONSIDE_BASECONF_KEYSLOTSTORAGESIZE must be 1536 B..41.5 KB"
#endif
#if (IRONSIDE_BASECONF_KEYSLOTSTORAGESIZE - IRONSIDE_KEY_SLOT_STORAGE_SIZE_MIN) % \
		IRONSIDE_RAM_SPLIT_GRANULE !=                                     \
	0
#error "IRONSIDE_BASECONF_KEYSLOTSTORAGESIZE must be 1536 B plus a multiple of this part's IRONSIDE_RAM_SPLIT_GRANULE"
#endif

/* ---- RAM: the non-secure base follows the bundle's key slot storage size ---- */
#define IRONSIDE_PSA_KEY_SLOTS   IRONSIDE_PSA_KEY_SLOTS_FOR(IRONSIDE_BASECONF_KEYSLOTSTORAGESIZE)
#define IRONSIDE_RAM_NS_ADDR     IRONSIDE_RAM_NS_ADDR_FOR(IRONSIDE_BASECONF_KEYSLOTSTORAGESIZE)
#define IRONSIDE_RAM_SECURE_SIZE (IRONSIDE_RAM_NS_ADDR - IRONSIDE_RAM_SECURE_ADDR)

/*
 * NVM partition addresses: DFUIMAGE/UROTSTORAGE/APPSTORAGE/SLOT1 placed
 * back-to-back down from the end of RRAM (fixed order) at the bundle's sizes,
 * and SLOT0 as the implicit remainder up to SLOT1. End-anchored so the
 * arithmetic is the same on every part.
 */
#define IRONSIDE_NVM_SLOT1_SIZE       (IRONSIDE_BASECONF_PARTITIONS_SLOT1_SIZE4KB * 4096)
#define IRONSIDE_NVM_APPSTORAGE_SIZE  (IRONSIDE_BASECONF_PARTITIONS_APPSTORAGE_SIZE4KB * 4096)
#define IRONSIDE_NVM_UROTSTORAGE_SIZE (IRONSIDE_BASECONF_PARTITIONS_UROTSTORAGE_SIZE4KB * 4096)
#define IRONSIDE_NVM_DFUIMAGE_SIZE    (IRONSIDE_BASECONF_PARTITIONS_DFUIMAGE_SIZE4KB * 4096)
#define IRONSIDE_NVM_DFUIMAGE_ADDR    (IRONSIDE_NVM_END - IRONSIDE_NVM_DFUIMAGE_SIZE)
#define IRONSIDE_NVM_UROTSTORAGE_ADDR (IRONSIDE_NVM_DFUIMAGE_ADDR - IRONSIDE_NVM_UROTSTORAGE_SIZE)
#define IRONSIDE_NVM_APPSTORAGE_ADDR  (IRONSIDE_NVM_UROTSTORAGE_ADDR - IRONSIDE_NVM_APPSTORAGE_SIZE)
#define IRONSIDE_NVM_SLOT1_ADDR       (IRONSIDE_NVM_APPSTORAGE_ADDR - IRONSIDE_NVM_SLOT1_SIZE)
#define IRONSIDE_NVM_SLOT0_SIZE       (IRONSIDE_NVM_SLOT1_ADDR - IRONSIDE_NVM_SLOT0_ADDR)

#endif /* IRONSIDE_CLIENT_LAYOUT_H */
