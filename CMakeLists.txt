if(CONFIG_HAS_NORDIC_DRIVERS)
  zephyr_library()

  add_subdirectory(drivers)
endif()
