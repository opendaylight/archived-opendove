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

import org.opendaylight.opendove.odmc.OpenDoveEGWSNATPool;

@XmlRootElement
@XmlAccessorType(XmlAccessType.NONE)
public class OpenDoveEGWSNATPoolRequest {

    @XmlElement(name="egw_snat_pool")
    OpenDoveEGWSNATPool singletonPool;

    @XmlElement(name="egw_snat_pools")
    List<OpenDoveEGWSNATPool> bulkPools;

    public OpenDoveEGWSNATPoolRequest() {
    }

    public OpenDoveEGWSNATPoolRequest(List<OpenDoveEGWSNATPool> bulk) {
        bulkPools = bulk;
        singletonPool = null;
    }

    public OpenDoveEGWSNATPoolRequest(OpenDoveEGWSNATPool single) {
        singletonPool = single;
    }

    public OpenDoveEGWSNATPool getSingleton() {
        return singletonPool;
    }

    public boolean isSingleton() {
        return (singletonPool != null);
    }

    public List<OpenDoveEGWSNATPool> getBulk() {
        return bulkPools;
    }
}
