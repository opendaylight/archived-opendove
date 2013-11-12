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

import org.opendaylight.opendove.odmc.OpenDoveGwIpv4;

@XmlRootElement
@XmlAccessorType(XmlAccessType.NONE)
public class OpenDoveGwIpv4Request {

    @XmlElement(name="gw_ipv4_assignment")
    OpenDoveGwIpv4 singletonPool;

    @XmlElement(name="gw_ipv4_assignments")
    List<OpenDoveGwIpv4> bulkPools;

    public OpenDoveGwIpv4Request() {
    }

    public OpenDoveGwIpv4Request(List<OpenDoveGwIpv4> bulk) {
        bulkPools = bulk;
        singletonPool = null;
    }

    public OpenDoveGwIpv4Request(OpenDoveGwIpv4 single) {
        singletonPool = single;
    }

    public OpenDoveGwIpv4 getSingleton() {
        return singletonPool;
    }

    public boolean isSingleton() {
        return (singletonPool != null);
    }

    public List<OpenDoveGwIpv4> getBulk() {
        return bulkPools;
    }
}
