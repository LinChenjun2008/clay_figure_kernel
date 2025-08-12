ifeq ($(shell uname -s),Linux)
    include $(SRC_PATH)/linux_tools_def.mk
else
    include $(SRC_PATH)/win_tools_def.mk
endif

KALLSYMS = ../build/kallsyms
IMGCOPY  = ../build/imgcopy

IMGCOPY_FLAGS = \
    -copy $(KERNEL_SRC_PATH)/config.txt config \

BOOTLOADER_FLAGS  = -Wall -Wextra -Werror \
                    -I$(BOOTLOADER_INCLUDE_PATH) \
                    -I$(SRC_PATH) \
                    -I$(BOOTLOADER_SRC_PATH) \
                    -e UefiMain \
                    -nostdinc -nostdlib \
                    -fno-stack-protector \
                    -m64 -mcmodel=small -fpic -fpie \
                    -fno-builtin -Wl,--subsystem,10

KERNEL_FLAGS  = -I$(KERNEL_ARCH_INCLUDE_PATH) \
                -I$(SRC_PATH) \
                -I$(KERNEL_INCLUDE_PATH) \
                -O0 \
                -g3 -gdwarf-2 -gstrict-dwarf \
                -nostdlib -nostdinc

KERNEL_FLAGS += -Wall -Wextra -Werror \
                -Wcast-align -Wwrite-strings \
                -Wredundant-decls -Wnested-externs \
                -Winline \
                -Wshadow \
                -Wpointer-arith \
                -Wmissing-prototypes \
                -Wmissing-declarations \
                -Wuninitialized \
                -Wno-long-long \
                -Wno-implicit-fallthrough

KERNEL_FLAGS += -finput-charset=UTF-8 -fexec-charset=UTF-8 \
                -fno-builtin -fno-strict-aliasing -ffreestanding \
                -fstrength-reduce -falign-loops -falign-jumps \
                -fPIE -fwrapv \
                -fno-use-linker-plugin

KERNEL_FLAGS += -mno-red-zone -m64 -mcmodel=large -march=x86-64 \
                -mstackrealign \

KERNEL_FLAGS += -Wa,--noexecstack \

KERNEL_C_FLAGS = $(KERNEL_FLAGS)
KERNEL_S_FLAGS = $(KERNEL_FLAGS) -D__ASM_INCLUDE__

QEMU_FLAGS := -m $(MEMORY) -bios $(OVMF) \
 -smp $(SMP_CORES),cores=$(SMP_CORES),threads=1,sockets=1 \
 -drive file=fat:rw:$(ESP_PATH),index=0,format=vvfat -net none \
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