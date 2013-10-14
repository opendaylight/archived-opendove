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

import org.opendaylight.opendove.odmc.OpenDovePolicy;

@XmlRootElement
@XmlAccessorType(XmlAccessType.NONE)
public class OpenDovePolicyRequest {

    @XmlElement(name="policy")
    OpenDovePolicy singletonPolicy;

    @XmlElement(name="policies")
    List<OpenDovePolicy> bulkPolicies;

    public OpenDovePolicyRequest() {
    }

    public OpenDovePolicyRequest(List<OpenDovePolicy> bulk) {
        bulkPolicies = bulk;
        singletonPolicy = null;
    }

    public OpenDovePolicyRequest(OpenDovePolicy single) {
        singletonPolicy = single;
    }

    public OpenDovePolicy getSingleton() {
        return singletonPolicy;
    }

    public boolean isSingleton() {
        return (singletonPolicy != null);
    }

    public List<OpenDovePolicy> getBulk() {
        return bulkPolicies;
    }
}
