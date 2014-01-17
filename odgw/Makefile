#
# Copyright (c) 2013 IBM Corporation
# All rights reserved.
#
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License v1.0 which accompanies this
# distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
#
# 


CWD=$(shell pwd)
KVERSION=$(shell uname -r)
GATEWAY_MODULE_BUILD=/usr/src/linux-headers-$(KVERSION)

CP=cp
MV=mv
RM=rm
MKDIR=mkdir

## LIBS to be reused from ODCS
OPENDOVE_DIR                = ..
ODCS_DIR		    = $(OPENDOVE_DIR)/odcs
MODULE_OSW                  = osw
MODULE_DPS                  = client_server_protocol
MODULE_FD_PROCESS           = fd_process
MODULE_TIMER                = timer

all:  build-prep

	make -C $(CWD)/dgadmin

clean: clean-libdirs
	make -C dgadmin clean

build-prep:
##
##      copy over needed library code from odcs ; cleanup objs
##
	@echo "** Copying required library code **"
	$(MKDIR) -p $(MODULE_FD_PROCESS)
	$(CP) -r $(ODCS_DIR)/$(MODULE_FD_PROCESS)/* $(MODULE_FD_PROCESS)
	find ./$(MODULE_FD_PROCESS) -name '*.o' | xargs $(RM)
	$(MKDIR) -p $(MODULE_OSW)
	$(CP) -r $(ODCS_DIR)/$(MODULE_OSW)/* $(MODULE_OSW)
	find ./$(MODULE_OSW) -name '*.o' | xargs $(RM)
	$(MKDIR) -p $(MODULE_DPS)
	$(CP) -r $(ODCS_DIR)/$(MODULE_DPS)/* $(MODULE_DPS)
	find ./$(MODULE_DPS) -name '*.o' | xargs $(RM)
	$(MKDIR) -p $(MODULE_TIMER)
	$(CP) -r $(ODCS_DIR)/$(MODULE_TIMER)/* $(MODULE_TIMER)
	find ./$(MODULE_TIMER) -name '*.o' | xargs $(RM)

clean-libdirs:
	$(RM) -rf $(MODULE_FD_PROCESS)
	$(RM) -rf $(MODULE_OSW)
	$(RM) -rf $(MODULE_DPS)
	$(RM) -rf $(MODULE_TIMER)
