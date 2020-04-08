/*
 * Copyright (c) 2017 - 2020, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef NRFX_LOG_H__
#define NRFX_LOG_H__

#include <logging/log.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NRFX_MODULE_PREFIX  _CONCAT(NRFX_, NRFX_LOG_MODULE)
/*
 * The following macros from nrfx_config control the log messages coming from
 * a given module:
 * - NRFX_<module>_CONFIG_LOG_ENABLED enables the messages (when set to 1)
 * - NRFX_<module>_CONFIG_LOG_LEVEL specifies the severity level of the messages
 *   that are to be output.
 */
#if !IS_ENABLED(_CONCAT(NRFX_MODULE_PREFIX, _CONFIG_LOG_ENABLED))
#define NRFX_MODULE_CONFIG_LOG_LEVEL 0
#else
#define NRFX_MODULE_CONFIG_LOG_LEVEL \
	_CONCAT(NRFX_MODULE_PREFIX, _CONFIG_LOG_LEVEL)
#endif

#if	NRFX_MODULE_CONFIG_LOG_LEVEL == 0
#define NRFX_MODULE_LOG_LEVEL		LOG_LEVEL_NONE
#elif	NRFX_MODULE_CONFIG_LOG_LEVEL == 1
#define NRFX_MODULE_LOG_LEVEL		LOG_LEVEL_ERR
#elif	NRFX_MODULE_CONFIG_LOG_LEVEL == 2
#define NRFX_MODULE_LOG_LEVEL		LOG_LEVEL_WRN
#elif	NRFX_MODULE_CONFIG_LOG_LEVEL == 3
#define NRFX_MODULE_LOG_LEVEL		LOG_LEVEL_INF
#elif	NRFX_MODULE_CONFIG_LOG_LEVEL == 4
#define NRFX_MODULE_LOG_LEVEL		LOG_LEVEL_DBG
#endif
LOG_MODULE_REGISTER(NRFX_MODULE_PREFIX, NRFX_MODULE_LOG_LEVEL);

/**
 * @defgroup nrfx_log nrfx_log.h
 * @{
 * @ingroup nrfx
 *
 * @brief This file contains macros that should be implemented according to
 *        the needs of the host environment into which @em nrfx is integrated.
 */

/**
 * @brief Macro for logging a message with the severity level ERROR.
 *
 * @param ... printf-style format string, optionally followed by arguments
 *            to be formatted and inserted in the resulting string.
 */
#define NRFX_LOG_ERROR(...)  LOG_ERR(__VA_ARGS__)

/**
 * @brief Macro for logging a message with the severity level WARNING.
 *
 * @param ... printf-style format string, optionally followed by arguments
 *            to be formatted and inserted in the resulting string.
 */
#define NRFX_LOG_WARNING(...)  LOG_WRN(__VA_ARGS__)

/**
 * @brief Macro for logging a message with the severity level INFO.
 *
 * @param ... printf-style format string, optionally followed by arguments
 *            to be formatted and inserted in the resulting string.
 */
#define NRFX_LOG_INFO(...)  LOG_INF(__VA_ARGS__)

/**
 * @brief Macro for logging a message with the severity level DEBUG.
 *
 * @param ... printf-style format string, optionally followed by arguments
 *            to be formatted and inserted in the resulting string.
 */
#define NRFX_LOG_DEBUG(...)  LOG_DBG(__VA_ARGS__)

/**
 * @brief Macro for logging a memory dump with the severity level ERROR.
 *
 * @param[in] p_memory Pointer to the memory region to be dumped.
 * @param[in] length   Length of the memory region in bytes.
 */
#define NRFX_LOG_HEXDUMP_ERROR(p_memory, length) \
	LOG_HEXDUMP_ERR(p_memory, length, "")

/**
 * @brief Macro for logging a memory dump with the severity level WARNING.
 *
 * @param[in] p_memory Pointer to the memory region to be dumped.
 * @param[in] length   Length of the memory region in bytes.
 */
#define NRFX_LOG_HEXDUMP_WARNING(p_memory, length) \
	LOG_HEXDUMP_WRN(p_memory, length, "")

/**
 * @brief Macro for logging a memory dump with the severity level INFO.
 *
 * @param[in] p_memory Pointer to the memory region to be dumped.
 * @param[in] length   Length of the memory region in bytes.
 */
#define NRFX_LOG_HEXDUMP_INFO(p_memory, length) \
	LOG_HEXDUMP_INF(p_memory, length, "")

/**
 * @brief Macro for logging a memory dump with the severity level DEBUG.
 *
 * @param[in] p_memory Pointer to the memory region to be dumped.
 * @param[in] length   Length of the memory region in bytes.
 */
#define NRFX_LOG_HEXDUMP_DEBUG(p_memory, length) \
	LOG_HEXDUMP_DBG(p_memory, length, "")

/**
 * @brief Macro for getting the textual representation of a given error code.
 *
 * @param[in] error_code Error code.
 *
 * @return String containing the textual representation of the error code.
 */
#define NRFX_LOG_ERROR_STRING_GET(error_code)  nrfx_error_string_get(error_code)
extern char const *nrfx_error_string_get(nrfx_err_t code);

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRFX_LOG_H__
