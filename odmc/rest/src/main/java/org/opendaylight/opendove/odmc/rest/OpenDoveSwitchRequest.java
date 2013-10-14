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

import org.opendaylight.opendove.odmc.OpenDoveSwitch;

@XmlRootElement
@XmlAccessorType(XmlAccessType.NONE)
public class OpenDoveSwitchRequest {
    // See OpenStack Network API v2.0 Reference for description of
    // annotated attributes

    @XmlElement(name="switch")
    OpenDoveSwitch singletonSwitch;

    @XmlElement(name="switches")
    List<OpenDoveSwitch> bulkSwitches;

    public OpenDoveSwitchRequest() {
    }

    public OpenDoveSwitchRequest(List<OpenDoveSwitch> bulk) {
        bulkSwitches = bulk;
        singletonSwitch = null;
    }

    public OpenDoveSwitchRequest(OpenDoveSwitch single) {
        singletonSwitch = single;
    }

    public OpenDoveSwitch getSingleton() {
        return singletonSwitch;
    }

    public boolean isSingleton() {
        return (singletonSwitch != null);
    }

    public List<OpenDoveSwitch> getBulk() {
        return bulkSwitches;
    }
}
