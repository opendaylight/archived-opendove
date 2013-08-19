package org.opendaylight.controller.odmc;

import javax.xml.bind.annotation.XmlAccessType;
import javax.xml.bind.annotation.XmlAccessorType;
import javax.xml.bind.annotation.XmlElement;
import javax.xml.bind.annotation.XmlRootElement;

@XmlRootElement
@XmlAccessorType(XmlAccessType.NONE)
public class OpenStackIPs {
    // See OpenStack Network API v2.0 Reference for description of
    // annotated attributes

    @XmlElement(name="ip_address")
    String ipAddress;

    @XmlElement(name="subnet_id")
    String subnetUUID;

    public OpenStackIPs() { }

    public OpenStackIPs(String uuid) {
        this.subnetUUID = uuid;
    }

    public String getIpAddress() {
        return ipAddress;
    }

    public void setIpAddress(String ipAddress) {
        this.ipAddress = ipAddress;
    }

    public String getSubnetUUID() {
        return subnetUUID;
    }

    public void setSubnetUUID(String subnetUUID) {
        this.subnetUUID = subnetUUID;
    }
}
