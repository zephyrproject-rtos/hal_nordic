if (CONFIG_NRF_802154_SL_OPENSOURCE)
  zephyr_include_directories(include)
  zephyr_library_sources(
    src/nrf_802154_sl_ant_div.c
    src/nrf_802154_sl_coex.c
    src/nrf_802154_sl_fem.c
    src/nrf_802154_sl_log.c
    src/nrf_802154_sl_rsch.c
    src/nrf_802154_sl_timer.c
    )

else()
  zephyr_library_sources(
    src/platform/gpiote/nrf_802154_gpiote_zephyr.c
    src/platform/gpiote/nrf_802154_gpiote_crit_sect.c
    src/platform/hp_timer/nrf_802154_hp_timer.c
    src/platform/lp_timer/nrf_802154_lp_timer.c
  )

endif()

zephyr_include_directories(src)
zephyr_library_sources(
  src/platform/clock/nrf_802154_clock_zephyr.c
  src/platform/irq/nrf_802154_irq_zephyr.c
)
