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

.PHONY: debug
debug: all
	-@"$(QEMU)" -S -s $(QEMU_FLAGS)

.PHONY: clean
clean:
	-@"$(RM)" $(KERNEL_C_SRC:c=o) $(KERNEL_C_SRC:c=dep) $(KERNEL_ASM_SRC:S=o) $(TMP_KERNEL_TARGET) $(TARGET_BOOTLOADER)

.PHONY: init
init:
	-$(MKDIR) "$(RUNNING_PATH)"
	-$(MKDIR) "$(ESP_PATH)"
	-$(MKDIR) "$(ESP_PATH)/EFI/"
	-$(MKDIR) "$(ESP_PATH)/EFI/Boot/"
	-$(MKDIR) "$(ESP_PATH)/Kernel/"

$(TARGET_BOOTLOADER): $(BOOTLOADER_SRC)
	@$(ECHO) compiling $^
	@"$(MINGW64_GCC)" $(BOOTLOADER_FLAGS) -o $@ $^

%.o: %.c
	@$(ECHO) [$(X86_64-ELF-GCC)] compiling $*.c
	@"$(X86_64-ELF-GCC)" $(KERNEL_FLAGS) -MP -MD -MF $*.dep -c -o $*.o $*.c

%.o: %.S
	@$(ECHO) [$(X86_64-ELF-AS)] compiling $*.S
	@"$(X86_64-ELF-GCC)" -E $(KERNEL_FLAGS) $*.S > $*.tmp.S
	@"$(X86_64-ELF-AS)" --64 -D -o $*.o $*.tmp.S
	@"$(RM)" $*.tmp.S

$(TARGET_KERNEL): $(TMP_KERNEL_TARGET)
	@"$(OBJCOPY)" -I elf64-x86-64 --strip-debug -S -R ".eh_frame" -R ".comment" -O binary $^ $@

$(TMP_KERNEL_TARGET): $(KERNEL_ASM_SRC:S=o) $(KERNEL_C_SRC:c=o) $(KERNEL_LINKER_SCRIPT)
	@$(ECHO) [$(X86_64-ELF-LD)] linking [1/2]
	@"$(X86_64-ELF-LD)" -T $(KERNEL_LINKER_SCRIPT) -pie -o $@ $(KERNEL_ASM_SRC:S=o) $(KERNEL_C_SRC:c=o)
	@"$(NM)" -W -n $@ | "./kallsyms" > $@_sym.c
	@"$(X86_64-ELF-GCC)" $(KERNEL_FLAGS) -c -o $@_sym.o $@_sym.c
	@$(ECHO) [$(X86_64-ELF-LD)] linking [2/2]
	@"$(X86_64-ELF-LD)" -T $(KERNEL_LINKER_SCRIPT) -pie -o $@ $(KERNEL_ASM_SRC:S=o) $(KERNEL_C_SRC:c=o) $@_sym.o
	@"$(RM)" $@_sym.c $@_sym.o

-include $(KERNEL_C_SRC:c=dep)