/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.opendove.odmc;

import java.util.List;

import javax.xml.bind.annotation.XmlAccessType;
import javax.xml.bind.annotation.XmlAccessorType;
import javax.xml.bind.annotation.XmlElement;
import javax.xml.bind.annotation.XmlRootElement;

@XmlRootElement
@XmlAccessorType(XmlAccessType.NONE)

public class OpenDoveDomain extends OpenDoveObject implements IfOpenDSATrackedObject {
    @XmlElement (name="id")
    String uuid;

    @XmlElement (name="name")
    String name;

    @XmlElement (name="is_tombstone")
    Boolean tombstoneFlag;

    @XmlElement (name="replication_factor")
    Integer replicationFactor;

    @XmlElement (name="change_version")
    Integer lastChangeVersion;

    @XmlElement (name="create_version")
    Integer createVersion;

    List<OpenDoveNetwork> scopedNetworks;

    public OpenDoveDomain() {
    }

    public String getUuid() {
        return uuid;
    }

    public void setUuid(String uuid) {
        this.uuid = uuid;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public Boolean getTombstoneFlag() {
        return tombstoneFlag;
    }

    public void setTombstoneFlag(Boolean tombstoneFlag) {
        this.tombstoneFlag = tombstoneFlag;
    }

    public Integer getReplicationFactor() {
        return replicationFactor;
    }

    public void setReplicationFactor(Integer replicationFactor) {
        this.replicationFactor = replicationFactor;
    }

    public Integer getLastChangeVersion() {
        return lastChangeVersion;
    }

    public void setLastChangeVersion(Integer lastChangeVersion) {
        this.lastChangeVersion = lastChangeVersion;
    }

    public Integer getCreateVersion() {
        return createVersion;
    }

    public void setCreateVersion(Integer createVersion) {
        this.createVersion = createVersion;
    }

    public boolean isTrackedByDSA() {
        return true;
    }
}
