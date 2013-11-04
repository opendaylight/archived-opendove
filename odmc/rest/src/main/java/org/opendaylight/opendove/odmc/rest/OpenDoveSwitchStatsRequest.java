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

import org.opendaylight.opendove.odmc.OpenDoveNVP;

@XmlRootElement
@XmlAccessorType(XmlAccessType.NONE)
public class OpenDoveSwitchStatsRequest {
    // See OpenStack Network API v2.0 Reference for description of
    // annotated attributes

    @XmlElement(name="stat")
    OpenDoveNVP stat;

    @XmlElement(name="stats")
    List<OpenDoveNVP> bulkStats;

    public OpenDoveSwitchStatsRequest() {
    }

    public OpenDoveSwitchStatsRequest(List<OpenDoveNVP> bulk) {
        bulkStats = bulk;
        stat = null;
    }

    public OpenDoveSwitchStatsRequest(OpenDoveNVP single) {
        stat = single;
    }

    public OpenDoveNVP getSingleton() {
        return stat;
    }

    public boolean isSingleton() {
        return (stat != null);
    }

    public List<OpenDoveNVP> getBulk() {
        return bulkStats;
    }
}
