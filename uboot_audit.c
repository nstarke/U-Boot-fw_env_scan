// SPDX-License-Identifier: MIT License - Copyright (c) 2026 Nicholas Starke

#include "uboot_scan.h"

#include <stdio.h>
#include <string.h>

static void usage(const char *prog)
{
	fprintf(stderr,
		"Usage: %s <subcommand> [options]\n"
		"\n"
		"Subcommands:\n"
		"  env     Scan for U-Boot environment candidates (fw_env_scan behavior)\n"
		"  image   Scan or extract U-Boot images (fw_image_scan behavior)\n"
		"\n"
		"Examples:\n"
		"  %s env --verbose\n"
		"  %s image --dev /dev/mtdblock4 --step 0x1000\n",
		prog, prog, prog);
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		usage(argv[0]);
		return 2;
	}

	if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help") || !strcmp(argv[1], "help")) {
		usage(argv[0]);
		return 0;
	}

	if (!strcmp(argv[1], "env"))
		return fw_env_scan_main(argc - 1, argv + 1);

	if (!strcmp(argv[1], "image"))
		return fw_image_scan_main(argc - 1, argv + 1);

	fprintf(stderr, "Unknown subcommand: %s\n\n", argv[1]);
	usage(argv[0]);
	return 2;
}
