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

import org.opendaylight.opendove.odmc.OpenDoveNetworkSubnetAssociation;

@XmlRootElement
@XmlAccessorType(XmlAccessType.NONE)
public class OpenDoveNetworkSubnetRequest {
    // See OpenStack Network API v2.0 Reference for description of
    // annotated attributes

    @XmlElement(name="networkSubnetAssociation")
    OpenDoveNetworkSubnetAssociation singletonNetwork;

    @XmlElement(name="networkSubnetAssociations")
    List<OpenDoveNetworkSubnetAssociation> bulkNetworks;

    public OpenDoveNetworkSubnetRequest() {
    }

    public OpenDoveNetworkSubnetRequest(List<OpenDoveNetworkSubnetAssociation> bulk) {
        bulkNetworks = bulk;
        singletonNetwork = null;
    }

    public OpenDoveNetworkSubnetRequest(OpenDoveNetworkSubnetAssociation single) {
        singletonNetwork = single;
    }

    public OpenDoveNetworkSubnetAssociation getSingleton() {
        return singletonNetwork;
    }

    public boolean isSingleton() {
        return (singletonNetwork != null);
    }

    public List<OpenDoveNetworkSubnetAssociation> getBulk() {
        return bulkNetworks;
    }
}
