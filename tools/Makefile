PROJECT_ROOT = ..
SRC_DIR      = ./
OBJ_DIR      = $(PROJECT_ROOT)/build/objects/$(MODULE_NAME)
MODULE_NAME  = imgcopy

CC = gcc
RM = rm

SRC = $(wildcard ./*.c)

TARGET = ./imgcopy

%.o: %.c
	@$(CC) -I$(PROJECT_ROOT)/include/ -MP -MD -MF $(@:.o=.dep) -c -o $@ $<

$(TARGET): $(SRC:.c=.o)
	$(CC) -o $@ $(SRC:.c=.o)

all: $(TARGET)

-include $(SRC:.c=.dep)