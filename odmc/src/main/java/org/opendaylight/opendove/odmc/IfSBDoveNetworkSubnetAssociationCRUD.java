/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.opendove.odmc;

import java.util.List;

public interface IfSBDoveNetworkSubnetAssociationCRUD {
    public boolean associationExists(String uuid);

    public OpenDoveNetworkSubnetAssociation getAssociation(String uuid);

    public void addNetworkSubnetAssociation(OpenDoveNetworkSubnetAssociation association);

    public List<OpenDoveNetworkSubnetAssociation> getAssociations();
    
    public void removeNetworkSubnetAssociation(String uuid);
}
