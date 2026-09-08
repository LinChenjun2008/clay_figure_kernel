PROJECT_ROOT = .
SCRIPTS_DIR  = $(PROJECT_ROOT)/scripts
BUILD_DIR    = $(PROJECT_ROOT)/build
TOOLS_DIR    = $(PROJECT_ROOT)/tools
ESP_DIR      = $(BUILD_DIR)/esp
RAMFS_DIR    = $(BUILD_DIR)/ramfs

include $(PROJECT_ROOT)/tools_def.mk

.PHONY: all
all:
	@$(ECHO) ---[ Build ]---
	@$(MAKE) -C bootloader/$(TARGET_ARCH) TARGET_ARCH=$(TARGET_ARCH) all
	@$(MAKE) -C kernel TARGET_ARCH=$(TARGET_ARCH) all
	@$(MAKE) -C lib TARGET_ARCH=$(TARGET_ARCH) all
	@$(MAKE) -C init TARGET_ARCH=$(TARGET_ARCH) all
	@$(MAKE) -C test TARGET_ARCH=$(TARGET_ARCH) all
	@$(MAKE) -r initramfs
	@$(ECHO) ---[ Done  ]---

.PHONY: clean
clean:
	@$(ECHO) ---[ Clean ]---
	@$(MAKE) -C bootloader/$(TARGET_ARCH) TARGET_ARCH=$(TARGET_ARCH) clean
	@$(MAKE) -C kernel TARGET_ARCH=$(TARGET_ARCH) clean
	@$(MAKE) -C lib TARGET_ARCH=$(TARGET_ARCH) clean
	@$(MAKE) -C init TARGET_ARCH=$(TARGET_ARCH) all
	@$(MAKE) -C test TARGET_ARCH=$(TARGET_ARCH) clean
	@$(RM) $(INTIRAMFS)
	@$(ECHO) ---[ Done  ]---

.PHONY: init
init:
	@$(ECHO) ---[ Init  ]---
	-@$(MKDIR) "$(BUILD_DIR)"
	-@$(MKDIR) "$(BUILD_DIR)/lib"
	-@$(MKDIR) "$(ESP_DIR)"
	-@$(MKDIR) "$(RAMFS_DIR)"
	-@$(MKDIR) "$(RAMFS_DIR)/kernel"
	-@$(MKDIR) "$(ESP_DIR)/efi"
	-@$(MKDIR) "$(ESP_DIR)/efi/boot"
	@$(MAKE) -C "$(TOOLS_DIR)" all
	@$(ECHO) ---[ Done  ]---

.PHONY: initramfs
initramfs:
	@$(ECHO) update initramfs
	@$(FIND) $(RAMFS_DIR) -type f | $(IMGCOPY) $(RAMFS_DIR)/ > $(INTIRAMFS)

.PHONY: run
run: all
	-@$(QEMU) $(QEMU_FLAGS)