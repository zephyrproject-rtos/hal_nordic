# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
# dfu_enter calls the DFU-entry-request service through the IronSide TE Client
# (ironside_client/services/dfu_request.h). The wrappers are static inline, but they call
# the client core, so this needs the compiled client and not just its headers.
ironside_client_add(${SAMPLE})
