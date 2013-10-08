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
public class OpenDovePolicy extends OpenDoveObject implements IfOpenDCSTrackedObject {

    @XmlElement(name="id")
    String uuid;

    @XmlElement(name="type")
    Integer policyType;

    @XmlElement(name="src_network")
    Integer sourceVNID;

    @XmlElement(name="dst_network")
    Integer destinationVNID;

    @XmlElement(name="ttl")
    Integer timeToLive;    // legacy

    @XmlElement(name="action")
    Integer policyAction;

    @XmlElement(name="domain_id")
    String domainUUID;

    @XmlElement(name="traffic_type")
    Integer trafficType;

    public OpenDovePolicy() { }

    public OpenDovePolicy(Integer src_vnid, Integer dst_vnid, String dom_UUID, Integer tType) {
    	uuid = java.util.UUID.randomUUID().toString();
    	sourceVNID = src_vnid;
    	destinationVNID = dst_vnid;
    	timeToLive = 1000;
    	policyAction = 1;
    	domainUUID = dom_UUID;
    	trafficType = tType;
    }
    
    @Override
    public String getUUID() {
        return uuid;
    }

    public void setUUID(String uuid) {
        this.uuid = uuid;
    }


    public Integer getPolicyType() {
        return policyType;
    }

    public void setPolicyType(Integer policyType) {
        this.policyType = policyType;
    }

    public Integer getSourceVNID() {
        return sourceVNID;
    }

    public void setSourceVNID(Integer sourceVNID) {
        this.sourceVNID = sourceVNID;
    }

    public Integer getDestinationVNID() {
        return destinationVNID;
    }

    public void setDestinationVNID(Integer destinationVNID) {
        this.destinationVNID = destinationVNID;
    }

    public Integer getTimeToLive() {
        return timeToLive;
    }

    public void setTimeToLive(Integer timeToLive) {
        this.timeToLive = timeToLive;
    }

    public Integer getPolicyAction() {
        return policyAction;
    }

    public void setPolicyAction(Integer policyAction) {
        this.policyAction = policyAction;
    }

    public String getDomainUUID() {
        return domainUUID;
    }

    public void setDomainUUID(String domainUUID) {
        this.domainUUID = domainUUID;
    }

    public Integer getTrafficType() {
        return trafficType;
    }

    public void setTrafficType(Integer trafficType) {
        this.trafficType = trafficType;
    }

    public boolean isTrackedByDCS() {
        return true;
    }

    public String getSBDcsUri() {
        return "/controller/sb/v2/opendove/odmc/domains/" + domainUUID + "/policy/" + uuid;
    }
}
