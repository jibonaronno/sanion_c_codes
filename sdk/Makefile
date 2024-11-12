####################################################################################
# PROJECT        : UR-IED
# AUTHOR         : sootoo23
# DATE           : 2016.11.04
# DESCRIPTION    : UR Master SDK LIB MAKE FILE
####################################################################################

export TI_SDK_PATH=/opt/ti-processor-sdk-linux-rt-am57xx-evm-05.02.00.10
URSDK_TOP=$(PWD)

LINUX_DEVKIT_PATH := $(TI_SDK_PATH)/linux-devkit
LINUX_INSTALL_PATH := /sanion/HUR-IED/bsp/linux-rt-4.14.79
LIB_PATH := $(URSDK_TOP)/lib
LIB_PATH2 := $(URSDK_TOP)/../libs

OBJDIR = $(URSDK_TOP)/obj
SRCDIR = $(URSDK_TOP)/src
OUTDIR = $(URSDK_TOP)/lib
MBUSDIR = $(URSDK_TOP)/linuxmodbus
SHAREDIR = /sanion/HUR-IED/application/shared

IMAGE_LIB = LU_MPU_FW

TOOL_PREFIX := arm-linux-gnueabihf-
# CC := $(TOOL_PREFIX)gcc
CC := $(TOOL_PREFIX)g++
LD := $(TOOL_PREFIX)ld
STRIP := $(TOOL_PREFIX)strip

# DEST_DIR=/WinShare/
DEST_DIR=/nfs_resource
TARGET_DIR=/sanion/HUR-IED/bsp/target-maker/apps/
INC_DIR = \
		  -I$(SRCDIR) \
		  -I$(URSDK_TOP)/include \
		  -I$(LINUX_DEVKIT_PATH)/sysroots/armv7ahf-neon-linux-gnueabi/usr/include \
		  -I$(SHAREDIR)  \
		#  -I$(LINUX_INSTALL_PATH)/include \
			$(INC_PATH)

EXTRA_CFLAGS += -fpermissive
EXTRA_CFLAGS += -c $(INC_DIR)
EXTRA_CFLAGS += -lc -lpthread -lm -lrt -lsqlite3 -l61850 -lticmem -lmodbus
#EXTRA_CFLAGS += -lc -lpthread -lm -lrt -lsqlite3 -l61850 -lticmem -lmodbus
#EXTRA_CFLAGS += -lc -lpthread -lm -lrt -lsqlite3 -lticmem

API_SRC = $(notdir $(shell find $(SRCDIR) -maxdepth 1 -name "*.c"))
API_OBJS := $(API_SRC:%.c=$(OBJDIR)/%.o)

$(OBJDIR)/%.o : $(SRCDIR)/%.c 
	@echo $@
	@$(CC) -L$(LIB_PATH) $(EXTRA_CFLAGS) -g -o "$@" "$<" 

.PHONY: all
all: setup combine install

setup:
	rm -f $(DEST_DIR)/$(IMAGE_LIB)

combine: $(IMAGE_LIB)
	@echo "Archieving Combined library :";

$(IMAGE_LIB): ${API_OBJS}
	$(CC) -L$(LIB_PATH) -L$(LIB_PATH2) -L$(SHAREDIR) -o $(DEST_DIR)/$(IMAGE_LIB) $(API_OBJS) -lc -lpthread -lm -lrt -lsqlite3 -l61850 -lticmem -lmodbus
	#$(CC) -L$(LIB_PATH) -L$(LIB_PATH2) -o $(DEST_DIR)/$(IMAGE_LIB) $(API_OBJS) -lc -lpthread -lm -lrt -lsqlite3 -lticmem -lmodbus
	echo "UR SDK LIB MAKE FINISHED!!";

install:
	$(STRIP) $(DEST_DIR)/$(IMAGE_LIB)
	cp $(URSDK_TOP)/include/UR_SDK.h $(SHAREDIR);
	cp $(SRCDIR)/global.h $(SHAREDIR);
	
clean:
	rm -f $(DEST_DIR)/$(IMAGE_LIB)
	rm $(OBJDIR)/*.o
