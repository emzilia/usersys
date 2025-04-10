CC = gcc
CC_FLAGS = -Wall -g
LD_FLAGS = -lsystemd

SRC = main.c
TARGET = usersys

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $< -o $@ $(CC_FLAGS) $(LD_FLAGS)
