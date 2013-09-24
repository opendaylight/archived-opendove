/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.opendove.odmc;

import java.util.List;

public interface IfOpenDoveServiceApplianceCRU {
    public boolean dsaIPExists(String ip);

    public OpenDoveServiceAppliance getDoveServiceAppliance(String dsaUUID);
    
    public List<OpenDoveServiceAppliance> getAppliances();
    
    public void addDoveServiceAppliance(String dsaUUID, OpenDoveServiceAppliance openDoveDSA);

	public boolean applianceExists(String saUUID);

}
