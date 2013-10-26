/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.opendove.odmc;

import java.util.ArrayList;
import java.util.List;

import javax.xml.bind.annotation.XmlAccessType;
import javax.xml.bind.annotation.XmlAccessorType;
import javax.xml.bind.annotation.XmlElement;
import javax.xml.bind.annotation.XmlRootElement;


@XmlAccessorType(XmlAccessType.NONE)
@XmlRootElement
public class OpenDoveDomain extends OpenDoveObject implements IfOpenDCSTrackedObject {
    @XmlElement (name="id")
    String uuid;

    @XmlElement (name="name")
    String name;

    @XmlElement (name="replication_factor")
    Integer replicationFactor;

    List<OpenDoveNetwork> scopedNetworks;
    List<OpenDoveSubnet> scopedSubnets;

    Integer domain_id;

    public OpenDoveDomain() {
        this.scopedNetworks = new ArrayList<OpenDoveNetwork>();
        this.scopedSubnets = new ArrayList<OpenDoveSubnet>();
    }

    public OpenDoveDomain(String name) {
        this.uuid = java.util.UUID.randomUUID().toString();
        this.domain_id = createVersion;
        this.name = name;
        this.tombstoneFlag = false;
        this.replicationFactor = 2;
        this.scopedNetworks = new ArrayList<OpenDoveNetwork>();
        this.scopedSubnets = new ArrayList<OpenDoveSubnet>();
    }

    @Override
    public String getUUID() {
        return uuid;
    }

    public void setUuid(String uuid) {
        this.uuid = uuid;
    }

    public Integer getDomainId() {
        return domain_id;
    }

    public void setDomainId(Integer domain_id) {
        this.domain_id = domain_id;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public Integer getReplicationFactor() {
        return replicationFactor;
    }

    public void setReplicationFactor(Integer replicationFactor) {
        this.replicationFactor = replicationFactor;
    }

    public boolean isTrackedByDCS() {
        return true;
    }

    public void addNetwork(OpenDoveNetwork network) {
        scopedNetworks.add(network);
    }

    public void addSubnet(OpenDoveSubnet subnet) {
        scopedSubnets.add(subnet);
    }

    public void removeSubnet(OpenDoveSubnet subnet) {
        scopedSubnets.remove(subnet);
    }

    // changed from uuid to createVersion to make oDCS life easier.
    public String getSBDcsUri() {
        //return "/controller/sb/v2/opendove/odmc/domains/bynumber/" + createVersion;
        this.domain_id = createVersion;
        return "/controller/sb/v2/opendove/odmc/domains/bynumber/" + domain_id;
    }

}
