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
public class OpenDoveEndpoint {
    @XmlElement (name="Host IP")
    String hostIP;

    @XmlElement (name="mac")
    String mac;

    @XmlElement (name="virtual IPs")
    String virtIP;

    @XmlElement (name="physical IPs")
    String physIP;

    public OpenDoveEndpoint() { }

	public String getHostIP() {
		return hostIP;
	}

	public void setHostIP(String hostIP) {
		this.hostIP = hostIP;
	}

	public String getMac() {
		return mac;
	}

	public void setMac(String mac) {
		this.mac = mac;
	}

	public String getVirtIP() {
		return virtIP;
	}

	public void setVirtIP(String virtIP) {
		this.virtIP = virtIP;
	}

	public String getPhysIP() {
		return physIP;
	}

	public void setPhysIP(String physIP) {
		this.physIP = physIP;
	}
}
