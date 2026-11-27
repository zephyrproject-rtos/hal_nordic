# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
# app_version reads system facts - the App slot address and the MCUboot image
# header format - from the IronSide TE Client headers (ironside_client/
# image.h + layout.h) instead of hardcoding them, and it calls the boot-report
# service (ironside_client/services/boot_report.h) to read the boot report, which is not
# in App-readable memory. That call needs the client core, not just the headers.
ironside_client_add(${SAMPLE})
