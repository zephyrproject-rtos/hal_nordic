/*
 * Copyright (c) 2019 - 2023, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

/**
 * @brief Module that defines the 802.15.4 driver peripheral usage.
 *
 */

#ifndef NRF_802154_PERIPHERALS_H__
#define NRF_802154_PERIPHERALS_H__

#include <nrfx.h>
#include "nrf_802154_config.h"
#include "nrf_802154_debug.h"
#include "nrf_802154_debug_core.h"

#if defined(NRF52_SERIES)
#include "nrf_802154_peripherals_nrf52.h"
#elif defined(NRF5340_XXAA)
#include "nrf_802154_peripherals_nrf53.h"
#endif

#ifdef NRF_802154_USE_INTERNAL_INCLUDES
#include "nrf_802154_peripherals_internal.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @def NRF_802154_HIGH_PRECISION_TIMER_INSTANCE_NO
 *
 * Number of the timer instance used for precise frame timestamps and synchronous radio operations.
 *
 */
#ifndef NRF_802154_HIGH_PRECISION_TIMER_INSTANCE_NO
#define NRF_802154_HIGH_PRECISION_TIMER_INSTANCE_NO 1
#endif

/**
 * @def NRF_802154_HIGH_PRECISION_TIMER_INSTANCE
 *
 * The timer instance used for precise frame timestamps and synchronous radio operations.
 *
 */
#define NRF_802154_HIGH_PRECISION_TIMER_INSTANCE \
    NRFX_CONCAT_2(NRF_TIMER, NRF_802154_HIGH_PRECISION_TIMER_INSTANCE_NO)

/**
 * @def NRF_802154_TIMER_INSTANCE_NO
 *
 * Number of the timer instance used both by the driver for ACK IFS and by the FEM module.
 *
 */
#ifndef NRF_802154_TIMER_INSTANCE_NO
#define NRF_802154_TIMER_INSTANCE_NO 0
#endif

/**
 * @def NRF_802154_TIMER_INSTANCE
 *
 * The timer instance used both by the driver for ACK IFS and by the FEM module.
 *
 */
#define NRF_802154_TIMER_INSTANCE \
    NRFX_CONCAT_2(NRF_TIMER, NRF_802154_TIMER_INSTANCE_NO)

/**
 * @def NRF_802154_RTC_INSTANCE
 *
 * The RTC instance used in the standalone timer driver implementation.
 *
 * @note This configuration is only applicable for the Low Power Timer Abstraction Layer
 *       implementation in nrf_802154_lp_timer_nodrv.c.
 *
 */
#define NRF_802154_RTC_INSTANCE    NRFX_CONCAT_2(NRF_RTC, NRF_802154_RTC_INSTANCE_NO)

/**
 * @def NRF_802154_RTC_IRQ_HANDLER
 *
 * The RTC interrupt handler name used in the standalone timer driver implementation.
 *
 * @note This configuration is only applicable for Low Power Timer Abstraction Layer implementation
 *       in nrf_802154_lp_timer_nodrv.c.
 *
 */
#define NRF_802154_RTC_IRQ_HANDLER NRFX_CONCAT_3(RTC, NRF_802154_RTC_INSTANCE_NO, _IRQHandler)

/**
 * @def NRF_802154_RTC_IRQN
 *
 * The RTC Interrupt number used in the standalone timer driver implementation.
 *
 * @note This configuration is only applicable for the Low Power Timer Abstraction Layer implementation
 *       in nrf_802154_lp_timer_nodrv.c.
 *
 */
#define NRF_802154_RTC_IRQN        NRFX_CONCAT_3(RTC, NRF_802154_RTC_INSTANCE_NO, _IRQn)

/**
 * @def NRF_802154_TIMERS_USED_MASK
 *
 * Bit mask of instances of timer peripherals used by the 802.15.4 driver.
 */
#ifndef NRF_802154_TIMERS_USED_MASK
#define NRF_802154_TIMERS_USED_MASK ((1 << NRF_802154_HIGH_PRECISION_TIMER_INSTANCE_NO) | \
                                     (1 << NRF_802154_TIMER_INSTANCE_NO))
#endif // NRF_802154_TIMERS_USED_MASK

/**
 * @def NRF_802154_RTC_USED_MASK
 *
 * Bit mask of instances of RTC peripherals used by the 802.15.4 driver.
 */
#ifndef NRF_802154_RTC_USED_MASK
#define NRF_802154_RTC_USED_MASK (1 << NRF_802154_RTC_INSTANCE_NO)
#endif

/**
 * @def NRF_802154_GPIO_PINS_USED_MASK
 *
 * Bit mask of GPIO pins used by the 802.15.4 driver.
 */
#ifndef NRF_802154_GPIO_PINS_USED_MASK
#define NRF_802154_GPIO_PINS_USED_MASK NRF_802154_DEBUG_PINS_USED_MASK
#endif // NRF_802154_GPIO_PINS_USED_MASK

/**
 * @def NRF_802154_GPIOTE_CHANNELS_USED_MASK
 *
 * Bit mask of GPIOTE peripherals used by the 802.15.4 driver.
 */
#ifndef NRF_802154_GPIOTE_CHANNELS_USED_MASK
#define NRF_802154_GPIOTE_CHANNELS_USED_MASK NRF_802154_DEBUG_GPIOTE_CHANNELS_USED_MASK
#endif // NRF_802154_GPIOTE_CHANNELS_USED_MASK

#ifdef __cplusplus
}
#endif

#endif // NRF_802154_PERIPHERALS_H__
