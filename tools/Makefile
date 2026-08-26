PROJECT_ROOT = ..
SRC_DIR      = ./

CC = gcc
RM = rm

SRC = $(wildcard ./*.c)

TARGET = ./imgcopy

%.o: %.c
	$(CC) -I$(PROJECT_ROOT)/include/ -MP -MD -MF $*.dep -c -o $*.o $*.c

$(TARGET): $(SRC:.c=.o)
	$(CC) -o $@ $(SRC:.c=.o)

all: $(TARGET)

-include $(SRC:.c=.dep)