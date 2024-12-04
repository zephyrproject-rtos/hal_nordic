#!/usr/bin/env python3
#
# Copyright (c) 2024, Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

"""
This script generates a module.yml file for the Zephyr project. The module.yml file contains
information about the blobs. The script computes the SHA-256 hash for each blob and renders
the Jinja2 template with the blob information.
"""

import argparse
import hashlib
import requests
import logging
from jinja2 import Environment, FileSystemLoader
from typing import Dict, Any, List
from collections import namedtuple

# Paths are relative to the sdk-nrfxlib repository
BlobInfo = namedtuple(
    "BlobInfo", ["name", "description", "version", "rpath", "lpath", "docpath"]
)

nordic_blobs: List[BlobInfo] = [
    BlobInfo(
        "suit_manifest_starter",
        "nRF54H20 series SUIT manifest starter",
        "5.0.0",
        "suit/bin/suit_manifest_starter.hex",
        "suit/bin/suit_manifest_starter.hex",
        "suit/doc"
        )
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
    env: Environment = Environment(loader=FileSystemLoader("."))
    template = env.get_template(template_path)

    # list of dictionaries containing blob information
    blobs: Dict[str, Dict[str, Any]] = {}
    # Compute SHA-256 for each blob based on the URL
    for blob in nordic_blobs:
        logger.debug(f"Processing blob: {blob.name}")
        nrfxlib_url = f"https://github.com/nrfconnect/sdk-nrfxlib/raw/{latest_sha}"
        blob_info: Dict[str, Any] = {}
        blob_info["path"] = blob.lpath
        blob_info["rpath"] = blob.rpath
        blob_info["version"] = blob.version
        blob_info["url"] = f"{nrfxlib_url}/{blob.rpath}"
        blob_info["doc_url"] = f"{nrfxlib_url}/{blob.docpath}"
        blob_info["sha256"] = compute_sha256(blob_info["url"])
        blob_info["description"] = blob.description
        blobs[blob] = blob_info

    logger.debug(blobs)
    # Render the template with the provided context
    rendered_content: str = template.render(blobs=blobs, latest_sha=latest_sha)

    # Write the rendered content to the output file
    with open(output_path, "w") as output_file:
        output_file.write(rendered_content)


def main() -> None:
    parser: argparse.ArgumentParser = argparse.ArgumentParser(
        description="Generate a module.yml file for the Zephyr project."
    )
    parser.add_argument(
        "-t",
        "--template",
        default="utils/module.yml.j2",
        help="Path to the Jinja2 template file.",
    )
    parser.add_argument(
        "-o",
        "--output",
        default="zephyr/module.yml",
        help="Path to the output YAML file.",
    )
    parser.add_argument(
        "-c",
        "--commit",
        required=True,
        help="The latest commit SHA for the nrfxlib repository.",
    )
    parser.add_argument(
        "-d", "--debug", action="store_true", help="Enable debug logging."
    )

    args: argparse.Namespace = parser.parse_args()

    if args.debug:
        logger.setLevel(logging.DEBUG)

    # Render the template
    render_template(args.template, args.output, args.commit)


if __name__ == "__main__":
    main()
