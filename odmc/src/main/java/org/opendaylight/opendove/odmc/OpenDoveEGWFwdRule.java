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
public class OpenDoveEGWFwdRule extends OpenDoveObject implements IfOpenDGWTrackedObject {

    @XmlElement(name="id")
    String uuid;

    @XmlElement(name="net_id")
    Integer vnid;

    @XmlElement(name="ip")
    String externalIP;

    @XmlElement(name="port")
    Integer externalPort;

    @XmlElement(name="real_ip")
    String internalIP;

    @XmlElement(name="real_port")
    Integer internalPort;

    @XmlElement(name="gatewayUUID")
    String gatewayUUID;

    @XmlElement(name="pip_min")
    String minProxyIP;

    @XmlElement(name="pip_max")
    String maxProxyIP;

    public OpenDoveEGWFwdRule() { }

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


    public String getGatewayUUID() {
        return gatewayUUID;
    }

    public void setGatewayUUID(String gatewayUUID) {
        this.gatewayUUID = gatewayUUID;
    }


    public String getExternalIP() {
        return externalIP;
    }

    public void setExternalIP(String externalIP) {
        this.externalIP = externalIP;
    }

    public Integer getExternalPort() {
        return externalPort;
    }

    public void setExternalPort(Integer externalPort) {
        this.externalPort = externalPort;
    }

    public String getInternalIP() {
        return internalIP;
    }

    public void setInternalIP(String internalIP) {
        this.internalIP = internalIP;
    }

    public Integer getInternalPort() {
        return internalPort;
    }

    public void setInternalPort(Integer internalPort) {
        this.internalPort = internalPort;
    }

    public String getMinProxyIP() {
        return minProxyIP;
    }

    public void setMinProxyIP(String minProxyIP) {
        this.minProxyIP = minProxyIP;
    }

    public String getMaxProxyIP() {
        return maxProxyIP;
    }

    public void setMaxProxyIP(String maxProxyIP) {
        this.maxProxyIP = maxProxyIP;
    }

    public boolean isTrackedByDGW() {
        return true;
    }

    public String getSBDgwUri() {
        return "/controller/sb/v2/opendove/odmc/egw-fwd-rules/"+uuid;
    }
}
