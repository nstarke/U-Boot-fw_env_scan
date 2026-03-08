// SPDX-License-Identifier: MIT License - Copyright (c) 2026 Nicholas Starke

#include "uboot_scan.h"

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif

enum uboot_output_format {
	FW_OUTPUT_TXT = 0,
	FW_OUTPUT_CSV,
	FW_OUTPUT_JSON,
};

static enum uboot_output_format detect_output_format(void)
{
	const char *fmt = getenv("FW_AUDIT_OUTPUT_FORMAT");

	if (!fmt || !*fmt || !strcmp(fmt, "txt"))
		return FW_OUTPUT_TXT;
	if (!strcmp(fmt, "csv"))
		return FW_OUTPUT_CSV;
	if (!strcmp(fmt, "json"))
		return FW_OUTPUT_JSON;

	return FW_OUTPUT_TXT;
}

static uint64_t parse_u64(const char *s)
{
	uint64_t v;

	if (uboot_parse_u64(s, &v)) {
		fprintf(stderr, "Invalid number: %s\n", s);
		exit(2);
	}

	return v;
}

static void usage(const char *prog)
{
	fprintf(stderr,
		"Usage: %s [--list-rules] [--rule <name>] --dev <device> [--offset <bytes>] --size <bytes> [--verbose]\n"
		"  --list-rules    List compiled audit rules\n"
		"  --rule <name>   Run only one rule by name\n"
		"  --dev <device>  Input device/file to audit\n"
		"  --offset <n>    Read offset (default: 0)\n"
		"  --size <n>      Number of bytes to read for audit\n"
		"  --verbose       Enable verbose audit output\n",
		prog);
}

static bool rule_name_selected(const char *filter, const struct uboot_audit_rule *rule)
{
	if (!rule || !rule->name || !*rule->name)
		return false;

	if (!filter || !*filter)
		return true;

	return !strcmp(filter, rule->name);
}

static void print_rule_record(enum uboot_output_format fmt,
			      const struct uboot_audit_rule *rule,
			      int rc,
			      const char *message)
{
	const char *status = (rc == 0) ? "pass" : ((rc > 0) ? "fail" : "error");

	if (fmt == FW_OUTPUT_CSV) {
		printf("audit_rule,%s,%s,%s\n",
		       rule->name ? rule->name : "",
		       status,
		       message ? message : "");
		return;
	}

	if (fmt == FW_OUTPUT_JSON) {
		printf("{\"record\":\"audit_rule\",\"rule\":\"%s\",\"status\":\"%s\",\"message\":\"",
		       rule->name ? rule->name : "", status);
		if (message) {
			for (const char *p = message; *p; p++) {
				if (*p == '\\' || *p == '"')
					putchar('\\');
				putchar(*p);
			}
		}
		printf("\"}\n");
		return;
	}

	printf("[%s] %s: %s\n",
	       status,
	       rule->name ? rule->name : "(unnamed-rule)",
	       message ? message : "");
}

static void print_rule_listing(enum uboot_output_format fmt, const struct uboot_audit_rule *rule)
{
	if (fmt == FW_OUTPUT_CSV) {
		printf("audit_rule_list,%s,%s\n",
		       rule->name ? rule->name : "",
		       (rule->description && *rule->description) ? rule->description : "");
		return;
	}

	if (fmt == FW_OUTPUT_JSON) {
		printf("{\"record\":\"audit_rule_list\",\"rule\":\"%s\",\"description\":\"",
		       rule->name ? rule->name : "");
		if (rule->description) {
			for (const char *p = rule->description; *p; p++) {
				if (*p == '\\' || *p == '"')
					putchar('\\');
				putchar(*p);
			}
		}
		printf("\"}\n");
		return;
	}

	printf("%s", rule->name ? rule->name : "");
	if (rule->description && *rule->description)
		printf(" - %s", rule->description);
	printf("\n");
}

int uboot_audit_scan_main(int argc, char **argv)
{
	const char *dev = NULL;
	const char *rule_filter = NULL;
	uint64_t offset = 0;
	uint64_t size = 0;
	bool verbose = false;
	bool list_rules = false;
	uint32_t crc32_table[256];
	const struct uboot_audit_rule * const *rulep;
	const struct uboot_audit_rule * const *start = __start_uboot_audit_rules;
	const struct uboot_audit_rule * const *stop = __stop_uboot_audit_rules;
	enum uboot_output_format fmt;
	int opt;
	int ret = 0;
	int fd = -1;
	uint8_t *buf = NULL;
	size_t read_len;
	ssize_t got;
	bool ran_any = false;

	static const struct option long_opts[] = {
		{ "help", no_argument, NULL, 'h' },
		{ "dev", required_argument, NULL, 'd' },
		{ "offset", required_argument, NULL, 'o' },
		{ "size", required_argument, NULL, 's' },
		{ "verbose", no_argument, NULL, 'v' },
		{ "rule", required_argument, NULL, 'r' },
		{ "list-rules", no_argument, NULL, 'l' },
		{ 0, 0, 0, 0 }
	};

	optind = 1;
	fmt = detect_output_format();

	while ((opt = getopt_long(argc, argv, "hd:o:s:vr:l", long_opts, NULL)) != -1) {
		switch (opt) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'd':
			dev = optarg;
			break;
		case 'o':
			offset = parse_u64(optarg);
			break;
		case 's':
			size = parse_u64(optarg);
			break;
		case 'v':
			verbose = true;
			break;
		case 'r':
			rule_filter = optarg;
			break;
		case 'l':
			list_rules = true;
			break;
		default:
			usage(argv[0]);
			return 2;
		}
	}

	if (list_rules) {
		if (fmt == FW_OUTPUT_CSV)
			printf("record,rule,description\n");

		for (rulep = start; rulep < stop; rulep++) {
			const struct uboot_audit_rule *rule = *rulep;

			if (!rule->name || !rule->run)
				continue;
			print_rule_listing(fmt, rule);
		}
		return 0;
	}

	if (!dev || !size) {
		usage(argv[0]);
		return 2;
	}

	if (size > (uint64_t)SIZE_MAX) {
		fprintf(stderr, "Requested --size is too large for this host\n");
		return 2;
	}

	read_len = (size_t)size;
	buf = malloc(read_len);
	if (!buf) {
		fprintf(stderr, "Unable to allocate %zu bytes for audit input\n", read_len);
		return 1;
	}

	fd = open(dev, O_RDONLY | O_CLOEXEC);
	if (fd < 0) {
		fprintf(stderr, "Cannot open %s: %s\n", dev, strerror(errno));
		ret = 1;
		goto out;
	}

	got = pread(fd, buf, read_len, (off_t)offset);
	if (got < 0 || (size_t)got != read_len) {
		fprintf(stderr, "Failed to read %zu bytes from %s at 0x%jx\n",
			read_len, dev, (uintmax_t)offset);
		ret = 1;
		goto out;
	}

	uboot_crc32_init(crc32_table);

	if (fmt == FW_OUTPUT_CSV)
		printf("record,rule,status,message\n");

	for (rulep = start; rulep < stop; rulep++) {
		const struct uboot_audit_rule *rule = *rulep;
		char message[512] = {0};
		int rc;
		struct uboot_audit_input input;

		if (!rule_name_selected(rule_filter, rule))
			continue;
		if (!rule->run)
			continue;

		ran_any = true;
		input.device = dev;
		input.offset = offset;
		input.data = buf;
		input.data_len = read_len;
		input.crc32_table = crc32_table;
		input.verbose = verbose;

		rc = rule->run(&input, message, sizeof(message));
		print_rule_record(fmt, rule, rc, message[0] ? message : NULL);
		if (rc != 0)
			ret = 1;
	}

	if (!ran_any) {
		fprintf(stderr, "No audit rules matched%s%s\n",
			rule_filter ? " filter: " : "",
			rule_filter ? rule_filter : "");
		ret = 2;
	}

out:
	if (fd >= 0)
		close(fd);
	free(buf);
	return ret;
}
