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

OVS_URL_STABLE="http://openvswitch.org/releases/openvswitch-2.0.0.tar.gz"
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
    cd ..
else
    exit 1
fi

LIBCURL_STABLE="http://curl.haxx.se/download/curl-7.33.0.tar.gz"

LIBCURL_DISTFILE=./libcurl.tar.gz
LIBCURL_DIR=./libcurl

## clean up any existing files
for file in libcurl*
do
    rm -rf $file
done

## fetch the distro 
wget $LIBCURL_STABLE -O $LIBCURL_DISTFILE
if [ -e libcurl.tar.gz ]
then
    tar xfz $LIBCURL_DISTFILE
    libcurldir=curl*
    ln -s $libcurldir $LIBCURL_DIR

    installdir=$PWD
    cd $LIBCURL_DIR
    echo $installdir
    ./configure --prefix=$installdir ;make; make install
    cd ..
else
    exit 1
fi

exit 0
