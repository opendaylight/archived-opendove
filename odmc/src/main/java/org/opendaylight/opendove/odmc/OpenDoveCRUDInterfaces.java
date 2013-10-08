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

    public static IfSBDovePolicyCRUD getIfDovePolicyCRUD(Object o) {
        IfSBDovePolicyCRUD answer = (IfSBDovePolicyCRUD) ServiceHelper.getGlobalInstance(IfSBDovePolicyCRUD.class, o);
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

    public static IfOpenDoveServiceApplianceCRU getIfDoveServiceApplianceCRU(Object o) {
        IfOpenDoveServiceApplianceCRU answer = (IfOpenDoveServiceApplianceCRU) ServiceHelper.getGlobalInstance(IfOpenDoveServiceApplianceCRU.class, o);
        return answer;
    }

    public static IfSBDoveGwIpv4CRUD getIfSBDoveGwIpv4CRUD(Object o) {
    	IfSBDoveGwIpv4CRUD answer = (IfSBDoveGwIpv4CRUD) ServiceHelper.getGlobalInstance(IfSBDoveGwIpv4CRUD.class, o);
        return answer;
    }
    
    public static IfSBDoveEGWSNATPoolCRUD getIfDoveEGWSNATPoolCRUD(Object o) {
        IfSBDoveEGWSNATPoolCRUD answer = (IfSBDoveEGWSNATPoolCRUD) ServiceHelper.getGlobalInstance(IfSBDoveEGWSNATPoolCRUD.class, o);
        return answer;
    }

    public static IfSBDoveEGWFwdRuleCRUD getIfSBDoveEGWFwdRuleCRUD(Object o) {
        IfSBDoveEGWFwdRuleCRUD answer = (IfSBDoveEGWFwdRuleCRUD) ServiceHelper.getGlobalInstance(IfSBDoveEGWFwdRuleCRUD.class, o);
        return answer;
    }

    public static IfSBDoveVGWVNIDMappingCRUD getIfSBDoveVGWVNIDMappingCRUD(Object o) {
    	IfSBDoveVGWVNIDMappingCRUD answer = (IfSBDoveVGWVNIDMappingCRUD) ServiceHelper.getGlobalInstance(IfSBDoveVGWVNIDMappingCRUD.class, o);
        return answer;
    }
}
