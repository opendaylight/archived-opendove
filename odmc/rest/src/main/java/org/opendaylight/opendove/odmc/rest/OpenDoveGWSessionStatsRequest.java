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

import org.opendaylight.opendove.odmc.OpenDoveGWSession;

@XmlRootElement
@XmlAccessorType(XmlAccessType.NONE)
public class OpenDoveGWSessionStatsRequest {
    // See OpenStack Session API v2.0 Reference for description of
    // annotated attributes

    @XmlElement(name="session")
    OpenDoveGWSession singletonSession;

    @XmlElement(name="sessions")
    List<OpenDoveGWSession> bulkSessions;

    public OpenDoveGWSessionStatsRequest() {
    }

    public OpenDoveGWSessionStatsRequest(List<OpenDoveGWSession> bulk) {
        bulkSessions = bulk;
        singletonSession = null;
    }

    public OpenDoveGWSessionStatsRequest(OpenDoveGWSession single) {
        singletonSession = single;
    }

    public OpenDoveGWSession getSingleton() {
        return singletonSession;
    }

    public boolean isSingleton() {
        return (singletonSession != null);
    }

    public List<OpenDoveGWSession> getBulk() {
        return bulkSessions;
    }
}
