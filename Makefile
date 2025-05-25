OUTPUT := output
HOST_ARCH :=$(shell arch)
WORK_DIR := $(shell pwd)
CC_FLAGS += -Wall -Wextra
CFLAGS = $(CC_FLAGS)
OBJ_DIR = $(OUTPUT)/obj

PROG_NAME = $(OUTPUT)/SW_APP
ELF_NAME = $(PROG_NAME).out
MAP_NAME = $(PROG_NAME).map
DIS_NAME = $(PROG_NAME).dis
BIN_NAME = $(PROG_NAME).bin

OBJ_DIR = $(OUTPUT)/obj
APPSRC_INCLUDE = $(WORK_DIR)/include

AS = $(TOOLS)as
CC = $(TOOLS)gcc
LD = $(TOOLS)gcc
OBJCOPY = $(TOOLS)objcopy
OBJDUMP = $(TOOLS)objdump
SIZE = $(TOOLS)size

SRCS_C := $(shell find $(WORK_DIR) -name '*.c')
SRCS_S := $(shell find $(WORK_DIR) -name '*.S')
SRCS_s := $(shell find $(WORK_DIR) -name '*.s')

OBJS := $(SRCS_C:%.c=$(OBJ_DIR)/%.o) $(SRCS_S:%.S=$(OBJ_DIR)/%.o) $(SRCS_s:%.s=$(OBJ_DIR)/%.o)
DEP_F:= $(OBJS:%.o=%d)

ifeq ($(HOST_ARCH),x86_64)
        TOOLS = /opt/arm-gnu-toolchain-13.2.Rel1-x86_64-arm-none-linux-gnueabihf/bin/arm-none-linux-gnueabihf-
else ifeq ($(HOST_ARCH),aarch64)
        TOOLS = /opt/arm-gnu-toolchain-12.3.rel1-aarch64-arm-none-linux-gnueabihf/bin/arm-none-linux-gnueabihf-
endif

inject:
	$(CC) $(CFLAGS) -o inject ptrace.c inject.c -g
demo-library.so: demo-library.c
	$(CC) $(CFLAGS) -shared -o demo-library.so -fPIC demo-library.c
demo-target: demo-target.c
	$(CC) $(CFLAGS) -o demo-target demo-target.c -g -no-pie


