/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide TE - the memory layout every bundle shares: fixed NVM anchors, the
 * per-part NVM and RAM size, the RAM split's fixed part and granule, and the
 * placement and per-slot cost of the PSA key slots.
 *
 * Internal to the Client: included by <ironside_client/layout.h>, which adds the
 * bundle's configuration and is what an App includes. The secure side builds
 * against this header alone. ld preprocesses it too, so it holds no #includes
 * and no C integer suffixes.
 */

#ifndef IRONSIDE_CLIENT_INTERNAL_LAYOUT_ABI_H
#define IRONSIDE_CLIENT_INTERNAL_LAYOUT_ABI_H

/* ---- NVM (RRAM): fixed anchors shared by every supported part ------------- */
#define IRONSIDE_NVM_IROT_ADDR         0x00000000
#define IRONSIDE_NVM_IROT_STORAGE_ADDR (IRONSIDE_NVM_IROT_ADDR + IRONSIDE_NVM_IROT_SIZE)
#define IRONSIDE_NVM_IROT_STORAGE_SIZE (1 * 1024)
#define IRONSIDE_NVM_UROT_ADDR         (IRONSIDE_NVM_IROT_STORAGE_ADDR + IRONSIDE_NVM_IROT_STORAGE_SIZE)
#define IRONSIDE_NVM_UROT_SIZE         (IRONSIDE_NVM_SLOT0_ADDR - IRONSIDE_NVM_UROT_ADDR)
#define IRONSIDE_NVM_SLOT0_ADDR        0x00023000 /* App slot base (NS), right after UROT */
#define IRONSIDE_FICR_ADDR             0x00FFC000 /* factory info; NS-attributed (MPC/SAU)  */

/* ---- RAM: the split shared by every supported part ------------------------ */
#define IRONSIDE_RAM_SECURE_ADDR 0x20000000  /* secure RAM         */
#define IRONSIDE_RAM_ROT_SIZE    (32 * 1024) /* ROT-RAM, IronSide's own, fixed */

/*
 * The PSA key slots. One array of IRONSIDE_PSA_KEY_SLOT_BUDGET bytes per slot
 * starts at IRONSIDE_PSA_KEY_ARRAY_ADDR, IRONSIDE_PSA_BASE_KEY_SLOTS slots below
 * the end of ROT-RAM, so the base slots every device has sit inside ROT-RAM and
 * cost the App nothing. A bundle sets the key slot storage size, the bytes the
 * array spans: at least IRONSIDE_KEY_SLOT_STORAGE_SIZE_MIN, the base slots, and
 * anything above that is secure RAM reserved above ROT-RAM, a multiple of
 * IRONSIDE_RAM_SPLIT_GRANULE. The array holds every whole slot that fits, and
 * the non-secure RAM starts where it ends.
 *
 * The slot budget is the key slot structure rounded up to the 32-byte granule
 * the SAU and the secure MPU program in. The App's RAM base follows from the
 * storage size alone, and the slot count from the storage size and this
 * constant, so changing it changes every configured App's slot count.
 */
#define IRONSIDE_PSA_KEY_SLOT_BUDGET    192
#define IRONSIDE_PSA_KEY_REGION_SIZE(n) ((n) * IRONSIDE_PSA_KEY_SLOT_BUDGET)
#define IRONSIDE_PSA_BASE_KEY_SLOTS     8
#define IRONSIDE_PSA_KEY_ARRAY_ADDR                         \
	(IRONSIDE_RAM_SECURE_ADDR + IRONSIDE_RAM_ROT_SIZE - \
	 IRONSIDE_PSA_KEY_REGION_SIZE(IRONSIDE_PSA_BASE_KEY_SLOTS))

/* The smallest and largest key slot storage size any bundle can set. The
 * largest reserves 40 kB above ROT-RAM, a multiple of every part's
 * IRONSIDE_RAM_SPLIT_GRANULE. */
#define IRONSIDE_KEY_SLOT_STORAGE_SIZE_MIN IRONSIDE_PSA_KEY_REGION_SIZE(IRONSIDE_PSA_BASE_KEY_SLOTS)
#define IRONSIDE_KEY_SLOT_STORAGE_SIZE_MAX (IRONSIDE_KEY_SLOT_STORAGE_SIZE_MIN + (40 * 1024))

/* The PSA key slots a key slot storage size of @p s bytes holds. */
#define IRONSIDE_PSA_KEY_SLOTS_FOR(s) ((s) / IRONSIDE_PSA_KEY_SLOT_BUDGET)
#define IRONSIDE_PSA_KEY_SLOTS_MAX    IRONSIDE_PSA_KEY_SLOTS_FOR(IRONSIDE_KEY_SLOT_STORAGE_SIZE_MAX)

/* The non-secure RAM start a key slot storage size of @p s bytes gives, and the
 * highest one any bundle can give. Non-secure RAM at or above
 * IRONSIDE_RAM_NS_ADDR_MAX is non-secure under every configuration. */
#define IRONSIDE_RAM_NS_ADDR_FOR(s) (IRONSIDE_PSA_KEY_ARRAY_ADDR + (s))
#define IRONSIDE_RAM_NS_ADDR_MAX    IRONSIDE_RAM_NS_ADDR_FOR(IRONSIDE_KEY_SLOT_STORAGE_SIZE_MAX)

/* Default APP-XRAM: a small executable non-secure RAM window carved from the top of
 * NS RAM (end-anchored at the top of the non-secure window). The App opts in at
 * runtime: it loads its RAM-resident code into these bytes while they are still plain
 * writable APP-RAM, then calls
 * ironside_devmgmt_xram_region_define(IRONSIDE_RAM_XRAM_SIZE) (ironside_client/services/devmgmt.h),
 * which takes write away and makes the window read+execute - W^X for RAM-resident
 * non-secure code, in one one-shot step. Without that call there is no APP-XRAM at
 * all. The App linker script always RESERVES this window (below) so the App
 * stack/heap never land in it, whether or not the image opts in. */
#define IRONSIDE_RAM_XRAM_SIZE   (4 * 1024)
#define IRONSIDE_RAM_XRAM_ADDR   (IRONSIDE_RAM_NS_END - IRONSIDE_RAM_XRAM_SIZE)
/* Top of the App's ordinary data/heap/stack RAM - one past the last non-XRAM byte. */
#define IRONSIDE_RAM_NS_DATA_END IRONSIDE_RAM_XRAM_ADDR

/*
 * IROT size, end of RRAM (the layout's top anchor), the RAM ends and the RAM
 * split granule. Keyed on the MDK device macro the bare-metal build defines
 * (nRF54LM20B uses the A headers). The nRF54L15 IROT carries the CRACEN
 * microcode, which the other parts have in ROM, so its UROT slot is smaller.
 *
 * IRONSIDE_RAM_END is where App RAM ends and IRONSIDE_RAM_PHYSICAL_END where the
 * part's RAM ends. On the nRF54LM20 and the nRF54LV10A the 1 kB between them is
 * non-secure but not App RAM: it holds the VPR saved context (VPR00_VPRSAVEADDR),
 * which the FLPR writes when it hibernates, and the CRACEN ProtectedRAM, which a
 * hardware MPC override refuses to every bus master but the KMU and CRACEN. An
 * App must not link anything there.
 *
 * IRONSIDE_RAM_SPLIT_GRANULE is the step the secure/non-secure RAM split moves
 * in: the coarsest of the MPC override's granularity (16 bytes on the nRF54LM20,
 * 32 bytes on the nRF54LV10A, 4 kB on the nRF54L15) and the SAU and secure MPU's
 * 32 bytes.
 */
#if defined(NRF54LM20A_XXAA) || defined(NRF54LM20B_XXAA)
#define IRONSIDE_NVM_IROT_SIZE     (15 * 1024)
#define IRONSIDE_NVM_END           0x001FD000 /* 2036 KB total      */
#define IRONSIDE_RAM_END           0x2007FC00 /* 511 KB             */
#define IRONSIDE_RAM_PHYSICAL_END  0x20080000 /* 512 KB          */
#define IRONSIDE_RAM_SPLIT_GRANULE 32
#elif defined(NRF54LV10A_XXAA)
#define IRONSIDE_NVM_IROT_SIZE     (15 * 1024)
#define IRONSIDE_NVM_END           0x000FD000 /* 1012 KB total      */
#define IRONSIDE_RAM_END           0x2002FC00 /* 191 KB             */
#define IRONSIDE_RAM_PHYSICAL_END  0x20030000 /* 192 KB          */
#define IRONSIDE_RAM_SPLIT_GRANULE 32
#elif defined(NRF54L15_XXAA)
#define IRONSIDE_NVM_IROT_SIZE     (19 * 1024)
#define IRONSIDE_NVM_END           0x0017D000 /* 1524 KB total      */
#define IRONSIDE_RAM_END           0x20040000 /* 256 KB             */
#define IRONSIDE_RAM_PHYSICAL_END  IRONSIDE_RAM_END
#define IRONSIDE_RAM_SPLIT_GRANULE 4096
#else
#error "ironside_client/layout.h: unknown SoC - define the MDK device macro (e.g. NRF54LM20A_XXAA)"
#endif

/* End of the non-secure App RAM window (one past the last byte). */
#define IRONSIDE_RAM_NS_END IRONSIDE_RAM_END

#endif /* IRONSIDE_CLIENT_INTERNAL_LAYOUT_ABI_H */
