RAMFS_DIR = $(PROJECT_ROOT)/build/ramfs
TARGET    = $(RAMFS_DIR)/kernel/system

include $(PROJECT_ROOT)/scripts/tools_def.mk

CFLAGS += -fno-pic -fno-pie -mcmodel=kernel

CFLAGS += -Wwrite-strings
CFLAGS += -Wa,--noexecstack
CFLAGS += -mno-sse -mno-mmx -mno-80387

LD_SCRIPT = $(SRC_DIR)/arch/$(TARGET_ARCH)/kernel.lds
LDFLAGS  += -T $(LD_SCRIPT)

OBJFLAGS  = -I elf64-x86-64
OBJFLAGS += --strip-debug -S -R ".eh_frame" -R ".comment" -O binary
