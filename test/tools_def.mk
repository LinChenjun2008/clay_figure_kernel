RAMFS_DIR = $(PROJECT_ROOT)/build/ramfs
TARGET    = $(RAMFS_DIR)/test

include $(PROJECT_ROOT)/scripts/tools_def.mk

CFLAGS += -fPIE -fpie -mcmodel=large

CFLAGS += -Wwrite-strings
CFLAGS += -Wa,--noexecstack
CFLAGS += -mno-sse -mno-mmx -mno-80387

LDFLAGS += -pie -static --no-dynamic-linker
