include target.txt
include srclist.txt
include tools_def.txt

KERNEL_ASM_SRC    := $(filter %.S,$(KERNEL_SRC))
KERNEL_C_SRC      := $(filter %.c,$(KERNEL_SRC))

.PHONY: all
all:
	@$(ECHO) compiling...
	@$(MAKE) -r $(TARGET_BOOTLOADER) $(TARGET_KERNEL) update-initramfs
	@$(ECHO) done.

.PHONY: update-initramfs
update-initramfs:
	@$(RM) $(TARGET_INITFAMFS)
	@$(MAKE) -r $(TARGET_INITFAMFS)

.PHONY: run
run: all
	-@"$(QEMU)" $(QEMU_FLAGS)

.PHONY: debug
debug: all
	-@"$(QEMU)" -S -s $(QEMU_FLAGS)

.PHONY: clean
clean:
	-@"$(RM)" $(KERNEL_C_SRC:c=o) $(KERNEL_C_SRC:c=dep)   \
	$(KERNEL_ASM_SRC:S=o) $(KERNEL_ASM_SRC:S=dep) \
	$(TMP_KERNEL_TARGET) $(TARGET_BOOTLOADER)

.PHONY: init
init:
	-$(MKDIR) "$(RUNNING_PATH)"
	-$(MKDIR) "$(ESP_PATH)"
	-$(MKDIR) "$(ESP_PATH)/EFI"
	-$(MKDIR) "$(ESP_PATH)/EFI/Boot"
	-$(MKDIR) "$(ESP_PATH)/Kernel"

$(TARGET_BOOTLOADER): $(BOOTLOADER_SRC)
	@$(ECHO) making $@
	@"$(MINGW64_GCC)" $(BOOTLOADER_FLAGS) -o $@ $^

%.o: %.c
	@$(ECHO) compiling $*.c
	@"$(X86_64-ELF-GCC)" $(KERNEL_FLAGS) -MP -MD -MF $*.dep -c -o $*.o $*.c

%.o: %.S
	@$(ECHO) compiling $*.S
	@"$(X86_64-ELF-GCC)" $(KERNEL_FLAGS) -MP -MD -MF $*.dep -c -o $*.o $*.S

$(TARGET_KERNEL): $(TMP_KERNEL_TARGET)
	@$(ECHO) making $@
	@"$(OBJCOPY)" -I elf64-x86-64 --strip-debug -S -R ".eh_frame" -R ".comment" -O binary $^ $@

$(TMP_KERNEL_TARGET): $(KERNEL_ASM_SRC:S=o) $(KERNEL_C_SRC:c=o) $(KERNEL_LINKER_SCRIPT)
	@$(ECHO) linking [1/2]
	@"$(X86_64-ELF-LD)" -T $(KERNEL_LINKER_SCRIPT) -pie -o $@ $(KERNEL_ASM_SRC:S=o) $(KERNEL_C_SRC:c=o)
	@"$(NM)" -W -n $@ | "$(KALLSYMS)" > $@_sym.c
	@"$(X86_64-ELF-GCC)" $(KERNEL_FLAGS) -c -o $@_sym.o $@_sym.c
	@$(ECHO) linking [2/2]
	@"$(X86_64-ELF-LD)" -T $(KERNEL_LINKER_SCRIPT) -pie -o $@ $(KERNEL_ASM_SRC:S=o) $(KERNEL_C_SRC:c=o) $@_sym.o
	@"$(RM)" $@_sym.c $@_sym.o

$(TARGET_INITFAMFS):
	@$(ECHO) make initramfs
	@"$(IMGCOPY)" $(IMGCOPY_FLAGS) > $(ESP_PATH)/Kernel/initramfs.img

-include $(KERNEL_ASM_SRC:S=dep)
-include $(KERNEL_C_SRC:c=dep)
