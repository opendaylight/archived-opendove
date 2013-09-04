package org.opendaylight.opendove.odmc.southbound;

import java.util.List;

import org.opendaylight.controller.containermanager.IContainerManager;
import org.opendaylight.controller.northbound.commons.RestMessages;
import org.opendaylight.controller.northbound.commons.exception.ResourceNotFoundException;
import org.opendaylight.controller.northbound.commons.exception.ServiceUnavailableException;
import org.opendaylight.controller.sal.utils.ServiceHelper;
import org.opendaylight.opendove.odmc.IfOpenDoveSouthbound;

public class OpenDoveSBInterfaces {
    // return a class that implements the IfOpenDoveSouthbound interface
    static IfOpenDoveSouthbound getSBInterface(String containerName, Object o) {
        IContainerManager containerManager = (IContainerManager) ServiceHelper
                .getGlobalInstance(IContainerManager.class, o);
        if (containerManager == null) {
            throw new ServiceUnavailableException("Container "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }

        boolean found = false;
        List<String> containerNames = containerManager.getContainerNames();
        for (String cName : containerNames) {
            if (cName.trim().equalsIgnoreCase(containerName.trim())) {
                found = true;
            }
        }

        if (found == false) {
            throw new ResourceNotFoundException(containerName + " "
                    + RestMessages.NOCONTAINER.toString());
        }

        IfOpenDoveSouthbound answer = (IfOpenDoveSouthbound) ServiceHelper.getInstance(
        		IfOpenDoveSouthbound.class, containerName, o);

        if (answer == null) {
            throw new ServiceUnavailableException("Network CRUD Service "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }

        return answer;
    }
}
