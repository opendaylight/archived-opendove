/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.opendove.odmc;

import javax.xml.bind.annotation.XmlElement;
import javax.xml.bind.annotation.XmlTransient;

/*
 * This class provides a container for the ChangeVersion object to track
 * OpenDove objects and not just POJOs.
 */

@XmlTransient
public abstract class OpenDoveObject {
    @XmlElement (name="is_tombstone")
    Boolean tombstoneFlag;

    @XmlElement (name="change_version")
    Integer lastChangeVersion;

    @XmlElement (name="create_version")
    Integer createVersion;

    public abstract String getUUID();

    public Boolean getTombstoneFlag() {
        return tombstoneFlag;
    }

    public void setTombstoneFlag(Boolean tombstoneFlag) {
        this.tombstoneFlag = tombstoneFlag;
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
