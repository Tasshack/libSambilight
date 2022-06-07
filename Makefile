-include ../toolchain.conf
-include ../install.conf

LIB_NAME=Sambilight

#CC=arm-linux-gnueabi-gcc
CC=~/VDLinux-arm-v7a9v3r1/bin/arm-v7a9v3r1-linux-gnueabi-gcc

OUTDIR?=${PWD}/out-${ARCH}

TARGETS=lib${LIB_NAME}.so
TARGETS:=$(TARGETS:%=${OUTDIR}/%)

CFLAGS += -fPIC -O2 -std=gnu99
CFLAGS += -ldl

C_FILES=sambilight.c hook.c
 
C_FILES+= led_manager.c 
CFLAGS+= -DLED_MANAGER=1 
CFLAGS+= -march=armv7-a -mtune=arm7 -ffast-math -mfpu=vfpv3 -funsafe-math-optimizations -mfloat-abi=softfp 


all: ${OUTDIR} ${TARGETS}
    
${OUTDIR}/lib${LIB_NAME}.so: ${C_FILES} $(wildcard *.h)
	$(CC) $(filter %.c %.cpp,$^) ${CFLAGS} -shared -Wl,-soname,$@ -o $@

${OUTDIR}:
	@mkdir -p ${OUTDIR}

clean:
	rm -f ${TARGETS}

ifeq (${TARGET_IP}, )
endif

install: ${TARGETS}
	ping -c1 -W1 -w1 ${TARGET_IP} >/dev/null && \
        lftp -v -c "open ${TARGET_IP};cd /mnt/opt/privateer/usr/libso;mput $^;"

.PHONY: clean
.PHONY: install