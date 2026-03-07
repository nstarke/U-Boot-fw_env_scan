#!/usr/bin/env bash
set -euo pipefail

REPO_URL="https://source.denx.de/u-boot/u-boot.git"
REPO_DIR="u-boot"

usage() {
  cat <<'USAGE'
Usage: build_fw_envtools.sh [--output-dir <dir>]

Builds static U-Boot env tools (fw_printenv/fw_setenv) for ARM (gnueabi).

Options:
  --output-dir <dir>   Existing directory to copy fw_* binaries into.
  -h, --help           Show this help.
USAGE
}

OUTPUT_DIR=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --output-dir)
      if [[ $# -lt 2 ]]; then
        echo "Missing value for --output-dir" >&2
        exit 2
      fi
      OUTPUT_DIR="$2"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown argument: $1" >&2
      usage
      exit 2
      ;;
  esac
done

if [[ -n "$OUTPUT_DIR" && ! -d "$OUTPUT_DIR" ]]; then
  echo "Output directory does not exist: $OUTPUT_DIR" >&2
  exit 1
fi

if ! command -v arm-linux-gnueabi-gcc >/dev/null 2>&1; then
  echo "arm-linux-gnueabi-gcc not found. Installing with apt..."
  if command -v sudo >/dev/null 2>&1; then
    sudo apt-get update
    sudo apt-get install -y gcc-arm-linux-gnueabi
  else
    apt-get update
    apt-get install -y gcc-arm-linux-gnueabi
  fi
fi

if [[ ! -d "$REPO_DIR/.git" ]]; then
  git clone "$REPO_URL" "$REPO_DIR"
else
  echo "Using existing repository: $REPO_DIR"
fi

cd "$REPO_DIR"

make CROSS_COMPILE=arm-linux-gnueabi- HOSTLDFLAGS='-static' envtools

if [[ ! -x tools/env/fw_printenv ]]; then
  echo "Build succeeded but tools/env/fw_printenv was not found" >&2
  exit 1
fi

if [[ ! -e tools/env/fw_setenv ]]; then
  cp tools/env/fw_printenv tools/env/fw_setenv
fi

DEST_DIR="${OUTPUT_DIR:-$PWD}"

cp tools/env/fw_printenv "$DEST_DIR/"
cp tools/env/fw_setenv "$DEST_DIR/"

echo "Copied binaries to: $DEST_DIR"
echo "  - $DEST_DIR/fw_printenv"
echo "  - $DEST_DIR/fw_setenv"
