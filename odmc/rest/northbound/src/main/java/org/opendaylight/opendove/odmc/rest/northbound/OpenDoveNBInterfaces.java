/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.opendove.odmc.rest.northbound;

import java.util.List;

import org.opendaylight.controller.containermanager.IContainerManager;
import org.opendaylight.controller.northbound.commons.RestMessages;
import org.opendaylight.controller.northbound.commons.exception.ResourceNotFoundException;
import org.opendaylight.controller.northbound.commons.exception.ServiceUnavailableException;
import org.opendaylight.opendove.odmc.IfNBSystemRU;
import org.opendaylight.controller.sal.utils.ServiceHelper;

public class OpenDoveNBInterfaces {

    public static IfNBSystemRU getIfNBSystemRU(String containerName, Object o) {
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

        IfNBSystemRU answer = (IfNBSystemRU) ServiceHelper.getInstance(
                IfNBSystemRU.class, containerName, o);

        if (answer == null) {
            throw new ServiceUnavailableException("System RU "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }

        return answer;
    }
}
