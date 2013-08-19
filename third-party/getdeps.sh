#!/bin/sh
#
# Copyright (c) 2013 IBM Corporation
# All rights reserved.
#
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License v1.0 which accompanies this
# distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
#

## Open vSwitch -- download and build latest snapshot


OVS_URL="http://openvswitch.org/releases/openvswitch-1.10.0.tar.gz"
OVS_URL_LATEST="http://openvswitch.org/cgi-bin/gitweb.cgi?p=openvswitch;a=snapshot;sf=tgz;h=HEAD"
OVS_DISTFILE=./openvswitch.tar.gz
OVS_DIR=./openvswitch

wget $OVS_URL -O $OVS_DISTFILE
if [ -e openvswitch.tar.gz ]
then
    tar xfz $OVS_DISTFILE
    ovsdir=openvswitch-*
    ln -s $ovsdir $OVS_DIR
# boot.sh requires the autoreconf program
    cd $OVS_DIR
    ./boot.sh
    ./configure;make
else
    return 1
fi

return 0
