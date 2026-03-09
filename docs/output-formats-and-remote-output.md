# Output Formats and Remote Output

Global option:

- `--output-format <csv|json|txt>` — select requested output format at the `uboot_audit` wrapper level (default: `txt`)
  - `txt`: existing human-readable output
  - `csv`: comma-separated records (header + rows)
  - `json`: newline-delimited JSON objects (one JSON object per line)
  - when `--verbose` is enabled with `csv`/`json`, verbose messages are emitted as structured `verbose` records (instead of plain text lines)

Remote output notes:

- `env --output-tcp <ip:port>` sends the same formatted stream selected by `--output-format` over TCP.
- `env --output-http <http://host:port/path>` sends the same formatted stream selected by `--output-format` in a single HTTP POST request.
- `env --output-https <https://host:port/path>` sends the same formatted stream selected by `--output-format` in a single HTTPS POST request using embedded CA certificates.
- `env --insecure` disables TLS certificate and hostname verification for HTTPS output.
- `image --output-tcp` is used for `--pull` binary streaming; for formatted scan/find-address output over TCP, use `image --send-logs --output-tcp ...`.
- `image --output-http <http://host:port/path>` can be used to POST formatted scan/find-address output, or to POST pulled image bytes when used with `--pull`.
- `image --output-https <https://host:port/path>` can be used to POST formatted scan/find-address output, or to POST pulled image bytes when used with `--pull`, using embedded CA certificates.
- `image --insecure` disables TLS certificate and hostname verification for HTTPS output.
- `dmesg --output-tcp <ip:port>` sends dmesg text output to TCP.
- `dmesg --output-http <http://host:port/path>` sends dmesg text output in a single HTTP POST request with `Content-Type: text/plain; charset=utf-8`.
- `dmesg --output-https <https://host:port/path>` sends dmesg text output in a single HTTPS POST request with `Content-Type: text/plain; charset=utf-8`, using embedded CA certificates.
- `dmesg --insecure` disables TLS certificate and hostname verification for HTTPS output.
- `--output-format` does not affect `dmesg`; if specified, a warning is emitted.
- `remote-copy --output-tcp <ip:port>` sends raw file bytes over TCP.
- `remote-copy --output-http <http://host:port/path>` sends raw file bytes in a single HTTP POST request with `Content-Type: application/octet-stream`.
- `remote-copy --output-https <https://host:port/path>` sends raw file bytes in a single HTTPS POST request with `Content-Type: application/octet-stream`, using embedded CA certificates.
- `remote-copy --insecure` disables TLS certificate and hostname verification for HTTPS output.
- `--output-format` does not affect `remote-copy`; if specified, a warning is emitted.
