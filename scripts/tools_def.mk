ifeq ($(shell uname -s),Linux)
    ifeq ($(TOOLS_DEF),bootloader)
        CC  = x86_64-w64-mingw32-gcc
    else
        CC  = gcc
    endif
    AS      = as
    LD      = ld
    OBJCOPY = objcopy
    QEMU    = qemu-system-x86_64
    ECHO    = echo
    MKDIR   = mkdir
    RM      = rm
    NM      = nm
    OVMF    = OVMF.fd
else
    TOOL_DIR = $(PROJECT_DIR)/../tools
    ifeq ($(TOOLS_DEF),bootloader)
        CC  = $(TOOL_DIR)/MinGW64/bin/gcc.exe
    else
        CC  = $(TOOL_DIR)/X86_64-elf-tools/bin/x86_64-elf-gcc.exe
    endif
    AS      = $(TOOL_DIR)/x86_64-elf-tools/x86_64-elf/bin/as.exe
    LD      = $(TOOL_DIR)/x86_64-elf-tools/x86_64-elf/bin/ld.exe
    OBJCOPY = $(TOOL_DIR)/objcopy
    QEMU    = $(TOOL_DIR)/qemu/qemu-system-x86_64.exe
    ECHO    = echo
    MKDIR   = mkdir
    RM      = $(TOOL_DIR)/rm
    NM      = $(TOOL_DIR)/x86_64-elf-tools/x86_64-elf/bin/nm.exe
    OVMF    = $(ESP_DIR)/../bios.bin
endif

KERNEL_LINKER_SCRIPT    = $(SCRIPTS_DIR)/kernel.lds

KALLSYMS = $(SRC_DIR)/../build/kallsyms
IMGCOPY  = $(SRC_DIR)/../build/imgcopy

IMGCOPY_FLAGS = \
    -copy $(SRC_DIR)/config.txt config \

ifeq ($(TOOLS_DEF),bootloader)
    CFLAGS += -Wall -Wextra -Werror
    CFLAGS += -I$(SRC_DIR)/arch/$(TARGET_ARCH)/bootloader
    CFLAGS += -I$(SRC_DIR)/arch/$(TARGET_ARCH)/bootloader/include
    CFLAGS += -I$(SRC_DIR)
    CFLAGS += -e UefiMain -nostdinc -nostdlib
    CFLAGS += -m64 -mcmodel=small
    CFLAGS += -fno-stack-protector -fpic -fpie -fno-builtin -Wl,--subsystem,10
else
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
    CFLAGS += -I$(SRC_DIR)/arch/$(TARGET_ARCH)/include
    CFLAGS += -I$(SRC_DIR)/include
    CFLAGS += -I$(SRC_DIR)
    CFLAGS += -O0 -g3 -gdwarf-2 -gstrict-dwarf -nostdlib -nostdinc
    CFLAGS += -Wcast-align -Wwrite-strings
    CFLAGS += -finput-charset=UTF-8 -fexec-charset=UTF-8
    CFLAGS += -fno-builtin -fno-strict-aliasing -ffreestanding
    CFLAGS += -fstrength-reduce -falign-loops -falign-jumps
    CFLAGS += -fPIE -fwrapv
    CFLAGS += -fno-use-linker-plugin
    CFLAGS += -mno-red-zone -m64 -mcmodel=large -march=x86-64
    CFLAGS += -mstackrealign
    CFLAGS += -Wa,--noexecstack

    AFLAGS = $(CFLAGS) -D__ASM_INCLUDE__

    LDFLAGS = -T $(KERNEL_LINKER_SCRIPT) -pie

    OBJFLAGS  = -I elf64-x86-64
    OBJFLAGS += --strip-debug -S -R ".eh_frame" -R ".comment" -O binary
endif

QEMU_FLAGS = -m $(MEMORY) -bios $(OVMF) \
 -smp $(SMP_CORES),cores=$(SMP_CORES),threads=1,sockets=1 \
 -drive file=fat:rw:$(ESP_DIR),index=0,format=vvfat -net none \
 -usb \
 -device nec-usb-xhci,id=xhci \
 -device usb-mouse \
 -no-shutdown \

# -device qemu-xhci,id=xhci
# -device usb-kbd
# -d cpu_reset

# -chardev stdio,mux=on,id=com1 \
 -serial chardev:com1

# -monitor stdio