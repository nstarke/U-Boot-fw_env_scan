#!/usr/bin/env bash

set -u

PASS_COUNT=0
FAIL_COUNT=0

run_with_output_override() {
    local args=("$@")
    local override_flag=""
    local override_value=""

    if [[ -n "${TEST_OUTPUT_HTTP:-}" && -n "${TEST_OUTPUT_HTTPS:-}" ]]; then
        echo "error: set only one of TEST_OUTPUT_HTTP or TEST_OUTPUT_HTTPS"
        return 2
    fi

    if [[ -n "${TEST_OUTPUT_HTTP:-}" ]]; then
        override_flag="--output-http"
        override_value="$TEST_OUTPUT_HTTP"
    elif [[ -n "${TEST_OUTPUT_HTTPS:-}" ]]; then
        override_flag="--output-https"
        override_value="$TEST_OUTPUT_HTTPS"
    fi

    if [[ -z "$override_flag" ]]; then
        "${args[@]}"
        return $?
    fi

    local rewritten=()
    local replaced=0
    local i=0
    while [[ $i -lt ${#args[@]} ]]; do
        local arg="${args[$i]}"
        case "$arg" in
            --output-http|--output-https)
                rewritten+=("$override_flag" "$override_value")
                replaced=1
                i=$((i + 2))
                continue
                ;;
            --output-http=*|--output-https=*)
                rewritten+=("${override_flag}=${override_value}")
                replaced=1
                i=$((i + 1))
                continue
                ;;
        esac

        rewritten+=("$arg")
        i=$((i + 1))
    done

    if [[ $replaced -eq 0 ]]; then
        rewritten+=("$override_flag" "$override_value")
    fi

    "${rewritten[@]}"
}

print_section() {
    local title="$1"
    printf '\n==== %s ====\n' "$title"
}

require_binary() {
    local bin="$1"
    if [[ ! -x "$bin" ]]; then
        echo "error: missing executable: $bin"
        echo "hint: build first with: make"
        exit 1
    fi
}

run_exact_case() {
    local name="$1"
    local expected_rc="$2"
    shift 2

    local log
    log="$(mktemp)"
    run_with_output_override "$@" >"$log" 2>&1
    local rc=$?

    if [[ $rc -eq $expected_rc ]]; then
        echo "[PASS] $name (rc=$rc)"
        PASS_COUNT=$((PASS_COUNT + 1))
    else
        echo "[FAIL] $name (rc=$rc, expected=$expected_rc)"
        sed -n '1,80p' "$log"
        FAIL_COUNT=$((FAIL_COUNT + 1))
    fi

    rm -f "$log"
}

run_accept_case() {
    local name="$1"
    shift

    local log
    log="$(mktemp)"
    run_with_output_override "$@" >"$log" 2>&1
    local rc=$?

    if [[ $rc -ne 2 ]]; then
        echo "[PASS] $name (rc=$rc)"
        PASS_COUNT=$((PASS_COUNT + 1))
    else
        echo "[FAIL] $name (rc=$rc, parser/usage failure)"
        sed -n '1,80p' "$log"
        FAIL_COUNT=$((FAIL_COUNT + 1))
    fi

    rm -f "$log"
}

finish_tests() {
    echo
    echo "Passed: $PASS_COUNT"
    echo "Failed: $FAIL_COUNT"
    [[ $FAIL_COUNT -eq 0 ]]
}
