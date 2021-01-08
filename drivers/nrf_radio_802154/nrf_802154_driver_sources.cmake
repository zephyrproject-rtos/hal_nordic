set(NRF_802154_DRIVER_INCLUDE_DIRS ${NRF_802154_DRIVER_ROOT}/src)

set(NRF_802154_DRIVER_SOURCES_COMMON
  ${NRF_802154_DRIVER_ROOT}/src/nrf_802154.c
  ${NRF_802154_DRIVER_ROOT}/src/nrf_802154_core.c
  ${NRF_802154_DRIVER_ROOT}/src/nrf_802154_core_hooks.c
  ${NRF_802154_DRIVER_ROOT}/src/nrf_802154_critical_section.c
  ${NRF_802154_DRIVER_ROOT}/src/nrf_802154_debug.c
  ${NRF_802154_DRIVER_ROOT}/src/nrf_802154_debug_assert.c
  ${NRF_802154_DRIVER_ROOT}/src/nrf_802154_pib.c
  ${NRF_802154_DRIVER_ROOT}/src/nrf_802154_peripherals_alloc.c
  ${NRF_802154_DRIVER_ROOT}/src/nrf_802154_queue.c
  ${NRF_802154_DRIVER_ROOT}/src/nrf_802154_rssi.c
  ${NRF_802154_DRIVER_ROOT}/src/nrf_802154_rx_buffer.c
  ${NRF_802154_DRIVER_ROOT}/src/nrf_802154_stats.c
  ${NRF_802154_DRIVER_ROOT}/src/nrf_802154_swi.c
  ${NRF_802154_DRIVER_ROOT}/src/nrf_802154_trx.c
  ${NRF_802154_DRIVER_ROOT}/src/mac_features/nrf_802154_csma_ca.c
  ${NRF_802154_DRIVER_ROOT}/src/mac_features/nrf_802154_delayed_trx.c
  ${NRF_802154_DRIVER_ROOT}/src/mac_features/nrf_802154_filter.c
  ${NRF_802154_DRIVER_ROOT}/src/mac_features/nrf_802154_frame_parser.c
  ${NRF_802154_DRIVER_ROOT}/src/mac_features/nrf_802154_ifs.c
  ${NRF_802154_DRIVER_ROOT}/src/mac_features/nrf_802154_precise_ack_timeout.c
  ${NRF_802154_DRIVER_ROOT}/src/mac_features/ack_generator/nrf_802154_ack_data.c
  ${NRF_802154_DRIVER_ROOT}/src/mac_features/ack_generator/nrf_802154_ack_generator.c
  ${NRF_802154_DRIVER_ROOT}/src/mac_features/ack_generator/nrf_802154_enh_ack_generator.c
  ${NRF_802154_DRIVER_ROOT}/src/mac_features/ack_generator/nrf_802154_imm_ack_generator.c
  ${NRF_802154_DRIVER_ROOT}/src/platform/random/nrf_802154_random_zephyr.c
  ${NRF_802154_DRIVER_ROOT}/src/platform/temperature/nrf_802154_temperature_none.c
  )

set(NRF_802154_DRIVER_SOURCES_NRF52
  ${NRF_802154_DRIVER_SOURCES_COMMON}
  ${NRF_802154_DRIVER_ROOT}/src/nrf_802154_trx_ppi.c
  )

set(NRF_802154_DRIVER_SOURCES_NRF53
  ${NRF_802154_DRIVER_SOURCES_COMMON}
  ${NRF_802154_DRIVER_ROOT}/src/nrf_802154_trx_dppi.c
  )

set(NRF_802154_DRIVER_SOURCES_DIRECT
  ${NRF_802154_DRIVER_ROOT}/src/nrf_802154_notification_direct.c
  ${NRF_802154_DRIVER_ROOT}/src/nrf_802154_request_direct.c
  )

set(NRF_802154_DRIVER_SOURCES_SWI
  ${NRF_802154_DRIVER_ROOT}/src/nrf_802154_notification_swi.c
  ${NRF_802154_DRIVER_ROOT}/src/nrf_802154_priority_drop_swi.c
  ${NRF_802154_DRIVER_ROOT}/src/nrf_802154_request_swi.c
  )
