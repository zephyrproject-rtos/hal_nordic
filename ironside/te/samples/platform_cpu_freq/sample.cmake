# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
# platform_cpu_freq calls the platform mem-read service through the IronSide TE
# Client (ironside_client/services/platform.h). The wrappers are static inline, but they
# call the client core, so this needs the compiled client and not just its headers.
ironside_client_add(${SAMPLE})
