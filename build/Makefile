include srclist.txt
include target.txt
include tools_def.txt

TMP_KERNEL_TARGET := .tmp_kernel

.PHONY: all
all: $(TARGET_BOOTLOADER) $(TARGET_KERNEL)

.PHONY: run
run: all
	-@"$(QEMU)" $(QEMU_FLAGS)

.PHONY: clean
clean:
	-@"$(RM)" $(KERNEL_SRC:c=o) $(TMP_KERNEL_TARGET) $(TARGET_BOOTLOADER)

$(TARGET_BOOTLOADER): $(BOOTLOADER_SRC)
	@$(ECHO) compile $^
	@"$(MINGW64_GCC)" $(BOOTLOADER_FLAGS) -o $@ $^

%.o: %.c
	@$(ECHO) compile $*.c
	@"$(X86_64-ELF-GCC)" $(KERNEL_FLAGS) -c -o $*.o $*.c

$(TARGET_KERNEL): $(TMP_KERNEL_TARGET)
	@"$(OBJCOPY)" -I elf64-x86-64 --strip-debug -S -R ".eh_frame" -R ".comment" -O binary $^ $@

$(TMP_KERNEL_TARGET): $(KERNEL_SRC:c=o)
	@$(ECHO) link
	@"$(X86_64-ELF-LD)" -T $(KERNEL_LINKER_SCRIPT) -o $@ $^
