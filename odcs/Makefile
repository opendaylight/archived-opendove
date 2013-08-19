#
# Copyright (c) 2013 IBM Corporation
# All rights reserved.
#
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License v1.0 which accompanies this
# distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
#
# 

#Variables
BUILD_TYPE := debug
#End of Variables
CP=cp
MV=mv
RM=rm
MKDIR=mkdir
CC=gcc
SVN=svn
PYTHON=python
FIND=find
BUILD_DIR = $(CURRENT_DIR)/build
CURRENT_DIR := $(shell pwd)
#Get the LINUX Vendor Distributor
OS_DISTRIBUTOR=$(shell $(CURRENT_DIR)/os_distributor_script)
#Get the PYTHON Include files
PYINC = $(shell python-config --includes)
#Get the PYTHON Linker defines
PYLD = $(shell python-config --ldflags)
ifeq ($(OS_DISTRIBUTOR), Ubuntu)
	PYLD += -levent -ljansson
else
ifeq ($(OS_DISTRIBUTOR), Fedora)
	PYLD += -levent -ljansson
else
	PYLD += -levent -ljansson -L/opt/boost/lib -L/opt/libev/lib -L/opt/libevent/lib -L/opt/jansson/lib
endif
endif

MODULE_INIT                    = init
MODULE_FD_PROCESS              = fd_process
MODULE_DPS_PROTOCOL            = client_server_protocol
MODULE_DATA_HANDLER            = data_handler
MODULE_TIMER                   = timer

INCLUDE_FOLDER                 = inc
DPS_PROTOCOL_INCLUDE           = $(MODULE_DPS_PROTOCOL)/inc
DATA_HANDLER_INCLUDE           = $(MODULE_DATA_HANDLER)/inc
TIMER_INCLUDE                  = $(MODULE_TIMER)/inc
INCLUDE_LIBEV                  = /opt/libev/include
INCLUDE_JANSSON                = /opt/jansson/include
INCLUDE_LIBEVENT2              = /opt/libevent/include

#PYTHON FOLDERS
MODULE_DATA_HANDLER_PY         = $(MODULE_DATA_HANDLER)/python

ALL_SOURCES = $(MODULE_INIT)/init.c
ALL_SOURCES += $(MODULE_INIT)/log.c
ALL_SOURCES += $(MODULE_INIT)/python_interface.c
ALL_SOURCES += $(MODULE_FD_PROCESS)/fd_process.c
ALL_SOURCES += $(MODULE_TIMER)/src/raw_proto_timer.c
ALL_SOURCES += $(MODULE_DATA_HANDLER)/src/client_protocol_interface.c
ALL_SOURCES += $(MODULE_DATA_HANDLER)/src/controller_interface.c
ALL_SOURCES += $(MODULE_DPS_PROTOCOL)/src/dps_svr_ctrl.c 
ALL_SOURCES += $(MODULE_DPS_PROTOCOL)/src/dps_pkt_process.c
ALL_SOURCES += $(MODULE_DPS_PROTOCOL)/src/dps_log.c

CLIENT_SOURCES = $(MODULE_DPS_PROTOCOL)/src/dps_svr_ctrl.c 
CLIENT_SOURCES += $(MODULE_DPS_PROTOCOL)/src/dps_pkt_process.c
CLIENT_SOURCES += $(MODULE_DPS_PROTOCOL)/src/dps_log.c
CLIENT_SOURCES += $(MODULE_DPS_PROTOCOL)/src/dps_client_ctrl.c

LIB_SOURCES = $(ALL_SOURCES)
CLIENT_LIB_SOURCES = $(CLIENT_SOURCES)

ifeq ($(BUILD_TYPE), debug)
#	CFLAGS := -fPIC -fno-strict-aliasing -g -fno-inline -O0 -Werror -DNDEBUG -DDOVE_SERVICE_APPLIANCE -fwrapv -Wall
	CFLAGS := -fPIC -fno-strict-aliasing -g -fno-inline -O0 -Werror -DNDEBUG -fwrapv -Wall
else
	CFLAGS := -fPIC -fno-strict-aliasing -g -O3 -Werror -DDOVE_SERVICE_APPLIANCE -fwrapv -Wall
endif

DEVKITINCS := -DDPS_SERVER

OBJECTS_C=$(ALL_SOURCES:.c=.o)
LIB_OBJECTS_C=$(ALL_SOURCES:.c=.o)
LIB_SHARED=dcslib.so

CLIENT_LIB_OBJECTS_C=$(CLIENT_SOURCES:.c=.o)

all: build-prep lib-modules move-modules

INCFLAGS = $(PYINC)                        \
           -I$(DPS_PROTOCOL_INCLUDE)       \
           -I$(DATA_HANDLER_INCLUDE)       \
           -I$(TIMER_INCLUDE)              \
           -I$(INCLUDE_FOLDER)             \
           -I$(INCLUDE_LIBEV)              \
           -I$(INCLUDE_JANSSON)            \
           -I$(INCLUDE_LIBEVENT2)

$(LIB_SHARED): $(LIB_OBJECTS_C)
	$(CC) $(LIB_OBJECTS_C) -shared $(PYLD) $(PYLIB) -o $@

.c.o:
	$(CC) -c $(INCFLAGS) $(CFLAGS) $(DEVKITINCS) $< -o $@

build-prep:
	$(RM) -rf $(BUILD_DIR)
	@echo "**  Preparing build tree"
	$(MKDIR) -p $(BUILD_DIR)

lib-modules: $(LIB_SOURCES) $(LIB_SHARED)

client: DEVKITINCS=
client: DOVE_SERVICE_APPLIANCE= 
client: $(CLIENT_LIB_OBJECTS_C)

move-modules:
	$(MV) $(LIB_SHARED) $(BUILD_DIR)/
	$(CP) -f $(MODULE_INIT)/*.py $(BUILD_DIR)/
	$(CP) -rf $(MODULE_DATA_HANDLER_PY)/* $(BUILD_DIR)/
	$(PYTHON) -m compileall $(BUILD_DIR)
	$(FIND) $(BUILD_DIR)/ -name "*.py"| xargs rm
	$(CP) -f $(MODULE_INIT)/dcs_server.py $(BUILD_DIR)/
	chmod +x $(BUILD_DIR)/dcs_server.py

clean:
	rm -rf $(MODULE_FD_PROCESS)/*.o
	rm -rf $(MODULE_DPS_PROTOCOL)/src/*.o
	rm -rf $(MODULE_DATA_HANDLER)/src/*.o
	rm -rf $(MODULE_INIT)/*.o
	rm -rf $(BUILD_DIR)
