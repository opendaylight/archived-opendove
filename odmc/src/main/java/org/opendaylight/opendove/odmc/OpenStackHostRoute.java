package org.opendaylight.opendove.odmc;

import javax.xml.bind.annotation.XmlAccessType;
import javax.xml.bind.annotation.XmlAccessorType;
import javax.xml.bind.annotation.XmlElement;
import javax.xml.bind.annotation.XmlRootElement;

@XmlRootElement
@XmlAccessorType(XmlAccessType.NONE)
public class OpenStackHostRoute {
    // See OpenStack Network API v2.0 Reference for description of
    // annotated attributes

    @XmlElement(name="destination")
    String destination;

    @XmlElement(name="nexthop")
    String nextHop;

    public OpenStackHostRoute() { }
}
