/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.opendove.odmc.southbound;

import java.util.List;

import javax.xml.bind.annotation.XmlAccessType;
import javax.xml.bind.annotation.XmlAccessorType;
import javax.xml.bind.annotation.XmlElement;
import javax.xml.bind.annotation.XmlRootElement;

import org.opendaylight.opendove.odmc.OpenDoveDomain;
import org.opendaylight.opendove.odmc.OpenStackNetworks;

@XmlRootElement
@XmlAccessorType(XmlAccessType.NONE)
public class OpenDoveDomainRequest {
    // See OpenStack Network API v2.0 Reference for description of
    // annotated attributes

    @XmlElement(name="domain")
    OpenDoveDomain singletonDomain;

    @XmlElement(name="domains")
    List<OpenDoveDomain> bulkDomains;

    OpenDoveDomainRequest() {
    }

    OpenDoveDomainRequest(List<OpenDoveDomain> bulk) {
        bulkDomains = bulk;
        singletonDomain = null;
    }

    OpenDoveDomainRequest(OpenDoveDomain single) {
        singletonDomain = single;
    }

    public OpenDoveDomain getSingleton() {
        return singletonDomain;
    }

    public boolean isSingleton() {
        return (singletonDomain != null);
    }

    public List<OpenDoveDomain> getBulk() {
        return bulkDomains;
    }
}
