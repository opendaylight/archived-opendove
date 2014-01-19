#
# Copyright (c) 2013 IBM Corporation
# All rights reserved.
#
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License v1.0 which accompanies this
# distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
#

 #Variables

OVS_DIR = ../third-party/openvswitch
OVS_LIB = $(OVS_DIR)/lib/libopenvswitch.a

CURL_DIR = ../third-party
CURL_LIB = $(CURL_DIR)/lib/libcurl.so

BUILD_TYPE := debug
#End of Variables
CP=cp
MV=mv
RM=rm
CD=cd
MKDIR=mkdir
CC=gcc
SVN=svn
PYTHON=python
FIND=find
BUILD_DIR = $(CURRENT_DIR)/build
CURRENT_DIR := $(shell pwd)
#Get the LINUX Vendor Distributor
OS_DISTRIBUTOR=$(shell $(CURRENT_DIR)/os_distributor_script)

MODULE_ODCS		   = ../odcs
MODULE_ODCS_FD_PROCESS     = $(MODULE_ODCS)/fd_process
MODULE_ODCS_DPS_PROTOCOL   = $(MODULE_ODCS)/client_server_protocol
MODULE_ODCS_DATA_HANDLER   = $(MODULE_ODCS)/data_handler
MODULE_ODCS_TIMER          = $(MODULE_ODCS)/timer

OVS_INCLUDE		   = -I$(OVS_DIR) -I$(OVS_DIR)/include  -I$(OVS_DIR)/lib
ODCS_INCLUDE		   = $(MODULE_ODCS)/inc
DPS_PROTOCOL_INCLUDE       = $(MODULE_ODCS_DPS_PROTOCOL)/inc
DATA_HANDLER_INCLUDE       = $(MODULE_ODCS_DATA_HANDLER)/inc
TIMER_INCLUDE              = $(MODULE_ODCS_TIMER)/inc
CURL_INCLUDE               = $(CURL_DIR)/include

CLIENT_PROT_INCFLAGS = -I$(ODCS_INCLUDE)       \
           -I$(DPS_PROTOCOL_INCLUDE)       \
           -I$(DATA_HANDLER_INCLUDE)       \
           -I$(TIMER_INCLUDE) 

INCFLAGS =  $(OVS_INCLUDE)                \
           -I$(CURL_INCLUDE)

OVS_AGENT = dove-controller
OVS_AGENT_SOURCES = dove-switch.c
OVS_AGENT_SOURCES += dove-controller.c
OVS_AGENT_SOURCES += dove-dmc-client.c

CLIENT_PROT_SOURCES = $(MODULE_ODCS_DPS_PROTOCOL)/src/dps_pkt_process.c
CLIENT_PROT_SOURCES += $(MODULE_ODCS_DPS_PROTOCOL)/src/dps_log.c
CLIENT_PROT_SOURCES += $(MODULE_ODCS_DPS_PROTOCOL)/src/dps_client_ctrl.c
TIMER_SOURCES += $(MODULE_ODCS_TIMER)/src/raw_proto_timer.c

ifeq ($(BUILD_TYPE), debug)
	CFLAGS := -fPIC -fno-strict-aliasing -g -fno-inline -O0 -Werror -DNDEBUG -fwrapv -Wall
else
	CFLAGS := -fPIC -fno-strict-aliasing -g -O3 -Werror -fwrapv -Wall
endif

OVS_AGENT_OBJECTS = $(OVS_AGENT_SOURCES:.c=.o)
CLIENT_PROT_OBJECTS=$(CLIENT_PROT_SOURCES:.c=.o)
TIMER_OBJECTS=$(TIMER_SOURCES:.c=.o)

OVS_AGENT_LDFLAGS = -lssl -lcrypto  -lpthread -lrt -lm -ljansson


all: $(OVS_AGENT)

$(CLIENT_PROT_OBJECTS):
	rm -f $(MODULE_ODCS_DPS_PROTOCOL)/src/*.o
	cd $(MODULE_ODCS); make client
#	
#	$(CC) -c $(CLIENT_PROT_INCFLAGS) $(CFLAGS) $< -o $@

dove-controller:  $(CLIENT_PROT_OBJECTS) $(OVS_AGENT_OBJECTS) $(OVS_LIB) 
	$(CC) -o $@ $(OVS_AGENT_OBJECTS) $(OVS_DIR)/lib/libopenvswitch.a $(CURL_LIB) $(CLIENT_PROT_OBJECTS) $(TIMER_OBJECTS) $(OVS_AGENT_LDFLAGS) 

dove-switch.o: dove-switch.c dove-switch.h 
	$(CC) -c $(INCFLAGS) $(CLIENT_PROT_INCFLAGS) $(CFLAGS) $< -o $@

dove-controller.o: dove-controller.c
	$(CC) -c $(INCFLAGS) $(CLIENT_PROT_INCFLAGS) $(CFLAGS) $< -o $@

.c.o:
	$(CC) -c $(INCFLAGS) $(CFLAGS) $< -o $@

lib-modules: $(LIB_SOURCES) $(LIB_SHARED)

client: DEVKITINCS=
client: DOVE_SERVICE_APPLIANCE= 
client: $(CLIENT_LIB_OBJECTS_C)

clean:
	$(RM) -rf $(OVS_AGENT)
	$(RM) -rf $(OVS_AGENT_OBJECTS)
	$(RM) -rf $(CLIENT_PROT_OBJECTS)
