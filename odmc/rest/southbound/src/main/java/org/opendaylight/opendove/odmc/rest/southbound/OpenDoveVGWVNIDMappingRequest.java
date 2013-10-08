/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.opendove.odmc.rest.southbound;

import java.util.List;

import javax.xml.bind.annotation.XmlAccessType;
import javax.xml.bind.annotation.XmlAccessorType;
import javax.xml.bind.annotation.XmlElement;
import javax.xml.bind.annotation.XmlRootElement;

import org.opendaylight.opendove.odmc.OpenDoveVGWVNIDMapping;

@XmlRootElement
@XmlAccessorType(XmlAccessType.NONE)
public class OpenDoveVGWVNIDMappingRequest {

    @XmlElement(name="vnid_mapping_rule")
    OpenDoveVGWVNIDMapping singletonMapping;

    @XmlElement(name="vnid_mapping_rules")
    List<OpenDoveVGWVNIDMapping> bulkMappings;

    OpenDoveVGWVNIDMappingRequest() {
    }

    OpenDoveVGWVNIDMappingRequest(List<OpenDoveVGWVNIDMapping> bulk) {
        bulkMappings = bulk;
        singletonMapping = null;
    }

    OpenDoveVGWVNIDMappingRequest(OpenDoveVGWVNIDMapping single) {
        singletonMapping = single;
    }

    public OpenDoveVGWVNIDMapping getSingleton() {
        return singletonMapping;
    }

    public boolean isSingleton() {
        return (singletonMapping != null);
    }

    public List<OpenDoveVGWVNIDMapping> getBulk() {
        return bulkMappings;
    }
}
