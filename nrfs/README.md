# nRF Services

## Overview

NRFS (nRF Services) is a library used for accessing services provided by the System Controller core.
The number of available services may vary depending on your device. Check the product specification
to see what services are available for the specific chip.

## Supported SoCs and SiPs

* nRF54H20

## Directories

```

nrfs
├── helpers             # helper files, shared between Local Domains and System Controller
├── include
│   ├── internal        # internal data structures definitions
│   │   ├── backends
│   │   ├── requests
│   │   └── services
│   └── services        # nRF Services API declaration
├── src
│   ├── internal
│   │   └── backends    # IPC configuration files
│   └── services        # nRF Services API implementation
└── zephyr

```
