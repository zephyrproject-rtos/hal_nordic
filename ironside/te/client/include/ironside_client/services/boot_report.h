/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide TE Client - boot report.
 *
 * ironside_boot_report_get() fetches the IronSide TE boot report: which non-secure
 * image IronSide TE booted and why, the device's lifecycle state, the outcome of
 * this boot's update check, the version of each image in the boot chain
 * (IROT / UROT / App / DFU), what IronSide TE recorded about the CPU fault that
 * caused the previous reset, the reset reason, and the non-secure partition
 * layout.
 *
 * The fault report is the only way an application learns why it restarted after a
 * fault. BusFault, HardFault and SecureFault are always taken to the secure side.
 * MemManage and UsageFault reach a non-secure handler if the application enables
 * it; otherwise they escalate to HardFault.
 */

#ifndef IRONSIDE_CLIENT_SERVICES_BOOT_REPORT_H
#define IRONSIDE_CLIENT_SERVICES_BOOT_REPORT_H

#include <stddef.h>
#include <stdint.h>

/**
 * Version of an image: the four fields a version has, one byte each, spelled the
 * way the release packages spell them. A field too large for its byte is clamped
 * to 255 rather than truncated. All-zero = none reported.
 */
struct ironside_image_version {
	uint8_t major;
	uint8_t minor;
	uint8_t patch;
	uint8_t tweak;
};

/** Which non-secure image IronSide TE booted (`booted_image`). */
#define IRONSIDE_BOOTED_IMAGE_NONE 0U /* nothing bootable - unreachable by a reader */
#define IRONSIDE_BOOTED_IMAGE_APP  1U
#define IRONSIDE_BOOTED_IMAGE_DFU  2U

/*
 * Why the DFU image was booted (`dfu_reason`) - one of these, never a mask. The
 * boot decision stops at the first condition that selects the DFU image, so this
 * is the condition that decided the boot rather than a list of everything that
 * was true. Do not test it with `&`.
 */
#define IRONSIDE_DFU_REASON_NONE           0U /* the App booted */
#define IRONSIDE_DFU_REASON_ADAC_BOOT_MODE 1U /* ADAC BOOTMODE_SET, from a debugger  */
#define IRONSIDE_DFU_REASON_REQUEST_SERVICE \
	2U /* a non-secure image asked (ironside_dfu_request_set) */
#define IRONSIDE_DFU_REASON_APP_SIGNATURE     3U /* App signature/header rejected       */
#define IRONSIDE_DFU_REASON_APP_ROLLBACK      4U /* App below its rollback counter      */
#define IRONSIDE_DFU_REASON_APP_UNAVAILABLE   5U /* no App image, or its counter is unusable */
#define IRONSIDE_DFU_REASON_TRIGGER_RESETREAS 6U /* configured reset-reason trigger fired */
#define IRONSIDE_DFU_REASON_TRIGGER_GPIO      7U /* configured GPIO sampled active      */
#define IRONSIDE_DFU_REASON_TRIGGER_FAULT     8U /* fault trigger, fault report latched */

/*
 * The outcome of this boot's update check (`update_status`) - one of these. At
 * most one secure image finds a candidate it owns in a boot, so this is that
 * check's result. `update_applied` is 1 exactly when this is APPLIED.
 */
#define IRONSIDE_UPDATE_STATUS_NONE               0U /* no update candidate          */
#define IRONSIDE_UPDATE_STATUS_APPLIED            1U /* copied into its slot         */
#define IRONSIDE_UPDATE_STATUS_REJECTED_SIGNATURE 2U /* signature rejected; kept     */
#define IRONSIDE_UPDATE_STATUS_REJECTED_ROLLBACK  3U /* counter too low; discarded   */
#define IRONSIDE_UPDATE_STATUS_REJECTED_SIZE      4U /* larger than a slot; discarded */
#define IRONSIDE_UPDATE_STATUS_REJECTED_SEC_CNT   5U /* no valid counter; discarded  */
#define IRONSIDE_UPDATE_STATUS_COUNTER_STORE      6U /* counter unreadable; kept     */
#define IRONSIDE_UPDATE_STATUS_WRITE_FAILED       7U /* copy failed; kept            */

/*
 * ---- Fault report ----------------------------------------------------------
 *
 * What IronSide TE recorded about the fault that caused the previous reset, if there
 * was one it is willing to describe.
 *
 * Two sources, one record, and `source` says which. A CPU exception -- BusFault,
 * MemManage, UsageFault, SecureFault or HardFault -- carries the architectural
 * fault status and the non-secure exception frame. An MPC memory access error
 * carries the address a bus master was refused and the kind of access it
 * attempted; it is an interrupt rather than an exception, so `exception` reads
 * NONE and every CPU-exception field reads 0. The second source is the only way
 * a refused DMA transfer is visible at all: a DMA engine's bus error goes back
 * to the engine rather than to the processor, and several of this chip's engines
 * have no error reporting of their own.
 *
 * When `present` is 0 the reasons are deliberately not distinguished: this boot
 * does not follow a fault; or the fault was inside IronSide TE, which reports
 * nothing about itself; or the reset that ended the faulting boot does not
 * retain RAM (a pin reset, a watchdog reset and a power cycle all clear it); or
 * it was a fault IronSide TE cannot attribute honestly. Every field below is then 0.
 *
 * What is not here, and never will be: IronSide TE's own program counter, link
 * register, stack pointer or registers; anything about its memory map, security
 * attribution or MPU configuration; floating-point registers; which bus master
 * port a refused transaction arrived on, or its owner id.
 *
 * Accuracy is best-effort by design. What is guaranteed is that producing the
 * report cannot have compromised IronSide TE. `flags` is load-bearing for a reader:
 * FRAME_UNAVAILABLE means `frame` and `frame_sp` were never captured, so a
 * consumer that ignores it will read zeros as a program counter.
 */

/** Stacking failed, so `frame` and `frame_sp` were never captured. */
#define IRONSIDE_FAULT_FLAG_FRAME_UNAVAILABLE (1UL << 0)
/** Exception-return unstacking failed, so any frame is not the expected one. */
#define IRONSIDE_FAULT_FLAG_UNSTACK_ERROR     (1UL << 1)
/**
 * The fault escalated to HardFault, because the fault it started as was disabled
 * or priority-blocked. One event, not two: the original cause is still in `cfsr`
 * (or `sfsr`), and `exception` reads HardFault.
 */
#define IRONSIDE_FAULT_FLAG_ESCALATED         (1UL << 2)

/*
 * Values of `exception`: the architectural exception number of the vector that
 * fired. Only these five can appear -- an NMI or a spurious system exception
 * resets the device without a report, because neither is a fault the
 * application caused -- plus NONE, which means no vector fired at all because
 * the record came from the MPC rather than from an exception.
 */
#define IRONSIDE_FAULT_EXC_NONE         0U
#define IRONSIDE_FAULT_EXC_HARD_FAULT   3U
#define IRONSIDE_FAULT_EXC_MEM_MANAGE   4U
#define IRONSIDE_FAULT_EXC_BUS_FAULT    5U
#define IRONSIDE_FAULT_EXC_USAGE_FAULT  6U
#define IRONSIDE_FAULT_EXC_SECURE_FAULT 7U

/*
 * The `sfsr` bits that can appear. The others describe a secure-stack or
 * secure-branch failure -- IronSide TE's own bug -- and a fault carrying one is not
 * reported at all, so a reader never sees them. `cfsr` follows the architecture's
 * own CFSR layout; use the CMSIS SCB_CFSR_* names for it.
 */
#define IRONSIDE_FAULT_SFSR_INVEP     (1UL << 0) /* a call to a secure non-SG address */
#define IRONSIDE_FAULT_SFSR_AUVIOL    (1UL << 3) /* an access to a secure address     */
#define IRONSIDE_FAULT_SFSR_SFARVALID (1UL << 6) /* `sfar` holds the faulting address */

/*
 * Values of `source`: which of the two paths above produced this record. Zero is
 * CPU_EXCEPTION, so an absent report reads as one like every other field.
 */
#define IRONSIDE_FAULT_SOURCE_CPU_EXCEPTION 0U
#define IRONSIDE_FAULT_SOURCE_MPC_MEMACCERR 1U

/*
 * The kind of access the refused bus transaction attempted, in `mpc_access`. At
 * least one bit is set. These are IronSide TE's own values, not the hardware status
 * word, which also carries fields that are not published.
 */
#define IRONSIDE_FAULT_MPC_ACCESS_READ    (1UL << 0)
#define IRONSIDE_FAULT_MPC_ACCESS_WRITE   (1UL << 1)
#define IRONSIDE_FAULT_MPC_ACCESS_EXECUTE (1UL << 2)

struct ironside_fault_report {
	uint32_t present; /* 0 = no fault IronSide TE will describe; see above    */
	uint32_t source;  /* IRONSIDE_FAULT_SOURCE_* -- read this one first    */
	uint32_t flags;   /* IRONSIDE_FAULT_FLAG_*                             */
	/* Faults recorded over the device's life, saturating. Resets to 0 only on
	 * a power cycle. */
	uint32_t count;

	/*
	 * ---- IRONSIDE_FAULT_SOURCE_CPU_EXCEPTION, all zero otherwise --------
	 *
	 * Architectural fault status, captured before anything decoded it.
	 *
	 * `cfsr`, `bfar`, `mmfar`, `sfsr` and `sfar` are the secure copies, and
	 * they are publishable because the fault came from non-secure state: every
	 * bit in them was set by an access or instruction the application
	 * executed (IronSide TE clears them at boot and every fault resets the
	 * device), and the secure MPU is not consulted for a non-secure access, so
	 * no secure configuration can appear in them. The addresses are ones the
	 * application's own instruction formed.
	 *
	 * An address register reads 0 unless its own valid bit was set, so there
	 * is no stale value to mistake for a live one.
	 */
	uint32_t exception; /* IRONSIDE_FAULT_EXC_*                            */
	uint32_t cfsr;
	uint32_t bfar;  /* 0 unless CFSR.BFARVALID                            */
	uint32_t mmfar; /* 0 unless CFSR.MMARVALID                            */
	uint32_t sfsr;  /* SecureFault status; only AUVIOL/INVEP can appear    */
	uint32_t sfar;  /* 0 unless SFSR.SFARVALID                            */
	/*
	 * The non-secure banked copies (CFSR and MMFAR), verbatim -- the
	 * application's own data. These are what keeps an escalated MemManage or
	 * UsageFault diagnosable: those faults are banked, so when the
	 * application has not enabled its own handler they escalate to the secure
	 * side and their cause is recorded here rather than in `cfsr`.
	 */
	uint32_t cfsr_ns;
	uint32_t mmfar_ns; /* 0 unless CFSR_NS.MMARVALID                      */

	/*
	 * The non-secure exception frame the hardware pushed: R0-R3, R12, LR, PC,
	 * xPSR, and the stack pointer it was read from. Copied at fault time, so
	 * it is the faulting instruction's own frame. Zero, and not to be read, if
	 * FRAME_UNAVAILABLE is set.
	 *
	 * The stacked xPSR carries the interrupted context's exception number in
	 * its low bits, so whether the fault came from thread or handler mode is
	 * recoverable from `frame[7]` without a separate field.
	 */
	uint32_t frame[8];
	uint32_t frame_sp;

	/*
	 * ---- IRONSIDE_FAULT_SOURCE_MPC_MEMACCERR, all zero otherwise --------
	 *
	 * Gate on `source`, not on these being non-zero: a refused transaction to
	 * address 0 is a perfectly ordinary way for an unarmed DMA pointer to fail.
	 *
	 * `mpc_address` is the address the refused transaction named -- the
	 * application's own DMA pointer, so it discloses nothing the application
	 * did not choose, the same argument that admits `sfar` and `bfar` above.
	 */
	uint32_t mpc_address;
	uint32_t mpc_access; /* IRONSIDE_FAULT_MPC_ACCESS_*                     */
};

/**
 * One non-secure NVM partition as IronSide TE placed it at boot. Size 0 = absent; a
 * DFUIMAGE of size 0 means DFU boot mode is disabled.
 */
struct ironside_boot_region {
	uint32_t addr;
	uint32_t size;
};

/*
 * The report carries no layout version, and needs none: IronSide TE promises it only
 * ever GROWS, by appending. A field never moves, never changes meaning and is
 * never removed, and the report is never served shorter than a build served it
 * before. IronSide TE also clamps its write to the buffer it is given rather than
 * rejecting one too small for the report it has.
 *
 * That leaves a caller one compatibility check - the reported length against the
 * capacity it offered:
 *
 *   report_length == report_size   every field of this struct is IronSide TE's.
 *   report_length <  report_size   this header is newer than the IronSide TE on the
 *                                  device. Nothing past report_length was
 *                                  written; treat it as an error, not as a
 *                                  partial report to read fields out of.
 *
 * A longer length cannot happen, because the write is clamped.
 *
 * This layout was rebuilt rather than appended to when the report gained
 * `booted_image`/`dfu_reason`/`lifecycle` and the version fields changed shape,
 * and again when `update_status` was inserted after `update_target`. A client
 * built against an older layout mis-parses this one rather than getting a valid
 * prefix of it, which is the one case the length check cannot catch. Deliberate,
 * while there is no fielded consumer to protect - and not a precedent.
 */
struct ironside_boot_report {
	uint32_t update_applied; /* 1 = an update candidate was applied this boot */
	uint32_t update_target;  /* partition base address of the applied update,
				  * or 0 when update_applied == 0                */
	uint32_t update_status;  /* IRONSIDE_UPDATE_STATUS_*                     */
	uint32_t booted_image;   /* IRONSIDE_BOOTED_IMAGE_*                      */
	uint32_t dfu_reason;     /* IRONSIDE_DFU_REASON_*; NONE when the App booted */
	uint32_t lifecycle;      /* PSA psa_lifecycle value, zero-extended: 0x1000
				  * Assembly and Test, 0x2000 RoT Provisioning,
				  * 0x3000 Secured, 0x4000 RoT Debug             */
	struct ironside_image_version irot_version;
	struct ironside_image_version urot_version;
	struct ironside_image_version app_version;
	struct ironside_image_version dfu_version;
	/* What IronSide TE recorded about the CPU fault that caused the previous reset,
	 * if there was one it will describe. All-zero on an ordinary boot; the
	 * fault-report block above says how to read it. */
	struct ironside_fault_report fault;
	/*
	 * Copy of RESET.RESETREAS taken at the start of this boot; the bits follow
	 * the product specification of the SoC (MDK RESET_RESETREAS_*_Msk).
	 * IronSide TE clears the register on every boot, so read the reset reason
	 * here, not from the register.
	 */
	uint32_t reset_reason;
	/* The non-secure partition layout in effect this boot, as provisioned. Use
	 * these rather than build-time addresses when writing to a partition. */
	struct ironside_boot_region slot0;
	struct ironside_boot_region slot1;
	struct ironside_boot_region appstorage;
	struct ironside_boot_region dfuimage;
	/* The non-secure RAM in effect this boot: APP-RAM and APP-XRAM together.
	 * Its base follows the PSA key-slot reservation in the IronSide TE base
	 * configuration, which the App must be linked for; see layout.h, which
	 * also gives the slot count the reservation holds. */
	struct ironside_boot_region ram_ns;
	/* IRONSIDE_BOOTED_IMAGE_* of the boot that left `fault`, NONE when `fault`
	 * is absent. On the boot after a fault it can differ from `booted_image`. */
	uint32_t fault_image;
};

/**
 * @brief Read the IronSide TE boot report.
 *
 * The caller states the capacity, IronSide TE reports how much of it it wrote. That
 * length is this report's only compatibility discriminator - the struct carries
 * no version.
 *
 * @param report        Receives the report. IronSide TE writes exactly the
 *                      @p report_length bytes it reports and touches nothing
 *                      else in the buffer; untouched entirely on failure.
 * @param report_size   Capacity of @p report in bytes, normally sizeof(*report).
 *                      Less is legal and is served that many leading bytes, with
 *                      nothing past them touched.
 * @param report_length Receives the bytes IronSide TE wrote, never more than
 *                      @p report_size; 0 on failure. Required. Anything less
 *                      than @p report_size is an IronSide TE older than this
 *                      header - see above.
 *
 * @retval 0     The leading @p report_length bytes of @p report are the report.
 * @retval -135  (PSA_ERROR_INVALID_ARGUMENT) @p report or @p report_length is
 *               NULL, or @p report_size is 0.
 * @retval -137  (PSA_ERROR_BAD_STATE) IronSide TE has no valid report for this boot.
 * @retval -249  (IRONSIDE_ERROR_CALL_IN_FLIGHT) another call is in flight; see integration.h.
 */
int32_t ironside_boot_report_get(struct ironside_boot_report *report, size_t report_size,
				 size_t *report_length);

#endif /* IRONSIDE_CLIENT_SERVICES_BOOT_REPORT_H */
