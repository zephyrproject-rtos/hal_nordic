/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide TE Client - status codes that IronSide TE defines.
 */

#ifndef IRONSIDE_CLIENT_ERROR_H
#define IRONSIDE_CLIENT_ERROR_H

#include <ironside_client/psa/error.h>

/*
 * A call refused because another call is already in flight. -249 is in the
 * PSA Certified Status code API's SPM-implementation-defined range
 * (-249..-256): no PSA service ran, and the caller's operation object is
 * untouched. Retry once the call already in flight returns. This is the value
 * IronSide TE returns for the refusal. When a caller can be refused, see the calling
 * contract in integration.h.
 */
#define IRONSIDE_ERROR_CALL_IN_FLIGHT ((psa_status_t) - 249)

#endif /* IRONSIDE_CLIENT_ERROR_H */
