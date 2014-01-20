#!/bin/bash
#
# Copyright (c) 2013 IBM Corporation
# All rights reserved.
#
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License v1.0 which accompanies this
# distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
#


function get_ovs {
    echo "###################################################"
    echo "## dependency: Open vSwitch -- download and build #"
    echo "###################################################"
    
    OVS_URL_STABLE="http://openvswitch.org/releases/openvswitch-2.0.0.tar.gz"
    OVS_URL_LATEST="http://openvswitch.org/cgi-bin/gitweb.cgi?p=openvswitch;a=snapshot;sf=tgz;h=HEAD"

    OVS_DISTFILE=./openvswitch.tar.gz
    OVS_DIR=./openvswitch

## clean up any existing files
    echo "Cleaning up existing files"
    for ovsfile in openvswitch*
    do
	rm -rf $ovsfile
    done

## fetch the distro 
    wget $OVS_URL_STABLE -O $OVS_DISTFILE
    if [ -e openvswitch.tar.gz ]
    then
	mkdir $OVS_DIR
	tar xfz $OVS_DISTFILE -C $OVS_DIR --strip-components 1
# NOTE: boot.sh requires the autoreconf program
	cd $OVS_DIR
	./boot.sh
	./configure;make
	cd ..
    else
	echo "$OVS_DISTFILE not found"
	return 1
    fi
    
    return 0
}



function get_libcurl {

    echo "##################################################"
    echo "## dependency: libcurl -- download and build #####"
    echo "##################################################"

    LIBCURL_STABLE="http://curl.haxx.se/download/curl-7.33.0.tar.gz"

    LIBCURL_DISTFILE=./libcurl.tar.gz
    LIBCURL_DIR=./libcurl

## clean up any existing files
    echo "Cleaning up existing files"
    for file in libcurl*
    do
	rm -rf $file
    done

## fetch the distro 
    wget $LIBCURL_STABLE -O $LIBCURL_DISTFILE
    if [ -e libcurl.tar.gz ]
    then
	mkdir $LIBCURL_DIR
	tar xfz $LIBCURL_DISTFILE -C $LIBCURL_DIR --strip-components 1
	cd $LIBCURL_DIR
	LIBCURL_INSTALL=$PWD/install
	./configure --prefix=$LIBCURL_INSTALL;make;make install
	cd ..
    else
	echo "$LIBCURL_DISTFILE not found"
	return 1
    fi

    return 0
}

################## MAIN ############
get_ovs
get_libcurl
