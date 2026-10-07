TARGET = $(PROJECT_ROOT)/build/esp/efi/boot/bootx64.efi

include $(PROJECT_ROOT)/scripts/tools_def.mk

CC = x86_64-w64-mingw32-gcc

CFLAGS += -fpic -fpie -mcmodel=small
CFLAGS += -fno-stack-protector
CFLAGS += -e efi_main -Wl,--subsystem,10