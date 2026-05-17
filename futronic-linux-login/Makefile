PREFIX ?= /opt/futronic-fs81
CC ?= cc
CFLAGS ?= -O2 -Wall -Wextra -Wpedantic
LDFLAGS ?=

SDK_DIR ?= ../futronic-/futronic-fs81/gtkdemo/Linux_gtk_demo_x64
BUILD_DIR := build
TARGET := $(BUILD_DIR)/futronic-auth

.PHONY: all clean install uninstall

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): src/futronic_auth.c src/ftrapi_compat.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Isrc -o $@ src/futronic_auth.c \
		-L$(SDK_DIR) -Wl,-rpath,'$$ORIGIN/../lib' -Wl,-rpath-link,$(SDK_DIR) \
		-l:ftrapi.so -l:libScanAPI.so $(LDFLAGS)

install: all
	install -d -m 0755 "$(DESTDIR)$(PREFIX)/bin" "$(DESTDIR)$(PREFIX)/lib" "$(DESTDIR)/usr/local/bin"
	install -m 0755 "$(TARGET)" "$(DESTDIR)$(PREFIX)/bin/futronic-auth"
	install -m 0644 "$(SDK_DIR)/ftrapi.so" "$(DESTDIR)$(PREFIX)/lib/ftrapi.so"
	install -m 0644 "$(SDK_DIR)/libScanAPI.so" "$(DESTDIR)$(PREFIX)/lib/libScanAPI.so"
	ln -sfn "$(PREFIX)/bin/futronic-auth" "$(DESTDIR)/usr/local/bin/futronic-auth"

uninstall:
	rm -f "$(DESTDIR)/usr/local/bin/futronic-auth"
	rm -rf "$(DESTDIR)$(PREFIX)"

clean:
	rm -rf "$(BUILD_DIR)"
