#!/bin/sh

scrub_sensitive_stream() {
    while IFS= read -r line || [ -n "$line" ]; do
        lower_line="$(printf '%s' "$line" | tr '[:upper:]' '[:lower:]')"
        case "$lower_line" in
            *efi-var*|*efi_vars*|*efivars*)
                printf '[REDACTED EFI VARS]\n'
                continue
                ;;
        esac

        printf '%s\n' "$line" | sed -E \
            -e 's/(data_hex=)[0-9A-Fa-f]+/\1<REDACTED>/g' \
            -e 's/(([Pp][Aa][Ss][Ss][Ww][Oo][Rr][Dd]|[Pp][Aa][Ss][Ss][Ww][Dd]|[Cc][Rr][Ee][Dd][Ee][Nn][Tt][Ii][Aa][Ll][Ss]?|[Aa][Pp][Ii][_-]?[Kk][Ee][Yy]|[Ss][Ee][Cc][Rr][Ee][Tt]|[Tt][Oo][Kk][Ee][Nn])[[:space:]]*[:=][[:space:]]*)[^[:space:],;"}]+/\1<REDACTED>/g' \
            -e 's/(([?&]([Pp][Aa][Ss][Ss][Ww][Oo][Rr][Dd]|[Pp][Aa][Ss][Ss][Ww][Dd]|[Cc][Rr][Ee][Dd][Ee][Nn][Tt][Ii][Aa][Ll][Ss]?|[Aa][Pp][Ii][_-]?[Kk][Ee][Yy]|[Ss][Ee][Cc][Rr][Ee][Tt]|[Tt][Oo][Kk][Ee][Nn]))=)[^&[:space:]]+/\1<REDACTED>/g' \
            -e 's/"data_hex"[[:space:]]*:[[:space:]]*"[^"]*"/"data_hex":"<REDACTED>"/g' \
            -e 's/"([Pp][Aa][Ss][Ss][Ww][Oo][Rr][Dd]|[Pp][Aa][Ss][Ss][Ww][Dd]|[Cc][Rr][Ee][Dd][Ee][Nn][Tt][Ii][Aa][Ll][Ss]?|[Aa][Pp][Ii][_-]?[Kk][Ee][Yy]|[Ss][Ee][Cc][Rr][Ee][Tt]|[Tt][Oo][Kk][Ee][Nn])"[[:space:]]*:[[:space:]]*"[^"]*"/"\1":"<REDACTED>"/g'
    done
}