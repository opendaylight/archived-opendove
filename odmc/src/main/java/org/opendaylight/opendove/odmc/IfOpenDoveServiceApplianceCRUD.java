/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.opendove.odmc;

import java.util.List;

public interface IfOpenDoveServiceApplianceCRUD {
    public boolean dsaIPExists(String ip);

    public OpenDoveServiceAppliance getDoveServiceAppliance(String dsaUUID);

    public List<OpenDoveServiceAppliance> getAppliances();
    public List<OpenDoveServiceAppliance> getRoleAssignedDcsAppliances();

    public void addDoveServiceAppliance(String dsaUUID, OpenDoveServiceAppliance openDoveDSA);

    public boolean applianceExists(String saUUID);
    public boolean dsaIPConflict(String ip, String uuid);
    public boolean updateDoveServiceAppliance(String dsaUUID, OpenDoveServiceAppliance input);

    public OpenDoveServiceAppliance getDCSSeed();

    public void deleteServiceAppliance(String saUUID);
}
