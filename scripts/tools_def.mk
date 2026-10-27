AR      = ar
AS      = as
CC      = gcc
ECHO    = echo
FIND    = find
LD      = ld
MKDIR   = mkdir
OBJCOPY = objcopy
OVMF    = OVMF.fd
QEMU    = qemu-system-x86_64
RM      = rm -f

# RELEASE_MODE = RELEASE
RELEASE_MODE = DEBUG

CFLAGS += -Wall -Wextra -Werror
CFLAGS += -Wredundant-decls -Wnested-externs
CFLAGS += -Winline -Wshadow -Wpointer-arith
CFLAGS += -Wmissing-prototypes -Wmissing-declarations
CFLAGS += -Wuninitialized -Wno-long-long -Wno-implicit-fallthrough
CFLAGS += -I$(PROJECT_ROOT)/include/
CFLAGS += -I$(PROJECT_ROOT)/include/arch/$(TARGET_ARCH)/
CFLAGS += -O0 -g3 -gdwarf-2 -gstrict-dwarf -nostdlib -nostdinc
CFLAGS += -Wcast-align
CFLAGS += -finput-charset=UTF-8 -fexec-charset=UTF-8
CFLAGS += -fno-builtin -fno-strict-aliasing -ffreestanding
CFLAGS += -falign-functions
CFLAGS += -fwrapv
CFLAGS += -fno-omit-frame-pointer
CFLAGS += -mno-red-zone -m64 -march=x86-64
CFLAGS += -mstackrealign
CFLAGS += -D$(RELEASE_MODE)
