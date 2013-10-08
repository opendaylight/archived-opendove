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
public class OpenDoveEGWSNATPool extends OpenDoveObject implements IfOpenDGWTrackedObject {

    @XmlElement(name="id")
    String uuid;

    @XmlElement(name="net_id")
    Integer vnid;

    @XmlElement(name="min_ip")
    String minIP;

    @XmlElement(name="max_ip")
    String maxIP;

    @XmlElement(name="min_port")
    Integer minPort;

    @XmlElement(name="max_port")
    Integer maxPort;

    @XmlElement(name="gatewayUUID")
    String gatewayUUID;

    @XmlElement(name="extmcastvnid")
    Integer externalMulticastVNID;

    @XmlElement(name="domain_id")
    String domainUUID;

    @XmlElement(name="ext_ip")
    String externalIP;

    public OpenDoveEGWSNATPool() { 
    	uuid = java.util.UUID.randomUUID().toString();
    }

    public OpenDoveEGWSNATPool(String uuid2, String ip_low, String ip_high,
			String domain_uuid, Integer vnid2, int i, int j) {
    	uuid = java.util.UUID.randomUUID().toString();
    	gatewayUUID = uuid2;
    	minIP = ip_low;
    	maxIP = ip_high;
    	domainUUID = domain_uuid;
    	vnid = vnid2;
    	minPort = i;
    	maxPort = j;
	}

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

    public String getMinIP() {
        return minIP;
    }

    public void setMinIP(String minIP) {
        this.minIP = minIP;
    }

    public String getMaxIP() {
        return maxIP;
    }

    public void setMaxIP(String maxIP) {
        this.maxIP = maxIP;
    }

    public Integer getMinPort() {
        return minPort;
    }

    public void setMinPort(Integer minPort) {
        this.minPort = minPort;
    }

    public Integer getMaxPort() {
        return maxPort;
    }

    public void setMaxPort(Integer maxPort) {
        this.maxPort = maxPort;
    }

    public String getGatewayUUID() {
        return gatewayUUID;
    }

    public void setGatewayUUID(String gatewayUUID) {
        this.gatewayUUID = gatewayUUID;
    }

    public Integer getExternalMulticastVNID() {
        return externalMulticastVNID;
    }

    public void setExternalMulticastVNID(Integer externalMulticastVNID) {
        this.externalMulticastVNID = externalMulticastVNID;
    }

    public String getDomainUUID() {
        return domainUUID;
    }

    public void setDomainUUID(String domainUUID) {
        this.domainUUID = domainUUID;
    }

    public String getExternalIP() {
        return externalIP;
    }

    public void setExternalIP(String externalIP) {
        this.externalIP = externalIP;
    }

    public boolean isTrackedByDGW() {
        return true;
    }

    public String getSBDgwUri() {
        return "/controller/sb/v2/opendove/odmc/ext-gws/"+uuid;
    }
}
