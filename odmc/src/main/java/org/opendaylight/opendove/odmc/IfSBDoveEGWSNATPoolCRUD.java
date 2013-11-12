/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.opendove.odmc;

import java.util.List;

public interface IfSBDoveEGWSNATPoolCRUD {
    public boolean egwSNATPoolExists(String poolUUID);

    public OpenDoveEGWSNATPool getEgwSNATPool(String poolUUID);

    public void addEgwSNATPool(String poolUUID, OpenDoveEGWSNATPool pool);

    public List<OpenDoveEGWSNATPool> getEgwSNATPools();

    public void removeEgwSNATPool(String poolUUID);

    public void updateSNATPool(String uuid, OpenDoveEGWSNATPool snatPool);
}
