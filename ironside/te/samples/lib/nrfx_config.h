/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * The nrfx configuration entry point every nrfx header and driver includes.
 * nrfx ships the per-device option sets and the dispatcher that selects one from
 * the device macro, but no top-level configuration header: each integration
 * supplies its own so it can set options before the defaults are applied. This
 * one changes no default -- the sample build passes the options it needs as
 * compile definitions, and every option in the per-device set is defined with
 * #ifndef, so a definition on the command line wins.
 */
#ifndef NRFX_CONFIG_H__
#define NRFX_CONFIG_H__

#include <nrfx_config_common.h>
#include <nrfx_templates_config.h>

#endif /* NRFX_CONFIG_H__ */
