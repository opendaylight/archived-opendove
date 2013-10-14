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

import org.opendaylight.opendove.odmc.OpenDoveNetwork;

@XmlRootElement
@XmlAccessorType(XmlAccessType.NONE)
public class OpenDoveNetworkRequest {
    // See OpenStack Network API v2.0 Reference for description of
    // annotated attributes

    @XmlElement(name="network")
    OpenDoveNetwork singletonDomain;

    @XmlElement(name="networks")
    List<OpenDoveNetwork> bulkDomains;

    public OpenDoveNetworkRequest() {
    }

    public OpenDoveNetworkRequest(List<OpenDoveNetwork> bulk) {
        bulkDomains = bulk;
        singletonDomain = null;
    }

    public OpenDoveNetworkRequest(OpenDoveNetwork single) {
        singletonDomain = single;
    }

    public OpenDoveNetwork getSingleton() {
        return singletonDomain;
    }

    public boolean isSingleton() {
        return (singletonDomain != null);
    }

    public List<OpenDoveNetwork> getBulk() {
        return bulkDomains;
    }
}
