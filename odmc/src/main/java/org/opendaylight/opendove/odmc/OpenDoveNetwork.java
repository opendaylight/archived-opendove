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
public class OpenDoveNetwork extends OpenDoveObject implements
        IfOpenDSATrackedObject {

    @XmlElement(name="id")
    String uuid;
    
    @XmlElement(name="network_id")
    Integer vnid;
    
    @XmlElement(name="name")
    String name;
    
    @XmlElement(name="domain_id")
    String domain_uuid;
    
    @XmlElement(name="is_tombstone")
    Boolean tombstoneFlag;
    
    @XmlElement (name="type")
    Integer networkType;
    
    @XmlElement (name="change_version")
    Integer lastChangeVersion;

    @XmlElement (name="create_version")
    Integer createVersion;

    public String getUuid() {
        return uuid;
    }

    public void setUuid(String uuid) {
        this.uuid = uuid;
    }

    public Integer getVnid() {
        return vnid;
    }

    public void setVnid(Integer vnid) {
        this.vnid = vnid;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public String getDomain_uuid() {
        return domain_uuid;
    }

    public void setDomain_uuid(String domain_uuid) {
        this.domain_uuid = domain_uuid;
    }

    public Boolean getTombstoneFlag() {
        return tombstoneFlag;
    }

    public void setTombstoneFlag(Boolean tombstoneFlag) {
        this.tombstoneFlag = tombstoneFlag;
    }

    public Integer getNetworkType() {
        return networkType;
    }

    public void setNetworkType(Integer networkType) {
        this.networkType = networkType;
    }

    public boolean isTrackedByDSA() {
        return true;
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

}
