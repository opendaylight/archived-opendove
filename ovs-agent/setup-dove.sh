#!/bin/sh
#
# Copyright (c) 2013 IBM Corporation
# All rights reserved.
#
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License v1.0 which accompanies this
# distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
#

if [ "$#" -ne 2 ] && [ "$#" -ne 3 ]; then
  echo "Usage: $0 <local_ip> <dcs_ip:port> [local_port]"
  exit 1
fi

LOCAL_IP=$1
DCS_IP=$2
LOCAL_PORT=""

if [ "$#" -eq 3 ]; then
  LOCAL_PORT=$3
fi

ovs-vsctl -- --may-exist add-br  br-dove

ovs-vsctl set Bridge br-dove protocols=OpenFlow10,OpenFlow12

ovs-vsctl set-controller br-dove tcp:$LOCAL_IP:$LOCAL_PORT

ovs-vsctl set-fail-mode br-dove secure

ovs-vsctl -- --may-exist add-port br-dove dove -- set Interface dove type=vxlan options:remote_ip=flow options:key=flow options:dst_port=8472

./dove-controller ptcp:$LOCAL_PORT -d $DCS_IP