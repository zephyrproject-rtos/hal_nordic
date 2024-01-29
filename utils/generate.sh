#!/bin/sh

SCRIPT_DIR="$(dirname "$(readlink -f "$0")")"

if [ -z "${ZEPHYR_BASE}" ]; then
    echo "ZEPHYR_BASE is not set"
    exit 1
fi

# FICR offsets for nRF54H20 (Eng0)
python ${SCRIPT_DIR}/gen_offsets.py \
    --svd "${SCRIPT_DIR}/../nrfx/mdk/nrf54h20_enga_application.svd" \
    --output "${ZEPHYR_BASE}/include/zephyr/dt-bindings/misc/nordic-nrf-ficr-nrf54h20-eng0.h" \
    --peripheral GLOBAL_FICR_NS \
    --guard ZEPHYR_INCLUDE_DT_BINDINGS_MISC_NORDIC_NRF_FICR_NRF54H20_ENG0_H_ \
    --prefix NRF_FICR_
