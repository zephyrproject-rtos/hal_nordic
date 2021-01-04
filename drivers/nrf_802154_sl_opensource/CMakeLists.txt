set(NRF_802154_SL_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

include(nrf_802154_sl_files.cmake)

if (CONFIG_NRF_802154_SL_OPENSOURCE)
  zephyr_include_directories(${NRF_802154_SL_OPENSOURCE_INCLUDE_DIRS})
  zephyr_library_sources(${NRF_802154_SL_OPENSOURCE_SOURCES})

else()
  zephyr_include_directories(${NRF_802154_SL_INCLUDE_DIRS})

  if (CONFIG_SOC_SERIES_NRF53X)
    zephyr_library_sources(${NRF_802154_SL_SOURCES_NRF53})
  else ()
    zephyr_library_sources(${NRF_802154_SL_SOURCES_NRF52})
  endif()

endif()

