package org.opendaylight.controller.odmc.northbound;

import java.util.List;

import org.opendaylight.controller.containermanager.IContainerManager;
import org.opendaylight.controller.northbound.commons.RestMessages;
import org.opendaylight.controller.northbound.commons.exception.ResourceNotFoundException;
import org.opendaylight.controller.northbound.commons.exception.ServiceUnavailableException;
import org.opendaylight.controller.odmc.IfNBFloatingIPCRUD;
import org.opendaylight.controller.odmc.IfNBNetworkCRUD;
import org.opendaylight.controller.odmc.IfNBPortCRUD;
import org.opendaylight.controller.odmc.IfNBRouterCRUD;
import org.opendaylight.controller.odmc.IfNBSubnetCRUD;
import org.opendaylight.controller.sal.utils.ServiceHelper;

public class OpenDoveNBInterfaces {

    // return a class that implements the IfNBNetworkCRUD interface
    static IfNBNetworkCRUD getIfNBNetworkCRUD(String containerName, Object o) {
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

        IfNBNetworkCRUD answer = (IfNBNetworkCRUD) ServiceHelper.getInstance(
                IfNBNetworkCRUD.class, containerName, o);

        if (answer == null) {
            throw new ServiceUnavailableException("Network CRUD Service "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }

        return answer;
    }

    // return a class that implements the IfNBSubnetCRUD interface
    static IfNBSubnetCRUD getIfNBSubnetCRUD(String containerName, Object o) {
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

        IfNBSubnetCRUD answer = (IfNBSubnetCRUD) ServiceHelper.getInstance(
                IfNBSubnetCRUD.class, containerName, o);

        if (answer == null) {
            throw new ServiceUnavailableException("Network CRUD Service "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }

        return answer;
    }

    // return a class that implements the IfNBPortCRUD interface
    static IfNBPortCRUD getIfNBPortCRUD(String containerName, Object o) {
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

        IfNBPortCRUD answer = (IfNBPortCRUD) ServiceHelper.getInstance(
                IfNBPortCRUD.class, containerName, o);

        if (answer == null) {
            throw new ServiceUnavailableException("Network CRUD Service "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }

        return answer;
    }

    // return a class that implements the IfNBRouterCRUD interface
    static IfNBRouterCRUD getIfNBRouterCRUD(String containerName, Object o) {
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

        IfNBRouterCRUD answer = (IfNBRouterCRUD) ServiceHelper.getInstance(
                IfNBRouterCRUD.class, containerName, o);

        if (answer == null) {
            throw new ServiceUnavailableException("Network CRUD Service "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }

        return answer;
    }

    // return a class that implements the IfNBFloatingIPCRUD interface
    static IfNBFloatingIPCRUD getIfNBFloatingIPCRUD(String containerName, Object o) {
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

        IfNBFloatingIPCRUD answer = (IfNBFloatingIPCRUD) ServiceHelper.getInstance(
                IfNBFloatingIPCRUD.class, containerName, o);

        if (answer == null) {
            throw new ServiceUnavailableException("Network CRUD Service "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }

        return answer;
    }
}
