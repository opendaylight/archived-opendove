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

    @XmlElement(name="mgmtip")
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
}
