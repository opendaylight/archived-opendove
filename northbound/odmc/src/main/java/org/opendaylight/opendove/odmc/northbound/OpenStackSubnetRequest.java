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

import org.opendaylight.opendove.odmc.OpenStackSubnets;

@XmlRootElement
@XmlAccessorType(XmlAccessType.NONE)

public class OpenStackSubnetRequest {
    // See OpenStack Network API v2.0 Reference for description of
    // annotated attributes

    @XmlElement(name="subnet")
    OpenStackSubnets singletonSubnet;

    @XmlElement(name="subnets")
    List<OpenStackSubnets> bulkRequest;

    OpenStackSubnetRequest() {
    }

    OpenStackSubnetRequest(List<OpenStackSubnets> bulk) {
        bulkRequest = bulk;
        singletonSubnet = null;
    }

    OpenStackSubnetRequest(OpenStackSubnets subnet) {
        singletonSubnet = subnet;
    }

    public OpenStackSubnets getSingleton() {
        return singletonSubnet;
    }

    public List<OpenStackSubnets> getBulk() {
        return bulkRequest;
    }

    public boolean isSingleton() {
        return (singletonSubnet != null);
    }
}
