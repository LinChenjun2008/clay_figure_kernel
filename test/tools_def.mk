RAMFS_DIR = $(PROJECT_ROOT)/build/ramfs
TARGET    = $(RAMFS_DIR)/test

CC      = gcc
LD      = ld

CFLAGS += -Wall -Wextra -Werror
CFLAGS += -Wredundant-decls -Wnested-externs
CFLAGS += -Winline
CFLAGS += -Wshadow
CFLAGS += -Wpointer-arith
CFLAGS += -Wmissing-prototypes
CFLAGS += -Wmissing-declarations
CFLAGS += -Wuninitialized
CFLAGS += -Wno-long-long
CFLAGS += -Wno-implicit-fallthrough
CFLAGS += -I$(PROJECT_ROOT)/include/
CFLAGS += -I$(PROJECT_ROOT)/include/arch/$(TARGET_ARCH)/
CFLAGS += -O0 -g3 -gdwarf-2 -gstrict-dwarf -nostdlib -nostdinc
CFLAGS += -Wcast-align -Wwrite-strings
CFLAGS += -finput-charset=UTF-8 -fexec-charset=UTF-8
CFLAGS += -fno-builtin -fno-strict-aliasing -ffreestanding
CFLAGS += -falign-functions
CFLAGS += -fPIE -fpie -fwrapv
CFLAGS += -fno-omit-frame-pointer
CFLAGS += -mno-red-zone -m64 -mcmodel=large -march=x86-64
CFLAGS += -mstackrealign
CFLAGS += -Wa,--noexecstack
CFLAGS += -mno-sse -mno-mmx -mno-80387

LDFLAGS += -pie -static --no-dynamic-linker
