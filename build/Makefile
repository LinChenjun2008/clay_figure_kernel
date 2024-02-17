include target.txt
include srclist.txt
include tools_def.txt

TMP_KERNEL_TARGET := .tmp_kernel
KERNEL_ASM_SRC    := $(filter %.S,$(KERNEL_SRC))
KERNEL_C_SRC      := $(filter %.c,$(KERNEL_SRC))

.PHONY: all
all: $(TARGET_BOOTLOADER) $(TARGET_KERNEL)

.PHONY: run
run: all
	-@"$(QEMU)" $(QEMU_FLAGS)

.PHONY: clean
clean:
	-@"$(RM)" $(KERNEL_C_SRC:c=o) $(KERNEL_ASM_SRC:S=o) $(TMP_KERNEL_TARGET) $(TARGET_BOOTLOADER)

$(TARGET_BOOTLOADER): $(BOOTLOADER_SRC)
	@$(ECHO) compile $^
	@"$(MINGW64_GCC)" $(BOOTLOADER_FLAGS) -o $@ $^

%.o: %.c $(KERNEL_SRC_PATH)/arch/$(TARGET_ARCH)/include/kernel/global.h
	@$(ECHO) compile $*.c
	@"$(X86_64-ELF-GCC)" $(KERNEL_FLAGS) -c -o $*.o $*.c

%.o: %.S
	@$(ECHO) compile $*.S
	@"$(X86_64-ELF-GCC)" -E $(KERNEL_FLAGS) $*.S > $*.tmp.S
	@"$(X86_64-ELF-AS)" --64 -o $*.o $*.tmp.S
	@"$(RM)" $*.tmp.S

$(TARGET_KERNEL): $(TMP_KERNEL_TARGET)
	@"$(OBJCOPY)" -I elf64-x86-64 --strip-debug -S -R ".eh_frame" -R ".comment" -O binary $^ $@

$(TMP_KERNEL_TARGET): $(KERNEL_C_SRC:c=o) $(KERNEL_ASM_SRC:S=o)
	@$(ECHO) link
	@"$(X86_64-ELF-LD)" -T $(KERNEL_LINKER_SCRIPT) -o $@ $^