/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.opendove.odmc;

import java.util.List;

public interface IfSBDoveVGWVNIDMappingCRUD {
    public boolean vgwVNIDMappingExists(String mappingUUID);

    public OpenDoveVGWVNIDMapping getVgwVNIDMapping(String mappingUUID);

    public void addVgwVNIDMapping(String mappingUUID, OpenDoveVGWVNIDMapping mapping);

    public List<OpenDoveVGWVNIDMapping> getVgwVNIDMappings();

    public void removeVgwVNIDMapping(String mappingUUID);
}
