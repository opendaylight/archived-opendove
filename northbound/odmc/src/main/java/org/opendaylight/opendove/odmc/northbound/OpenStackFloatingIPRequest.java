/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.opendove.odmc.northbound;

import java.util.List;

import javax.xml.bind.annotation.XmlAccessType;
import javax.xml.bind.annotation.XmlAccessorType;
import javax.xml.bind.annotation.XmlElement;
import javax.xml.bind.annotation.XmlRootElement;

import org.opendaylight.opendove.odmc.OpenStackFloatingIPs;

@XmlRootElement
@XmlAccessorType(XmlAccessType.NONE)
public class OpenStackFloatingIPRequest {
    // See OpenStack Network API v2.0 Reference for description of
    // annotated attributes

    @XmlElement(name="floatingip")
    OpenStackFloatingIPs singletonFloatingIP;

    @XmlElement(name="floatingips")
    List<OpenStackFloatingIPs> bulkRequest;

    OpenStackFloatingIPRequest() {
    }

    OpenStackFloatingIPRequest(List<OpenStackFloatingIPs> bulk) {
        bulkRequest = bulk;
        singletonFloatingIP = null;
    }

    OpenStackFloatingIPRequest(OpenStackFloatingIPs singleton) {
        bulkRequest = null;
        singletonFloatingIP = singleton;
    }

    public OpenStackFloatingIPs getSingleton() {
        return singletonFloatingIP;
    }

    public boolean isSingleton() {
        return (singletonFloatingIP != null);
    }
}
