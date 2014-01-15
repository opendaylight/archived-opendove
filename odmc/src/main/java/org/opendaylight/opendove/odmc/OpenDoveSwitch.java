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

@XmlAccessorType(XmlAccessType.NONE)
@XmlRootElement
public class OpenDoveSwitch extends OpenDoveObject {
    @XmlElement(name="id")
    String uuid;

    @XmlElement(name="name")
    String name;

    @XmlElement(name="tunnelip")
    String tunnelIP;

    @XmlElement(name="managementip")
    String mgmtIP;

    @XmlElement(name="timestamp")
    String timestamp;

    Boolean reRegister;

    public OpenDoveSwitch() {
    }

    public void setUUID(String uuid) {
        this.uuid = uuid;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public String getTunnelIP() {
        return tunnelIP;
    }

    public void setTunnelIP(String tunnelIP) {
        this.tunnelIP = tunnelIP;
    }

    public String getMgmtIP() {
        return mgmtIP;
    }

    public void setMgmtIP(String mgmtIP) {
        this.mgmtIP = mgmtIP;
    }

    public String getTimestamp() {
        return timestamp;
    }

    public void setTimestamp(String timestamp) {
        this.timestamp = timestamp;
    }

    @Override
    public String getUUID() {
        return uuid;
    }

    public Boolean getReRegister() {
        return reRegister;
    }

    public void setReRegister(Boolean reRegister) {
        this.reRegister = reRegister;
    }

    public boolean overwrite(OpenDoveSwitch delta) {
        boolean answer = false;  // whether we need to update the change version number or not
        if (delta.getName() != null) {
            if (!getName().equalsIgnoreCase(delta.getName())) {
                setName(delta.getName());
                answer = true;
            }
        }
        if (delta.getTunnelIP() != null) {
            if (!getTunnelIP().equalsIgnoreCase(delta.getTunnelIP())) {
                setTunnelIP(delta.getTunnelIP());
                answer = true;
            }
        }
        if (delta.getMgmtIP() != null) {
            if (!getMgmtIP().equalsIgnoreCase(delta.getMgmtIP())) {
                setMgmtIP(delta.getMgmtIP());
                answer = true;
            }
        }
        if (delta.getTimestamp() != null) {
            if (!getTimestamp().equalsIgnoreCase(delta.getTimestamp())) {
                setTimestamp(delta.getTimestamp());
            }
        }
        if (delta.getReRegister() != null) {
            if (getReRegister() != delta.getReRegister()) {
                setReRegister(delta.getReRegister());
            }
        }
        return answer;
    }
}
