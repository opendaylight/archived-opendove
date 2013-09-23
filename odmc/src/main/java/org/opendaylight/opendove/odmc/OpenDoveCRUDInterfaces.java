/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.opendove.odmc;

import org.opendaylight.controller.sal.utils.ServiceHelper;

public class OpenDoveCRUDInterfaces {

    public static IfNBSystemRU getIfSystemRU(Object o) {
        IfNBSystemRU answer = (IfNBSystemRU) ServiceHelper.getGlobalInstance(IfNBSystemRU.class, o);
        return answer;
    }

    public static IfSBDoveDomainCRU getIfDoveDomainCRU(Object o) {
        IfSBDoveDomainCRU answer = (IfSBDoveDomainCRU) ServiceHelper.getGlobalInstance(IfSBDoveDomainCRU.class, o);
        return answer;
    }

    public static IfSBDoveNetworkCRU getIfDoveNetworkCRU(Object o) {
        IfSBDoveNetworkCRU answer = (IfSBDoveNetworkCRU) ServiceHelper.getGlobalInstance(IfSBDoveNetworkCRU.class, o);
        return answer;
    }

    public static IfSBDoveSubnetCRUD getIfDoveSubnetCRUD(Object o) {
        IfSBDoveSubnetCRUD answer = (IfSBDoveSubnetCRUD) ServiceHelper.getGlobalInstance(IfSBDoveSubnetCRUD.class, o);
        return answer;
    }

	public static IfSBOpenDoveChangeVersionR getIfSBOpenDoveChangeVersionR(Object o) {
		IfSBOpenDoveChangeVersionR answer = (IfSBOpenDoveChangeVersionR) ServiceHelper.getGlobalInstance(IfSBOpenDoveChangeVersionR.class, o);
        return answer;
	}

	public static IfSBDoveNetworkSubnetAssociationCRUD getIfDoveNetworkSubnetAssociationCRUD(Object o) {
		IfSBDoveNetworkSubnetAssociationCRUD answer = (IfSBDoveNetworkSubnetAssociationCRUD) ServiceHelper.getGlobalInstance(IfSBDoveNetworkSubnetAssociationCRUD.class, o);
        return answer;
	}
}
