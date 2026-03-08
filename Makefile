CC      ?= gcc
CFLAGS  ?= -O2 -Wall -Wextra
LDFLAGS ?=

TARGET := uboot_audit
SRC    := uboot_audit.c uboot_env_scan.c uboot_image_scan.c uboot_scan.c

.PHONY: all env image static clean

all: $(TARGET)

env: $(TARGET)

image: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC)

static: LDFLAGS += -static
static: all

clean:
	rm -f $(TARGET)