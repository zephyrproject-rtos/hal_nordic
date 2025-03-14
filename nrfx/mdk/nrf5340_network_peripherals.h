/*

Copyright (c) 2010 - 2025, Nordic Semiconductor ASA All rights reserved.

SPDX-License-Identifier: BSD-3-Clause

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

3. Neither the name of Nordic Semiconductor ASA nor the names of its
   contributors may be used to endorse or promote products derived from this
   software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef _NRF5340_NETWORK_PERIPHERALS_H
#define _NRF5340_NETWORK_PERIPHERALS_H


/* Clock Peripheral */
#define CLOCK_PRESENT
#define CLOCK_COUNT 1

/* Power Peripheral */
#define POWER_PRESENT
#define POWER_COUNT 1

/* Non-Volatile Memory Controller */
#define NVMC_PRESENT
#define NVMC_COUNT 1

#define NVMC_FEATURE_CACHE_PRESENT

/* Voltage request peripheral */
#define VREQCTRL_PRESENT
#define VREQCTRL_COUNT 1

/* Volatile Memory Controller Peripheral */
#define VMC_PRESENT
#define VMC_COUNT 1

#define VMC_FEATURE_RAM_REGISTERS_PRESENT
#define VMC_FEATURE_RAM_REGISTERS_COUNT 4

/* Systick timer */
#define SYSTICK_PRESENT
#define SYSTICK_COUNT 1

/* Inter-Processor Communication */
#define IPC_PRESENT
#define IPC_COUNT 1

#define IPC_CH_NUM 16
#define IPC_CONF_NUM 16
#define IPC_GPMEM_NUM 2

/* GPIO */
#define GPIO_PRESENT
#define GPIO_COUNT 2

#define P0_PIN_NUM 32
#define P1_PIN_NUM 16

#define P0_FEATURE_PINS_PRESENT 0xFFFFFFFFUL
#define P1_FEATURE_PINS_PRESENT 0x0000FFFFUL


/* ACL */
#define ACL_PRESENT
#define ACL_REGIONS_COUNT 8

/* Radio */
#define RADIO_PRESENT
#define RADIO_COUNT 1

#define RADIO_EASYDMA_MAXCNT_SIZE 9
#define RADIO_FEATURE_IEEE_802_15_4_PRESENT

#define RADIO_TXPOWER_TXPOWER_Max RADIO_TXPOWER_TXPOWER_0dBm

/* Accelerated Address Resolver */
#define AAR_PRESENT
#define AAR_COUNT 1

#define AAR_MAX_IRK_NUM 16

/* AES Electronic CodeBook mode encryption */
#define ECB_PRESENT
#define ECB_COUNT 1

/* AES CCM mode encryption */
#define CCM_PRESENT
#define CCM_COUNT 1

/* Distributed Peripheral to Peripheral Interconnect */
#define DPPIC_PRESENT
#define DPPIC_COUNT 1

#define DPPIC_CH_NUM 32
#define DPPIC_GROUP_NUM 6

/* Event Generator Unit */
#define EGU_PRESENT
#define EGU_COUNT 1

#define EGU0_CH_NUM 16

/* Timer/Counter */
#define TIMER_PRESENT
#define TIMER_COUNT 3

#define TIMER0_MAX_SIZE 32
#define TIMER1_MAX_SIZE 32
#define TIMER2_MAX_SIZE 32

#define TIMER0_CC_NUM 8
#define TIMER1_CC_NUM 8
#define TIMER2_CC_NUM 8

/* Real Time Counter */
#define RTC_PRESENT
#define RTC_COUNT 2

#define RTC0_CC_NUM 4
#define RTC1_CC_NUM 4

/* RNG */
#define RNG_PRESENT
#define RNG_COUNT 1

/* Watchdog Timer */
#define WDT_PRESENT
#define WDT_COUNT 1

/* Temperature Sensor */
#define TEMP_PRESENT
#define TEMP_COUNT 1

/* Universal Asynchronous Receiver-Transmitter with DMA */
#define UARTE_PRESENT
#define UARTE_COUNT 1

#define UARTE0_EASYDMA_MAXCNT_SIZE 16

/* Serial Peripheral Interface Master with DMA */
#define SPIM_PRESENT
#define SPIM_COUNT 1

#define SPIM0_MAX_DATARATE  8

#define SPIM0_FEATURE_HARDWARE_CSN_PRESENT  0

#define SPIM0_FEATURE_DCX_PRESENT  0

#define SPIM0_FEATURE_RXDELAY_PRESENT  0

#define SPIM0_EASYDMA_MAXCNT_SIZE 16

/* Serial Peripheral Interface Slave with DMA*/
#define SPIS_PRESENT
#define SPIS_COUNT 1

#define SPIS0_EASYDMA_MAXCNT_SIZE 16

/* Two Wire Interface Master with DMA */
#define TWIM_PRESENT
#define TWIM_COUNT 1

#define TWIM0_EASYDMA_MAXCNT_SIZE 16

/* Two Wire Interface Slave with DMA */
#define TWIS_PRESENT
#define TWIS_COUNT 1

#define TWIS0_EASYDMA_MAXCNT_SIZE 16

/* GPIO Tasks and Events */
#define GPIOTE_PRESENT
#define GPIOTE_COUNT 1

#define GPIOTE_CH_NUM 8

#define GPIOTE_FEATURE_SET_PRESENT
#define GPIOTE_FEATURE_CLR_PRESENT

/* Software Interrupts */
#define SWI_PRESENT
#define SWI_COUNT 4

/* Mutex*/
#define MUTEX_PRESENT
#define MUTEX_COUNT 1

#endif      // _NRF5340_NETWORK_PERIPHERALS_H
