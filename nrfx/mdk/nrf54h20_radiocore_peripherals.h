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

#ifndef NRF54H20_RADIOCORE_PERIPHERALS_H
#define NRF54H20_RADIOCORE_PERIPHERALS_H

#ifdef __cplusplus
    extern "C" {
#endif

#include <stdbool.h>
/*CACHEDATA*/
#define ICACHEDATA_PRESENT 1
#define ICACHEDATA_COUNT 1

#define ICACHEDATA_NUMSETS 256                       /*!< Number of sets : 256                                                 */
#define ICACHEDATA_NUMWAYS 2                         /*!< Number of ways : 2                                                   */
#define ICACHEDATA_NUMDATAUNIT 4                     /*!< Number of data units : 4                                             */
#define ICACHEDATA_DATAWIDTH 2                       /*!< Data width of a data unit : 2 word(s)                                */

/*CACHEINFO*/
#define ICACHEINFO_PRESENT 1
#define ICACHEINFO_COUNT 1

#define ICACHEINFO_NUMSETS 256                       /*!< Number of sets : 256                                                 */
#define ICACHEINFO_NUMWAYS 2                         /*!< Number of ways : 2                                                   */
#define ICACHEINFO_NUMDATAUNIT 4                     /*!< Number of data units : 4                                             */
#define ICACHEINFO_DATAWIDTH 2                       /*!< Data width of a data unit : 2 word(s)                                */
#define ICACHEINFO_TAGWIDTH 19                       /*!< TAG width : 19                                                       */
#define ICACHEINFO_DU_EXTENSION 0                    /*!< (unspecified)                                                        */

/*User information configuration registers*/
#define UICR_PRESENT 1
#define UICR_COUNT 1

/*CACHEDATA*/
#define DCACHEDATA_PRESENT 1
#define DCACHEDATA_COUNT 1

#define DCACHEDATA_NUMSETS 256                       /*!< Number of sets : 256                                                 */
#define DCACHEDATA_NUMWAYS 2                         /*!< Number of ways : 2                                                   */
#define DCACHEDATA_NUMDATAUNIT 8                     /*!< Number of data units : 8                                             */
#define DCACHEDATA_DATAWIDTH 1                       /*!< Data width of a data unit : 1 word(s)                                */

/*CACHEINFO*/
#define DCACHEINFO_PRESENT 1
#define DCACHEINFO_COUNT 1

#define DCACHEINFO_NUMSETS 256                       /*!< Number of sets : 256                                                 */
#define DCACHEINFO_NUMWAYS 2                         /*!< Number of ways : 2                                                   */
#define DCACHEINFO_NUMDATAUNIT 8                     /*!< Number of data units : 8                                             */
#define DCACHEINFO_DATAWIDTH 1                       /*!< Data width of a data unit : 1 word(s)                                */
#define DCACHEINFO_TAGWIDTH 19                       /*!< TAG width : 19                                                       */
#define DCACHEINFO_DU_EXTENSION 1                    /*!< (unspecified)                                                        */

/*Embedded Trace Macrocell*/
#define ETM_PRESENT 1
#define ETM_COUNT 1

/*Cross-Trigger Interface control*/
#define CTI_PRESENT 1
#define CTI_COUNT 3

/*Cache*/
#define CACHE_PRESENT 1
#define CACHE_COUNT 2

#define ICACHE_VIRTUALCACHE 0                        /*!< (unspecified)                                                        */
#define ICACHE_FLUSH 0                               /*!< (unspecified)                                                        */
#define ICACHE_CLEAN 0                               /*!< (unspecified)                                                        */
#define ICACHE_NONCACHEABLEMISS 1                    /*!< (unspecified)                                                        */
#define ICACHE_BUSWIDTH_MIN 0                        /*!< Data bus width : 0..63                                               */
#define ICACHE_BUSWIDTH_MAX 63                       /*!< Data bus width : 0..63                                               */
#define ICACHE_BUSWIDTH_SIZE 64                      /*!< Data bus width : 0..63                                               */
#define ICACHE_SECUREINVALIDATE 1                    /*!< (unspecified)                                                        */

#define DCACHE_VIRTUALCACHE 0                        /*!< (unspecified)                                                        */
#define DCACHE_FLUSH 1                               /*!< (unspecified)                                                        */
#define DCACHE_CLEAN 1                               /*!< (unspecified)                                                        */
#define DCACHE_NONCACHEABLEMISS 1                    /*!< (unspecified)                                                        */
#define DCACHE_BUSWIDTH_MIN 0                        /*!< Data bus width : 0..63                                               */
#define DCACHE_BUSWIDTH_MAX 63                       /*!< Data bus width : 0..63                                               */
#define DCACHE_BUSWIDTH_SIZE 64                      /*!< Data bus width : 0..63                                               */
#define DCACHE_SECUREINVALIDATE 1                    /*!< (unspecified)                                                        */

/*System protection unit*/
#define SPU_PRESENT 1
#define SPU_COUNT 4

#define SPU000_BELLS 0                               /*!< (unspecified)                                                        */
#define SPU000_IPCT 0                                /*!< (unspecified)                                                        */
#define SPU000_DPPI 0                                /*!< (unspecified)                                                        */
#define SPU000_GPIOTE 0                              /*!< (unspecified)                                                        */
#define SPU000_GRTC 0                                /*!< (unspecified)                                                        */
#define SPU000_GPIO 0                                /*!< (unspecified)                                                        */
#define SPU000_CRACEN 0                              /*!< (unspecified)                                                        */
#define SPU000_MRAMC 0                               /*!< (unspecified)                                                        */
#define SPU000_COEXC 0                               /*!< (unspecified)                                                        */
#define SPU000_ANTSWC 0                              /*!< (unspecified)                                                        */
#define SPU000_SLAVE_BITS 4                          /*!< SLAVE_BITS=4 (number of address bits required to represent the
                                                          peripheral slave index)*/

#define SPU010_BELLS 0                               /*!< (unspecified)                                                        */
#define SPU010_IPCT 0                                /*!< (unspecified)                                                        */
#define SPU010_DPPI 0                                /*!< (unspecified)                                                        */
#define SPU010_GPIOTE 0                              /*!< (unspecified)                                                        */
#define SPU010_GRTC 0                                /*!< (unspecified)                                                        */
#define SPU010_GPIO 0                                /*!< (unspecified)                                                        */
#define SPU010_CRACEN 0                              /*!< (unspecified)                                                        */
#define SPU010_MRAMC 0                               /*!< (unspecified)                                                        */
#define SPU010_COEXC 0                               /*!< (unspecified)                                                        */
#define SPU010_ANTSWC 0                              /*!< (unspecified)                                                        */
#define SPU010_SLAVE_BITS 4                          /*!< SLAVE_BITS=4 (number of address bits required to represent the
                                                          peripheral slave index)*/

#define SPU020_BELLS 0                               /*!< (unspecified)                                                        */
#define SPU020_IPCT 1                                /*!< (unspecified)                                                        */
#define SPU020_DPPI 1                                /*!< (unspecified)                                                        */
#define SPU020_GPIOTE 0                              /*!< (unspecified)                                                        */
#define SPU020_GRTC 0                                /*!< (unspecified)                                                        */
#define SPU020_GPIO 0                                /*!< (unspecified)                                                        */
#define SPU020_CRACEN 0                              /*!< (unspecified)                                                        */
#define SPU020_MRAMC 0                               /*!< (unspecified)                                                        */
#define SPU020_COEXC 0                               /*!< (unspecified)                                                        */
#define SPU020_ANTSWC 0                              /*!< (unspecified)                                                        */
#define SPU020_SLAVE_BITS 4                          /*!< SLAVE_BITS=4 (number of address bits required to represent the
                                                          peripheral slave index)*/

#define SPU030_BELLS 0                               /*!< (unspecified)                                                        */
#define SPU030_IPCT 0                                /*!< (unspecified)                                                        */
#define SPU030_DPPI 1                                /*!< (unspecified)                                                        */
#define SPU030_GPIOTE 0                              /*!< (unspecified)                                                        */
#define SPU030_GRTC 0                                /*!< (unspecified)                                                        */
#define SPU030_GPIO 0                                /*!< (unspecified)                                                        */
#define SPU030_CRACEN 0                              /*!< (unspecified)                                                        */
#define SPU030_MRAMC 0                               /*!< (unspecified)                                                        */
#define SPU030_COEXC 0                               /*!< (unspecified)                                                        */
#define SPU030_ANTSWC 0                              /*!< (unspecified)                                                        */
#define SPU030_SLAVE_BITS 4                          /*!< SLAVE_BITS=4 (number of address bits required to represent the
                                                          peripheral slave index)*/

/*Memory Privilege Controller*/
#define MPC_PRESENT 1
#define MPC_COUNT 1

#define MPC_EXTEND_CLOCK_REQ 0                       /*!< (unspecified)                                                        */
#define MPC_RTCHOKE 1                                /*!< (unspecified)                                                        */
#define MPC_OVERRIDE_GRAN 4096                       /*!< The override region granularity is 4096 bytes                        */

/*CM33 SubSystem*/
#define CM33SS_PRESENT 1
#define CM33SS_COUNT 1

#define CPUC_FPUAVAILABLE 1                          /*!< (unspecified)                                                        */

/*MVDMA performs direct-memory-accesses between memories. Data is transferred according to job descriptor lists. Each transfer has corresponding source and sink descriptor lists with matching data amounts. The lists are in memory and they contain data buffer information, address pointers, buffer sizes and data type attributes.*/

#define MVDMA_PRESENT 1
#define MVDMA_COUNT 2

#define MVDMA_COMPLETED_EVENT 1                      /*!< (unspecified)                                                        */
#define MVDMA_DPPI_DISCONNECTED 0                    /*!< (unspecified)                                                        */
#define MVDMA_INSTANCE_IN_WRAPPER 0                  /*!< (unspecified)                                                        */
#define MVDMA_MVDMA_JOBLISTCNT_MIN 0                 /*!< Number of MVDMA job lists : 0..7                                     */
#define MVDMA_MVDMA_JOBLISTCNT_MAX 7                 /*!< Number of MVDMA job lists : 0..7                                     */
#define MVDMA_MVDMA_JOBLISTCNT_SIZE 8                /*!< Number of MVDMA job lists : 0..7                                     */

#define MVDMA120_COMPLETED_EVENT 1                   /*!< (unspecified)                                                        */
#define MVDMA120_DPPI_DISCONNECTED 1                 /*!< (unspecified)                                                        */
#define MVDMA120_INSTANCE_IN_WRAPPER 1               /*!< (unspecified)                                                        */
#define MVDMA120_MVDMA_JOBLISTCNT_MIN 0              /*!< Number of MVDMA job lists : 0..7                                     */
#define MVDMA120_MVDMA_JOBLISTCNT_MAX 7              /*!< Number of MVDMA job lists : 0..7                                     */
#define MVDMA120_MVDMA_JOBLISTCNT_SIZE 8             /*!< Number of MVDMA job lists : 0..7                                     */

/*RAM Controller*/
#define RAMC_PRESENT 1
#define RAMC_COUNT 2

#define RAMC000_ECC 0                                /*!< (unspecified)                                                        */
#define RAMC000_SEC 1                                /*!< (unspecified)                                                        */

#define RAMC122_ECC 0                                /*!< (unspecified)                                                        */
#define RAMC122_SEC 0                                /*!< (unspecified)                                                        */

/*HSFLL*/
#define HSFLL_PRESENT 1
#define HSFLL_COUNT 1

#define HSFLL_DITHER_32B 1                           /*!< (unspecified)                                                        */
#define HSFLL_CLOCKCTRL_MULT_RESET 4                 /*!< Reset value of register CLOCKCTRL.MULT: clockctrl_mult_reset         */
#define HSFLL_CLOCKCTRL_INTEGER_DIVISION 0           /*!< (unspecified)                                                        */

/*LRCCONF*/
#define LRCCONF_PRESENT 1
#define LRCCONF_COUNT 3

#define LRCCONF000_POWERON 0                         /*!< (unspecified)                                                        */
#define LRCCONF000_RETAIN 0                          /*!< (unspecified)                                                        */
#define LRCCONF000_SYSTEMOFF 0                       /*!< (unspecified)                                                        */
#define LRCCONF000_LRCREQHFXO 0                      /*!< (unspecified)                                                        */
#define LRCCONF000_NCLK_MIN 0                        /*!< (unspecified)                                                        */
#define LRCCONF000_NCLK_MAX 0                        /*!< (unspecified)                                                        */
#define LRCCONF000_NCLK_SIZE 1                       /*!< (unspecified)                                                        */
#define LRCCONF000_CLKCTRL 1                         /*!< (unspecified)                                                        */
#define LRCCONF000_NACTPD_MIN 0                      /*!< (unspecified)                                                        */
#define LRCCONF000_NACTPD_MAX 7                      /*!< (unspecified)                                                        */
#define LRCCONF000_NACTPD_SIZE 8                     /*!< (unspecified)                                                        */
#define LRCCONF000_PDACT 0                           /*!< (unspecified)                                                        */
#define LRCCONF000_NPD_MIN 0                         /*!< (unspecified)                                                        */
#define LRCCONF000_NPD_MAX 7                         /*!< (unspecified)                                                        */
#define LRCCONF000_NPD_SIZE 8                        /*!< (unspecified)                                                        */
#define LRCCONF000_OTHERON 0                         /*!< (unspecified)                                                        */
#define LRCCONF000_NDOMAINS_MIN 0                    /*!< (unspecified)                                                        */
#define LRCCONF000_NDOMAINS_MAX 15                   /*!< (unspecified)                                                        */
#define LRCCONF000_NDOMAINS_SIZE 16                  /*!< (unspecified)                                                        */
#define LRCCONF000_AX2XWAITSTATES 0                  /*!< (unspecified)                                                        */
#define LRCCONF000_POWERON_MAIN_RESET 0              /*!< (unspecified)                                                        */
#define LRCCONF000_POWERON_ACT_RESET 0               /*!< (unspecified)                                                        */
#define LRCCONF000_RETAIN_MAIN_RESET 1               /*!< (unspecified)                                                        */
#define LRCCONF000_RETAIN_ACT_RESET 1                /*!< (unspecified)                                                        */

#define LRCCONF010_POWERON 1                         /*!< (unspecified)                                                        */
#define LRCCONF010_RETAIN 1                          /*!< (unspecified)                                                        */
#define LRCCONF010_SYSTEMOFF 1                       /*!< (unspecified)                                                        */
#define LRCCONF010_LRCREQHFXO 1                      /*!< (unspecified)                                                        */
#define LRCCONF010_NCLK_MIN 0                        /*!< (unspecified)                                                        */
#define LRCCONF010_NCLK_MAX 0                        /*!< (unspecified)                                                        */
#define LRCCONF010_NCLK_SIZE 1                       /*!< (unspecified)                                                        */
#define LRCCONF010_CLKCTRL 1                         /*!< (unspecified)                                                        */
#define LRCCONF010_NACTPD_MIN 0                      /*!< (unspecified)                                                        */
#define LRCCONF010_NACTPD_MAX 1                      /*!< (unspecified)                                                        */
#define LRCCONF010_NACTPD_SIZE 2                     /*!< (unspecified)                                                        */
#define LRCCONF010_PDACT 1                           /*!< (unspecified)                                                        */
#define LRCCONF010_NPD_MIN 0                         /*!< (unspecified)                                                        */
#define LRCCONF010_NPD_MAX 7                         /*!< (unspecified)                                                        */
#define LRCCONF010_NPD_SIZE 8                        /*!< (unspecified)                                                        */
#define LRCCONF010_OTHERON 0                         /*!< (unspecified)                                                        */
#define LRCCONF010_NDOMAINS_MIN 0                    /*!< (unspecified)                                                        */
#define LRCCONF010_NDOMAINS_MAX 15                   /*!< (unspecified)                                                        */
#define LRCCONF010_NDOMAINS_SIZE 16                  /*!< (unspecified)                                                        */
#define LRCCONF010_AX2XWAITSTATES 0                  /*!< (unspecified)                                                        */
#define LRCCONF010_POWERON_MAIN_RESET 1              /*!< Reset value of register POWERON.MAIN: 1                              */
#define LRCCONF010_POWERON_ACT_RESET 0               /*!< Reset value of register POWERON.ACT: 0                               */
#define LRCCONF010_RETAIN_MAIN_RESET 0               /*!< Reset value of register RETAIN.MAIN: 0                               */
#define LRCCONF010_RETAIN_ACT_RESET 0                /*!< Reset value of register RETAIN.ACT: 0                                */

#define LRCCONF020_POWERON 0                         /*!< (unspecified)                                                        */
#define LRCCONF020_RETAIN 0                          /*!< (unspecified)                                                        */
#define LRCCONF020_SYSTEMOFF 0                       /*!< (unspecified)                                                        */
#define LRCCONF020_LRCREQHFXO 0                      /*!< (unspecified)                                                        */
#define LRCCONF020_NCLK_MIN 0                        /*!< (unspecified)                                                        */
#define LRCCONF020_NCLK_MAX 7                        /*!< (unspecified)                                                        */
#define LRCCONF020_NCLK_SIZE 8                       /*!< (unspecified)                                                        */
#define LRCCONF020_CLKCTRL 0                         /*!< (unspecified)                                                        */
#define LRCCONF020_NACTPD_MIN 0                      /*!< (unspecified)                                                        */
#define LRCCONF020_NACTPD_MAX 7                      /*!< (unspecified)                                                        */
#define LRCCONF020_NACTPD_SIZE 8                     /*!< (unspecified)                                                        */
#define LRCCONF020_PDACT 0                           /*!< (unspecified)                                                        */
#define LRCCONF020_NPD_MIN 0                         /*!< (unspecified)                                                        */
#define LRCCONF020_NPD_MAX 7                         /*!< (unspecified)                                                        */
#define LRCCONF020_NPD_SIZE 8                        /*!< (unspecified)                                                        */
#define LRCCONF020_OTHERON 0                         /*!< (unspecified)                                                        */
#define LRCCONF020_NDOMAINS_MIN 0                    /*!< (unspecified)                                                        */
#define LRCCONF020_NDOMAINS_MAX 15                   /*!< (unspecified)                                                        */
#define LRCCONF020_NDOMAINS_SIZE 16                  /*!< (unspecified)                                                        */
#define LRCCONF020_AX2XWAITSTATES 0                  /*!< (unspecified)                                                        */
#define LRCCONF020_POWERON_MAIN_RESET 0              /*!< (unspecified)                                                        */
#define LRCCONF020_POWERON_ACT_RESET 0               /*!< (unspecified)                                                        */
#define LRCCONF020_RETAIN_MAIN_RESET 1               /*!< (unspecified)                                                        */
#define LRCCONF020_RETAIN_ACT_RESET 1                /*!< (unspecified)                                                        */

/*Memory configuration*/
#define MEMCONF_PRESENT 1
#define MEMCONF_COUNT 1

#define MEMCONF_RETTRIM 1                            /*!< (unspecified)                                                        */
#define MEMCONF_REPAIR 0                             /*!< (unspecified)                                                        */
#define MEMCONF_POWER 1                              /*!< (unspecified)                                                        */
#define MEMCONF_NUMADDRBITS_MIN 0                    /*!< Number of bits in repair address at MEMCONF.REPAIR.BITLINE.ADDR :
                                                          [0..6]*/
#define MEMCONF_NUMADDRBITS_MAX 6                    /*!< Number of bits in repair address at MEMCONF.REPAIR.BITLINE.ADDR :
                                                          [0..6]*/
#define MEMCONF_NUMADDRBITS_SIZE 7                   /*!< Number of bits in repair address at MEMCONF.REPAIR.BITLINE.ADDR :
                                                          [0..6]*/

/*Watchdog Timer*/
#define WDT_PRESENT 1
#define WDT_COUNT 4

#define WDT010_ALLOW_STOP 0                          /*!< (unspecified)                                                        */
#define WDT010_HAS_INTEN 1                           /*!< (unspecified)                                                        */

#define WDT011_ALLOW_STOP 0                          /*!< (unspecified)                                                        */
#define WDT011_HAS_INTEN 1                           /*!< (unspecified)                                                        */

#define WDT131_ALLOW_STOP 0                          /*!< (unspecified)                                                        */
#define WDT131_HAS_INTEN 1                           /*!< (unspecified)                                                        */

#define WDT132_ALLOW_STOP 0                          /*!< (unspecified)                                                        */
#define WDT132_HAS_INTEN 1                           /*!< (unspecified)                                                        */

/*RESETINFO*/
#define RESETINFO_PRESENT 1
#define RESETINFO_COUNT 1

#define RESETINFO_HASRESETREAS 1                     /*!< (unspecified)                                                        */
#define RESETINFO_CROSSDOMAINRESET 0                 /*!< (unspecified)                                                        */

/*Distributed programmable peripheral interconnect controller*/
#define DPPIC_PRESENT 1
#define DPPIC_COUNT 10

#define DPPIC020_HASCHANNELGROUPS 1                  /*!< (unspecified)                                                        */
#define DPPIC020_CH_NUM_MIN 0                        /*!< (unspecified)                                                        */
#define DPPIC020_CH_NUM_MAX 31                       /*!< (unspecified)                                                        */
#define DPPIC020_CH_NUM_SIZE 32                      /*!< (unspecified)                                                        */
#define DPPIC020_GROUP_NUM_MIN 0                     /*!< (unspecified)                                                        */
#define DPPIC020_GROUP_NUM_MAX 3                     /*!< (unspecified)                                                        */
#define DPPIC020_GROUP_NUM_SIZE 4                    /*!< (unspecified)                                                        */

#define DPPIC030_HASCHANNELGROUPS 1                  /*!< (unspecified)                                                        */
#define DPPIC030_CH_NUM_MIN 0                        /*!< (unspecified)                                                        */
#define DPPIC030_CH_NUM_MAX 15                       /*!< (unspecified)                                                        */
#define DPPIC030_CH_NUM_SIZE 16                      /*!< (unspecified)                                                        */
#define DPPIC030_GROUP_NUM_MIN 0                     /*!< (unspecified)                                                        */
#define DPPIC030_GROUP_NUM_MAX 0                     /*!< (unspecified)                                                        */
#define DPPIC030_GROUP_NUM_SIZE 1                    /*!< (unspecified)                                                        */

#define DPPIC120_HASCHANNELGROUPS 1                  /*!< (unspecified)                                                        */
#define DPPIC120_CH_NUM_MIN 0                        /*!< (unspecified)                                                        */
#define DPPIC120_CH_NUM_MAX 7                        /*!< (unspecified)                                                        */
#define DPPIC120_CH_NUM_SIZE 8                       /*!< (unspecified)                                                        */
#define DPPIC120_GROUP_NUM_MIN 0                     /*!< (unspecified)                                                        */
#define DPPIC120_GROUP_NUM_MAX 1                     /*!< (unspecified)                                                        */
#define DPPIC120_GROUP_NUM_SIZE 2                    /*!< (unspecified)                                                        */

#define DPPIC130_HASCHANNELGROUPS 1                  /*!< (unspecified)                                                        */
#define DPPIC130_CH_NUM_MIN 0                        /*!< (unspecified)                                                        */
#define DPPIC130_CH_NUM_MAX 7                        /*!< (unspecified)                                                        */
#define DPPIC130_CH_NUM_SIZE 8                       /*!< (unspecified)                                                        */
#define DPPIC130_GROUP_NUM_MIN 0                     /*!< (unspecified)                                                        */
#define DPPIC130_GROUP_NUM_MAX 1                     /*!< (unspecified)                                                        */
#define DPPIC130_GROUP_NUM_SIZE 2                    /*!< (unspecified)                                                        */

#define DPPIC131_HASCHANNELGROUPS 1                  /*!< (unspecified)                                                        */
#define DPPIC131_CH_NUM_MIN 0                        /*!< (unspecified)                                                        */
#define DPPIC131_CH_NUM_MAX 7                        /*!< (unspecified)                                                        */
#define DPPIC131_CH_NUM_SIZE 8                       /*!< (unspecified)                                                        */
#define DPPIC131_GROUP_NUM_MIN 0                     /*!< (unspecified)                                                        */
#define DPPIC131_GROUP_NUM_MAX 1                     /*!< (unspecified)                                                        */
#define DPPIC131_GROUP_NUM_SIZE 2                    /*!< (unspecified)                                                        */

#define DPPIC132_HASCHANNELGROUPS 1                  /*!< (unspecified)                                                        */
#define DPPIC132_CH_NUM_MIN 0                        /*!< (unspecified)                                                        */
#define DPPIC132_CH_NUM_MAX 7                        /*!< (unspecified)                                                        */
#define DPPIC132_CH_NUM_SIZE 8                       /*!< (unspecified)                                                        */
#define DPPIC132_GROUP_NUM_MIN 0                     /*!< (unspecified)                                                        */
#define DPPIC132_GROUP_NUM_MAX 1                     /*!< (unspecified)                                                        */
#define DPPIC132_GROUP_NUM_SIZE 2                    /*!< (unspecified)                                                        */

#define DPPIC133_HASCHANNELGROUPS 1                  /*!< (unspecified)                                                        */
#define DPPIC133_CH_NUM_MIN 0                        /*!< (unspecified)                                                        */
#define DPPIC133_CH_NUM_MAX 7                        /*!< (unspecified)                                                        */
#define DPPIC133_CH_NUM_SIZE 8                       /*!< (unspecified)                                                        */
#define DPPIC133_GROUP_NUM_MIN 0                     /*!< (unspecified)                                                        */
#define DPPIC133_GROUP_NUM_MAX 1                     /*!< (unspecified)                                                        */
#define DPPIC133_GROUP_NUM_SIZE 2                    /*!< (unspecified)                                                        */

#define DPPIC134_HASCHANNELGROUPS 1                  /*!< (unspecified)                                                        */
#define DPPIC134_CH_NUM_MIN 0                        /*!< (unspecified)                                                        */
#define DPPIC134_CH_NUM_MAX 7                        /*!< (unspecified)                                                        */
#define DPPIC134_CH_NUM_SIZE 8                       /*!< (unspecified)                                                        */
#define DPPIC134_GROUP_NUM_MIN 0                     /*!< (unspecified)                                                        */
#define DPPIC134_GROUP_NUM_MAX 1                     /*!< (unspecified)                                                        */
#define DPPIC134_GROUP_NUM_SIZE 2                    /*!< (unspecified)                                                        */

#define DPPIC135_HASCHANNELGROUPS 1                  /*!< (unspecified)                                                        */
#define DPPIC135_CH_NUM_MIN 0                        /*!< (unspecified)                                                        */
#define DPPIC135_CH_NUM_MAX 7                        /*!< (unspecified)                                                        */
#define DPPIC135_CH_NUM_SIZE 8                       /*!< (unspecified)                                                        */
#define DPPIC135_GROUP_NUM_MIN 0                     /*!< (unspecified)                                                        */
#define DPPIC135_GROUP_NUM_MAX 1                     /*!< (unspecified)                                                        */
#define DPPIC135_GROUP_NUM_SIZE 2                    /*!< (unspecified)                                                        */

#define DPPIC136_HASCHANNELGROUPS 1                  /*!< (unspecified)                                                        */
#define DPPIC136_CH_NUM_MIN 0                        /*!< (unspecified)                                                        */
#define DPPIC136_CH_NUM_MAX 7                        /*!< (unspecified)                                                        */
#define DPPIC136_CH_NUM_SIZE 8                       /*!< (unspecified)                                                        */
#define DPPIC136_GROUP_NUM_MIN 0                     /*!< (unspecified)                                                        */
#define DPPIC136_GROUP_NUM_MAX 1                     /*!< (unspecified)                                                        */
#define DPPIC136_GROUP_NUM_SIZE 2                    /*!< (unspecified)                                                        */

/*PPIB APB registers*/
#define PPIB_PRESENT 1
#define PPIB_COUNT 2

#define PPIB020_NTASKSEVENTS_MIN 0                   /*!< (unspecified)                                                        */
#define PPIB020_NTASKSEVENTS_MAX 15                  /*!< (unspecified)                                                        */
#define PPIB020_NTASKSEVENTS_SIZE 16                 /*!< (unspecified)                                                        */

#define PPIB030_NTASKSEVENTS_MIN 0                   /*!< (unspecified)                                                        */
#define PPIB030_NTASKSEVENTS_MAX 15                  /*!< (unspecified)                                                        */
#define PPIB030_NTASKSEVENTS_SIZE 16                 /*!< (unspecified)                                                        */

/*Event generator unit*/
#define EGU_PRESENT 1
#define EGU_COUNT 2

#define EGU020_PEND 0                                /*!< (unspecified)                                                        */
#define EGU020_CH_NUM_MIN 0                          /*!< (unspecified)                                                        */
#define EGU020_CH_NUM_MAX 15                         /*!< (unspecified)                                                        */
#define EGU020_CH_NUM_SIZE 16                        /*!< (unspecified)                                                        */

#define EGU130_PEND 0                                /*!< (unspecified)                                                        */
#define EGU130_CH_NUM_MIN 0                          /*!< (unspecified)                                                        */
#define EGU130_CH_NUM_MAX 7                          /*!< (unspecified)                                                        */
#define EGU130_CH_NUM_SIZE 8                         /*!< (unspecified)                                                        */

/*GPIO Tasks and Events*/
#define GPIOTE_PRESENT 1
#define GPIOTE_COUNT 2

#define GPIOTE_IRQ_COUNT 1
#define GPIOTE_GPIOTE_NCHANNELS_MIN 0                /*!< Number of GPIOTE channels: 0..7                                      */
#define GPIOTE_GPIOTE_NCHANNELS_MAX 7                /*!< Number of GPIOTE channels: 0..7                                      */
#define GPIOTE_GPIOTE_NCHANNELS_SIZE 8               /*!< Number of GPIOTE channels: 0..7                                      */
#define GPIOTE_GPIOTE_NPORTEVENTS 0                  /*!< Number of GPIOTE port events: 0                                      */
#define GPIOTE_GPIOTE_NINTERRUPTS_MIN 0              /*!< Number of GPIOTE interrupts: 0..0                                    */
#define GPIOTE_GPIOTE_NINTERRUPTS_MAX 0              /*!< Number of GPIOTE interrupts: 0..0                                    */
#define GPIOTE_GPIOTE_NINTERRUPTS_SIZE 1             /*!< Number of GPIOTE interrupts: 0..0                                    */
#define GPIOTE_HAS_PORT_EVENT 0                      /*!< (unspecified)                                                        */

#define GPIOTE130_IRQ_COUNT 2
#define GPIOTE130_GPIOTE_NCHANNELS_MIN 0             /*!< Number of GPIOTE channels: 0..7                                      */
#define GPIOTE130_GPIOTE_NCHANNELS_MAX 7             /*!< Number of GPIOTE channels: 0..7                                      */
#define GPIOTE130_GPIOTE_NCHANNELS_SIZE 8            /*!< Number of GPIOTE channels: 0..7                                      */
#define GPIOTE130_GPIOTE_NPORTEVENTS_MIN 0           /*!< Number of GPIOTE port events: 0..3                                   */
#define GPIOTE130_GPIOTE_NPORTEVENTS_MAX 3           /*!< Number of GPIOTE port events: 0..3                                   */
#define GPIOTE130_GPIOTE_NPORTEVENTS_SIZE 4          /*!< Number of GPIOTE port events: 0..3                                   */
#define GPIOTE130_GPIOTE_NINTERRUPTS_MIN 0           /*!< Number of GPIOTE interrupts: 0..1                                    */
#define GPIOTE130_GPIOTE_NINTERRUPTS_MAX 1           /*!< Number of GPIOTE interrupts: 0..1                                    */
#define GPIOTE130_GPIOTE_NINTERRUPTS_SIZE 2          /*!< Number of GPIOTE interrupts: 0..1                                    */
#define GPIOTE130_HAS_PORT_EVENT 1                   /*!< (unspecified)                                                        */

/*Timer/Counter*/
#define TIMER_PRESENT 1
#define TIMER_COUNT 13

#define TIMER020_CC_NUM_MIN 0                        /*!< (unspecified)                                                        */
#define TIMER020_CC_NUM_MAX 7                        /*!< (unspecified)                                                        */
#define TIMER020_CC_NUM_SIZE 8                       /*!< (unspecified)                                                        */
#define TIMER020_MAX_SIZE_MIN 0                      /*!< (unspecified)                                                        */
#define TIMER020_MAX_SIZE_MAX 31                     /*!< (unspecified)                                                        */
#define TIMER020_MAX_SIZE_SIZE 32                    /*!< (unspecified)                                                        */
#define TIMER020_PCLK_MHZ 32                         /*!< Peripheral clock frequency (PCLK) is 32 MHz                          */
#define TIMER020_PCLK_VARIABLE 0                     /*!< (unspecified)                                                        */
#define TIMER020_SHUTDOWN_TASK 0                     /*!< (unspecified)                                                        */

#define TIMER021_CC_NUM_MIN 0                        /*!< (unspecified)                                                        */
#define TIMER021_CC_NUM_MAX 7                        /*!< (unspecified)                                                        */
#define TIMER021_CC_NUM_SIZE 8                       /*!< (unspecified)                                                        */
#define TIMER021_MAX_SIZE_MIN 0                      /*!< (unspecified)                                                        */
#define TIMER021_MAX_SIZE_MAX 31                     /*!< (unspecified)                                                        */
#define TIMER021_MAX_SIZE_SIZE 32                    /*!< (unspecified)                                                        */
#define TIMER021_PCLK_MHZ 32                         /*!< Peripheral clock frequency (PCLK) is 32 MHz                          */
#define TIMER021_PCLK_VARIABLE 0                     /*!< (unspecified)                                                        */
#define TIMER021_SHUTDOWN_TASK 0                     /*!< (unspecified)                                                        */

#define TIMER022_CC_NUM_MIN 0                        /*!< (unspecified)                                                        */
#define TIMER022_CC_NUM_MAX 7                        /*!< (unspecified)                                                        */
#define TIMER022_CC_NUM_SIZE 8                       /*!< (unspecified)                                                        */
#define TIMER022_MAX_SIZE_MIN 0                      /*!< (unspecified)                                                        */
#define TIMER022_MAX_SIZE_MAX 31                     /*!< (unspecified)                                                        */
#define TIMER022_MAX_SIZE_SIZE 32                    /*!< (unspecified)                                                        */
#define TIMER022_PCLK_MHZ 32                         /*!< Peripheral clock frequency (PCLK) is 32 MHz                          */
#define TIMER022_PCLK_VARIABLE 0                     /*!< (unspecified)                                                        */
#define TIMER022_SHUTDOWN_TASK 0                     /*!< (unspecified)                                                        */

#define TIMER120_CC_NUM_MIN 0                        /*!< (unspecified)                                                        */
#define TIMER120_CC_NUM_MAX 5                        /*!< (unspecified)                                                        */
#define TIMER120_CC_NUM_SIZE 6                       /*!< (unspecified)                                                        */
#define TIMER120_MAX_SIZE_MIN 0                      /*!< (unspecified)                                                        */
#define TIMER120_MAX_SIZE_MAX 31                     /*!< (unspecified)                                                        */
#define TIMER120_MAX_SIZE_SIZE 32                    /*!< (unspecified)                                                        */
#define TIMER120_PCLK_MHZ 320                        /*!< Peripheral clock frequency (PCLK) is 320 MHz                         */
#define TIMER120_PCLK_VARIABLE 0                     /*!< (unspecified)                                                        */
#define TIMER120_SHUTDOWN_TASK 0                     /*!< (unspecified)                                                        */

#define TIMER121_CC_NUM_MIN 0                        /*!< (unspecified)                                                        */
#define TIMER121_CC_NUM_MAX 5                        /*!< (unspecified)                                                        */
#define TIMER121_CC_NUM_SIZE 6                       /*!< (unspecified)                                                        */
#define TIMER121_MAX_SIZE_MIN 0                      /*!< (unspecified)                                                        */
#define TIMER121_MAX_SIZE_MAX 31                     /*!< (unspecified)                                                        */
#define TIMER121_MAX_SIZE_SIZE 32                    /*!< (unspecified)                                                        */
#define TIMER121_PCLK_MHZ 320                        /*!< Peripheral clock frequency (PCLK) is 320 MHz                         */
#define TIMER121_PCLK_VARIABLE 0                     /*!< (unspecified)                                                        */
#define TIMER121_SHUTDOWN_TASK 0                     /*!< (unspecified)                                                        */

#define TIMER130_CC_NUM_MIN 0                        /*!< (unspecified)                                                        */
#define TIMER130_CC_NUM_MAX 5                        /*!< (unspecified)                                                        */
#define TIMER130_CC_NUM_SIZE 6                       /*!< (unspecified)                                                        */
#define TIMER130_MAX_SIZE_MIN 0                      /*!< (unspecified)                                                        */
#define TIMER130_MAX_SIZE_MAX 31                     /*!< (unspecified)                                                        */
#define TIMER130_MAX_SIZE_SIZE 32                    /*!< (unspecified)                                                        */
#define TIMER130_PCLK_MHZ 16                         /*!< Peripheral clock frequency (PCLK) is 16 MHz                          */
#define TIMER130_PCLK_VARIABLE 0                     /*!< (unspecified)                                                        */
#define TIMER130_SHUTDOWN_TASK 0                     /*!< (unspecified)                                                        */

#define TIMER131_CC_NUM_MIN 0                        /*!< (unspecified)                                                        */
#define TIMER131_CC_NUM_MAX 5                        /*!< (unspecified)                                                        */
#define TIMER131_CC_NUM_SIZE 6                       /*!< (unspecified)                                                        */
#define TIMER131_MAX_SIZE_MIN 0                      /*!< (unspecified)                                                        */
#define TIMER131_MAX_SIZE_MAX 31                     /*!< (unspecified)                                                        */
#define TIMER131_MAX_SIZE_SIZE 32                    /*!< (unspecified)                                                        */
#define TIMER131_PCLK_MHZ 16                         /*!< Peripheral clock frequency (PCLK) is 16 MHz                          */
#define TIMER131_PCLK_VARIABLE 0                     /*!< (unspecified)                                                        */
#define TIMER131_SHUTDOWN_TASK 0                     /*!< (unspecified)                                                        */

#define TIMER132_CC_NUM_MIN 0                        /*!< (unspecified)                                                        */
#define TIMER132_CC_NUM_MAX 5                        /*!< (unspecified)                                                        */
#define TIMER132_CC_NUM_SIZE 6                       /*!< (unspecified)                                                        */
#define TIMER132_MAX_SIZE_MIN 0                      /*!< (unspecified)                                                        */
#define TIMER132_MAX_SIZE_MAX 31                     /*!< (unspecified)                                                        */
#define TIMER132_MAX_SIZE_SIZE 32                    /*!< (unspecified)                                                        */
#define TIMER132_PCLK_MHZ 16                         /*!< Peripheral clock frequency (PCLK) is 16 MHz                          */
#define TIMER132_PCLK_VARIABLE 0                     /*!< (unspecified)                                                        */
#define TIMER132_SHUTDOWN_TASK 0                     /*!< (unspecified)                                                        */

#define TIMER133_CC_NUM_MIN 0                        /*!< (unspecified)                                                        */
#define TIMER133_CC_NUM_MAX 5                        /*!< (unspecified)                                                        */
#define TIMER133_CC_NUM_SIZE 6                       /*!< (unspecified)                                                        */
#define TIMER133_MAX_SIZE_MIN 0                      /*!< (unspecified)                                                        */
#define TIMER133_MAX_SIZE_MAX 31                     /*!< (unspecified)                                                        */
#define TIMER133_MAX_SIZE_SIZE 32                    /*!< (unspecified)                                                        */
#define TIMER133_PCLK_MHZ 16                         /*!< Peripheral clock frequency (PCLK) is 16 MHz                          */
#define TIMER133_PCLK_VARIABLE 0                     /*!< (unspecified)                                                        */
#define TIMER133_SHUTDOWN_TASK 0                     /*!< (unspecified)                                                        */

#define TIMER134_CC_NUM_MIN 0                        /*!< (unspecified)                                                        */
#define TIMER134_CC_NUM_MAX 5                        /*!< (unspecified)                                                        */
#define TIMER134_CC_NUM_SIZE 6                       /*!< (unspecified)                                                        */
#define TIMER134_MAX_SIZE_MIN 0                      /*!< (unspecified)                                                        */
#define TIMER134_MAX_SIZE_MAX 31                     /*!< (unspecified)                                                        */
#define TIMER134_MAX_SIZE_SIZE 32                    /*!< (unspecified)                                                        */
#define TIMER134_PCLK_MHZ 16                         /*!< Peripheral clock frequency (PCLK) is 16 MHz                          */
#define TIMER134_PCLK_VARIABLE 0                     /*!< (unspecified)                                                        */
#define TIMER134_SHUTDOWN_TASK 0                     /*!< (unspecified)                                                        */

#define TIMER135_CC_NUM_MIN 0                        /*!< (unspecified)                                                        */
#define TIMER135_CC_NUM_MAX 5                        /*!< (unspecified)                                                        */
#define TIMER135_CC_NUM_SIZE 6                       /*!< (unspecified)                                                        */
#define TIMER135_MAX_SIZE_MIN 0                      /*!< (unspecified)                                                        */
#define TIMER135_MAX_SIZE_MAX 31                     /*!< (unspecified)                                                        */
#define TIMER135_MAX_SIZE_SIZE 32                    /*!< (unspecified)                                                        */
#define TIMER135_PCLK_MHZ 16                         /*!< Peripheral clock frequency (PCLK) is 16 MHz                          */
#define TIMER135_PCLK_VARIABLE 0                     /*!< (unspecified)                                                        */
#define TIMER135_SHUTDOWN_TASK 0                     /*!< (unspecified)                                                        */

#define TIMER136_CC_NUM_MIN 0                        /*!< (unspecified)                                                        */
#define TIMER136_CC_NUM_MAX 5                        /*!< (unspecified)                                                        */
#define TIMER136_CC_NUM_SIZE 6                       /*!< (unspecified)                                                        */
#define TIMER136_MAX_SIZE_MIN 0                      /*!< (unspecified)                                                        */
#define TIMER136_MAX_SIZE_MAX 31                     /*!< (unspecified)                                                        */
#define TIMER136_MAX_SIZE_SIZE 32                    /*!< (unspecified)                                                        */
#define TIMER136_PCLK_MHZ 16                         /*!< Peripheral clock frequency (PCLK) is 16 MHz                          */
#define TIMER136_PCLK_VARIABLE 0                     /*!< (unspecified)                                                        */
#define TIMER136_SHUTDOWN_TASK 0                     /*!< (unspecified)                                                        */

#define TIMER137_CC_NUM_MIN 0                        /*!< (unspecified)                                                        */
#define TIMER137_CC_NUM_MAX 5                        /*!< (unspecified)                                                        */
#define TIMER137_CC_NUM_SIZE 6                       /*!< (unspecified)                                                        */
#define TIMER137_MAX_SIZE_MIN 0                      /*!< (unspecified)                                                        */
#define TIMER137_MAX_SIZE_MAX 31                     /*!< (unspecified)                                                        */
#define TIMER137_MAX_SIZE_SIZE 32                    /*!< (unspecified)                                                        */
#define TIMER137_PCLK_MHZ 16                         /*!< Peripheral clock frequency (PCLK) is 16 MHz                          */
#define TIMER137_PCLK_VARIABLE 0                     /*!< (unspecified)                                                        */
#define TIMER137_SHUTDOWN_TASK 0                     /*!< (unspecified)                                                        */

/*Real-time counter*/
#define RTC_PRESENT 1
#define RTC_COUNT 3

#define RTC_CC_NUM_MIN 0                             /*!< (unspecified)                                                        */
#define RTC_CC_NUM_MAX 7                             /*!< (unspecified)                                                        */
#define RTC_CC_NUM_SIZE 8                            /*!< (unspecified)                                                        */
#define RTC_BIT_WIDTH_MIN 0                          /*!< (unspecified)                                                        */
#define RTC_BIT_WIDTH_MAX 23                         /*!< (unspecified)                                                        */
#define RTC_BIT_WIDTH_SIZE 24                        /*!< (unspecified)                                                        */
#define RTC_LFCLK_ENABLE 0                           /*!< (unspecified)                                                        */

#define RTC130_CC_NUM_MIN 0                          /*!< (unspecified)                                                        */
#define RTC130_CC_NUM_MAX 3                          /*!< (unspecified)                                                        */
#define RTC130_CC_NUM_SIZE 4                         /*!< (unspecified)                                                        */
#define RTC130_BIT_WIDTH_MIN 0                       /*!< (unspecified)                                                        */
#define RTC130_BIT_WIDTH_MAX 23                      /*!< (unspecified)                                                        */
#define RTC130_BIT_WIDTH_SIZE 24                     /*!< (unspecified)                                                        */
#define RTC130_LFCLK_ENABLE 0                        /*!< (unspecified)                                                        */

#define RTC131_CC_NUM_MIN 0                          /*!< (unspecified)                                                        */
#define RTC131_CC_NUM_MAX 3                          /*!< (unspecified)                                                        */
#define RTC131_CC_NUM_SIZE 4                         /*!< (unspecified)                                                        */
#define RTC131_BIT_WIDTH_MIN 0                       /*!< (unspecified)                                                        */
#define RTC131_BIT_WIDTH_MAX 23                      /*!< (unspecified)                                                        */
#define RTC131_BIT_WIDTH_SIZE 24                     /*!< (unspecified)                                                        */
#define RTC131_LFCLK_ENABLE 0                        /*!< (unspecified)                                                        */

/*2.4 GHz radio*/
#define RADIO_PRESENT 1
#define RADIO_COUNT 1

#define RADIO_IRQ_COUNT 2
#define RADIO_WHITENINGPOLY 0                        /*!< (unspecified)                                                        */
#define RADIO_ADPLLCOMPANION_INCLUDE_DMA 0           /*!< (unspecified)                                                        */

/*Accelerated Address Resolver*/
#define AAR_PRESENT 1
#define AAR_COUNT 2

#define AAR030_DMAERROR 1                            /*!< (unspecified)                                                        */
#define AAR030_ERRORSTATUS 0                         /*!< (unspecified)                                                        */
#define AAR030_ERROREVENT 0                          /*!< (unspecified)                                                        */

#define AAR031_DMAERROR 1                            /*!< (unspecified)                                                        */
#define AAR031_ERRORSTATUS 0                         /*!< (unspecified)                                                        */
#define AAR031_ERROREVENT 0                          /*!< (unspecified)                                                        */

/*AES CCM Mode Encryption*/
#define CCM_PRESENT 1
#define CCM_COUNT 2

#define CCM030_AMOUNTREG 0                           /*!< (unspecified)                                                        */
#define CCM030_ONTHEFLYDECRYPTION 0                  /*!< (unspecified)                                                        */
#define CCM030_DMAERROR 1                            /*!< (unspecified)                                                        */

#define CCM031_AMOUNTREG 0                           /*!< (unspecified)                                                        */
#define CCM031_ONTHEFLYDECRYPTION 0                  /*!< (unspecified)                                                        */
#define CCM031_DMAERROR 1                            /*!< (unspecified)                                                        */

/*AES ECB Mode Encryption*/
#define ECB_PRESENT 1
#define ECB_COUNT 2

#define ECB030_AMOUNTREG 0                           /*!< (unspecified)                                                        */
#define ECB030_DMAERROR 1                            /*!< (unspecified)                                                        */
#define ECB030_ERRORSTATUS 0                         /*!< (unspecified)                                                        */

#define ECB031_AMOUNTREG 0                           /*!< (unspecified)                                                        */
#define ECB031_DMAERROR 1                            /*!< (unspecified)                                                        */
#define ECB031_ERRORSTATUS 0                         /*!< (unspecified)                                                        */

/*IPCT APB registers*/
#define IPCT_PRESENT 1
#define IPCT_COUNT 3

#define IPCT_IRQ_COUNT 2

#define IPCT120_IRQ_COUNT 1

#define IPCT130_IRQ_COUNT 1

/*Software interrupt*/
#define SWI_PRESENT 1
#define SWI_COUNT 8

/*BELLBOARD APB registers*/
#define BELLBOARD_PRESENT 1
#define BELLBOARD_COUNT 1

#define BELLBOARD_IRQ_COUNT 4

/*Factory Information Configuration Registers*/
#define FICR_PRESENT 1
#define FICR_COUNT 1

/*USBHSCORE*/
#define USBHSCORE_PRESENT 1
#define USBHSCORE_COUNT 1

/*MCAN*/
#define MCAN_PRESENT 1
#define MCAN_COUNT 1

/*DMU*/
#define DMU_PRESENT 1
#define DMU_COUNT 1

/*System Trace Macrocell data buffer*/
#define STMDATA_PRESENT 1
#define STMDATA_COUNT 1

/*TDDCONF*/
#define TDDCONF_PRESENT 1
#define TDDCONF_COUNT 1

#define TDDCONF_FEATEN_TDDCONF_CLK_320MHZ 1          /*!< (unspecified)                                                        */
#define TDDCONF_FEATEN_TDDCONF_CLK_400MHZ 0          /*!< (unspecified)                                                        */

/*System Trace Macrocell*/
#define STM_PRESENT 1
#define STM_COUNT 1

/*Trace Port Interface Unit*/
#define TPIU_PRESENT 1
#define TPIU_COUNT 1

/*Embedded Trace Buffer*/
#define ETB_PRESENT 1
#define ETB_COUNT 1

/*ATB Replicator module*/
#define ATBREPLICATOR_PRESENT 1
#define ATBREPLICATOR_COUNT 4

/*ATB funnel module*/
#define ATBFUNNEL_PRESENT 1
#define ATBFUNNEL_COUNT 4

/*Granular Power Requester*/
#define GPR_PRESENT 1
#define GPR_COUNT 1

#define GPR_NUM_CPWRUPM 9                            /*!< Number of power-control interfaces: 9                                */

/*Global Real-time counter*/
#define GRTC_PRESENT 1
#define GRTC_COUNT 1

#define GRTC_IRQ_COUNT 3
#define GRTC_MSBWIDTH_MIN 0                          /*!< Width of the RTCOUNTERH, RTCOMPAREH and RTCOMPARESYNCH registers :
                                                          0..14*/
#define GRTC_MSBWIDTH_MAX 14                         /*!< Width of the RTCOUNTERH, RTCOMPAREH and RTCOMPARESYNCH registers :
                                                          0..14*/
#define GRTC_MSBWIDTH_SIZE 15                        /*!< Width of the RTCOUNTERH, RTCOMPAREH and RTCOMPARESYNCH registers :
                                                          0..14*/
#define GRTC_NCC_MIN 0                               /*!< Number of compare/capture registers : 0..15                          */
#define GRTC_NCC_MAX 15                              /*!< Number of compare/capture registers : 0..15                          */
#define GRTC_NCC_SIZE 16                             /*!< Number of compare/capture registers : 0..15                          */
#define GRTC_NTIMEOUT_MIN 0                          /*!< Width of the TIMEOUT register : 0..15                                */
#define GRTC_NTIMEOUT_MAX 15                         /*!< Width of the TIMEOUT register : 0..15                                */
#define GRTC_NTIMEOUT_SIZE 16                        /*!< Width of the TIMEOUT register : 0..15                                */
#define GRTC_GRTC_NINTERRUPTS_MIN 0                  /*!< Number of GRTC interrupts : 0..2                                     */
#define GRTC_GRTC_NINTERRUPTS_MAX 2                  /*!< Number of GRTC interrupts : 0..2                                     */
#define GRTC_GRTC_NINTERRUPTS_SIZE 3                 /*!< Number of GRTC interrupts : 0..2                                     */
#define GRTC_PWMREGS 1                               /*!< (unspecified)                                                        */
#define GRTC_CLKOUTREG 1                             /*!< (unspecified)                                                        */
#define GRTC_CLKSELREG 1                             /*!< (unspecified)                                                        */
#define GRTC_CLKSELLFLPRC 1                          /*!< (unspecified)                                                        */
#define GRTC_CCADD_WRITE_ONLY 1                      /*!< (unspecified)                                                        */
#define GRTC_READY_STATUS_AND_EVENTS 0               /*!< (unspecified)                                                        */
#define GRTC_SYSCOUNTER_LOADED_STATUS 0              /*!< (unspecified)                                                        */
#define GRTC_CC_PAST_STATUS 0                        /*!< (unspecified)                                                        */
#define GRTC_SYSCOUNTER_WRITEABLE 0                  /*!< (unspecified)                                                        */

/*Trace buffer monitor*/
#define TBM_PRESENT 1
#define TBM_COUNT 1

/*USBHS*/
#define USBHS_PRESENT 1
#define USBHS_COUNT 1

#define USBHS_HAS_CORE_EVENT 1                       /*!< (unspecified)                                                        */
#define USBHS_HAS_SOF_EVENT 0                        /*!< (unspecified)                                                        */
#define USBHS_RTUNE_AVAILABLE 0                      /*!< (unspecified)                                                        */

/*External Memory Interface*/
#define EXMIF_PRESENT 1
#define EXMIF_COUNT 1

/*BELLBOARD public registers*/
#define BELLBOARDPUBLIC_PRESENT 1
#define BELLBOARDPUBLIC_COUNT 1

/*AUXPLL*/
#define AUXPLL_PRESENT 1
#define AUXPLL_COUNT 1

/*VPR peripheral registers*/
#define VPRPUBLIC_PRESENT 1
#define VPRPUBLIC_COUNT 1

#define VPR120_VEVIF_NTASKS_MIN 0                    /*!< VEVIF tasks: 0..31                                                   */
#define VPR120_VEVIF_NTASKS_MAX 31                   /*!< VEVIF tasks: 0..31                                                   */
#define VPR120_VEVIF_NTASKS_SIZE 32                  /*!< VEVIF tasks: 0..31                                                   */
#define VPR120_VEVIF_TASKS_MASK 0xFFFFF0FF           /*!< Mask of supported VEVIF tasks: 0xFFFFF0FF                            */

/*MUTEX*/
#define MUTEX_PRESENT 1
#define MUTEX_COUNT 2

/*VPR peripheral registers*/
#define VPR_PRESENT 1
#define VPR_COUNT 2

#define VPR121_INIT_PC_RESET_VALUE 0x00000000        /*!< Boot vector (INIT_PC_RESET_VALUE): 0x00000000                        */
#define VPR121_VPR_START_RESET_VALUE 0               /*!< Self-booting (VPR_START_RESET_VALUE): 0                              */
#define VPR121_RAM_BASE_ADDR 0x2F890000              /*!< (unspecified)                                                        */
#define VPR121_RAM_SZ 15                             /*!< (unspecified)                                                        */
#define VPR121_VPRSAVEDCTX_REGNAME NRF_MEMCONF120->POWER[0].RET /*!< (unspecified)                                             */
#define VPR121_VPRSAVEDCTX_REGBIT 23                 /*!< (unspecified)                                                        */
#define VPR121_RETAINED 0                            /*!< (unspecified)                                                        */
#define VPR121_VPRSAVEDCTX 1                         /*!< (unspecified)                                                        */
#define VPR121_VPRSAVEADDR 0x2F800000                /*!< (unspecified)                                                        */
#define VPR121_VPRREMAPADDRVTOB 0x00000000           /*!< (unspecified)                                                        */
#define VPR121_VEVIF_NTASKS_MIN 0                    /*!< VEVIF tasks: 0..31                                                   */
#define VPR121_VEVIF_NTASKS_MAX 31                   /*!< VEVIF tasks: 0..31                                                   */
#define VPR121_VEVIF_NTASKS_SIZE 32                  /*!< VEVIF tasks: 0..31                                                   */
#define VPR121_VEVIF_TASKS_MASK 0xFFFF0000           /*!< Mask of supported VEVIF tasks: 0xFFFF0000                            */
#define VPR121_VEVIF_NDPPI_MIN 24                    /*!< VEVIF DPPI channels: 24..27                                          */
#define VPR121_VEVIF_NDPPI_MAX 27                    /*!< VEVIF DPPI channels: 24..27                                          */
#define VPR121_VEVIF_NDPPI_SIZE 28                   /*!< VEVIF DPPI channels: 24..27                                          */
#define VPR121_VEVIF_NEVENTS_MIN 28                  /*!< VEVIF events: 28..31                                                 */
#define VPR121_VEVIF_NEVENTS_MAX 31                  /*!< VEVIF events: 28..31                                                 */
#define VPR121_VEVIF_NEVENTS_SIZE 32                 /*!< VEVIF events: 28..31                                                 */
#define VPR121_DEBUGGER_OFFSET 1024                  /*!< Debugger interface register offset: 0x5F8D4400                       */

#define VPR130_INIT_PC_RESET_VALUE 0x00000000        /*!< Boot vector (INIT_PC_RESET_VALUE): 0x00000000                        */
#define VPR130_VPR_START_RESET_VALUE 0               /*!< Self-booting (VPR_START_RESET_VALUE): 0                              */
#define VPR130_RAM_BASE_ADDR 0x2FC00000              /*!< (unspecified)                                                        */
#define VPR130_RAM_SZ 15                             /*!< (unspecified)                                                        */
#define VPR130_VPRSAVEDCTX_REGNAME NRF_MEMCONF130->POWER[0].RET /*!< (unspecified)                                             */
#define VPR130_VPRSAVEDCTX_REGBIT 5                  /*!< (unspecified)                                                        */
#define VPR130_RETAINED 1                            /*!< (unspecified)                                                        */
#define VPR130_VPRSAVEDCTX 1                         /*!< (unspecified)                                                        */
#define VPR130_VPRSAVEADDR 0x2F800000                /*!< (unspecified)                                                        */
#define VPR130_VPRREMAPADDRVTOB 0x00000000           /*!< (unspecified)                                                        */
#define VPR130_VEVIF_NTASKS_MIN 0                    /*!< VEVIF tasks: 0..15                                                   */
#define VPR130_VEVIF_NTASKS_MAX 15                   /*!< VEVIF tasks: 0..15                                                   */
#define VPR130_VEVIF_NTASKS_SIZE 16                  /*!< VEVIF tasks: 0..15                                                   */
#define VPR130_VEVIF_TASKS_MASK 0x0000FFF0           /*!< Mask of supported VEVIF tasks: 0x0000FFF0                            */
#define VPR130_VEVIF_NDPPI_MIN 8                     /*!< VEVIF DPPI channels: 8..11                                           */
#define VPR130_VEVIF_NDPPI_MAX 11                    /*!< VEVIF DPPI channels: 8..11                                           */
#define VPR130_VEVIF_NDPPI_SIZE 12                   /*!< VEVIF DPPI channels: 8..11                                           */
#define VPR130_VEVIF_NEVENTS_MIN 12                  /*!< VEVIF events: 12..15                                                 */
#define VPR130_VEVIF_NEVENTS_MAX 15                  /*!< VEVIF events: 12..15                                                 */
#define VPR130_VEVIF_NEVENTS_SIZE 16                 /*!< VEVIF events: 12..15                                                 */
#define VPR130_DEBUGGER_OFFSET 1024                  /*!< Debugger interface register offset: 0x5F908400                       */

/*Controller Area Network*/
#define CAN_PRESENT 1
#define CAN_COUNT 1

/*Pulse width modulation unit*/
#define PWM_PRESENT 1
#define PWM_COUNT 5

#define PWM120_IDLE_OUT 1                            /*!< (unspecified)                                                        */
#define PWM120_COMPARE_MATCH 1                       /*!< (unspecified)                                                        */
#define PWM120_FEATURES_V2 0                         /*!< (unspecified)                                                        */
#define PWM120_NO_FEATURES_V2 1                      /*!< (unspecified)                                                        */
#define PWM120_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 1 /*!< (unspecified)                                                   */

#define PWM130_IDLE_OUT 1                            /*!< (unspecified)                                                        */
#define PWM130_COMPARE_MATCH 1                       /*!< (unspecified)                                                        */
#define PWM130_FEATURES_V2 0                         /*!< (unspecified)                                                        */
#define PWM130_NO_FEATURES_V2 1                      /*!< (unspecified)                                                        */
#define PWM130_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 1 /*!< (unspecified)                                                   */

#define PWM131_IDLE_OUT 1                            /*!< (unspecified)                                                        */
#define PWM131_COMPARE_MATCH 1                       /*!< (unspecified)                                                        */
#define PWM131_FEATURES_V2 0                         /*!< (unspecified)                                                        */
#define PWM131_NO_FEATURES_V2 1                      /*!< (unspecified)                                                        */
#define PWM131_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 1 /*!< (unspecified)                                                   */

#define PWM132_IDLE_OUT 1                            /*!< (unspecified)                                                        */
#define PWM132_COMPARE_MATCH 1                       /*!< (unspecified)                                                        */
#define PWM132_FEATURES_V2 0                         /*!< (unspecified)                                                        */
#define PWM132_NO_FEATURES_V2 1                      /*!< (unspecified)                                                        */
#define PWM132_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 1 /*!< (unspecified)                                                   */

#define PWM133_IDLE_OUT 1                            /*!< (unspecified)                                                        */
#define PWM133_COMPARE_MATCH 1                       /*!< (unspecified)                                                        */
#define PWM133_FEATURES_V2 0                         /*!< (unspecified)                                                        */
#define PWM133_NO_FEATURES_V2 1                      /*!< (unspecified)                                                        */
#define PWM133_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 1 /*!< (unspecified)                                                   */

/*SPI Slave*/
#define SPIS_PRESENT 1
#define SPIS_COUNT 9

#define SPIS120_EASYDMA_MAXCNT_MIN 0                 /*!< (unspecified)                                                        */
#define SPIS120_EASYDMA_MAXCNT_MAX 15                /*!< (unspecified)                                                        */
#define SPIS120_EASYDMA_MAXCNT_SIZE 16               /*!< (unspecified)                                                        */
#define SPIS120_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 1 /*!< (unspecified)                                                  */

#define SPIS130_EASYDMA_MAXCNT_MIN 0                 /*!< (unspecified)                                                        */
#define SPIS130_EASYDMA_MAXCNT_MAX 15                /*!< (unspecified)                                                        */
#define SPIS130_EASYDMA_MAXCNT_SIZE 16               /*!< (unspecified)                                                        */
#define SPIS130_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 1 /*!< (unspecified)                                                  */

#define SPIS131_EASYDMA_MAXCNT_MIN 0                 /*!< (unspecified)                                                        */
#define SPIS131_EASYDMA_MAXCNT_MAX 15                /*!< (unspecified)                                                        */
#define SPIS131_EASYDMA_MAXCNT_SIZE 16               /*!< (unspecified)                                                        */
#define SPIS131_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 1 /*!< (unspecified)                                                  */

#define SPIS132_EASYDMA_MAXCNT_MIN 0                 /*!< (unspecified)                                                        */
#define SPIS132_EASYDMA_MAXCNT_MAX 15                /*!< (unspecified)                                                        */
#define SPIS132_EASYDMA_MAXCNT_SIZE 16               /*!< (unspecified)                                                        */
#define SPIS132_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 1 /*!< (unspecified)                                                  */

#define SPIS133_EASYDMA_MAXCNT_MIN 0                 /*!< (unspecified)                                                        */
#define SPIS133_EASYDMA_MAXCNT_MAX 15                /*!< (unspecified)                                                        */
#define SPIS133_EASYDMA_MAXCNT_SIZE 16               /*!< (unspecified)                                                        */
#define SPIS133_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 1 /*!< (unspecified)                                                  */

#define SPIS134_EASYDMA_MAXCNT_MIN 0                 /*!< (unspecified)                                                        */
#define SPIS134_EASYDMA_MAXCNT_MAX 15                /*!< (unspecified)                                                        */
#define SPIS134_EASYDMA_MAXCNT_SIZE 16               /*!< (unspecified)                                                        */
#define SPIS134_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 1 /*!< (unspecified)                                                  */

#define SPIS135_EASYDMA_MAXCNT_MIN 0                 /*!< (unspecified)                                                        */
#define SPIS135_EASYDMA_MAXCNT_MAX 15                /*!< (unspecified)                                                        */
#define SPIS135_EASYDMA_MAXCNT_SIZE 16               /*!< (unspecified)                                                        */
#define SPIS135_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 1 /*!< (unspecified)                                                  */

#define SPIS136_EASYDMA_MAXCNT_MIN 0                 /*!< (unspecified)                                                        */
#define SPIS136_EASYDMA_MAXCNT_MAX 15                /*!< (unspecified)                                                        */
#define SPIS136_EASYDMA_MAXCNT_SIZE 16               /*!< (unspecified)                                                        */
#define SPIS136_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 1 /*!< (unspecified)                                                  */

#define SPIS137_EASYDMA_MAXCNT_MIN 0                 /*!< (unspecified)                                                        */
#define SPIS137_EASYDMA_MAXCNT_MAX 15                /*!< (unspecified)                                                        */
#define SPIS137_EASYDMA_MAXCNT_SIZE 16               /*!< (unspecified)                                                        */
#define SPIS137_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 1 /*!< (unspecified)                                                  */

/*Serial Peripheral Interface Master with EasyDMA*/
#define SPIM_PRESENT 1
#define SPIM_COUNT 10

#define SPIM120_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 1 /*!< (unspecified)                                                  */
#define SPIM120_MAX_DATARATE 32                      /*!< (unspecified)                                                        */
#define SPIM120_EASYDMA_MAXCNT_MIN 0                 /*!< (unspecified)                                                        */
#define SPIM120_EASYDMA_MAXCNT_MAX 15                /*!< (unspecified)                                                        */
#define SPIM120_EASYDMA_MAXCNT_SIZE 16               /*!< (unspecified)                                                        */
#define SPIM120_FEATURE_HARDWARE_CSN_PRESENT 1       /*!< (unspecified)                                                        */
#define SPIM120_FEATURE_HARDWARE_DCX_PRESENT 1       /*!< (unspecified)                                                        */
#define SPIM120_FEATURE_RXDELAY_PRESENT 1            /*!< (unspecified)                                                        */
#define SPIM120_STALL_STATUS_PRESENT 0               /*!< (unspecified)                                                        */
#define SPIM120_STALL_STATUS_TX_PRESENT 1            /*!< (unspecified)                                                        */
#define SPIM120_NUM_CHIPSELECT_MIN 0                 /*!< (unspecified)                                                        */
#define SPIM120_NUM_CHIPSELECT_MAX 0                 /*!< (unspecified)                                                        */
#define SPIM120_NUM_CHIPSELECT_SIZE 1                /*!< (unspecified)                                                        */
#define SPIM120_CORE_FREQUENCY 320                   /*!< Peripheral core frequency is 320 MHz.                                */
#define SPIM120_PRESCALER_PRESENT 1                  /*!< (unspecified)                                                        */
#define SPIM120_PRESCALER_DIVISOR_RANGE_MIN 4        /*!< (unspecified)                                                        */
#define SPIM120_PRESCALER_DIVISOR_RANGE_MAX 126      /*!< (unspecified)                                                        */
#define SPIM120_PRESCALER_DIVISOR_RANGE_SIZE 127     /*!< (unspecified)                                                        */
#define SPIM120_RXDELAY_VALUE_RANGE_MIN 0            /*!< (unspecified)                                                        */
#define SPIM120_RXDELAY_VALUE_RANGE_MAX 40           /*!< (unspecified)                                                        */
#define SPIM120_RXDELAY_VALUE_RANGE_SIZE 41          /*!< (unspecified)                                                        */
#define SPIM120_RXDELAY_RESET_VALUE 2                /*!< (unspecified)                                                        */
#define SPIM120_RXDELAY_FIELD_WIDTH_MIN 0            /*!< (unspecified)                                                        */
#define SPIM120_RXDELAY_FIELD_WIDTH_MAX 5            /*!< (unspecified)                                                        */
#define SPIM120_RXDELAY_FIELD_WIDTH_SIZE 6           /*!< (unspecified)                                                        */

#define SPIM121_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 1 /*!< (unspecified)                                                  */
#define SPIM121_MAX_DATARATE 32                      /*!< (unspecified)                                                        */
#define SPIM121_EASYDMA_MAXCNT_MIN 0                 /*!< (unspecified)                                                        */
#define SPIM121_EASYDMA_MAXCNT_MAX 15                /*!< (unspecified)                                                        */
#define SPIM121_EASYDMA_MAXCNT_SIZE 16               /*!< (unspecified)                                                        */
#define SPIM121_FEATURE_HARDWARE_CSN_PRESENT 1       /*!< (unspecified)                                                        */
#define SPIM121_FEATURE_HARDWARE_DCX_PRESENT 1       /*!< (unspecified)                                                        */
#define SPIM121_FEATURE_RXDELAY_PRESENT 1            /*!< (unspecified)                                                        */
#define SPIM121_STALL_STATUS_PRESENT 0               /*!< (unspecified)                                                        */
#define SPIM121_STALL_STATUS_TX_PRESENT 1            /*!< (unspecified)                                                        */
#define SPIM121_NUM_CHIPSELECT_MIN 0                 /*!< (unspecified)                                                        */
#define SPIM121_NUM_CHIPSELECT_MAX 0                 /*!< (unspecified)                                                        */
#define SPIM121_NUM_CHIPSELECT_SIZE 1                /*!< (unspecified)                                                        */
#define SPIM121_CORE_FREQUENCY 320                   /*!< Peripheral core frequency is 320 MHz.                                */
#define SPIM121_PRESCALER_PRESENT 1                  /*!< (unspecified)                                                        */
#define SPIM121_PRESCALER_DIVISOR_RANGE_MIN 4        /*!< (unspecified)                                                        */
#define SPIM121_PRESCALER_DIVISOR_RANGE_MAX 126      /*!< (unspecified)                                                        */
#define SPIM121_PRESCALER_DIVISOR_RANGE_SIZE 127     /*!< (unspecified)                                                        */
#define SPIM121_RXDELAY_VALUE_RANGE_MIN 0            /*!< (unspecified)                                                        */
#define SPIM121_RXDELAY_VALUE_RANGE_MAX 40           /*!< (unspecified)                                                        */
#define SPIM121_RXDELAY_VALUE_RANGE_SIZE 41          /*!< (unspecified)                                                        */
#define SPIM121_RXDELAY_RESET_VALUE 2                /*!< (unspecified)                                                        */
#define SPIM121_RXDELAY_FIELD_WIDTH_MIN 0            /*!< (unspecified)                                                        */
#define SPIM121_RXDELAY_FIELD_WIDTH_MAX 5            /*!< (unspecified)                                                        */
#define SPIM121_RXDELAY_FIELD_WIDTH_SIZE 6           /*!< (unspecified)                                                        */

#define SPIM130_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 1 /*!< (unspecified)                                                  */
#define SPIM130_MAX_DATARATE 8                       /*!< (unspecified)                                                        */
#define SPIM130_EASYDMA_MAXCNT_MIN 0                 /*!< (unspecified)                                                        */
#define SPIM130_EASYDMA_MAXCNT_MAX 15                /*!< (unspecified)                                                        */
#define SPIM130_EASYDMA_MAXCNT_SIZE 16               /*!< (unspecified)                                                        */
#define SPIM130_FEATURE_HARDWARE_CSN_PRESENT 1       /*!< (unspecified)                                                        */
#define SPIM130_FEATURE_HARDWARE_DCX_PRESENT 1       /*!< (unspecified)                                                        */
#define SPIM130_FEATURE_RXDELAY_PRESENT 1            /*!< (unspecified)                                                        */
#define SPIM130_STALL_STATUS_PRESENT 0               /*!< (unspecified)                                                        */
#define SPIM130_STALL_STATUS_TX_PRESENT 1            /*!< (unspecified)                                                        */
#define SPIM130_NUM_CHIPSELECT_MIN 0                 /*!< (unspecified)                                                        */
#define SPIM130_NUM_CHIPSELECT_MAX 0                 /*!< (unspecified)                                                        */
#define SPIM130_NUM_CHIPSELECT_SIZE 1                /*!< (unspecified)                                                        */
#define SPIM130_CORE_FREQUENCY 16                    /*!< Peripheral core frequency is 16 MHz.                                 */
#define SPIM130_PRESCALER_PRESENT 1                  /*!< (unspecified)                                                        */
#define SPIM130_PRESCALER_DIVISOR_RANGE_MIN 2        /*!< (unspecified)                                                        */
#define SPIM130_PRESCALER_DIVISOR_RANGE_MAX 126      /*!< (unspecified)                                                        */
#define SPIM130_PRESCALER_DIVISOR_RANGE_SIZE 127     /*!< (unspecified)                                                        */
#define SPIM130_RXDELAY_VALUE_RANGE_MIN 0            /*!< (unspecified)                                                        */
#define SPIM130_RXDELAY_VALUE_RANGE_MAX 40           /*!< (unspecified)                                                        */
#define SPIM130_RXDELAY_VALUE_RANGE_SIZE 41          /*!< (unspecified)                                                        */
#define SPIM130_RXDELAY_RESET_VALUE 2                /*!< (unspecified)                                                        */
#define SPIM130_RXDELAY_FIELD_WIDTH_MIN 0            /*!< (unspecified)                                                        */
#define SPIM130_RXDELAY_FIELD_WIDTH_MAX 5            /*!< (unspecified)                                                        */
#define SPIM130_RXDELAY_FIELD_WIDTH_SIZE 6           /*!< (unspecified)                                                        */

#define SPIM131_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 1 /*!< (unspecified)                                                  */
#define SPIM131_MAX_DATARATE 8                       /*!< (unspecified)                                                        */
#define SPIM131_EASYDMA_MAXCNT_MIN 0                 /*!< (unspecified)                                                        */
#define SPIM131_EASYDMA_MAXCNT_MAX 15                /*!< (unspecified)                                                        */
#define SPIM131_EASYDMA_MAXCNT_SIZE 16               /*!< (unspecified)                                                        */
#define SPIM131_FEATURE_HARDWARE_CSN_PRESENT 1       /*!< (unspecified)                                                        */
#define SPIM131_FEATURE_HARDWARE_DCX_PRESENT 1       /*!< (unspecified)                                                        */
#define SPIM131_FEATURE_RXDELAY_PRESENT 1            /*!< (unspecified)                                                        */
#define SPIM131_STALL_STATUS_PRESENT 0               /*!< (unspecified)                                                        */
#define SPIM131_STALL_STATUS_TX_PRESENT 1            /*!< (unspecified)                                                        */
#define SPIM131_NUM_CHIPSELECT_MIN 0                 /*!< (unspecified)                                                        */
#define SPIM131_NUM_CHIPSELECT_MAX 0                 /*!< (unspecified)                                                        */
#define SPIM131_NUM_CHIPSELECT_SIZE 1                /*!< (unspecified)                                                        */
#define SPIM131_CORE_FREQUENCY 16                    /*!< Peripheral core frequency is 16 MHz.                                 */
#define SPIM131_PRESCALER_PRESENT 1                  /*!< (unspecified)                                                        */
#define SPIM131_PRESCALER_DIVISOR_RANGE_MIN 2        /*!< (unspecified)                                                        */
#define SPIM131_PRESCALER_DIVISOR_RANGE_MAX 126      /*!< (unspecified)                                                        */
#define SPIM131_PRESCALER_DIVISOR_RANGE_SIZE 127     /*!< (unspecified)                                                        */
#define SPIM131_RXDELAY_VALUE_RANGE_MIN 0            /*!< (unspecified)                                                        */
#define SPIM131_RXDELAY_VALUE_RANGE_MAX 40           /*!< (unspecified)                                                        */
#define SPIM131_RXDELAY_VALUE_RANGE_SIZE 41          /*!< (unspecified)                                                        */
#define SPIM131_RXDELAY_RESET_VALUE 2                /*!< (unspecified)                                                        */
#define SPIM131_RXDELAY_FIELD_WIDTH_MIN 0            /*!< (unspecified)                                                        */
#define SPIM131_RXDELAY_FIELD_WIDTH_MAX 5            /*!< (unspecified)                                                        */
#define SPIM131_RXDELAY_FIELD_WIDTH_SIZE 6           /*!< (unspecified)                                                        */

#define SPIM132_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 1 /*!< (unspecified)                                                  */
#define SPIM132_MAX_DATARATE 8                       /*!< (unspecified)                                                        */
#define SPIM132_EASYDMA_MAXCNT_MIN 0                 /*!< (unspecified)                                                        */
#define SPIM132_EASYDMA_MAXCNT_MAX 15                /*!< (unspecified)                                                        */
#define SPIM132_EASYDMA_MAXCNT_SIZE 16               /*!< (unspecified)                                                        */
#define SPIM132_FEATURE_HARDWARE_CSN_PRESENT 1       /*!< (unspecified)                                                        */
#define SPIM132_FEATURE_HARDWARE_DCX_PRESENT 1       /*!< (unspecified)                                                        */
#define SPIM132_FEATURE_RXDELAY_PRESENT 1            /*!< (unspecified)                                                        */
#define SPIM132_STALL_STATUS_PRESENT 0               /*!< (unspecified)                                                        */
#define SPIM132_STALL_STATUS_TX_PRESENT 1            /*!< (unspecified)                                                        */
#define SPIM132_NUM_CHIPSELECT_MIN 0                 /*!< (unspecified)                                                        */
#define SPIM132_NUM_CHIPSELECT_MAX 0                 /*!< (unspecified)                                                        */
#define SPIM132_NUM_CHIPSELECT_SIZE 1                /*!< (unspecified)                                                        */
#define SPIM132_CORE_FREQUENCY 16                    /*!< Peripheral core frequency is 16 MHz.                                 */
#define SPIM132_PRESCALER_PRESENT 1                  /*!< (unspecified)                                                        */
#define SPIM132_PRESCALER_DIVISOR_RANGE_MIN 2        /*!< (unspecified)                                                        */
#define SPIM132_PRESCALER_DIVISOR_RANGE_MAX 126      /*!< (unspecified)                                                        */
#define SPIM132_PRESCALER_DIVISOR_RANGE_SIZE 127     /*!< (unspecified)                                                        */
#define SPIM132_RXDELAY_VALUE_RANGE_MIN 0            /*!< (unspecified)                                                        */
#define SPIM132_RXDELAY_VALUE_RANGE_MAX 40           /*!< (unspecified)                                                        */
#define SPIM132_RXDELAY_VALUE_RANGE_SIZE 41          /*!< (unspecified)                                                        */
#define SPIM132_RXDELAY_RESET_VALUE 2                /*!< (unspecified)                                                        */
#define SPIM132_RXDELAY_FIELD_WIDTH_MIN 0            /*!< (unspecified)                                                        */
#define SPIM132_RXDELAY_FIELD_WIDTH_MAX 5            /*!< (unspecified)                                                        */
#define SPIM132_RXDELAY_FIELD_WIDTH_SIZE 6           /*!< (unspecified)                                                        */

#define SPIM133_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 1 /*!< (unspecified)                                                  */
#define SPIM133_MAX_DATARATE 8                       /*!< (unspecified)                                                        */
#define SPIM133_EASYDMA_MAXCNT_MIN 0                 /*!< (unspecified)                                                        */
#define SPIM133_EASYDMA_MAXCNT_MAX 15                /*!< (unspecified)                                                        */
#define SPIM133_EASYDMA_MAXCNT_SIZE 16               /*!< (unspecified)                                                        */
#define SPIM133_FEATURE_HARDWARE_CSN_PRESENT 1       /*!< (unspecified)                                                        */
#define SPIM133_FEATURE_HARDWARE_DCX_PRESENT 1       /*!< (unspecified)                                                        */
#define SPIM133_FEATURE_RXDELAY_PRESENT 1            /*!< (unspecified)                                                        */
#define SPIM133_STALL_STATUS_PRESENT 0               /*!< (unspecified)                                                        */
#define SPIM133_STALL_STATUS_TX_PRESENT 1            /*!< (unspecified)                                                        */
#define SPIM133_NUM_CHIPSELECT_MIN 0                 /*!< (unspecified)                                                        */
#define SPIM133_NUM_CHIPSELECT_MAX 0                 /*!< (unspecified)                                                        */
#define SPIM133_NUM_CHIPSELECT_SIZE 1                /*!< (unspecified)                                                        */
#define SPIM133_CORE_FREQUENCY 16                    /*!< Peripheral core frequency is 16 MHz.                                 */
#define SPIM133_PRESCALER_PRESENT 1                  /*!< (unspecified)                                                        */
#define SPIM133_PRESCALER_DIVISOR_RANGE_MIN 2        /*!< (unspecified)                                                        */
#define SPIM133_PRESCALER_DIVISOR_RANGE_MAX 126      /*!< (unspecified)                                                        */
#define SPIM133_PRESCALER_DIVISOR_RANGE_SIZE 127     /*!< (unspecified)                                                        */
#define SPIM133_RXDELAY_VALUE_RANGE_MIN 0            /*!< (unspecified)                                                        */
#define SPIM133_RXDELAY_VALUE_RANGE_MAX 40           /*!< (unspecified)                                                        */
#define SPIM133_RXDELAY_VALUE_RANGE_SIZE 41          /*!< (unspecified)                                                        */
#define SPIM133_RXDELAY_RESET_VALUE 2                /*!< (unspecified)                                                        */
#define SPIM133_RXDELAY_FIELD_WIDTH_MIN 0            /*!< (unspecified)                                                        */
#define SPIM133_RXDELAY_FIELD_WIDTH_MAX 5            /*!< (unspecified)                                                        */
#define SPIM133_RXDELAY_FIELD_WIDTH_SIZE 6           /*!< (unspecified)                                                        */

#define SPIM134_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 1 /*!< (unspecified)                                                  */
#define SPIM134_MAX_DATARATE 8                       /*!< (unspecified)                                                        */
#define SPIM134_EASYDMA_MAXCNT_MIN 0                 /*!< (unspecified)                                                        */
#define SPIM134_EASYDMA_MAXCNT_MAX 15                /*!< (unspecified)                                                        */
#define SPIM134_EASYDMA_MAXCNT_SIZE 16               /*!< (unspecified)                                                        */
#define SPIM134_FEATURE_HARDWARE_CSN_PRESENT 1       /*!< (unspecified)                                                        */
#define SPIM134_FEATURE_HARDWARE_DCX_PRESENT 1       /*!< (unspecified)                                                        */
#define SPIM134_FEATURE_RXDELAY_PRESENT 1            /*!< (unspecified)                                                        */
#define SPIM134_STALL_STATUS_PRESENT 0               /*!< (unspecified)                                                        */
#define SPIM134_STALL_STATUS_TX_PRESENT 1            /*!< (unspecified)                                                        */
#define SPIM134_NUM_CHIPSELECT_MIN 0                 /*!< (unspecified)                                                        */
#define SPIM134_NUM_CHIPSELECT_MAX 0                 /*!< (unspecified)                                                        */
#define SPIM134_NUM_CHIPSELECT_SIZE 1                /*!< (unspecified)                                                        */
#define SPIM134_CORE_FREQUENCY 16                    /*!< Peripheral core frequency is 16 MHz.                                 */
#define SPIM134_PRESCALER_PRESENT 1                  /*!< (unspecified)                                                        */
#define SPIM134_PRESCALER_DIVISOR_RANGE_MIN 2        /*!< (unspecified)                                                        */
#define SPIM134_PRESCALER_DIVISOR_RANGE_MAX 126      /*!< (unspecified)                                                        */
#define SPIM134_PRESCALER_DIVISOR_RANGE_SIZE 127     /*!< (unspecified)                                                        */
#define SPIM134_RXDELAY_VALUE_RANGE_MIN 0            /*!< (unspecified)                                                        */
#define SPIM134_RXDELAY_VALUE_RANGE_MAX 40           /*!< (unspecified)                                                        */
#define SPIM134_RXDELAY_VALUE_RANGE_SIZE 41          /*!< (unspecified)                                                        */
#define SPIM134_RXDELAY_RESET_VALUE 2                /*!< (unspecified)                                                        */
#define SPIM134_RXDELAY_FIELD_WIDTH_MIN 0            /*!< (unspecified)                                                        */
#define SPIM134_RXDELAY_FIELD_WIDTH_MAX 5            /*!< (unspecified)                                                        */
#define SPIM134_RXDELAY_FIELD_WIDTH_SIZE 6           /*!< (unspecified)                                                        */

#define SPIM135_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 1 /*!< (unspecified)                                                  */
#define SPIM135_MAX_DATARATE 8                       /*!< (unspecified)                                                        */
#define SPIM135_EASYDMA_MAXCNT_MIN 0                 /*!< (unspecified)                                                        */
#define SPIM135_EASYDMA_MAXCNT_MAX 15                /*!< (unspecified)                                                        */
#define SPIM135_EASYDMA_MAXCNT_SIZE 16               /*!< (unspecified)                                                        */
#define SPIM135_FEATURE_HARDWARE_CSN_PRESENT 1       /*!< (unspecified)                                                        */
#define SPIM135_FEATURE_HARDWARE_DCX_PRESENT 1       /*!< (unspecified)                                                        */
#define SPIM135_FEATURE_RXDELAY_PRESENT 1            /*!< (unspecified)                                                        */
#define SPIM135_STALL_STATUS_PRESENT 0               /*!< (unspecified)                                                        */
#define SPIM135_STALL_STATUS_TX_PRESENT 1            /*!< (unspecified)                                                        */
#define SPIM135_NUM_CHIPSELECT_MIN 0                 /*!< (unspecified)                                                        */
#define SPIM135_NUM_CHIPSELECT_MAX 0                 /*!< (unspecified)                                                        */
#define SPIM135_NUM_CHIPSELECT_SIZE 1                /*!< (unspecified)                                                        */
#define SPIM135_CORE_FREQUENCY 16                    /*!< Peripheral core frequency is 16 MHz.                                 */
#define SPIM135_PRESCALER_PRESENT 1                  /*!< (unspecified)                                                        */
#define SPIM135_PRESCALER_DIVISOR_RANGE_MIN 2        /*!< (unspecified)                                                        */
#define SPIM135_PRESCALER_DIVISOR_RANGE_MAX 126      /*!< (unspecified)                                                        */
#define SPIM135_PRESCALER_DIVISOR_RANGE_SIZE 127     /*!< (unspecified)                                                        */
#define SPIM135_RXDELAY_VALUE_RANGE_MIN 0            /*!< (unspecified)                                                        */
#define SPIM135_RXDELAY_VALUE_RANGE_MAX 40           /*!< (unspecified)                                                        */
#define SPIM135_RXDELAY_VALUE_RANGE_SIZE 41          /*!< (unspecified)                                                        */
#define SPIM135_RXDELAY_RESET_VALUE 2                /*!< (unspecified)                                                        */
#define SPIM135_RXDELAY_FIELD_WIDTH_MIN 0            /*!< (unspecified)                                                        */
#define SPIM135_RXDELAY_FIELD_WIDTH_MAX 5            /*!< (unspecified)                                                        */
#define SPIM135_RXDELAY_FIELD_WIDTH_SIZE 6           /*!< (unspecified)                                                        */

#define SPIM136_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 1 /*!< (unspecified)                                                  */
#define SPIM136_MAX_DATARATE 8                       /*!< (unspecified)                                                        */
#define SPIM136_EASYDMA_MAXCNT_MIN 0                 /*!< (unspecified)                                                        */
#define SPIM136_EASYDMA_MAXCNT_MAX 15                /*!< (unspecified)                                                        */
#define SPIM136_EASYDMA_MAXCNT_SIZE 16               /*!< (unspecified)                                                        */
#define SPIM136_FEATURE_HARDWARE_CSN_PRESENT 1       /*!< (unspecified)                                                        */
#define SPIM136_FEATURE_HARDWARE_DCX_PRESENT 1       /*!< (unspecified)                                                        */
#define SPIM136_FEATURE_RXDELAY_PRESENT 1            /*!< (unspecified)                                                        */
#define SPIM136_STALL_STATUS_PRESENT 0               /*!< (unspecified)                                                        */
#define SPIM136_STALL_STATUS_TX_PRESENT 1            /*!< (unspecified)                                                        */
#define SPIM136_NUM_CHIPSELECT_MIN 0                 /*!< (unspecified)                                                        */
#define SPIM136_NUM_CHIPSELECT_MAX 0                 /*!< (unspecified)                                                        */
#define SPIM136_NUM_CHIPSELECT_SIZE 1                /*!< (unspecified)                                                        */
#define SPIM136_CORE_FREQUENCY 16                    /*!< Peripheral core frequency is 16 MHz.                                 */
#define SPIM136_PRESCALER_PRESENT 1                  /*!< (unspecified)                                                        */
#define SPIM136_PRESCALER_DIVISOR_RANGE_MIN 2        /*!< (unspecified)                                                        */
#define SPIM136_PRESCALER_DIVISOR_RANGE_MAX 126      /*!< (unspecified)                                                        */
#define SPIM136_PRESCALER_DIVISOR_RANGE_SIZE 127     /*!< (unspecified)                                                        */
#define SPIM136_RXDELAY_VALUE_RANGE_MIN 0            /*!< (unspecified)                                                        */
#define SPIM136_RXDELAY_VALUE_RANGE_MAX 40           /*!< (unspecified)                                                        */
#define SPIM136_RXDELAY_VALUE_RANGE_SIZE 41          /*!< (unspecified)                                                        */
#define SPIM136_RXDELAY_RESET_VALUE 2                /*!< (unspecified)                                                        */
#define SPIM136_RXDELAY_FIELD_WIDTH_MIN 0            /*!< (unspecified)                                                        */
#define SPIM136_RXDELAY_FIELD_WIDTH_MAX 5            /*!< (unspecified)                                                        */
#define SPIM136_RXDELAY_FIELD_WIDTH_SIZE 6           /*!< (unspecified)                                                        */

#define SPIM137_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 1 /*!< (unspecified)                                                  */
#define SPIM137_MAX_DATARATE 8                       /*!< (unspecified)                                                        */
#define SPIM137_EASYDMA_MAXCNT_MIN 0                 /*!< (unspecified)                                                        */
#define SPIM137_EASYDMA_MAXCNT_MAX 15                /*!< (unspecified)                                                        */
#define SPIM137_EASYDMA_MAXCNT_SIZE 16               /*!< (unspecified)                                                        */
#define SPIM137_FEATURE_HARDWARE_CSN_PRESENT 1       /*!< (unspecified)                                                        */
#define SPIM137_FEATURE_HARDWARE_DCX_PRESENT 1       /*!< (unspecified)                                                        */
#define SPIM137_FEATURE_RXDELAY_PRESENT 1            /*!< (unspecified)                                                        */
#define SPIM137_STALL_STATUS_PRESENT 0               /*!< (unspecified)                                                        */
#define SPIM137_STALL_STATUS_TX_PRESENT 1            /*!< (unspecified)                                                        */
#define SPIM137_NUM_CHIPSELECT_MIN 0                 /*!< (unspecified)                                                        */
#define SPIM137_NUM_CHIPSELECT_MAX 0                 /*!< (unspecified)                                                        */
#define SPIM137_NUM_CHIPSELECT_SIZE 1                /*!< (unspecified)                                                        */
#define SPIM137_CORE_FREQUENCY 16                    /*!< Peripheral core frequency is 16 MHz.                                 */
#define SPIM137_PRESCALER_PRESENT 1                  /*!< (unspecified)                                                        */
#define SPIM137_PRESCALER_DIVISOR_RANGE_MIN 2        /*!< (unspecified)                                                        */
#define SPIM137_PRESCALER_DIVISOR_RANGE_MAX 126      /*!< (unspecified)                                                        */
#define SPIM137_PRESCALER_DIVISOR_RANGE_SIZE 127     /*!< (unspecified)                                                        */
#define SPIM137_RXDELAY_VALUE_RANGE_MIN 0            /*!< (unspecified)                                                        */
#define SPIM137_RXDELAY_VALUE_RANGE_MAX 40           /*!< (unspecified)                                                        */
#define SPIM137_RXDELAY_VALUE_RANGE_SIZE 41          /*!< (unspecified)                                                        */
#define SPIM137_RXDELAY_RESET_VALUE 2                /*!< (unspecified)                                                        */
#define SPIM137_RXDELAY_FIELD_WIDTH_MIN 0            /*!< (unspecified)                                                        */
#define SPIM137_RXDELAY_FIELD_WIDTH_MAX 5            /*!< (unspecified)                                                        */
#define SPIM137_RXDELAY_FIELD_WIDTH_SIZE 6           /*!< (unspecified)                                                        */

/*UART with EasyDMA*/
#define UARTE_PRESENT 1
#define UARTE_COUNT 9

#define UARTE120_EASYDMA_MAXCNT_MIN 0                /*!< (unspecified)                                                        */
#define UARTE120_EASYDMA_MAXCNT_MAX 15               /*!< (unspecified)                                                        */
#define UARTE120_EASYDMA_MAXCNT_SIZE 16              /*!< (unspecified)                                                        */
#define UARTE120_TIMEOUT_INTERRUPT 1                 /*!< (unspecified)                                                        */
#define UARTE120_CONFIGURABLE_DATA_FRAME_SIZE 1      /*!< (unspecified)                                                        */
#define UARTE120_CORE_FREQUENCY 320                  /*!< Peripheral clock frequency is 320 MHz.                               */
#define UARTE120_CORE_CLOCK_320 1                    /*!< (unspecified)                                                        */
#define UARTE120_SHORTS_ENDTX_STOPTX 1               /*!< (unspecified)                                                        */
#define UARTE120_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 1 /*!< (unspecified)                                                 */

#define UARTE130_EASYDMA_MAXCNT_MIN 0                /*!< (unspecified)                                                        */
#define UARTE130_EASYDMA_MAXCNT_MAX 15               /*!< (unspecified)                                                        */
#define UARTE130_EASYDMA_MAXCNT_SIZE 16              /*!< (unspecified)                                                        */
#define UARTE130_TIMEOUT_INTERRUPT 1                 /*!< (unspecified)                                                        */
#define UARTE130_CONFIGURABLE_DATA_FRAME_SIZE 1      /*!< (unspecified)                                                        */
#define UARTE130_CORE_FREQUENCY 16                   /*!< Peripheral clock frequency is 16 MHz.                                */
#define UARTE130_CORE_CLOCK_16 1                     /*!< (unspecified)                                                        */
#define UARTE130_SHORTS_ENDTX_STOPTX 1               /*!< (unspecified)                                                        */
#define UARTE130_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 1 /*!< (unspecified)                                                 */

#define UARTE131_EASYDMA_MAXCNT_MIN 0                /*!< (unspecified)                                                        */
#define UARTE131_EASYDMA_MAXCNT_MAX 15               /*!< (unspecified)                                                        */
#define UARTE131_EASYDMA_MAXCNT_SIZE 16              /*!< (unspecified)                                                        */
#define UARTE131_TIMEOUT_INTERRUPT 1                 /*!< (unspecified)                                                        */
#define UARTE131_CONFIGURABLE_DATA_FRAME_SIZE 1      /*!< (unspecified)                                                        */
#define UARTE131_CORE_FREQUENCY 16                   /*!< Peripheral clock frequency is 16 MHz.                                */
#define UARTE131_CORE_CLOCK_16 1                     /*!< (unspecified)                                                        */
#define UARTE131_SHORTS_ENDTX_STOPTX 1               /*!< (unspecified)                                                        */
#define UARTE131_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 1 /*!< (unspecified)                                                 */

#define UARTE132_EASYDMA_MAXCNT_MIN 0                /*!< (unspecified)                                                        */
#define UARTE132_EASYDMA_MAXCNT_MAX 15               /*!< (unspecified)                                                        */
#define UARTE132_EASYDMA_MAXCNT_SIZE 16              /*!< (unspecified)                                                        */
#define UARTE132_TIMEOUT_INTERRUPT 1                 /*!< (unspecified)                                                        */
#define UARTE132_CONFIGURABLE_DATA_FRAME_SIZE 1      /*!< (unspecified)                                                        */
#define UARTE132_CORE_FREQUENCY 16                   /*!< Peripheral clock frequency is 16 MHz.                                */
#define UARTE132_CORE_CLOCK_16 1                     /*!< (unspecified)                                                        */
#define UARTE132_SHORTS_ENDTX_STOPTX 1               /*!< (unspecified)                                                        */
#define UARTE132_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 1 /*!< (unspecified)                                                 */

#define UARTE133_EASYDMA_MAXCNT_MIN 0                /*!< (unspecified)                                                        */
#define UARTE133_EASYDMA_MAXCNT_MAX 15               /*!< (unspecified)                                                        */
#define UARTE133_EASYDMA_MAXCNT_SIZE 16              /*!< (unspecified)                                                        */
#define UARTE133_TIMEOUT_INTERRUPT 1                 /*!< (unspecified)                                                        */
#define UARTE133_CONFIGURABLE_DATA_FRAME_SIZE 1      /*!< (unspecified)                                                        */
#define UARTE133_CORE_FREQUENCY 16                   /*!< Peripheral clock frequency is 16 MHz.                                */
#define UARTE133_CORE_CLOCK_16 1                     /*!< (unspecified)                                                        */
#define UARTE133_SHORTS_ENDTX_STOPTX 1               /*!< (unspecified)                                                        */
#define UARTE133_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 1 /*!< (unspecified)                                                 */

#define UARTE134_EASYDMA_MAXCNT_MIN 0                /*!< (unspecified)                                                        */
#define UARTE134_EASYDMA_MAXCNT_MAX 15               /*!< (unspecified)                                                        */
#define UARTE134_EASYDMA_MAXCNT_SIZE 16              /*!< (unspecified)                                                        */
#define UARTE134_TIMEOUT_INTERRUPT 1                 /*!< (unspecified)                                                        */
#define UARTE134_CONFIGURABLE_DATA_FRAME_SIZE 1      /*!< (unspecified)                                                        */
#define UARTE134_CORE_FREQUENCY 16                   /*!< Peripheral clock frequency is 16 MHz.                                */
#define UARTE134_CORE_CLOCK_16 1                     /*!< (unspecified)                                                        */
#define UARTE134_SHORTS_ENDTX_STOPTX 1               /*!< (unspecified)                                                        */
#define UARTE134_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 1 /*!< (unspecified)                                                 */

#define UARTE135_EASYDMA_MAXCNT_MIN 0                /*!< (unspecified)                                                        */
#define UARTE135_EASYDMA_MAXCNT_MAX 15               /*!< (unspecified)                                                        */
#define UARTE135_EASYDMA_MAXCNT_SIZE 16              /*!< (unspecified)                                                        */
#define UARTE135_TIMEOUT_INTERRUPT 1                 /*!< (unspecified)                                                        */
#define UARTE135_CONFIGURABLE_DATA_FRAME_SIZE 1      /*!< (unspecified)                                                        */
#define UARTE135_CORE_FREQUENCY 16                   /*!< Peripheral clock frequency is 16 MHz.                                */
#define UARTE135_CORE_CLOCK_16 1                     /*!< (unspecified)                                                        */
#define UARTE135_SHORTS_ENDTX_STOPTX 1               /*!< (unspecified)                                                        */
#define UARTE135_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 1 /*!< (unspecified)                                                 */

#define UARTE136_EASYDMA_MAXCNT_MIN 0                /*!< (unspecified)                                                        */
#define UARTE136_EASYDMA_MAXCNT_MAX 15               /*!< (unspecified)                                                        */
#define UARTE136_EASYDMA_MAXCNT_SIZE 16              /*!< (unspecified)                                                        */
#define UARTE136_TIMEOUT_INTERRUPT 1                 /*!< (unspecified)                                                        */
#define UARTE136_CONFIGURABLE_DATA_FRAME_SIZE 1      /*!< (unspecified)                                                        */
#define UARTE136_CORE_FREQUENCY 16                   /*!< Peripheral clock frequency is 16 MHz.                                */
#define UARTE136_CORE_CLOCK_16 1                     /*!< (unspecified)                                                        */
#define UARTE136_SHORTS_ENDTX_STOPTX 1               /*!< (unspecified)                                                        */
#define UARTE136_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 1 /*!< (unspecified)                                                 */

#define UARTE137_EASYDMA_MAXCNT_MIN 0                /*!< (unspecified)                                                        */
#define UARTE137_EASYDMA_MAXCNT_MAX 15               /*!< (unspecified)                                                        */
#define UARTE137_EASYDMA_MAXCNT_SIZE 16              /*!< (unspecified)                                                        */
#define UARTE137_TIMEOUT_INTERRUPT 1                 /*!< (unspecified)                                                        */
#define UARTE137_CONFIGURABLE_DATA_FRAME_SIZE 1      /*!< (unspecified)                                                        */
#define UARTE137_CORE_FREQUENCY 16                   /*!< Peripheral clock frequency is 16 MHz.                                */
#define UARTE137_CORE_CLOCK_16 1                     /*!< (unspecified)                                                        */
#define UARTE137_SHORTS_ENDTX_STOPTX 1               /*!< (unspecified)                                                        */
#define UARTE137_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 1 /*!< (unspecified)                                                 */

/*GPIO Port*/
#define GPIO_PRESENT 1
#define GPIO_COUNT 6

#define P0_CTRLSEL_MAP1 1                            /*!< (unspecified)                                                        */
#define P0_CTRLSEL_MAP2 0                            /*!< (unspecified)                                                        */
#define P0_CTRLSEL_MAP3 0                            /*!< (unspecified)                                                        */
#define P0_PIN_NUM_MIN 0                             /*!< (unspecified)                                                        */
#define P0_PIN_NUM_MAX 11                            /*!< (unspecified)                                                        */
#define P0_PIN_NUM_SIZE 12                           /*!< (unspecified)                                                        */
#define P0_FEATURE_PINS_PRESENT 0x00000FFFUL         /*!< (unspecified)                                                        */
#define P0_PIN_SENSE_MECHANISM 1                     /*!< (unspecified)                                                        */
#define P0_DRIVECTRL 0                               /*!< (unspecified)                                                        */
#define P0_RETAIN 1                                  /*!< (unspecified)                                                        */
#define P0_PWRCTRL 0                                 /*!< (unspecified)                                                        */
#define P0_PIN_OWNER_SEC 0                           /*!< (unspecified)                                                        */
#define P0_BIASCTRL 0                                /*!< (unspecified)                                                        */

#define P1_CTRLSEL_MAP1 1                            /*!< (unspecified)                                                        */
#define P1_CTRLSEL_MAP2 0                            /*!< (unspecified)                                                        */
#define P1_CTRLSEL_MAP3 0                            /*!< (unspecified)                                                        */
#define P1_PIN_NUM_MIN 0                             /*!< (unspecified)                                                        */
#define P1_PIN_NUM_MAX 11                            /*!< (unspecified)                                                        */
#define P1_PIN_NUM_SIZE 12                           /*!< (unspecified)                                                        */
#define P1_FEATURE_PINS_PRESENT 0x00000FFFUL         /*!< (unspecified)                                                        */
#define P1_PIN_SENSE_MECHANISM 1                     /*!< (unspecified)                                                        */
#define P1_DRIVECTRL 0                               /*!< (unspecified)                                                        */
#define P1_RETAIN 1                                  /*!< (unspecified)                                                        */
#define P1_PWRCTRL 0                                 /*!< (unspecified)                                                        */
#define P1_PIN_OWNER_SEC 0                           /*!< (unspecified)                                                        */
#define P1_BIASCTRL 0                                /*!< (unspecified)                                                        */

#define P2_CTRLSEL_MAP1 1                            /*!< (unspecified)                                                        */
#define P2_CTRLSEL_MAP2 0                            /*!< (unspecified)                                                        */
#define P2_CTRLSEL_MAP3 0                            /*!< (unspecified)                                                        */
#define P2_PIN_NUM_MIN 0                             /*!< (unspecified)                                                        */
#define P2_PIN_NUM_MAX 11                            /*!< (unspecified)                                                        */
#define P2_PIN_NUM_SIZE 12                           /*!< (unspecified)                                                        */
#define P2_FEATURE_PINS_PRESENT 0x00000FFFUL         /*!< (unspecified)                                                        */
#define P2_PIN_SENSE_MECHANISM 1                     /*!< (unspecified)                                                        */
#define P2_DRIVECTRL 0                               /*!< (unspecified)                                                        */
#define P2_RETAIN 1                                  /*!< (unspecified)                                                        */
#define P2_PWRCTRL 0                                 /*!< (unspecified)                                                        */
#define P2_PIN_OWNER_SEC 0                           /*!< (unspecified)                                                        */
#define P2_BIASCTRL 0                                /*!< (unspecified)                                                        */

#define P6_CTRLSEL_MAP1 1                            /*!< (unspecified)                                                        */
#define P6_CTRLSEL_MAP2 0                            /*!< (unspecified)                                                        */
#define P6_CTRLSEL_MAP3 0                            /*!< (unspecified)                                                        */
#define P6_PIN_NUM_MIN 0                             /*!< (unspecified)                                                        */
#define P6_PIN_NUM_MAX 13                            /*!< (unspecified)                                                        */
#define P6_PIN_NUM_SIZE 14                           /*!< (unspecified)                                                        */
#define P6_FEATURE_PINS_PRESENT 0x00003FFFUL         /*!< (unspecified)                                                        */
#define P6_PIN_SENSE_MECHANISM 1                     /*!< (unspecified)                                                        */
#define P6_DRIVECTRL 1                               /*!< (unspecified)                                                        */
#define P6_RETAIN 1                                  /*!< (unspecified)                                                        */
#define P6_PWRCTRL 0                                 /*!< (unspecified)                                                        */
#define P6_PIN_OWNER_SEC 0                           /*!< (unspecified)                                                        */
#define P6_BIASCTRL 0                                /*!< (unspecified)                                                        */

#define P7_CTRLSEL_MAP1 1                            /*!< (unspecified)                                                        */
#define P7_CTRLSEL_MAP2 0                            /*!< (unspecified)                                                        */
#define P7_CTRLSEL_MAP3 0                            /*!< (unspecified)                                                        */
#define P7_PIN_NUM_MIN 0                             /*!< (unspecified)                                                        */
#define P7_PIN_NUM_MAX 7                             /*!< (unspecified)                                                        */
#define P7_PIN_NUM_SIZE 8                            /*!< (unspecified)                                                        */
#define P7_FEATURE_PINS_PRESENT 0x000000FFUL         /*!< (unspecified)                                                        */
#define P7_PIN_SENSE_MECHANISM 1                     /*!< (unspecified)                                                        */
#define P7_DRIVECTRL 1                               /*!< (unspecified)                                                        */
#define P7_RETAIN 1                                  /*!< (unspecified)                                                        */
#define P7_PWRCTRL 0                                 /*!< (unspecified)                                                        */
#define P7_PIN_OWNER_SEC 0                           /*!< (unspecified)                                                        */
#define P7_BIASCTRL 0                                /*!< (unspecified)                                                        */

#define P9_CTRLSEL_MAP1 1                            /*!< (unspecified)                                                        */
#define P9_CTRLSEL_MAP2 0                            /*!< (unspecified)                                                        */
#define P9_CTRLSEL_MAP3 0                            /*!< (unspecified)                                                        */
#define P9_PIN_NUM_MIN 0                             /*!< (unspecified)                                                        */
#define P9_PIN_NUM_MAX 5                             /*!< (unspecified)                                                        */
#define P9_PIN_NUM_SIZE 6                            /*!< (unspecified)                                                        */
#define P9_FEATURE_PINS_PRESENT 0x0000003FUL         /*!< (unspecified)                                                        */
#define P9_PIN_SENSE_MECHANISM 1                     /*!< (unspecified)                                                        */
#define P9_DRIVECTRL 0                               /*!< (unspecified)                                                        */
#define P9_RETAIN 1                                  /*!< (unspecified)                                                        */
#define P9_PWRCTRL 1                                 /*!< (unspecified)                                                        */
#define P9_PIN_OWNER_SEC 0                           /*!< (unspecified)                                                        */
#define P9_BIASCTRL 0                                /*!< (unspecified)                                                        */

/*Analog to Digital Converter*/
#define SAADC_PRESENT 1
#define SAADC_COUNT 1

#define SAADC_PSEL_V2 1                              /*!< (unspecified)                                                        */
#define SAADC_TASKS_CALIBRATEGAIN 1                  /*!< (unspecified)                                                        */
#define SAADC_SAMPLERATE_CC_VALUERANGE_MIN 4         /*!< (unspecified)                                                        */
#define SAADC_SAMPLERATE_CC_VALUERANGE_MAX 2047      /*!< (unspecified)                                                        */
#define SAADC_SAMPLERATE_CC_VALUERANGE_SIZE 2048     /*!< (unspecified)                                                        */
#define SAADC_TACQ_VALUE_RANGE_MIN 0                 /*!< (unspecified)                                                        */
#define SAADC_TACQ_VALUE_RANGE_MAX 319               /*!< (unspecified)                                                        */
#define SAADC_TACQ_VALUE_RANGE_SIZE 320              /*!< (unspecified)                                                        */
#define SAADC_TCONV_VALUE_RANGE_MIN 0                /*!< (unspecified)                                                        */
#define SAADC_TCONV_VALUE_RANGE_MAX 7                /*!< (unspecified)                                                        */
#define SAADC_TCONV_VALUE_RANGE_SIZE 8               /*!< (unspecified)                                                        */
#define SAADC_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 0 /*!< (unspecified)                                                    */

/*Comparator*/
#define COMP_PRESENT 1
#define COMP_COUNT 1

/*Low-power comparator*/
#define LPCOMP_PRESENT 1
#define LPCOMP_COUNT 1

/*Temperature Sensor*/
#define TEMP_PRESENT 1
#define TEMP_COUNT 1

/*NFC-A compatible radio NFC-A compatible radio*/
#define NFCT_PRESENT 1
#define NFCT_COUNT 1

#define NFCT_NFCTFIELDDETCFG_RESET 1                 /*!< Reset value of register NFCTFIELDDETCFG: 1                           */

/*Time division multiplexed audio interface*/
#define TDM_PRESENT 1
#define TDM_COUNT 2

#define TDM130_NUM_CHANNELS_MIN 0                    /*!< (unspecified)                                                        */
#define TDM130_NUM_CHANNELS_MAX 7                    /*!< (unspecified)                                                        */
#define TDM130_NUM_CHANNELS_SIZE 8                   /*!< (unspecified)                                                        */
#define TDM130_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 1 /*!< (unspecified)                                                   */

#define TDM131_NUM_CHANNELS_MIN 0                    /*!< (unspecified)                                                        */
#define TDM131_NUM_CHANNELS_MAX 7                    /*!< (unspecified)                                                        */
#define TDM131_NUM_CHANNELS_SIZE 8                   /*!< (unspecified)                                                        */
#define TDM131_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 1 /*!< (unspecified)                                                   */

/*Pulse Density Modulation (Digital Microphone) Interface*/
#define PDM_PRESENT 1
#define PDM_COUNT 1

#define PDM_SAMPLE16 0                               /*!< (unspecified)                                                        */
#define PDM_SAMPLE48 1                               /*!< (unspecified)                                                        */
#define PDM_PRESCALER_PRESENT 0                      /*!< (unspecified)                                                        */
#define PDM_PDMV2 0                                  /*!< (unspecified)                                                        */
#define PDM_PCLK24M 0                                /*!< (unspecified)                                                        */
#define PDM_AUDIOPLL 1                               /*!< (unspecified)                                                        */
#define PDM_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 1 /*!< (unspecified)                                                      */

/*Quadrature Decoder*/
#define QDEC_PRESENT 1
#define QDEC_COUNT 2

/*I2C compatible Two-Wire Master Interface with EasyDMA*/
#define TWIM_PRESENT 1
#define TWIM_COUNT 8

#define TWIM130_EASYDMA_MAXCNT_MIN 0                 /*!< (unspecified)                                                        */
#define TWIM130_EASYDMA_MAXCNT_MAX 15                /*!< (unspecified)                                                        */
#define TWIM130_EASYDMA_MAXCNT_SIZE 16               /*!< (unspecified)                                                        */
#define TWIM130_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 1 /*!< (unspecified)                                                  */

#define TWIM131_EASYDMA_MAXCNT_MIN 0                 /*!< (unspecified)                                                        */
#define TWIM131_EASYDMA_MAXCNT_MAX 15                /*!< (unspecified)                                                        */
#define TWIM131_EASYDMA_MAXCNT_SIZE 16               /*!< (unspecified)                                                        */
#define TWIM131_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 1 /*!< (unspecified)                                                  */

#define TWIM132_EASYDMA_MAXCNT_MIN 0                 /*!< (unspecified)                                                        */
#define TWIM132_EASYDMA_MAXCNT_MAX 15                /*!< (unspecified)                                                        */
#define TWIM132_EASYDMA_MAXCNT_SIZE 16               /*!< (unspecified)                                                        */
#define TWIM132_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 1 /*!< (unspecified)                                                  */

#define TWIM133_EASYDMA_MAXCNT_MIN 0                 /*!< (unspecified)                                                        */
#define TWIM133_EASYDMA_MAXCNT_MAX 15                /*!< (unspecified)                                                        */
#define TWIM133_EASYDMA_MAXCNT_SIZE 16               /*!< (unspecified)                                                        */
#define TWIM133_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 1 /*!< (unspecified)                                                  */

#define TWIM134_EASYDMA_MAXCNT_MIN 0                 /*!< (unspecified)                                                        */
#define TWIM134_EASYDMA_MAXCNT_MAX 15                /*!< (unspecified)                                                        */
#define TWIM134_EASYDMA_MAXCNT_SIZE 16               /*!< (unspecified)                                                        */
#define TWIM134_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 1 /*!< (unspecified)                                                  */

#define TWIM135_EASYDMA_MAXCNT_MIN 0                 /*!< (unspecified)                                                        */
#define TWIM135_EASYDMA_MAXCNT_MAX 15                /*!< (unspecified)                                                        */
#define TWIM135_EASYDMA_MAXCNT_SIZE 16               /*!< (unspecified)                                                        */
#define TWIM135_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 1 /*!< (unspecified)                                                  */

#define TWIM136_EASYDMA_MAXCNT_MIN 0                 /*!< (unspecified)                                                        */
#define TWIM136_EASYDMA_MAXCNT_MAX 15                /*!< (unspecified)                                                        */
#define TWIM136_EASYDMA_MAXCNT_SIZE 16               /*!< (unspecified)                                                        */
#define TWIM136_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 1 /*!< (unspecified)                                                  */

#define TWIM137_EASYDMA_MAXCNT_MIN 0                 /*!< (unspecified)                                                        */
#define TWIM137_EASYDMA_MAXCNT_MAX 15                /*!< (unspecified)                                                        */
#define TWIM137_EASYDMA_MAXCNT_SIZE 16               /*!< (unspecified)                                                        */
#define TWIM137_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 1 /*!< (unspecified)                                                  */

/*I2C compatible Two-Wire Slave Interface with EasyDMA*/
#define TWIS_PRESENT 1
#define TWIS_COUNT 8

#define TWIS130_EASYDMA_MAXCNT_MIN 0                 /*!< (unspecified)                                                        */
#define TWIS130_EASYDMA_MAXCNT_MAX 15                /*!< (unspecified)                                                        */
#define TWIS130_EASYDMA_MAXCNT_SIZE 16               /*!< (unspecified)                                                        */
#define TWIS130_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 1 /*!< (unspecified)                                                  */

#define TWIS131_EASYDMA_MAXCNT_MIN 0                 /*!< (unspecified)                                                        */
#define TWIS131_EASYDMA_MAXCNT_MAX 15                /*!< (unspecified)                                                        */
#define TWIS131_EASYDMA_MAXCNT_SIZE 16               /*!< (unspecified)                                                        */
#define TWIS131_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 1 /*!< (unspecified)                                                  */

#define TWIS132_EASYDMA_MAXCNT_MIN 0                 /*!< (unspecified)                                                        */
#define TWIS132_EASYDMA_MAXCNT_MAX 15                /*!< (unspecified)                                                        */
#define TWIS132_EASYDMA_MAXCNT_SIZE 16               /*!< (unspecified)                                                        */
#define TWIS132_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 1 /*!< (unspecified)                                                  */

#define TWIS133_EASYDMA_MAXCNT_MIN 0                 /*!< (unspecified)                                                        */
#define TWIS133_EASYDMA_MAXCNT_MAX 15                /*!< (unspecified)                                                        */
#define TWIS133_EASYDMA_MAXCNT_SIZE 16               /*!< (unspecified)                                                        */
#define TWIS133_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 1 /*!< (unspecified)                                                  */

#define TWIS134_EASYDMA_MAXCNT_MIN 0                 /*!< (unspecified)                                                        */
#define TWIS134_EASYDMA_MAXCNT_MAX 15                /*!< (unspecified)                                                        */
#define TWIS134_EASYDMA_MAXCNT_SIZE 16               /*!< (unspecified)                                                        */
#define TWIS134_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 1 /*!< (unspecified)                                                  */

#define TWIS135_EASYDMA_MAXCNT_MIN 0                 /*!< (unspecified)                                                        */
#define TWIS135_EASYDMA_MAXCNT_MAX 15                /*!< (unspecified)                                                        */
#define TWIS135_EASYDMA_MAXCNT_SIZE 16               /*!< (unspecified)                                                        */
#define TWIS135_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 1 /*!< (unspecified)                                                  */

#define TWIS136_EASYDMA_MAXCNT_MIN 0                 /*!< (unspecified)                                                        */
#define TWIS136_EASYDMA_MAXCNT_MAX 15                /*!< (unspecified)                                                        */
#define TWIS136_EASYDMA_MAXCNT_SIZE 16               /*!< (unspecified)                                                        */
#define TWIS136_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 1 /*!< (unspecified)                                                  */

#define TWIS137_EASYDMA_MAXCNT_MIN 0                 /*!< (unspecified)                                                        */
#define TWIS137_EASYDMA_MAXCNT_MAX 15                /*!< (unspecified)                                                        */
#define TWIS137_EASYDMA_MAXCNT_SIZE 16               /*!< (unspecified)                                                        */
#define TWIS137_EASYDMA_CURRENT_AMOUNT_REGISTER_INCLUDED 1 /*!< (unspecified)                                                  */

/* ============================================= SPU020 Split Security Features ============================================== */
/**
  * @brief Indexes in SPU020.FEATURES controlling access permissions of features with split security
  */
typedef enum {
  NRF_RADIOCORE_SPU020_FEATURES_IPCT_CH_0    = 0,    /*!< Index of access permissions for channel 0 of IPCT                    */
  NRF_RADIOCORE_SPU020_FEATURES_IPCT_CH_1    = 1,    /*!< Index of access permissions for channel 1 of IPCT                    */
  NRF_RADIOCORE_SPU020_FEATURES_IPCT_CH_2    = 2,    /*!< Index of access permissions for channel 2 of IPCT                    */
  NRF_RADIOCORE_SPU020_FEATURES_IPCT_CH_3    = 3,    /*!< Index of access permissions for channel 3 of IPCT                    */
  NRF_RADIOCORE_SPU020_FEATURES_IPCT_CH_4    = 4,    /*!< Index of access permissions for channel 4 of IPCT                    */
  NRF_RADIOCORE_SPU020_FEATURES_IPCT_CH_5    = 5,    /*!< Index of access permissions for channel 5 of IPCT                    */
  NRF_RADIOCORE_SPU020_FEATURES_IPCT_CH_6    = 6,    /*!< Index of access permissions for channel 6 of IPCT                    */
  NRF_RADIOCORE_SPU020_FEATURES_IPCT_CH_7    = 7,    /*!< Index of access permissions for channel 7 of IPCT                    */
  NRF_RADIOCORE_SPU020_FEATURES_IPCT_INTERRUPT_0 = 24, /*!< Index of access permissions for interrupt 0 of IPCT                */
  NRF_RADIOCORE_SPU020_FEATURES_IPCT_INTERRUPT_1 = 25, /*!< Index of access permissions for interrupt 1 of IPCT                */
  NRF_RADIOCORE_SPU020_FEATURES_DPPIC020_CH_0 = 32,  /*!< Index of access permissions for channel 0 of DPPIC020                */
  NRF_RADIOCORE_SPU020_FEATURES_DPPIC020_CH_1 = 33,  /*!< Index of access permissions for channel 1 of DPPIC020                */
  NRF_RADIOCORE_SPU020_FEATURES_DPPIC020_CH_2 = 34,  /*!< Index of access permissions for channel 2 of DPPIC020                */
  NRF_RADIOCORE_SPU020_FEATURES_DPPIC020_CH_3 = 35,  /*!< Index of access permissions for channel 3 of DPPIC020                */
  NRF_RADIOCORE_SPU020_FEATURES_DPPIC020_CH_4 = 36,  /*!< Index of access permissions for channel 4 of DPPIC020                */
  NRF_RADIOCORE_SPU020_FEATURES_DPPIC020_CH_5 = 37,  /*!< Index of access permissions for channel 5 of DPPIC020                */
  NRF_RADIOCORE_SPU020_FEATURES_DPPIC020_CH_6 = 38,  /*!< Index of access permissions for channel 6 of DPPIC020                */
  NRF_RADIOCORE_SPU020_FEATURES_DPPIC020_CH_7 = 39,  /*!< Index of access permissions for channel 7 of DPPIC020                */
  NRF_RADIOCORE_SPU020_FEATURES_DPPIC020_CH_8 = 40,  /*!< Index of access permissions for channel 8 of DPPIC020                */
  NRF_RADIOCORE_SPU020_FEATURES_DPPIC020_CH_9 = 41,  /*!< Index of access permissions for channel 9 of DPPIC020                */
  NRF_RADIOCORE_SPU020_FEATURES_DPPIC020_CH_10 = 42, /*!< Index of access permissions for channel 10 of DPPIC020               */
  NRF_RADIOCORE_SPU020_FEATURES_DPPIC020_CH_11 = 43, /*!< Index of access permissions for channel 11 of DPPIC020               */
  NRF_RADIOCORE_SPU020_FEATURES_DPPIC020_CH_12 = 44, /*!< Index of access permissions for channel 12 of DPPIC020               */
  NRF_RADIOCORE_SPU020_FEATURES_DPPIC020_CH_13 = 45, /*!< Index of access permissions for channel 13 of DPPIC020               */
  NRF_RADIOCORE_SPU020_FEATURES_DPPIC020_CH_14 = 46, /*!< Index of access permissions for channel 14 of DPPIC020               */
  NRF_RADIOCORE_SPU020_FEATURES_DPPIC020_CH_15 = 47, /*!< Index of access permissions for channel 15 of DPPIC020               */
  NRF_RADIOCORE_SPU020_FEATURES_DPPIC020_CH_16 = 48, /*!< Index of access permissions for channel 16 of DPPIC020               */
  NRF_RADIOCORE_SPU020_FEATURES_DPPIC020_CH_17 = 49, /*!< Index of access permissions for channel 17 of DPPIC020               */
  NRF_RADIOCORE_SPU020_FEATURES_DPPIC020_CH_18 = 50, /*!< Index of access permissions for channel 18 of DPPIC020               */
  NRF_RADIOCORE_SPU020_FEATURES_DPPIC020_CH_19 = 51, /*!< Index of access permissions for channel 19 of DPPIC020               */
  NRF_RADIOCORE_SPU020_FEATURES_DPPIC020_CH_20 = 52, /*!< Index of access permissions for channel 20 of DPPIC020               */
  NRF_RADIOCORE_SPU020_FEATURES_DPPIC020_CH_21 = 53, /*!< Index of access permissions for channel 21 of DPPIC020               */
  NRF_RADIOCORE_SPU020_FEATURES_DPPIC020_CH_22 = 54, /*!< Index of access permissions for channel 22 of DPPIC020               */
  NRF_RADIOCORE_SPU020_FEATURES_DPPIC020_CH_23 = 55, /*!< Index of access permissions for channel 23 of DPPIC020               */
  NRF_RADIOCORE_SPU020_FEATURES_DPPIC020_CH_24 = 56, /*!< Index of access permissions for channel 24 of DPPIC020               */
  NRF_RADIOCORE_SPU020_FEATURES_DPPIC020_CH_25 = 57, /*!< Index of access permissions for channel 25 of DPPIC020               */
  NRF_RADIOCORE_SPU020_FEATURES_DPPIC020_CH_26 = 58, /*!< Index of access permissions for channel 26 of DPPIC020               */
  NRF_RADIOCORE_SPU020_FEATURES_DPPIC020_CH_27 = 59, /*!< Index of access permissions for channel 27 of DPPIC020               */
  NRF_RADIOCORE_SPU020_FEATURES_DPPIC020_CH_28 = 60, /*!< Index of access permissions for channel 28 of DPPIC020               */
  NRF_RADIOCORE_SPU020_FEATURES_DPPIC020_CH_29 = 61, /*!< Index of access permissions for channel 29 of DPPIC020               */
  NRF_RADIOCORE_SPU020_FEATURES_DPPIC020_CH_30 = 62, /*!< Index of access permissions for channel 30 of DPPIC020               */
  NRF_RADIOCORE_SPU020_FEATURES_DPPIC020_CH_31 = 63, /*!< Index of access permissions for channel 31 of DPPIC020               */
  NRF_RADIOCORE_SPU020_FEATURES_DPPIC020_CHG_0 = 64, /*!< Index of access permissions for channel group 0 of DPPIC020          */
  NRF_RADIOCORE_SPU020_FEATURES_DPPIC020_CHG_1 = 65, /*!< Index of access permissions for channel group 1 of DPPIC020          */
  NRF_RADIOCORE_SPU020_FEATURES_DPPIC020_CHG_2 = 66, /*!< Index of access permissions for channel group 2 of DPPIC020          */
  NRF_RADIOCORE_SPU020_FEATURES_DPPIC020_CHG_3 = 67, /*!< Index of access permissions for channel group 3 of DPPIC020          */
} NRF_RADIOCORE_SPU020_FEATURES_ENUM_t;

/* ============================================= SPU030 Split Security Features ============================================== */
/**
  * @brief Indexes in SPU030.FEATURES controlling access permissions of features with split security
  */
typedef enum {
  NRF_RADIOCORE_SPU030_FEATURES_DPPIC030_CH_0 = 32,  /*!< Index of access permissions for channel 0 of DPPIC030                */
  NRF_RADIOCORE_SPU030_FEATURES_DPPIC030_CH_1 = 33,  /*!< Index of access permissions for channel 1 of DPPIC030                */
  NRF_RADIOCORE_SPU030_FEATURES_DPPIC030_CH_2 = 34,  /*!< Index of access permissions for channel 2 of DPPIC030                */
  NRF_RADIOCORE_SPU030_FEATURES_DPPIC030_CH_3 = 35,  /*!< Index of access permissions for channel 3 of DPPIC030                */
  NRF_RADIOCORE_SPU030_FEATURES_DPPIC030_CH_4 = 36,  /*!< Index of access permissions for channel 4 of DPPIC030                */
  NRF_RADIOCORE_SPU030_FEATURES_DPPIC030_CH_5 = 37,  /*!< Index of access permissions for channel 5 of DPPIC030                */
  NRF_RADIOCORE_SPU030_FEATURES_DPPIC030_CH_6 = 38,  /*!< Index of access permissions for channel 6 of DPPIC030                */
  NRF_RADIOCORE_SPU030_FEATURES_DPPIC030_CH_7 = 39,  /*!< Index of access permissions for channel 7 of DPPIC030                */
  NRF_RADIOCORE_SPU030_FEATURES_DPPIC030_CH_8 = 40,  /*!< Index of access permissions for channel 8 of DPPIC030                */
  NRF_RADIOCORE_SPU030_FEATURES_DPPIC030_CH_9 = 41,  /*!< Index of access permissions for channel 9 of DPPIC030                */
  NRF_RADIOCORE_SPU030_FEATURES_DPPIC030_CH_10 = 42, /*!< Index of access permissions for channel 10 of DPPIC030               */
  NRF_RADIOCORE_SPU030_FEATURES_DPPIC030_CH_11 = 43, /*!< Index of access permissions for channel 11 of DPPIC030               */
  NRF_RADIOCORE_SPU030_FEATURES_DPPIC030_CH_12 = 44, /*!< Index of access permissions for channel 12 of DPPIC030               */
  NRF_RADIOCORE_SPU030_FEATURES_DPPIC030_CH_13 = 45, /*!< Index of access permissions for channel 13 of DPPIC030               */
  NRF_RADIOCORE_SPU030_FEATURES_DPPIC030_CH_14 = 46, /*!< Index of access permissions for channel 14 of DPPIC030               */
  NRF_RADIOCORE_SPU030_FEATURES_DPPIC030_CH_15 = 47, /*!< Index of access permissions for channel 15 of DPPIC030               */
  NRF_RADIOCORE_SPU030_FEATURES_DPPIC030_CHG_0 = 56, /*!< Index of access permissions for channel group 0 of DPPIC030          */
} NRF_RADIOCORE_SPU030_FEATURES_ENUM_t;


#ifdef __cplusplus
}
#endif
#endif /* NRF54H20_RADIOCORE_PERIPHERALS_H */

