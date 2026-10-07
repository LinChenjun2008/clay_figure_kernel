TARGET = $(PROJECT_ROOT)/build/lib/libsys.a

include $(PROJECT_ROOT)/scripts/tools_def.mk

ARFLAGS = rcs

CFLAGS += -fPIE -fpie -mcmodel=large

CFLAGS += -Wwrite-strings
CFLAGS += -Wa,--noexecstack
CFLAGS += -mno-sse -mno-mmx -mno-80387
