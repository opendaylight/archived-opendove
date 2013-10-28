#!/bin/sh
#
# Copyright (c) 2013 IBM Corporation
# All rights reserved.
#
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License v1.0 which accompanies this
# distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
#

if [ "$#" -ne 1 ]; then
  echo "Usage: $0 <PORT_NAME>"
  exit 1
fi

PORT_NAME=$1

ofport=`ovs-vsctl get Interface $PORT_NAME ofport`
ovs-ofctl del-flows br-dove "in_port=$ofport"

ovs-vsctl del-port $PORT_NAME
