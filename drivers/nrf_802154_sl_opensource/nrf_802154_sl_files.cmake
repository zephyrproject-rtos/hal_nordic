set(NRF_802154_SL_OPENSOURCE_SOURCES
  ${NRF_802154_SL_ROOT}/src/nrf_802154_sl_ant_div.c
  ${NRF_802154_SL_ROOT}/src/nrf_802154_sl_coex.c
  ${NRF_802154_SL_ROOT}/src/nrf_802154_sl_fem.c
  ${NRF_802154_SL_ROOT}/src/nrf_802154_sl_log.c
  ${NRF_802154_SL_ROOT}/src/nrf_802154_sl_rsch.c
  ${NRF_802154_SL_ROOT}/src/nrf_802154_sl_timer.c
  ${NRF_802154_SL_ROOT}/src/platform/clock/nrf_802154_clock_zephyr.c
  ${NRF_802154_SL_ROOT}/src/platform/irq/nrf_802154_irq_zephyr.c
  ${NRF_802154_SL_ROOT}/src/platform/temperature/nrf_802154_temperature_zephyr.c
)

set(NRF_802154_SL_OPENSOURCE_INCLUDE_DIRS
  ${NRF_802154_SL_ROOT}/include
  ${NRF_802154_SL_ROOT}/src
)

set(NRF_802154_SL_SOURCES
  ${NRF_802154_SL_ROOT}/src/platform/gpiote/nrf_802154_gpiote_zephyr.c
  ${NRF_802154_SL_ROOT}/src/platform/gpiote/nrf_802154_gpiote_crit_sect.c
  ${NRF_802154_SL_ROOT}/src/platform/hp_timer/nrf_802154_hp_timer.c
  ${NRF_802154_SL_ROOT}/src/platform/lp_timer/nrf_802154_lp_timer.c
  ${NRF_802154_SL_ROOT}/src/platform/clock/nrf_802154_clock_zephyr.c
  ${NRF_802154_SL_ROOT}/src/platform/irq/nrf_802154_irq_zephyr.c
  ${NRF_802154_SL_ROOT}/src/platform/temperature/nrf_802154_temperature_zephyr.c
)

set(NRF_802154_SL_INCLUDE_DIRS
  ${NRF_802154_SL_ROOT}/src
)
