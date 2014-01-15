#!/bin/bash
# Sample script to bringup up the gateway appliance 
# bridge interface (APBR). Add all interfaces to the bridge.

VAR_PATH=`pwd`

for interface in $(ifconfig -a | busybox awk '/Ethernet  HWaddr/{print $1}')
do     
    if [ $interface = "APBR" ] ; then
        echo "APP BRIDGE ACTIVE ALREADY : EXIT"
        exit 
    fi  
done

brctl addbr APBR

for interface in $(ifconfig -a | busybox awk '/Ethernet  HWaddr/{print $1}')
do          
    if [ $interface != "APBR" ] ; then
        brctl addif APBR $interface
        /sbin/ifconfig $interface 0.0.0.0 up mtu 9000
        ethtool -K $interface rx off 
    fi
done

sleep 1
eth0mac=`/sbin/ifconfig eth0 | busybox awk '/Ethernet  HWaddr/{print $5}'`
/sbin/ifconfig APBR up
/sbin/ifconfig APBR mtu 9000
/sbin/ifconfig APBR down hw ether $eth0mac up

#Route related 
if [ -e /etc/iproute2/rt_tables ]
then
	if [ -e /etc/iproute2/rt_tables_orig ]
	then 
		cat /etc/iproute2/rt_tables_orig > /etc/iproute2/rt_tables
		
	else
		cat /etc/iproute2/rt_tables > /etc/iproute2/rt_tables_orig
	fi
fi
echo 1 > /proc/sys/net/ipv4/ip_forward

modprobe 8021q
insmod module/dgwy_module.ko

$VAR_PATH/build/dove_gateway.py
#Wait for few seconds for dhcp if any
#dhclient APBR &
#sleep 10

