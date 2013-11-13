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
public class OpenDoveVGWVNIDMapping extends OpenDoveObject implements IfOpenDGWTrackedObject {

    @XmlElement(name="id")
    String uuid;

    @XmlElement(name="net_id")
    Integer vnid;

    @XmlElement(name="vlan")
    Integer vlanID;

    @XmlElement(name="gatewayUUID")
    String gatewayUUID;

    public OpenDoveVGWVNIDMapping() { }

    @Override
    public String getUUID() {
        return uuid;
    }

    public void setUUID(String uuid) {
        this.uuid = uuid;
    }

    public Integer getVnid() {
        return vnid;
    }

    public void setVnid(Integer vnid) {
        this.vnid = vnid;
    }

    public Integer getVlan() {
        return vlanID;
    }

    public void setVlan(Integer vlan_id) {
        this.vlanID = vlan_id;
    }

    public String getGatewayUUID() {
        return gatewayUUID;
    }

    public void setGatewayUUID(String gatewayUUID) {
        this.gatewayUUID = gatewayUUID;
    }

    public boolean isTrackedByDGW() {
        return true;
    }

    public String getSBDgwUri() {
        return "/controller/sb/v2/opendove/odmc/vlan-gws/"+uuid;
    }

    public boolean overwrite(OpenDoveVGWVNIDMapping delta) {
        boolean answer = false; // whether we need to bump change version number or not
        if (delta.getVnid() != null) {
            if (getVnid() != delta.getVnid()) {
                setVnid(delta.getVnid());
                answer = true;
            }
        }
        if (delta.getGatewayUUID() != null) {
            if (!getGatewayUUID().equalsIgnoreCase(delta.getGatewayUUID())) {
                setGatewayUUID(delta.getGatewayUUID());
                answer = true;
            }
        }
        return answer;
    }
}
