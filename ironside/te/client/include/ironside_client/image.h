/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide TE Client - MCUboot image format (non-secure App view).
 *
 * Every image in the IronSide TE partition map (UROT / App / DFU) is a signed
 * MCUboot image: the slot base holds an `image_header`, whose `ih_ver` is the
 * version a non-secure App can read back to prove which build is running. This
 * header lets an App parse its own MCUboot header (or peek a peer slot's) by the
 * struct layout alone.
 */

#ifndef IRONSIDE_CLIENT_IMAGE_H
#define IRONSIDE_CLIENT_IMAGE_H

#include <stdbool.h>
#include <stdint.h>

#include <ironside_client/internal/layout_abi.h>

/* MCUboot image magic - the first word of every valid image header. */
#define IRONSIDE_IMAGE_MAGIC 0x96f3b83dUL

/*
 * IronSide TE boots an image from its vector table this far into its slot, so the
 * image links its vector table at slot_base + this. The 32-byte MCUboot header is
 * at the slot base; the bytes between the two belong to the image body.
 */
#define IRONSIDE_IMAGE_ENTRY_OFFSET 0x800U

/* mirrors MCUboot ih_ver */
struct image_version {
	uint8_t iv_major;
	uint8_t iv_minor;
	uint16_t iv_revision;
	uint32_t iv_build_num;
} __attribute__((packed));

struct image_header {
	uint32_t ih_magic;
	uint32_t ih_load_addr;
	uint16_t ih_hdr_size;
	uint16_t ih_protect_tlv_size;
	uint32_t ih_img_size;
	uint32_t ih_flags;
	struct image_version ih_ver;
	uint32_t _pad1;
} __attribute__((packed));

/* True if @hdr begins with the MCUboot magic - cheap first-pass validity check. */
static inline bool ironside_image_header_valid(const volatile struct image_header *hdr)
{
	return hdr->ih_magic == (uint32_t)IRONSIDE_IMAGE_MAGIC;
}

/*
 * The non-secure App's own MCUboot header, at the App slot base. Volatile
 * because it points into NVM the App only ever reads (and never via the
 * compiler's aliasing assumptions).
 */
static inline const volatile struct image_header *ironside_app_image_header(void)
{
	return (const volatile struct image_header *)(uintptr_t)IRONSIDE_NVM_SLOT0_ADDR;
}

#endif /* IRONSIDE_CLIENT_IMAGE_H */
