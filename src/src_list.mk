SRC += $(SRC_DIR)/kernel/main.c
SRC += $(SRC_DIR)/kernel/symbols.c
SRC += $(SRC_DIR)/kernel/config.c
SRC += $(SRC_DIR)/kernel/service/kernel.c
SRC += $(SRC_DIR)/kernel/service/kern_task.c
SRC += $(SRC_DIR)/kernel/service/kern_mem.c

SRC += $(SRC_DIR)/softirq/softirq.c
SRC += $(SRC_DIR)/service/service.c
SRC += $(SRC_DIR)/service/tick/tick.c
SRC += $(SRC_DIR)/service/view/view.c

SRC += $(SRC_DIR)/mem/allocator.c
SRC += $(SRC_DIR)/mem/service/mm.c
SRC += $(SRC_DIR)/mem/vmm.c

SRC += $(SRC_DIR)/ramfs/ramfs.c
SRC += $(SRC_DIR)/sync/semaphore.c
SRC += $(SRC_DIR)/lib/bitmap.c
SRC += $(SRC_DIR)/lib/list.c
SRC += $(SRC_DIR)/lib/fifo.c
SRC += $(SRC_DIR)/lib/stdio.c
SRC += $(SRC_DIR)/lib/string.c
SRC += $(SRC_DIR)/lib/math.c
SRC += $(SRC_DIR)/graphic/print.c
SRC += $(SRC_DIR)/graphic/character.c

SRC += $(SRC_DIR)/ulib/ulib.c

SRC += $(SRC_DIR)/elf/elf.c