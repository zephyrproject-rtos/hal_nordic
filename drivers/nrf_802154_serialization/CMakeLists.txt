#
#  Copyright (c) 2020, Nordic Semiconductor ASA
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#  1. Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#  3. Neither the name of the copyright holder nor the
#     names of its contributors may be used to endorse or promote products
#     derived from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
#  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
#  POSSIBILITY OF SUCH DAMAGE.
#

cmake_minimum_required(VERSION 3.13.1)

zephyr_library_named(nrf_802154_ser)

zephyr_library_sources(
    spinel_base/spinel.c
    src/nrf_802154_buffer_allocator.c
    src/nrf_802154_buffer_mgr_dst.c
    src/nrf_802154_buffer_mgr_src.c
    src/nrf_802154_kvmap.c
    src/nrf_802154_spinel.c
    src/nrf_802154_spinel_dec.c
    src/platform/zephyr/nrf_802154_serialization_crit_sect.c
    src/platform/zephyr/nrf_802154_spinel_log.c
    src/platform/zephyr/nrf_802154_spinel_backend_ipc.c
    src/platform/zephyr/nrf_802154_spinel_response_notifier.c
)

if (CONFIG_NRF_802154_SER_HOST)
zephyr_library_sources(
    src/nrf_802154_spinel_app.c
    src/nrf_802154_spinel_dec_app.c
)
endif()

if (CONFIG_NRF_802154_SER_RADIO)
zephyr_library_sources(
    src/nrf_802154_spinel_net.c
    src/nrf_802154_spinel_dec_net.c
    src/platform/zephyr/nrf_802154_init_net.c
)
endif()

zephyr_include_directories(include)
zephyr_library_include_directories(src/include)
