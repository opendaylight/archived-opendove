CWD=$(shell pwd)
KVERSION=$(shell uname -r)
GATEWAY_MODULE_BUILD=/usr/src/linux-headers-$(KVERSION)

all:  
	make -C $(CWD)/dgadmin

clean:
	make -C dgadmin clean
