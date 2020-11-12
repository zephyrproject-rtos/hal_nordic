if(CONFIG_NRF_802154_RADIO_DRIVER)
  add_subdirectory(nrf_radio_802154)
  add_subdirectory(nrf_802154_sl_opensource)
endif()

if(CONFIG_NRF_802154_SER_HOST OR CONFIG_NRF_802154_SER_RADIO)
  add_subdirectory(nrf_802154_serialization)
endif()
