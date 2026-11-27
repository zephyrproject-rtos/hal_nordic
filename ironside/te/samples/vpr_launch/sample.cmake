# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
# vpr_launch needs the IronSide TE Client for one request -- the platform GPIO
# CTRLSEL selection (ironside_client/services/platform.h). VPR00, the GPIO port and the
# mailbox in non-secure RAM are all reached directly.
#
# The FLPR's own code is assembled from flpr_stub.S at build time, so this is
# the one sample that needs the OPTIONAL RISC-V toolchain (see
# ../PREREQUISITES.md). Configuring it without one fails with instructions;
# nothing else in the IronSide Integration Kit acquires that dependency.
ironside_client_add(${SAMPLE})

# The MDK device macro picks the pad in vpr_pad.h and the RAM end in the
# client's layout.h, exactly as it does for the App half -- so one source
# assembles a correct blob for every supported part.
integration_kit_vpr_add_stub(${SAMPLE}
  SOURCE "${SAMPLE_DIR}/flpr_stub.S"
  HEADER flpr_stub.inc
  ARRAY flpr_stub
  ALIGN 128                      # VPR00.INITPC's alignment requirement
  DEFINES -D${INTEGRATION_KIT_MDK_MACRO}
  INCLUDES "${IRONSIDE_CLIENT_ROOT}/include" "${IRONSIDE_BUNDLE_CONFIG_DIR}" "${SAMPLE_DIR}"
)
