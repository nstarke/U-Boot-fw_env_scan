// SPDX-License-Identifier: MIT License - Copyright (c) 2026 Nicholas Starke

#include "uboot_scan.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

static int ensure_fw_env_config_exists(void)
{
	char *argv[] = {
		"env",
		"--output-config",
		NULL,
	};

	if (access("fw_env.config", F_OK) == 0)
		return 0;

	return fw_env_scan_main(2, argv);
}

static int run_validate_crc32(const struct fw_audit_input *input, char *message, size_t message_len)
{
	uint32_t stored_le;
	uint32_t stored_be;
	uint32_t calc_std;
	uint32_t calc_redund = 0;
	int env_scan_rc;

	env_scan_rc = ensure_fw_env_config_exists();
	if (env_scan_rc != 0) {
		if (message && message_len)
			snprintf(message, message_len,
				 "fw_env.config not found and env scan failed (rc=%d)", env_scan_rc);
		return -1;
	}

	if (!input || !input->data || !input->crc32_table || input->data_len < 8) {
		if (message && message_len)
			snprintf(message, message_len, "input too small (need at least 8 bytes)");
		return -1;
	}

	stored_le = (uint32_t)input->data[0] |
		((uint32_t)input->data[1] << 8) |
		((uint32_t)input->data[2] << 16) |
		((uint32_t)input->data[3] << 24);
	stored_be = fw_read_be32(input->data);

	calc_std = fw_crc32_calc(input->crc32_table, input->data + 4, input->data_len - 4);
	if (input->data_len > 5)
		calc_redund = fw_crc32_calc(input->crc32_table, input->data + 5, input->data_len - 5);

	if (calc_std == stored_le || calc_std == stored_be) {
		if (message && message_len) {
			snprintf(message, message_len,
				 "standard env CRC32 matched (%s-endian), offset=0x%jx size=0x%zx",
				 (calc_std == stored_le) ? "LE" : "BE",
				 (uintmax_t)input->offset,
				 input->data_len);
		}
		return 0;
	}

	if (input->data_len > 5 && (calc_redund == stored_le || calc_redund == stored_be)) {
		if (message && message_len) {
			snprintf(message, message_len,
				 "redundant env CRC32 matched (%s-endian), offset=0x%jx size=0x%zx",
				 (calc_redund == stored_le) ? "LE" : "BE",
				 (uintmax_t)input->offset,
				 input->data_len);
		}
		return 0;
	}

	if (message && message_len) {
		snprintf(message, message_len,
			 "crc32 mismatch: stored_le=0x%08x stored_be=0x%08x calc_std=0x%08x calc_redund=0x%08x",
			 stored_le, stored_be, calc_std, calc_redund);
	}

	return 1;
}

static const struct fw_audit_rule uboot_validate_crc32_rule = {
	.name = "uboot_validate_crc32",
	.description = "Validate U-Boot environment CRC32 checksum (standard/redundant layouts)",
	.run = run_validate_crc32,
};

FW_REGISTER_AUDIT_RULE(uboot_validate_crc32_rule);
