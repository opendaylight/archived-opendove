/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.opendove.odmc;

import java.util.List;

public interface IfOpenDoveNetworkCRUD {
    public boolean networkExists(String networkUUID);

    public boolean networkExistsByVnid(int vnid);

    public OpenDoveNetwork getNetwork(String networkUUID);

    public OpenDoveNetwork getNetworkByVnid(int parseInt);

    public OpenDoveNetwork getNetworkByName(String name);

    public void addNetwork(String networkUUID, OpenDoveNetwork network);

    public int allocateVNID();

    public List<OpenDoveNetwork> getNetworks();

    public List<OpenDoveEndpoint> getEndpoints(String saUUID);

    public void removeNetwork(String networkUUID);

    public void updateNetwork(String uuid);
}
