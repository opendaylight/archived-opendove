/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.opendove.odmc;

import javax.xml.bind.annotation.XmlAccessType;
import javax.xml.bind.annotation.XmlAccessorType;
import javax.xml.bind.annotation.XmlElement;
import javax.xml.bind.annotation.XmlRootElement;

@XmlRootElement
@XmlAccessorType(XmlAccessType.NONE)

public class OpenDoveNeutronControlBlock {
    @XmlElement(name="domain_separation")
    Boolean domainSeparation;

    @XmlElement(name="snat_pool_size")
    Integer snatPoolSize;

    @XmlElement(name="egw_replication_factor")
    Integer egwReplicationFactor;

    public OpenDoveNeutronControlBlock() {
        domainSeparation = false;
        snatPoolSize = new Integer(1);
        egwReplicationFactor = new Integer(1);
    }

    public Boolean getDomainSeparation() {
        return domainSeparation;
    }

    public void setDomainSeparation(Boolean domainSeparation) {
        this.domainSeparation = domainSeparation;
    }

    public Integer getSnatPoolSize() {
        return snatPoolSize;
    }

    public void setSnatPoolSize(Integer snatPoolSize) {
        this.snatPoolSize = snatPoolSize;
    }

    public Integer getEgwReplicationFactor() {
        return egwReplicationFactor;
    }

    public void setEgwReplicationFactor(Integer egwReplicationFactor) {
        this.egwReplicationFactor = egwReplicationFactor;
    }
}
