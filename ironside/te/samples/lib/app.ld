/* SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide Integration Kit sample linker script - non-secure App image.
 *
 * Part-independent, and deliberately named so. Every address in it is derived
 * from <ironside_client/layout.h>, which resolves them from the MDK device macro
 * the build defines (NRF54LM20A_XXAA, NRF54LV10A_XXAA, ...) -- so one script
 * serves every supported part and the part is chosen by that -D, not by picking a
 * different file. It was called nrf54lm20a_app.ld while only one part was
 * supported; the name claimed a specificity the contents never had.
 *
 * The App's FLASH and RAM regions are DERIVED from the single source of truth,
 * the client's memory map (ironside_client/layout.h), rather than hardcoded
 * here. ld cannot #include a C header or expand macros, so the build runs this
 * file through the C preprocessor first (see integration_kit/CMakeLists.txt) with the
 * client on the include path and the MDK device macro defined.
 *
 * Secure boot: the App is signed (scripts/sign.py) into an MCUboot image whose
 * 32-byte header sits at the slot base. UROT boots the vector table at the fixed
 * entry offset, IRONSIDE_IMAGE_ENTRY_OFFSET (0x800), so the image links at
 * IRONSIDE_NVM_SLOT0_ADDR + APP_ENTRY_OFFSET, and sign.py --pad-header prepends the
 * gap. Because the slot base holds the MCUboot header, the raw (unsigned) .hex
 * must be signed before flashing.
 *
 * RAM is the NON-SECURE region only (IRONSIDE_RAM_NS_ADDR .. IRONSIDE_RAM_NS_END):
 * UROT keeps the low 32 KB secure and hands the rest to the App (UROT's own
 * MPC/SAU split). Putting App data in secure RAM would fault after the handover.
 *
 * The top IRONSIDE_RAM_XRAM_SIZE bytes of NS RAM are RESERVED as APP-XRAM - the
 * executable window an App asks IronSide for at runtime, once it has loaded code
 * into those (still writable) bytes; see the .app_xram section below.
 * RAM (the App's data/heap/stack) therefore ends at
 * IRONSIDE_RAM_XRAM_ADDR, so the stack (placed at the top of RAM by nrf_common.ld)
 * never lands in APP-XRAM. Reserving it unconditionally costs an image that does
 * not use APP-XRAM only those bytes of otherwise-unused high RAM.
 *
 * Section definitions are reused from the nrfx MDK's nrf_common.ld - add the MDK
 * directory to the linker search path with -L <nrfx>/bsp/stable/mdk. Startup
 * (gcc_startup_<device>_application.S) comes from that same MDK.
 */

#include <ironside_client/layout.h>

/* IRONSIDE_IMAGE_ENTRY_OFFSET: the vector table links this far into the App slot. */
#define APP_ENTRY_OFFSET 0x800

MEMORY
{
  FLASH (rx) : ORIGIN = IRONSIDE_NVM_SLOT0_ADDR + APP_ENTRY_OFFSET,
               LENGTH = IRONSIDE_NVM_SLOT0_SIZE - APP_ENTRY_OFFSET
  RAM  (rwx) : ORIGIN = IRONSIDE_RAM_NS_ADDR,
               LENGTH = IRONSIDE_RAM_NS_DATA_END - IRONSIDE_RAM_NS_ADDR
  XRAM (rwx) : ORIGIN = IRONSIDE_RAM_XRAM_ADDR,
               LENGTH = IRONSIDE_RAM_XRAM_SIZE
}

INCLUDE "nrf_common.ld"

/*
 * APP-XRAM: RAM-resident non-secure code. An image places a function (and any
 * data it needs) here via __attribute__((section(".app_xram"))); the bytes have
 * their VMA in XRAM but a LOAD image in FLASH, appended past all of nrf_common.ld's
 * RAM-init load images (__fast_load_end is the last of them). The App copies the
 * section from its flash LMA into XRAM at runtime WHILE THOSE BYTES ARE STILL PLAIN
 * WRITABLE RAM, then asks IronSide to define the XRAM region - which is what makes
 * it RX - before calling into it (see the region define call in the client's
 * device-management API, referenced from layout.h). This is a separate
 * SECTIONS block (GNU ld merges it with the one nrf_common.ld defines); assigning
 * it to the XRAM region keeps it out of the RAM location counter, and the explicit
 * AT() keeps its flash LOAD image from colliding with the .data LOAD image (which
 * also starts at __etext but lives in the RAM region's VMA space).
 */
SECTIONS
{
  .app_xram : AT(ALIGN(__fast_load_end, 4))
  {
    . = ALIGN(4);
    __app_xram_start__ = .;
    KEEP(*(.app_xram))
    KEEP(*(.app_xram.*))
    . = ALIGN(4);
    __app_xram_end__ = .;
  } > XRAM
  __app_xram_load_start__ = LOADADDR(.app_xram);
}
