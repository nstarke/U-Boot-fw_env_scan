#!/usr/bin/env bash

set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

rc=0

for test_script in \
    "$SCRIPT_DIR/test_env_args.sh" \
    "$SCRIPT_DIR/test_image_args.sh" \
    "$SCRIPT_DIR/test_audit_args.sh"
do
    echo
    echo "===== Running $(basename "$test_script") ====="
    if ! bash "$test_script"; then
        rc=1
    fi
done

exit "$rc"