/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.controller.odmc.northbound;

import java.util.List;

import javax.xml.bind.annotation.XmlAccessType;
import javax.xml.bind.annotation.XmlAccessorType;
import javax.xml.bind.annotation.XmlElement;
import javax.xml.bind.annotation.XmlRootElement;

import org.opendaylight.controller.odmc.OpenStackPorts;

@XmlRootElement
@XmlAccessorType(XmlAccessType.NONE)
public class OpenStackPortRequest {
    // See OpenStack Network API v2.0 Reference for description of
    // annotated attributes

    @XmlElement(name="port")
    OpenStackPorts singletonPort;

    @XmlElement(name="ports")
    List<OpenStackPorts> bulkRequest;

    OpenStackPortRequest() {
    }

    OpenStackPortRequest(List<OpenStackPorts> bulk) {
        bulkRequest = bulk;
        singletonPort = null;
    }

    OpenStackPortRequest(OpenStackPorts port) {
        singletonPort = port;
    }

    public OpenStackPorts getSingleton() {
        return singletonPort;
    }

    public boolean isSingleton() {
        return (singletonPort != null);
    }

    public List<OpenStackPorts> getBulk() {
        return bulkRequest;
    }
}
