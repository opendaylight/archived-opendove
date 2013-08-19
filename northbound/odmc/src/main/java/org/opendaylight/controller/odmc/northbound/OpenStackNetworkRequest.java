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

import org.opendaylight.controller.odmc.OpenStackNetworks;

@XmlRootElement
@XmlAccessorType(XmlAccessType.NONE)
public class OpenStackNetworkRequest {
    // See OpenStack Network API v2.0 Reference for description of
    // annotated attributes

    @XmlElement(name="network")
    OpenStackNetworks singletonNetwork;

    @XmlElement(name="networks")
    List<OpenStackNetworks> bulkRequest;

    OpenStackNetworkRequest() {
    }

    OpenStackNetworkRequest(List<OpenStackNetworks> bulk) {
        bulkRequest = bulk;
        singletonNetwork = null;
    }

    OpenStackNetworkRequest(OpenStackNetworks net) {
        singletonNetwork = net;
    }

    public OpenStackNetworks getSingleton() {
        return singletonNetwork;
    }

    public boolean isSingleton() {
        return (singletonNetwork != null);
    }

    public List<OpenStackNetworks> getBulk() {
        return bulkRequest;
    }
}
