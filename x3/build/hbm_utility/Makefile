ifeq ($(PARAM_FILE),) 
    PARAM_FILE:=../Makefile.param
    include $(PARAM_FILE)
endif

ifeq (${BUILD_OUTPUT_PATH},)
export BUILD_OUTPUT_PATH=$(shell pwd)
endif

.PHONY: all all_32 clean install

SRCS = $(wildcard *.c)

OBJECT := ${BUILD_OUTPUT_PATH}/hrut_hbm

LIBS= -lcjson
INCS += -I/opt/gcc-arm-11.2-2022.02-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu/libc/usr/include
INCS += -I${IMAGE_DEPLOY_DIR}/rootfs/usr/include/cjson

all all_32: $(OBJECT)

$(OBJECT): $(SRCS)
	@mkdir -p $(abspath $(dir $@))
	${CC} ${CFLAGS} $^ ${INCS} ${LDFLAGS} $(LIBS) -o $@

install:$(OBJECT)
	@install -d ${BUILD_OUTPUT_PATH}/../../debian/usr/bin/
	install $(OBJECT) ${BUILD_OUTPUT_PATH}/../../debian/usr/bin/

clean:
	rm -rf $(OBJECT)

