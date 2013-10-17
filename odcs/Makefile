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
MODULE_CLUSTER_PROTOCOL        = cluster_protocol
MODULE_CONTROLLER_PROTOCOL     = controller_protocol
MODULE_DPS_PROTOCOL            = client_server_protocol
MODULE_DATA_HANDLER            = data_handler
MODULE_TIMER                   = timer
MODULE_OS_WRAPPER              = osw
MODULE_CLI                     = cli

INCLUDE_FOLDER                 = inc
DPS_PROTOCOL_INCLUDE           = $(MODULE_DPS_PROTOCOL)/inc
DATA_HANDLER_INCLUDE           = $(MODULE_DATA_HANDLER)/inc
TIMER_INCLUDE                  = $(MODULE_TIMER)/inc
CLUSTER_INCLUDE                = $(MODULE_CLUSTER_PROTOCOL)/inc
CONTROLLER_INCLUDE             = $(MODULE_CONTROLLER_PROTOCOL)/inc
INCLUDE_LIBEV                  = /opt/libev/include
INCLUDE_JANSSON                = /opt/jansson/include
INCLUDE_LIBEVENT2              = /opt/libevent/include
INCLUDE_FDPROCESS              = $(MODULE_FD_PROCESS)
INCLUDE_OS_WRAPPER             = $(MODULE_OS_WRAPPER)/inc
CLI_INCLUDE                    = $(MODULE_CLI)/inc

#PYTHON FOLDERS
MODULE_DATA_HANDLER_PY         = $(MODULE_DATA_HANDLER)/python
MODULE_CLI_PY                  = $(MODULE_CLI)/python

ALL_SOURCES = $(MODULE_INIT)/init.c
ALL_SOURCES += $(MODULE_INIT)/log.c
ALL_SOURCES += $(MODULE_INIT)/python_interface.c
ALL_SOURCES += $(MODULE_FD_PROCESS)/fd_process.c
ALL_SOURCES += $(MODULE_TIMER)/src/raw_proto_timer.c
ALL_SOURCES += $(MODULE_DATA_HANDLER)/src/client_protocol_interface.c
ALL_SOURCES += $(MODULE_DATA_HANDLER)/src/controller_interface.c
ALL_SOURCES += $(MODULE_DATA_HANDLER)/src/cluster_database.c
ALL_SOURCES += $(MODULE_DATA_HANDLER)/src/uuid.c
ALL_SOURCES += $(MODULE_DATA_HANDLER)/src/statistics.c
ALL_SOURCES += $(MODULE_DATA_HANDLER)/src/heartbeat.c
ALL_SOURCES += $(MODULE_DATA_HANDLER)/src/retransmit_interface.c
ALL_SOURCES += $(MODULE_DATA_HANDLER)/src/debug_interface.c
ALL_SOURCES += $(MODULE_DPS_PROTOCOL)/src/dps_svr_ctrl.c 
ALL_SOURCES += $(MODULE_DPS_PROTOCOL)/src/dps_pkt_process.c
ALL_SOURCES += $(MODULE_DPS_PROTOCOL)/src/dps_log.c
ALL_SOURCES += $(MODULE_CONTROLLER_PROTOCOL)/src/rest_main.c
ALL_SOURCES += $(MODULE_CONTROLLER_PROTOCOL)/src/rest_api.c
ALL_SOURCES += $(MODULE_CONTROLLER_PROTOCOL)/src/evhttp_helper.c
ALL_SOURCES += $(MODULE_CONTROLLER_PROTOCOL)/src/rest_req_handler.c
ALL_SOURCES += $(MODULE_CONTROLLER_PROTOCOL)/src/dove_rest_client.c
ALL_SOURCES += $(MODULE_CONTROLLER_PROTOCOL)/src/rest_forward.c
ALL_SOURCES += $(MODULE_CONTROLLER_PROTOCOL)/src/rest_client_dove_controller.c
ALL_SOURCES += $(MODULE_CONTROLLER_PROTOCOL)/src/rest_client_cluster_leader.c
ALL_SOURCES += $(MODULE_CONTROLLER_PROTOCOL)/src/rest_sync.c
ALL_SOURCES += $(MODULE_CLUSTER_PROTOCOL)/src/cluster_rest_client.c
ALL_SOURCES += $(MODULE_CLUSTER_PROTOCOL)/src/cluster_rest_req_handler.c
ALL_SOURCES += $(MODULE_CLI)/src/interface.c $(MODULE_CLI)/src/config.c
ALL_SOURCES += $(MODULE_CLI)/src/client_server_protocol.c
ALL_SOURCES += $(MODULE_CLI)/src/data_objects.c
ALL_SOURCES += $(MODULE_CLI)/src/cluster.c
ALL_SOURCES += $(MODULE_CLI)/src/debug.c
ALL_SOURCES += $(MODULE_OS_WRAPPER)/init.c
ALL_SOURCES += $(MODULE_OS_WRAPPER)/queue.c
ALL_SOURCES += $(MODULE_OS_WRAPPER)/semaphore.c
ALL_SOURCES += $(MODULE_OS_WRAPPER)/task.c
ALL_SOURCES += $(MODULE_OS_WRAPPER)/timer.c

CLIENT_SOURCES = $(MODULE_DPS_PROTOCOL)/src/dps_svr_ctrl.c 
CLIENT_SOURCES += $(MODULE_DPS_PROTOCOL)/src/dps_pkt_process.c
CLIENT_SOURCES += $(MODULE_DPS_PROTOCOL)/src/dps_log.c
CLIENT_SOURCES += $(MODULE_DPS_PROTOCOL)/src/dps_client_ctrl.c

LIB_SOURCES = $(ALL_SOURCES)
CLIENT_LIB_SOURCES = $(CLIENT_SOURCES)

ifeq ($(BUILD_TYPE), debug)
#	CFLAGS := -fPIC -fno-strict-aliasing -g -fno-inline -O0 -Werror -DNDEBUG -DNON_POSIX_MQUEUE -DDOVE_SERVICE_APPLIANCE -fwrapv -Wall
	CFLAGS := -fPIC -fno-strict-aliasing -g -fno-inline -O0 -Werror -DNDEBUG -DNON_POSIX_MQUEUE -fwrapv -Wall
else
#	CFLAGS := -fPIC -fno-strict-aliasing -g -O3 -Werror -DNON_POSIX_MQUEUE -DDOVE_SERVICE_APPLIANCE -fwrapv -Wall
	CFLAGS := -fPIC -fno-strict-aliasing -g -O3 -Werror -DNON_POSIX_MQUEUE -fwrapv -Wall
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
           -I$(CONTROLLER_INCLUDE)         \
           -I$(CLUSTER_INCLUDE)            \
           -I$(INCLUDE_FOLDER)             \
           -I$(INCLUDE_LIBEV)              \
           -I$(INCLUDE_JANSSON)            \
           -I$(INCLUDE_FDPROCESS)          \
           -I$(INCLUDE_OS_WRAPPER)         \
           -I$(CLI_INCLUDE)                \
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
	$(CP) -rf $(MODULE_CLI_PY)/* $(BUILD_DIR)/
	$(PYTHON) -m compileall $(BUILD_DIR)
	$(FIND) $(BUILD_DIR)/ -name "*.py"| xargs rm
	$(CP) -f $(MODULE_INIT)/dcs_server.py $(BUILD_DIR)/
	chmod +x $(BUILD_DIR)/dcs_server.py

clean:
	rm -rf $(MODULE_FD_PROCESS)/*.o
	rm -rf $(MODULE_DPS_PROTOCOL)/src/*.o
	rm -rf $(MODULE_DATA_HANDLER)/src/*.o
	rm -rf $(MODULE_CONTROLLER_PROTOCOL)/src/*.o
	rm -rf $(MODULE_CLUSTER_PROTOCOL)/src/*.o
	rm -rf $(MODULE_INIT)/*.o
	rm -rf $(MODULE_CLI)/src/*.o
	rm -rf $(MODULE_TIMER)/src/*.o
	rm -rf $(MODULE_OS_WRAPPER)/*.o
	rm -rf $(BUILD_DIR)
