package org.opendaylight.opendove.odmc;

import org.opendaylight.controller.sal.utils.ServiceHelper;

public class OpenDoveCRUDInterfaces {

    public static IfNBNetworkCRUD getIfNBNetworkCRUD(Object o) {
        IfNBNetworkCRUD answer = (IfNBNetworkCRUD) ServiceHelper.getGlobalInstance(IfNBNetworkCRUD.class, o);
        return answer;
    }
    
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
}
