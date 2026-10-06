PROJECT_ROOT = .
SCRIPTS_DIR  = $(PROJECT_ROOT)/scripts
BUILD_DIR    = $(PROJECT_ROOT)/build
TOOLS_DIR    = $(PROJECT_ROOT)/tools
ESP_DIR      = $(BUILD_DIR)/esp
OBJ_DIR      = $(BUILD_DIR)/objects
RAMFS_DIR    = $(BUILD_DIR)/ramfs

include $(PROJECT_ROOT)/tools_def.mk

include $(PROJECT_ROOT)/modules.mk

.PHONY: all
all:
	@$(ECHO) ---[ Build ]---
	@$(MAKE) -r $(addsuffix /all,$(MODULES))
	@$(MAKE) -r initramfs
	@$(ECHO) ---[ Done  ]---

.PHONY: clean
clean:
	@$(ECHO) ---[ Clean ]---
	@$(MAKE) -r $(addsuffix /clean,$(MODULES))
	@$(RM) $(INTIRAMFS)
	@$(RM) -r $(OBJ_DIR)
	@$(ECHO) ---[ Done  ]---

.PHONY: init
init:
	@$(ECHO) ---[ Init  ]---
	-@$(MKDIR) -p "$(BUILD_DIR)"
	-@$(MKDIR) -p "$(BUILD_DIR)/lib"
	-@$(MKDIR) -p "$(ESP_DIR)"
	-@$(MKDIR) -p "$(RAMFS_DIR)"
	-@$(MKDIR) -p "$(RAMFS_DIR)/kernel"
	-@$(MKDIR) -p "$(ESP_DIR)/efi"
	-@$(MKDIR) -p "$(ESP_DIR)/efi/boot"
	@$(MAKE) -r -C "$(TOOLS_DIR)" all
	@$(ECHO) ---[ Done  ]---

.PHONY: initramfs
initramfs:
	@$(ECHO) update initramfs
	@$(FIND) $(RAMFS_DIR) -type f | $(IMGCOPY) $(RAMFS_DIR)/ > $(INTIRAMFS)

.PHONY: run
run: all
	-@$(QEMU) $(QEMU_FLAGS)

%/all:
	@$(MAKE) -r -C $(@D) TARGET_ARCH=$(TARGET_ARCH) all

%/clean:
	@$(MAKE) -r -C $(@D) TARGET_ARCH=$(TARGET_ARCH) clean