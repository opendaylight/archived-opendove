#!/bin/sh
#
# Copyright (c) 2013 IBM Corporation
# All rights reserved.
#
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License v1.0 which accompanies this
# distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
#

##################################################
## dependency: Open vSwitch -- download and build
#################################################

OVS_URL_STABLE="http://openvswitch.org/releases/openvswitch-1.10.0.tar.gz"
OVS_URL_LATEST="http://openvswitch.org/cgi-bin/gitweb.cgi?p=openvswitch;a=snapshot;sf=tgz;h=HEAD"
OVS_DISTFILE=./openvswitch.tar.gz
OVS_DIR=./openvswitch

## clean up any existing files
for ovsfile in openvswitch*
do
    rm -rf $ovsfile
done

## fetch the distro 
wget $OVS_URL_STABLE -O $OVS_DISTFILE
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
    exit 1
fi

exit 0
