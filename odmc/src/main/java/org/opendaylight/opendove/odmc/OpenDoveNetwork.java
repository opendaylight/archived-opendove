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

import org.opendaylight.controller.sal.utils.ServiceHelper;

@XmlRootElement
@XmlAccessorType(XmlAccessType.NONE)
public class OpenDoveNetwork extends OpenDoveObject implements IfOpenDCSTrackedObject, IfOpenDGWTrackedObject {

    @XmlElement(name="id")
    String uuid;

    @XmlElement(name="network_id")
    Integer vnid;

    @XmlElement(name="name")
    String name;

    @XmlElement(name="domain_id")
    String domain_uuid;

    @XmlElement (name="type")
    Integer networkType;

    String associatedOSNetworkUUID;

    public OpenDoveNetwork() { }

    public OpenDoveNetwork(String name, int vnid, OpenDoveDomain scopingDomain, int type, String oSNetworkUUID) {
        this.uuid = java.util.UUID.randomUUID().toString();
        this.vnid = vnid;
        this.name = name;
        this.domain_uuid = scopingDomain.getUUID();
        this.tombstoneFlag = false;
        this.networkType = type;
        this.associatedOSNetworkUUID = oSNetworkUUID;
    }

    @Override
    public String getUUID() {
        return uuid;
    }

    public void setUuid(String uuid) {
        this.uuid = uuid;
    }

    public Integer getVnid() {
        return vnid;
    }

    public void setVnid(Integer vnid) {
        this.vnid = vnid;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public String getDomain_uuid() {
        return domain_uuid;
    }

    public void setDomain_uuid(String domain_uuid) {
        this.domain_uuid = domain_uuid;
    }

    public Integer getNetworkType() {
        return networkType;
    }

    public void setNetworkType(Integer networkType) {
        this.networkType = networkType;
    }

    public boolean isTrackedByDCS() {
        return true;
    }

    public String getNeutronNetwork() {
        return associatedOSNetworkUUID;
    }

    public String getSBDcsUri() {
        return "/controller/sb/v2/opendove/odmc/domains/" + domain_uuid + "/networks/" + vnid;
    }

    public boolean isTrackedByDGW() {
        return true;
    }

    public String getSBDgwUri() {
        return "/controller/sb/v2/opendove/odmc/networks/" + uuid;
    }
}