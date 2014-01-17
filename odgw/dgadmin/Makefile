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
CC=g++
SVN=svn
PYTHON=python
FIND=find
BUILD_DIR = $(CURRENT_DIR)/build
CURRENT_DIR := $(shell pwd)
#Get the LINUX Vendor Distributor
OS_DISTRIBUTOR := $(shell $(CURRENT_DIR)/os_distributor_script)
#Get the PYTHON Include files
PYINC = $(shell python-config --includes)
#Get the PYTHON Linker defines
PYLD = $(shell python-config --ldflags)
PYLD += -levent -ljansson
##NO BOOST FOR NOW
#PYLD += -lboost_system -lboost_thread -lboost_date_time
ifeq ($(OS_DISTRIBUTOR), Ubuntu)
	PYLD += -levent -ljansson -lnl -luuid
else
ifeq ($(OS_DISTRIBUTOR), Fedora)
	PYLD += -levent -ljansson -lnl -luuid
else
	PYLD += -levent -ljansson -L/opt/boost/lib -L/opt/libev/lib -L/opt/libevent/lib -L/opt/jansson/lib -lnl -luuid
endif
endif
#PYINC = /usr/include/python2.6
#PYINC = /usr/include/python2.7

PARENT_DIR                  = ..
ODCS_DIR		    = $(PARENT_DIR)/../odcs
MODULE_DPS                  = $(PARENT_DIR)/client_server_protocol
MODULE_FD_PROCESS           = $(PARENT_DIR)/fd_process
MODULE_COMMON               = $(PARENT_DIR)/common
MODULE_OSW                  = $(PARENT_DIR)/osw
MODULE_CLI                  = cli
MODULE_TIMER                = $(PARENT_DIR)/timer
MODULE_CTRL_INTERFCE        = controller_interface

INCLUDE_FOLDER              = inc
CLI_INCLUDE                 = $(MODULE_CLI)/inc
DPS_INCLUDE                 = $(MODULE_DPS)/inc
COMMON_INCLUDE              = $(MODULE_COMMON)
OSW_INCLUDE                 = $(MODULE_OSW)/inc
TIMER_INCLUDE               = $(MODULE_TIMER)/inc
CONTROLLER_INCLUDE          = $(MODULE_CTRL_INTERFCE)/inc
BOOST_INCLUDE               = /opt/boost/include
INCLUDE_LIBEV               = /opt/libev/include
INCLUDE_JANSSON             = /opt/jansson/include
INCLUDE_LIBEVENT2           = /opt/libevent/include

#PYTHON FOLDERS
MODULE_CLI_PY               = $(MODULE_CLI)/python

ALL_SOURCES = dgadm.c dgadmutil.c dgwy_ctrl_task.c dgwy_vlan_ha_task.c
ALL_SOURCES += log.c python_interface.c init.c dgadmin_generic_api.c
ALL_SOURCES += $(MODULE_OSW)/init.c $(MODULE_OSW)/task.c $(MODULE_OSW)/semaphore.c
ALL_SOURCES += $(MODULE_OSW)/queue.c $(MODULE_OSW)/timer.c
ALL_SOURCES += $(MODULE_FD_PROCESS)/fd_process.c
ALL_SOURCES += $(MODULE_CLI)/src/cli_interface.c
ALL_SOURCES += $(MODULE_CLI)/src/main_menu.c
ALL_SOURCES += $(MODULE_CLI)/src/service.c
ALL_SOURCES += $(MODULE_DPS)/src/dps_client_ctrl.c 
ALL_SOURCES += $(MODULE_DPS)/src/dps_pkt_process.c
ALL_SOURCES += $(MODULE_DPS)/src/dps_log.c
ALL_SOURCES += $(MODULE_TIMER)/src/raw_proto_timer.c
ALL_SOURCES += $(MODULE_CTRL_INTERFCE)/src/dgadmin_rest_main.c
ALL_SOURCES += $(MODULE_CTRL_INTERFCE)/src/dgadmin_rest_api.c
ALL_SOURCES += $(MODULE_CTRL_INTERFCE)/src/dgadmin_evhttp_helper.c
ALL_SOURCES += $(MODULE_CTRL_INTERFCE)/src/dgadmin_rest_req_handler.c
ALL_SOURCES += $(MODULE_CTRL_INTERFCE)/src/dgadmin_rest_client.c
ALL_SOURCES += $(MODULE_CTRL_INTERFCE)/src/dgadmin_rest_sync.c

EXEC_SOURCES = $(ALL_SOURCES)
LIB_SOURCES = $(ALL_SOURCES)
LIB_SOURCES_CPP = $(ALL_SOURCES_CPP)

ifeq ($(BUILD_TYPE), debug)
	CFLAGS := -fPIC -fno-strict-aliasing -g -lstdc++ -std=c++0x -O0 -DNDEBUG -DNON_POSIX_MQUEUE -DDOVE_SERVICE_APPLIANCE -fwrapv -Wall -Werror
else
	CFLAGS := -fPIC -fno-strict-aliasing -g -lstdc++ -std=c++0x -O3 -DNON_POSIX_MQUEUE -DDOVE_SERVICE_APPLIANCE -fwrapv -Wall -Werror
endif

#OBJECTS=$(EXEC_SOURCES:.c=.o)
#EXECUTABLE=dgadmin

LIB_OBJECTS_C=$(LIB_SOURCES:.c=.o)
LIB_OBJECTS_CPP=$(LIB_SOURCES_CPP:.cpp=.o)
LIB_SHARED=dgadmin.so

#all: build-prep exec-modules lib-modules move-modules move-executable
all: build-prep lib-modules move-modules

INCFLAGS = -I$(PYINC)                      \
           -I$(COMMON_INCLUDE)             \
           -I$(DPS_INCLUDE)                \
           -I$(OSW_INCLUDE)                \
           -I$(CLI_INCLUDE)                \
           -I$(CONTROLLER_INCLUDE)         \
           -I$(TIMER_INCLUDE)              \
           -I$(INCLUDE_LIBEV)              \
           -I$(INCLUDE_JANSSON)            \
           -I$(INCLUDE_LIBEVENT2)          \
           -I$(BOOST_INCLUDE)              \
           -I$(INCLUDE_FOLDER)

$(EXECUTABLE): $(OBJECTS_CPP) $(OBJECTS_C)
	$(CC) $(OBJECTS_CPP) $(OBJECTS_C) $(PYLD) $(PYLIB) -o $@

$(LIB_SHARED): $(LIB_OBJECTS_CPP) $(LIB_OBJECTS_C)
	$(CC) $(LIB_OBJECTS_CPP) $(LIB_OBJECTS_C) -shared $(PYLD) $(PYLIB) -o $@

.c.o:
	$(CC) -c $(INCFLAGS) $(CFLAGS) $< -o $@

.cpp.o:
	$(CC) -c $(INCFLAGS) $(CFLAGS) $(DEVKITINCS) $< -o $@

build-prep:
	$(RM) -rf $(BUILD_DIR)
	@echo "**  Preparing build tree"
	$(MKDIR) -p $(BUILD_DIR)


exec-modules: $(EXEC_SOURCES) $(EXECUTABLE)

lib-modules: $(LIB_SOURCES) $(LIB_SHARED)
	@echo $(LIB_SOURCES)

move-modules:
	$(MV) $(LIB_SHARED) $(BUILD_DIR)/
	$(CP) -rf $(MODULE_CLI_PY)/* $(BUILD_DIR)/
	$(CP) -f *.py $(BUILD_DIR)/
	$(PYTHON) -m compileall $(BUILD_DIR)
	$(FIND) $(BUILD_DIR)/ -name "*.py"| xargs rm
	$(CP) -f dove_gateway.py $(BUILD_DIR)/
	chmod +x $(BUILD_DIR)/dove_gateway.py

move-executable: $(MV) $(EXECUTABLE) $(BUILD_DIR)

clean:
	rm -rf $(MODULE_CTRL_INTERFCE)/src/*.o
	rm -rf $(MODULE_CLI)/src/*.o
	rm -rf $(MODULE_CTRL_INTERFCE)/*.o
	rm -rf *.o
	rm -rf $(EXECUTABLE)
	rm -rf $(BUILD_DIR)
	rm -rf $(LIB_SHARED)

