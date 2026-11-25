/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide Integration Kit - app_version.
 *
 * A minimal non-secure App that prints:
 *   1. its own image version, read from its MCUboot header `ih_ver`; and
 *   2. the IronSide boot report - the versions of IROT / UROT / App / DFU, the
 *      update_applied/update_target/update_status flags, the boot decision, and what IronSide
 *      recorded about the fault that caused the previous reset.
 *
 * It REPORTS that fault rather than stopping on it, unlike the psa_sha256
 * sample: this one's job is to say what IronSide decided this boot, which it
 * cannot do from an idle loop.
 *
 * This is the application used to exercise DFU: sign it at version A, boot it
 * (prints A), then DFU a new copy/candidate signed at version B and boot again.
 * The printed version is the proof of which image runs. Reading the boot report
 * also lets it confirm a UROT/DFU **candidate** update: those slots aren't
 * directly readable by a non-secure image (UROT's is secure NVM), but their
 * version shows up in the report after IROT validates + copies the candidate.
 * IROT's own version comes from the report too, and only from there - IROT is
 * never signed, so it has no image header for anyone to read.
 *
 * The signed App's MCUboot header sits at the App slot base (the Support
 * Package's IRONSIDE_NVM_SLOT0_ADDR; vector table at +IRONSIDE_IMAGE_ENTRY_OFFSET);
 * the signer (-v MAJ.MIN.REV+BUILD) writes the version. The boot report is not
 * in App-readable memory at all - it comes back from IronSide's boot-report
 * secure service, which is why this sample links the client core.
 *
 * Uses the nrfx UARTE *driver* (nrfx_uarte.c, compiled in via this sample's
 * sample.cmake) rather than poking the HAL directly.
 *
 * Bare-metal nrfx, console: UARTE20, TX = P1.16, 115200 8N1, virtual serial port 1.
 */

#include <integration_kit_fault.h>
#include <integration_kit_uart.h>
#include <integration_kit_test.h>
#include <ironside_client/services/boot_report.h>
#include <ironside_client/image.h>
#include <stddef.h>
#include <stdint.h>

/*
 * The App slot address and the MCUboot image-header format are not hardcoded
 * here: the App slot base (IRONSIDE_NVM_SLOT0_ADDR) and the header layout
 * (struct image_header / IRONSIDE_IMAGE_MAGIC) come from the IronSide TE
 * Client (ironside_client/image.h, which pulls in ironside_client/layout.h),
 * the one place these system facts are defined for a non-secure App.
 */

/* "vMAJ.MIN.PATCH+TWEAK", the spelling the DFU runner greps for. */
#define VER_FMT "v%u.%u.%u+%u"
#define VER_ARGS(v)                                                                \
	(unsigned int)(v).major, (unsigned int)(v).minor, (unsigned int)(v).patch, \
		(unsigned int)(v).tweak

static void print_own_version(void)
{
	const volatile struct image_header *hdr = ironside_app_image_header();

	if (!ironside_image_header_valid(hdr)) {
		integration_kit_uart_puts(
			"app_version: no MCUboot header at App slot (unsigned image?)\r\n");
		integration_kit_uart_puts(INTEGRATION_KIT_TEST_FAIL_LINE);
		integration_kit_uart_puts(INTEGRATION_KIT_TEST_FINISHED_LINE("app_version"));
		for (;;) {
			__asm__ volatile("wfi");
		}
	}

	integration_kit_uart_printf("app_version: " VER_FMT "\r\n",
				    (unsigned int)hdr->ih_ver.iv_major,
				    (unsigned int)hdr->ih_ver.iv_minor,
				    (unsigned int)hdr->ih_ver.iv_revision,
				    (unsigned int)hdr->ih_ver.iv_build_num);
}

static void print_boot_report(void)
{
	struct ironside_boot_report r;
	size_t report_length = 0U;

	/*
	 * The report lives in IronSide's secure RAM, so it is fetched over the NSC
	 * boundary rather than read directly.
	 */
	if (ironside_boot_report_get(&r, sizeof(r), &report_length) != 0) {
		integration_kit_uart_puts("boot report: absent\r\n");
		return;
	}

	/*
	 * One compatibility check for the whole report. IronSide writes exactly the
	 * bytes it reports, so a length short of the capacity offered says some field
	 * of this struct was never written: this build is newer than the IronSide on
	 * the device. Nothing below is printed on that reading -- the fields past the
	 * report hold whatever this image left in the buffer, and even zeros there
	 * would be positive claims: a `booted_image` of 0 says nothing was bootable,
	 * a `dfu_reason` of 0 says the App booted, a `fault.present` of 0 says the
	 * previous boot ended cleanly.
	 */
	if (report_length != sizeof(r)) {
		integration_kit_uart_printf("boot report: short (%u of %u bytes)\r\n",
					    (unsigned int)report_length, (unsigned int)sizeof(r));
		return;
	}

	/* Versions line - the interesting bit for DFU verification. */
	integration_kit_uart_printf("boot report: irot " VER_FMT "  urot " VER_FMT "  app " VER_FMT
				    "  dfu " VER_FMT "\r\n",
				    VER_ARGS(r.irot_version), VER_ARGS(r.urot_version),
				    VER_ARGS(r.app_version), VER_ARGS(r.dfu_version));

	/* Flags line - whether a candidate was applied this boot, where, and the
	 * outcome of the update check (IRONSIDE_UPDATE_STATUS_*). */
	integration_kit_uart_printf("  flags: update_applied=%u target=0x%x update_status=%u\r\n",
				    (unsigned int)r.update_applied, (unsigned int)r.update_target,
				    (unsigned int)r.update_status);

	/* Decision line - which image IronSide booted, the single reason it picked
	 * the DFU image if it did (IRONSIDE_DFU_REASON_*, one value and never a
	 * mask), and the lifecycle state as a PSA psa_lifecycle value. */
	integration_kit_uart_printf("  boot: image=%u dfu_reason=%u lcs=0x%x\r\n",
				    (unsigned int)r.booted_image, (unsigned int)r.dfu_reason,
				    (unsigned int)r.lifecycle);

	/* RAM line - the non-secure RAM window in effect. The base follows the
	 * key-slot reservation in IronSide's base configuration, so an App built for
	 * another reservation would not have reached this line. */
	integration_kit_uart_printf("  ram: ns=0x%x size=0x%x\r\n", (unsigned int)r.ram_ns.addr,
				    (unsigned int)r.ram_ns.size);

	/*
	 * What IronSide recorded about the fault that caused the previous reset, if
	 * there was one it will describe. integration_kit_fault_print(), not
	 * integration_kit_fault_handle(): this sample reports the fault and carries on to
	 * its markers, where the handler would print the same record and stop.
	 */
	integration_kit_fault_print(&r.fault);
}

int main(void)
{
	integration_kit_uart_init();
	print_own_version();
	print_boot_report();

	integration_kit_uart_puts(INTEGRATION_KIT_TEST_SUCCESS_LINE("app_version"));
	integration_kit_uart_puts(INTEGRATION_KIT_TEST_FINISHED_LINE("app_version"));

	for (;;) {
		__asm__ volatile("wfi");
	}
}
