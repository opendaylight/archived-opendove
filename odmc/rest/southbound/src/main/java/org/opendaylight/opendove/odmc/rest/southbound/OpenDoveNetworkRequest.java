/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.opendove.odmc.rest.southbound;

import java.util.List;

import javax.xml.bind.annotation.XmlAccessType;
import javax.xml.bind.annotation.XmlAccessorType;
import javax.xml.bind.annotation.XmlElement;
import javax.xml.bind.annotation.XmlRootElement;

import org.opendaylight.opendove.odmc.OpenDoveNetwork;

@XmlRootElement
@XmlAccessorType(XmlAccessType.NONE)
public class OpenDoveNetworkRequest {
    // See OpenStack Network API v2.0 Reference for description of
    // annotated attributes

    @XmlElement(name="network")
    OpenDoveNetwork singletonNetwork;

    @XmlElement(name="networks")
    List<OpenDoveNetwork> bulkNetworks;

    OpenDoveNetworkRequest() {
    }

    OpenDoveNetworkRequest(List<OpenDoveNetwork> bulk) {
        bulkNetworks = bulk;
        singletonNetwork = null;
    }

    OpenDoveNetworkRequest(OpenDoveNetwork single) {
        singletonNetwork = single;
    }

    public OpenDoveNetwork getSingleton() {
        return singletonNetwork;
    }

    public boolean isSingleton() {
        return (singletonNetwork != null);
    }

    public List<OpenDoveNetwork> getBulk() {
        return bulkNetworks;
    }
}
