#!/usr/bin/env python3
#
# Copyright (c) 2024, Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

'''
This script generates a module.yml file for the Zephyr project. The module.yml file contains
information about the firmware binaries for the nRF Wi-Fi firmware. The script computes the SHA-256
hash for each firmware binary and renders the Jinja2 template with the firmware information.
'''

import argparse
import hashlib
import requests
import logging
from jinja2 import Environment, FileSystemLoader
from typing import Dict, Any, List

FW_BIN_NAME: str = 'nrf70.bin'

# Fixed list of firmware variants
firmware_variants: List[str] = [
    'default',
    'scan_only',
    'radio_test',
    'system_with_raw'
]

logger: logging.Logger = logging.getLogger(__name__)
logging.basicConfig(level=logging.INFO)

def compute_sha256(url: str) -> str:
    response = requests.get(url)
    response.raise_for_status()
    sha256_hash: str = hashlib.sha256(response.content).hexdigest()
    return sha256_hash

def render_template(template_path: str, output_path: str, latest_sha: str) -> None:
    # Load the Jinja2 template
    env: Environment = Environment(loader=FileSystemLoader('.'))
    template = env.get_template(template_path)

    # list of dictionaries containing firmware information
    firmwares: Dict[str, Dict[str, Any]] = {}
    # Compute SHA-256 for each firmware based on the URL
    for firmware in firmware_variants:
        url = f"https://github.com/nrfconnect/sdk-nrfxlib/raw/{latest_sha}/nrf_wifi/fw_bins/{firmware}/{FW_BIN_NAME}"
        firmware_info: Dict[str, Any] = {}
        firmware_info['path'] = f"wifi_fw_bins/{firmware}/{FW_BIN_NAME}"
        firmware_info['rpath'] = f"{firmware}/{FW_BIN_NAME}"
        firmware_info['sha256'] = compute_sha256(url)
        firmware_info['description'] = firmware.rsplit('.', 1)[0]  # Filename without extension
        firmwares[firmware] = firmware_info

    logger.debug(firmwares)
    # Render the template with the provided context
    rendered_content: str = template.render(firmwares=firmwares, latest_sha=latest_sha)

    # Write the rendered content to the output file
    with open(output_path, 'w') as output_file:
        output_file.write(rendered_content)

def main() -> None:
    parser: argparse.ArgumentParser = argparse.ArgumentParser(description='Generate a module.yml file for the Zephyr project.')
    parser.add_argument('-t', '--template', default='utils/module.yml.j2', help='Path to the Jinja2 template file.')
    parser.add_argument('-o', '--output', default='zephyr/module.yml', help='Path to the output YAML file.')
    parser.add_argument('-c', '--commit', required=True, help='The latest commit SHA for the nrfxlib repository.')
    parser.add_argument('-d', '--debug', action='store_true', help='Enable debug logging.')

    args: argparse.Namespace = parser.parse_args()

    if args.debug:
        logger.setLevel(logging.DEBUG)

    # Render the template
    render_template(args.template, args.output, args.commit)

if __name__ == '__main__':
    main()
