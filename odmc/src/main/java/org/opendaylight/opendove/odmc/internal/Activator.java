/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.opendove.odmc.internal;

import java.util.Hashtable;
import java.util.Dictionary;
import org.apache.felix.dm.Component;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import org.opendaylight.controller.clustering.services.IClusterContainerServices;
import org.opendaylight.controller.sal.core.ComponentActivatorAbstractBase;
import org.opendaylight.opendove.odmc.IfNBFloatingIPCRUD;
import org.opendaylight.opendove.odmc.IfNBNetworkCRUD;
import org.opendaylight.opendove.odmc.IfNBPortCRUD;
import org.opendaylight.opendove.odmc.IfNBRouterCRUD;
import org.opendaylight.opendove.odmc.IfNBSubnetCRUD;

public class Activator extends ComponentActivatorAbstractBase {
    protected static final Logger logger = LoggerFactory
    .getLogger(Activator.class);

    /**
     * Function called when the activator starts just after some
     * initializations are done by the
     * ComponentActivatorAbstractBase.
     *
     */
    public void init() {

    }

    /**
     * Function called when the activator stops just before the
     * cleanup done by ComponentActivatorAbstractBase
     *
     */
    public void destroy() {

    }

    /**
     * Function that is used to communicate to dependency manager the
     * list of known implementations for services inside a container
     *
     *
     * @return An array containing all the CLASS objects that will be
     * instantiated in order to get an fully working implementation
     * Object
     */
    public Object[] getImplementations() {
        Object[] res = { OpenDoveManagementConsole.class };
        return res;
    }

    /**
     * Function that is called when configuration of the dependencies
     * is required.
     *
     * @param c dependency manager Component object, used for
     * configuring the dependencies exported and imported
     * @param imp Implementation class that is being configured,
     * needed as long as the same routine can configure multiple
     * implementations
     * @param containerName The containerName being configured, this allow
     * also optional per-container different behavior if needed, usually
     * should not be the case though.
     */
    public void configureInstance(Component c, Object imp, String containerName) {
        if (imp.equals(OpenDoveManagementConsole.class)) {
            // export the service
            c.setInterface(
                    new String[] { IfNBNetworkCRUD.class.getName(),
                            IfNBSubnetCRUD.class.getName(),
                            IfNBPortCRUD.class.getName(),
                            IfNBRouterCRUD.class.getName(),
                            IfNBFloatingIPCRUD.class.getName() }, null);
            Dictionary<String, String> props = new Hashtable<String, String>();
            props.put("salListenerName", "opendove");
            c.add(createContainerServiceDependency(containerName)
                    .setService(IClusterContainerServices.class)
                    .setCallbacks("setClusterContainerService",
                    "unsetClusterContainerService").setRequired(true));
        }
    }
}
