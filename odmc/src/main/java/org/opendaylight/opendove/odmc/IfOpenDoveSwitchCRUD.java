/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.opendove.odmc;

import java.util.List;

public interface IfOpenDoveSwitchCRUD {
    public boolean switchExists(String switchUUID);

    public OpenDoveSwitch getSwitch(String switchUUID);

    public void addSwitch(String switchUUID, OpenDoveSwitch openDoveSwitch);

    public List<OpenDoveSwitch> getSwitches();

    public Object getStats(String queryIPAddr, String queryVNID,
            String queryMAC);

    public void updateSwitch(String uuid, OpenDoveSwitch target);

    public Integer deleteStats(String queryIPAddr, String queryVNID,
            String queryMAC);

    public void removeSwitch(String uuid);
}
