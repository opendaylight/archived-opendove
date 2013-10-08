/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */


package org.opendaylight.opendove.odmc;

import javax.xml.bind.annotation.XmlAccessType;
import javax.xml.bind.annotation.XmlAccessorType;
import javax.xml.bind.annotation.XmlElement;
import javax.xml.bind.annotation.XmlRootElement;

@XmlRootElement
@XmlAccessorType(XmlAccessType.NONE)
public class OpenDoveGwIpv4 extends OpenDoveObject implements IfOpenDGWTrackedObject {
    @XmlElement(name="id")
    String uuid;

    @XmlElement(name="ip")
    String ip;

    @XmlElement(name="mask")
    String mask;

    @XmlElement(name="nexthop")
    String nexthop;

    @XmlElement (name="type")
    String type;

    @XmlElement (name="gwUUID")
    String gwUUID;

    @XmlElement (name="vlan")
    Integer vlan;


    public OpenDoveGwIpv4() { }

    public OpenDoveGwIpv4(String ip, String mask, String nexthop, String type, String gwUUID, Integer vlan) {
        this.uuid = java.util.UUID.randomUUID().toString();
        this.ip = ip;
        this.mask = mask;
        this.nexthop = nexthop;
        this.type = type;
        this.gwUUID = gwUUID;
        this.vlan = vlan;
        this.tombstoneFlag = false;
    }

    @Override
    public String getUUID() {
        return uuid;
    }

    public void setUuid(String uuid) {
        this.uuid = uuid;
    }

    public String getIP() {
        return ip;
    }

    public void setIP(String ip) {
        this.ip = ip;
    }

    public String getMask() {
        return mask;
    }

    public void setMask(String mask) {
        this.mask = mask;
    }

    public String getNexthop() {
        return nexthop;
    }

    public void setNexthop(String nexthop) {
        this.nexthop = nexthop;
    }

    public String getType() {
        return type;
    }

    public void setType(String type) {
        this.type = type;
    }

    public String getGWIndex() {
        return gwUUID;
    }

    public void setGWIndex(String gwUUID) {
        this.gwUUID = gwUUID;
    }

    public Integer getVlan() {
        return vlan;
    }

    public void setVlan(Integer vlan) {
        this.vlan = vlan;
    }

    public boolean isTrackedByDGW() {
        return true;
    }

    public String getSBDgwUri() {
        return "/controller/sb/v2/opendove/odmc/odgw-ipv4/" + uuid;
    }
}