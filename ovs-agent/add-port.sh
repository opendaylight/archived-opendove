#!/bin/sh
#
# Copyright (c) 2013 IBM Corporation
# All rights reserved.
#
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License v1.0 which accompanies this
# distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
#

if [ "$#" -lt 2 ]; then
  echo "Usage: $0 <PORT_NAME> <VNID> [VM_IP] [VM_NET_MASK] [VM_GW_IP]"
  exit 1
fi

PORT_NAME=$1
VNID=$2

ip_a_to_i () {
    echo $1 | awk -F '.' '{printf "%d\n", ($1 * 2^24) + ($2 * 2^16) + ($3 * 2^8) + $4}'
}

if [ -z "$3" ]; then
    VM_IP="0"
else
    VM_IP=`ip_a_to_i $3`
fi

if [ -z "$4" ]; then
    VM_NET_MASK="0"
else
    VM_NET_MASK=`ip_a_to_i $4`
fi

if [ -z "$5" ]; then
    VM_GW_IP="0"
else
    VM_GW_IP=`ip_a_to_i $5`
fi

echo "Add port $PORT_NAME to VNID=$VNID with ip=$VM_IP netmask=$VM_NETMASK gw=$VM_GW_IP"
 
ovs-vsctl add-port br-dove $PORT_NAME

ofport=`ovs-vsctl get Interface $PORT_NAME ofport`

ovs-ofctl add-flow br-dove "priority=2,in_port=$ofport actions=learn(table=1,NXM_OF_ETH_DST[]=NXM_OF_ETH_SRC[],output:NXM_OF_IN_PORT[]),load:$VNID->NXM_NX_REG0[],load:$VM_IP->NXM_NX_REG1[],load:$VM_NET_MASK->NXM_NX_REG2[],load:$VM_GW_IP->NXM_NX_REG3[],controller(reason=no_match)"