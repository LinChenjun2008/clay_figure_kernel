RAMFS_DIR = $(PROJECT_ROOT)/build/ramfs
TARGET    = $(RAMFS_DIR)/kernel/system

AS      = as
CC      = gcc
ECHO    = echo
LD      = ld
MKDIR   = mkdir
OBJCOPY = objcopy
RM      = rm -f

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
CFLAGS += -fno-pic -fno-pie -fwrapv
CFLAGS += -mno-red-zone -m64 -mcmodel=kernel -march=x86-64
CFLAGS += -mstackrealign
CFLAGS += -Wa,--noexecstack
CFLAGS += -mno-sse -mno-mmx -mno-80387

LD_SCRIPT = $(SRC_DIR)/arch/$(TARGET_ARCH)/kernel.lds
LDFLAGS = -T $(LD_SCRIPT)

OBJFLAGS  = -I elf64-x86-64
OBJFLAGS += --strip-debug -S -R ".eh_frame" -R ".comment" -O binary
