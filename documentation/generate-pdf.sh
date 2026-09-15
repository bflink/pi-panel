#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source_file="${script_dir}/pi-panel-guide.html"
output_file="${script_dir}/pi-panel-guide.pdf"

chromium \
    --headless \
    --disable-gpu \
    --no-pdf-header-footer \
    --print-to-pdf="${output_file}" \
    "file://${source_file}"

printf 'Generated %s\n' "${output_file}"