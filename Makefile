
ifndef PLATFORM
	$(error please sepcific PLATFORM (export PLATFORM=ARM / export PLATFORM=ARM64))
endif

OUTPUT := output
WORK_DIR := $(shell pwd)

CC_FLAGS += -Wall -Wextra -g 

OBJ_DIR = $(OUTPUT)/obj

DEMO_LIBRARY_DIR = $(WORK_DIR)/inject-library
DEMO_TARGET_DIR = $(WORK_DIR)/target
INJECT_TOOL_DIR = $(WORK_DIR)/inject-tools

INJECT_NAME = $(OUTPUT)/Inject
INJECT_ELF_NAME = $(INJECT_NAME).out
INJECT_MAP_NAME = $(INJECT_NAME).map
INJECT_DIS_NAME = $(INJECT_NAME).dis
INJECT_BIN_NAME = $(INJECT_NAME).bin

DEMO_LIBRARY_NAME = $(OUTPUT)/demo_library.so

DEMO_TARGET_NAME = $(OUTPUT)/demo_process
DEMO_TARGET_ELF_NAME = $(DEMO_TARGET_NAME).out
DEMO_TARGET_MAP_NAME = $(DEMO_TARGET_NAME).map
DEMO_TARGET_DIS_NAME = $(DEMO_TARGET_NAME).dis
DEMO_TARGET_BIN_NAME = $(DEMO_TARGET_NAME).bin

DEMO_LIBRARY_INCLUDE = $(WORK_DIR)/inject-library/inc
DEMO_TARGET_INCLUDE = $(WORK_DIR)/target/inc
INJECT_TOOL_INCLUDE = $(WORK_DIR)/inject-tools/inc

DEMO_LIBRARY_SRC = $(WORK_DIR)/inject-library/src
DEMO_TARGET_SRC = $(WORK_DIR)/target/src
INJECT_TOOL_SRC = $(WORK_DIR)/inject-tools/src


ifeq ($(PLATFORM),ARM)
        TOOLS = /opt/arm-gnu-toolchain-13.2.Rel1-x86_64-arm-none-linux-gnueabihf/bin/arm-none-linux-gnueabihf-
else ifeq ($(PLATFORM),ARM64)
        TOOLS = /opt/arm-gnu-toolchain-13.2.Rel1-x86_64-aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu-
endif

AS = $(TOOLS)as
CC = $(TOOLS)gcc
LD = $(TOOLS)gcc
OBJCOPY = $(TOOLS)objcopy
OBJDUMP = $(TOOLS)objdump
SIZE = $(TOOLS)size

CFLAGS += $(CC_FLAGS)
LDFLAGS += 

DEMO_LIBRARY_SRCS_C := $(shell find $(DEMO_LIBRARY_SRC) -name '*.c')
DEMO_LIBRARY_SRCS_S := $(shell find $(DEMO_LIBRARY_SRC) -name '*.S')
DEMO_LIBRARY_SRCS_s := $(shell find $(DEMO_LIBRARY_SRC) -name '*.s')

DEMO_TARGET_SRCS_C := $(shell find $(DEMO_TARGET_SRC) -name '*.c')
DEMO_TARGET_SRCS_S := $(shell find $(DEMO_TARGET_SRC) -name '*.S')
DEMO_TARGET_SRCS_s := $(shell find $(DEMO_TARGET_SRC) -name '*.s')

INJECT_TOOL_SRCS_C := $(shell find $(INJECT_TOOL_SRC) -name '*.c')
INJECT_TOOL_SRCS_S := $(shell find $(INJECT_TOOL_SRC) -name '*.S')
INJECT_TOOL_SRCS_s := $(shell find $(INJECT_TOOL_SRC) -name '*.s')

DEMO_LIBRARY_OBJS := $(DEMO_LIBRARY_SRCS_C:%.c=$(OBJ_DIR)/DEMO_LIBRARY_DIR/%.o) $(DEMO_LIBRARY_SRCS_S:%.S=$(OBJ_DIR)/DEMO_LIBRARY_DIR/%.o) $(DEMO_LIBRARY_SRCS_s:%.s=$(OBJ_DIR)/DEMO_LIBRARY_DIR/%.o)
DEMO_TARGET_OBJS := $(DEMO_TARGET_SRCS_C:%.c=$(OBJ_DIR)/DEMO_TARGET_DIR/%.o) $(DEMO_TARGET_SRCS_S:%.S=$(OBJ_DIR)/DEMO_TARGET_DIR/%.o) $(DEMO_TARGET_SRCS_s:%.s=$(OBJ_DIR)/DEMO_TARGET_DIR/%.o)
INJECT_TOOL_OBJS := $(INJECT_TOOL_SRCS_C:%.c=$(OBJ_DIR)/INJECT_TOOL_DIR/%.o) $(INJECT_TOOL_SRCS_S:%.S=$(OBJ_DIR)/INJECT_TOOL_DIR/%.o) $(INJECT_TOOL_SRCS_s:%.s=$(OBJ_DIR)/INJECT_TOOL_DIR/%.o)

DEMO_LIBRARY_F:= $(DEMO_LIBRARY_OBJS:%.o=%d)
DEMO_TARGET_OBJS_F :=$(DEMO_TARGET_OBJS:%.o=%d)
INJECT_TOOL_OBJS_F :=$(INJECT_TOOL_OBJS:%.o=%d)

-include $(DEMO_LIBRARY_F)
-include $(DEMO_TARGET_OBJS_F)
-include $(INJECT_TOOL_OBJS_F)

.PHONY: all clean

all: $(INJECT_ELF_NAME) $(DEMO_TARGET_ELF_NAME) $(DEMO_LIBRARY_NAME)
##############################################################
$(DEMO_LIBRARY_NAME):$(DEMO_LIBRARY_OBJS)
	$(CC) -shared -fPIC -o $@ $^
	@echo $(THREADX_LIB) has been created
##############################################################
$(INJECT_ELF_NAME):$(INJECT_TOOL_OBJS)
	$(LD) $(INJECT_TOOL_OBJS) $(LDFLAGS) -o $@ -Map $(INJECT_MAP_NAME)
	@echo $(INJECT_ELF_NAME) has been created
$(INJECT_DIS_NAME):$(INJECT_ELF_NAME)
	@$(OBJDUMP) -h -d $< > $@
	@echo $(INJECT_DIS_NAME) has been created
##############################################################
$(DEMO_TARGET_ELF_NAME):$(DEMO_TARGET_OBJS)
	$(LD) $(DEMO_TARGET_OBJS) $(LDFLAGS) -lpthread -o $@ -Map $(DEMO_TARGET_MAP_NAME)
	@echo $(DEMO_TARGET_ELF_NAME) has been created
$(DEMO_TARGET_DIS_NAME):$(DEMO_TARGET_ELF_NAME)
	@$(OBJDUMP) -h -d $< > $@
	@echo $(DEMO_TARGET_DIS_NAME) has been created

##############################################################
$(OBJ_DIR)/INJECT_TOOL_DIR/%.o:%.c
	@echo "COMPILING INJECT TOOL SOURCE $< TO OBJECT $@"
	@mkdir -p '$(@D)'
	$(CC) $(CFLAGS) $(INJECT_TOOL_INCLUDE) -o $@ -c $^

$(OBJ_DIR)/INJECT_TOOL_DIR/%.o:%.S
	@echo "COMPILING INJECT TOOL SOURCE $< TO OBJECT $@"
	@mkdir -p '$(@D)'
	$(CC) $(CFLAGS) $(INJECT_TOOL_INCLUDE) -o $@ -c $^

$(OBJ_DIR)/INJECT_TOOL_DIR/%.o:%.s
	@echo "COMPILING INJECT TOOL SOURCE $< TO OBJECT $@"
	@mkdir -p '$(@D)'
	$(CC) $(CFLAGS) $(INJECT_TOOL_INCLUDE) -o $@ -c $^
#############################################################
$(OBJ_DIR)/DEMO_TARGET_DIR/%.o:%.c
	@echo "COMPILING DEMO TARGET SOURCE $< TO OBJECT $@"
	@mkdir -p '$(@D)'
	@$(CC) $(CFLAGS) $(DEMO_TARGET_INCLUDE) -o $@ -c $^

$(OBJ_DIR)/DEMO_TARGET_DIR/%.o:%.s
	@echo "COMPILING  DEMO TARGET SOURCE $< TO OBJECT $@"
	@mkdir -p '$(@D)'
	@$(CC) $(CFLAGS) $(DEMO_TARGET_INCLUDE) -o $@ -c $^

$(OBJ_DIR)/DEMO_TARGET_DIR/%.o:%.S
	@echo "COMPILING DEMO TARGET SOURCE $< TO OBJECT $@"
	@mkdir -p '$(@D)'
	@$(CC) $(CFLAGS) $(DEMO_TARGET_INCLUDE) -o $@ -c $^
#############################################################
$(OBJ_DIR)/DEMO_LIBRARY_DIR/%.o:%.c
	@echo "COMPILING DEMO_LIBRARY SOURCE $< TO OBJECT $@"
	@mkdir -p '$(@D)'
	@$(CC) $(CFLAGS) $(DEMO_LIBRARY_INCLUDE) -o $@  -c $^

$(OBJ_DIR)/DEMO_LIBRARY_DIR/%.o:%.s
	@echo "COMPILING DEMO_LIBRARY SOURCE $< TO OBJECT $@"
	@mkdir -p '$(@D)'
	@$(CC) $(CFLAGS) $(DEMO_LIBRARY_INCLUDE) -o $@ -c $^

$(OBJ_DIR)/DEMO_LIBRARY_DIR/%.o:%.S
	@echo "COMPILING DEMO_LIBRARY SOURCE $< TO OBJECT $@"
	@mkdir -p '$(@D)'
	@$(CC) $(CFLAGS) $(DEMO_LIBRARY_INCLUDE) -o $@ -c $^
###############################################################

clean:
	rem -rf $(OUTPUT)
