/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.opendove.odmc.rest.northbound;

import java.util.List;

import javax.xml.bind.annotation.XmlAccessType;
import javax.xml.bind.annotation.XmlAccessorType;
import javax.xml.bind.annotation.XmlElement;
import javax.xml.bind.annotation.XmlRootElement;

import org.opendaylight.controller.networkconfig.neutron.NeutronNetwork;
import org.opendaylight.opendove.odmc.OpenDoveServiceAppliance;

@XmlRootElement
@XmlAccessorType(XmlAccessType.NONE)
public class OpenDoveServiceApplianceRequest {
    // See OpenStack Network API v2.0 Reference for description of
    // annotated attributes

    @XmlElement(name="service_appliance")
    OpenDoveServiceAppliance singletonAppliance;

    @XmlElement(name="service_appliances")
    List<OpenDoveServiceAppliance> bulkAppliances;

    OpenDoveServiceApplianceRequest() {
    }

    OpenDoveServiceApplianceRequest(List<OpenDoveServiceAppliance> bulk) {
        bulkAppliances = bulk;
        singletonAppliance = null;
    }

    OpenDoveServiceApplianceRequest(OpenDoveServiceAppliance single) {
        singletonAppliance = single;
    }

    public OpenDoveServiceAppliance getSingleton() {
        return singletonAppliance;
    }

    public boolean isSingleton() {
        return (singletonAppliance != null);
    }

    public List<OpenDoveServiceAppliance> getBulk() {
        return bulkAppliances;
    }
}
