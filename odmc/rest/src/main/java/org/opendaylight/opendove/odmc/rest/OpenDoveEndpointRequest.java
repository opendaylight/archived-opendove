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

import org.opendaylight.opendove.odmc.OpenDoveEndpoint;

@XmlRootElement
@XmlAccessorType(XmlAccessType.NONE)
public class OpenDoveEndpointRequest {
    // See OpenStack Network API v2.0 Reference for description of
    // annotated attributes

    @XmlElement(name="endpoint")
    OpenDoveEndpoint singletonEndpoint;

    @XmlElement(name="endpoints")
    List<OpenDoveEndpoint> bulkEndpoints;

    public OpenDoveEndpointRequest() {
    }

    public OpenDoveEndpointRequest(List<OpenDoveEndpoint> bulk) {
        bulkEndpoints = bulk;
        singletonEndpoint = null;
    }

    public OpenDoveEndpointRequest(OpenDoveEndpoint single) {
        singletonEndpoint = single;
    }

    public OpenDoveEndpoint getSingleton() {
        return singletonEndpoint;
    }

    public boolean isSingleton() {
        return (singletonEndpoint != null);
    }

    public List<OpenDoveEndpoint> getBulk() {
        return bulkEndpoints;
    }
}
