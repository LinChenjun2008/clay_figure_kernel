TARGET = $(PROJECT_ROOT)/build/esp/efi/boot/bootx64.efi

CC   = x86_64-w64-mingw32-gcc
ECHO = echo
LD   = ld
RM   = rm

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
CFLAGS += -I$(PROJECT_ROOT)/include
CFLAGS += -I$(PROJECT_ROOT)/include/arch/$(TARGET_ARCH)/
CFLAGS += -e efi_main -nostdinc -nostdlib
CFLAGS += -m64 -mcmodel=small
CFLAGS += -fno-stack-protector -fpic -fpie -fno-builtin -Wl,--subsystem,10