SRC_PATH = ../../../

include $(SRC_PATH)/target.mk
include $(SRC_PATH)/tools_def.mk
include src_list.mk

CFLAGS += -Wall -Wextra -Werror
CFLAGS += -I./ -I./include -I$(SRC_PATH)
CFLAGS += -e UefiMain -nostdinc -nostdlib
CFLAGS += -m64 -mcmodel=small
CFLAGS += -fno-stack-protector -fpic -fpie -fno-builtin -Wl,--subsystem,10

$(TARGET_BOOTLOADER): $(SRC)
	@$(ECHO) making $@
	@"$(MINGW64_GCC)" $(CFLAGS) -o $@ $^