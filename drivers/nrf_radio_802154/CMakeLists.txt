zephyr_include_directories(.)

zephyr_library_sources(
  nrf_802154_core_hooks.c
  nrf_802154_core.c
  nrf_802154_critical_section.c
  nrf_802154_debug.c
  nrf_802154_pib.c
  nrf_802154_rssi.c
  nrf_802154_rx_buffer.c
  nrf_802154_timer_coord.c
  nrf_802154.c
  mac_features/nrf_802154_csma_ca.c
  mac_features/nrf_802154_filter.c
  mac_features/nrf_802154_frame_parser.c
  mac_features/nrf_802154_precise_ack_timeout.c
  mac_features/ack_generator/nrf_802154_ack_data.c
  mac_features/ack_generator/nrf_802154_ack_generator.c
  mac_features/ack_generator/nrf_802154_enh_ack_generator.c
  mac_features/ack_generator/nrf_802154_imm_ack_generator.c
  mac_features/nrf_802154_delayed_trx.c
  platform/clock/nrf_802154_clock_zephyr.c
  platform/coex/nrf_802154_wifi_coex_none.c
  platform/hp_timer/nrf_802154_hp_timer.c
  platform/irq/nrf_802154_irq_zephyr.c
  platform/lp_timer/nrf_802154_lp_timer.c
  platform/random/nrf_802154_random_zephyr.c
  platform/temperature/nrf_802154_temperature_none.c
  rsch/nrf_802154_rsch.c
  rsch/nrf_802154_rsch_crit_sect.c
  timer_scheduler/nrf_802154_timer_sched.c
  )

if (CONFIG_NRF_802154_MULTIPROTOCOL_SUPPORT)
  zephyr_library_sources(
    nrf_802154_swi.c
    nrf_802154_notification_swi.c
    nrf_802154_priority_drop_swi.c
    nrf_802154_request_swi.c
    )

else()
  zephyr_library_sources(
    fal/nrf_802154_fal.c
    rsch/raal/single_phy/single_phy.c
    nrf_802154_notification_direct.c
    nrf_802154_priority_drop_direct.c
    nrf_802154_request_direct.c
  )

endif()

if(     CONFIG_NRF_802154_CCA_MODE_ED)
  set(radio_cca_mode NRF_RADIO_CCA_MODE_ED)

elseif( CONFIG_NRF_802154_CCA_MODE_CARRIER)
  set(radio_cca_mode NRF_RADIO_CCA_MODE_CARRIER)

elseif( CONFIG_NRF_802154_CCA_MODE_CARRIER_AND_ED)
  set(radio_cca_mode NRF_RADIO_CCA_MODE_CARRIER_AND_ED)

elseif( CONFIG_NRF_802154_CCA_MODE_CARRIER_OR_ED)
  set(radio_cca_mode NRF_RADIO_CCA_MODE_CARRIER_OR_ED)

endif()

zephyr_compile_definitions(
  # Until IRQ configuration abstraction is added to the radio driver do not set
  # radio IRQ priority to 0, or the radio IRQ will ignore IRQ lock.
  NRF_802154_IRQ_PRIORITY=1
  NRF_802154_INTERNAL_RADIO_IRQ_HANDLING=0

  # Radio driver shim layer uses raw api
  NRF_802154_USE_RAW_API=1

  # Number of slots containing short addresses of nodes for which
  # pending data is stored.
  NRF_802154_PENDING_SHORT_ADDRESSES=16

  # Number of slots containing extended addresses of nodes for which
  # pending data is stored.
  NRF_802154_PENDING_EXTENDED_ADDRESSES=16

  # Number of buffers in receive queue.
  NRF_802154_RX_BUFFERS=8

  # CCA mode
  NRF_802154_CCA_MODE_DEFAULT=${radio_cca_mode}

  # CCA mode options
  NRF_802154_CCA_CORR_LIMIT_DEFAULT=${CONFIG_NRF_802154_CCA_CORR_LIMIT}
  NRF_802154_CCA_CORR_THRESHOLD_DEFAULT=${CONFIG_NRF_802154_CCA_CORR_THRESHOLD}
  NRF_802154_CCA_ED_THRESHOLD_DEFAULT=${CONFIG_NRF_802154_CCA_ED_THRESHOLD}

  # Enable CSMA/CA
  NRF_802154_CSMA_CA_ENABLED=1
  NRF_802154_TX_STARTED_NOTIFY_ENABLED=1

  # Timestamps
  NRF_802154_FRAME_TIMESTAMP_ENABLED=1
  NRF_802154_DELAYED_TRX_ENABLED=1

  # ACK timeout
  NRF_802154_ACK_TIMEOUT_ENABLED=1
)
