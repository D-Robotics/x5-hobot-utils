CROSS_COMPILE = /opt/gcc-arm-11.2-2022.02-x86_64-aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu-
CC = $(CROSS_COMPILE)gcc
STRIP = $(CROSS_COMPILE)strip

CFLAGS = -Wall -O2
LDFLAGS = -s

SOURCES := $(wildcard *.c)
EXECUTABLES := $(patsubst %.c,%,$(SOURCES))

all: $(EXECUTABLES)

%: %.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)
	$(STRIP) $@

PREFIX = ../debian/usr/bin/
install:
	@[ -d $(PREFIX) ] || mkdir -p $(PREFIX)
	@cp -rf $(EXECUTABLES) $(PREFIX)

clean:
	@rm -fv $(EXECUTABLES)

.PHONY: all clean install
