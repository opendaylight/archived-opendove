/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.opendove.odmc.rest;

import java.util.List;

import javax.xml.bind.annotation.XmlAccessType;
import javax.xml.bind.annotation.XmlAccessorType;
import javax.xml.bind.annotation.XmlElement;
import javax.xml.bind.annotation.XmlRootElement;

import org.opendaylight.opendove.odmc.OpenDoveSubnet;

@XmlRootElement
@XmlAccessorType(XmlAccessType.NONE)
public class OpenDoveSubnetRequest {
    // See OpenStack Subnet API v2.0 Reference for description of
    // annotated attributes

    @XmlElement(name="subnet")
    OpenDoveSubnet singletonSubnet;

    @XmlElement(name="subnets")
    List<OpenDoveSubnet> bulkSubnets;

    public OpenDoveSubnetRequest() {
    }

    public OpenDoveSubnetRequest(List<OpenDoveSubnet> bulk) {
        bulkSubnets = bulk;
        singletonSubnet = null;
    }

    public OpenDoveSubnetRequest(OpenDoveSubnet single) {
        singletonSubnet = single;
    }

    public OpenDoveSubnet getSingleton() {
        return singletonSubnet;
    }

    public boolean isSingleton() {
        return (singletonSubnet != null);
    }

    public List<OpenDoveSubnet> getBulk() {
        return bulkSubnets;
    }
}
