/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.opendove.odmc;

import java.util.List;

public interface IfOpenDoveDomainCRUD {
    public boolean domainExists(String domainUUID);

    public boolean domainExistsByName(String name);

    public OpenDoveDomain getDomain(String domainUUID);

    public OpenDoveDomain getDomainByName(String name);

    public void addDomain(String domainUUID, OpenDoveDomain domain);

    public void addNetworkToDomain(String domainUUID, OpenDoveNetwork network);

    public List<OpenDoveDomain> getDomains();

    public List<OpenDoveServiceAppliance> getDCSList(String saUUID);

    public boolean domainExistsByNumber(String domainID);

    public OpenDoveDomain getDomainByNumber(String domainID);

    public void removeDomain(String domainUUID);
}
