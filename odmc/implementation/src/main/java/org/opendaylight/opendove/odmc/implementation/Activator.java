/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.opendove.odmc.implementation;

import java.util.Hashtable;
import java.util.Dictionary;
import org.apache.felix.dm.Component;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import org.opendaylight.controller.clustering.services.IClusterContainerServices;
import org.opendaylight.controller.networkconfig.neutron.INeutronFloatingIPAware;
import org.opendaylight.controller.networkconfig.neutron.INeutronNetworkAware;
import org.opendaylight.controller.networkconfig.neutron.INeutronPortAware;
import org.opendaylight.controller.networkconfig.neutron.INeutronRouterAware;
import org.opendaylight.controller.networkconfig.neutron.INeutronSubnetAware;
import org.opendaylight.controller.sal.core.ComponentActivatorAbstractBase;
import org.opendaylight.opendove.odmc.IfNBSystemRU;
import org.opendaylight.opendove.odmc.IfOpenDoveDomainCRU;
import org.opendaylight.opendove.odmc.IfOpenDoveSwitchCRU;
import org.opendaylight.opendove.odmc.IfSBDoveEGWFwdRuleCRUD;
import org.opendaylight.opendove.odmc.IfSBDoveGwIpv4CRUD;
import org.opendaylight.opendove.odmc.IfSBDoveEGWSNATPoolCRUD;
import org.opendaylight.opendove.odmc.IfOpenDoveNetworkCRU;
import org.opendaylight.opendove.odmc.IfSBDoveNetworkSubnetAssociationCRUD;
import org.opendaylight.opendove.odmc.IfOpenDoveServiceApplianceCRUD;
import org.opendaylight.opendove.odmc.IfSBDovePolicyCRUD;
import org.opendaylight.opendove.odmc.IfSBDoveSubnetCRUD;
import org.opendaylight.opendove.odmc.IfSBDoveVGWVNIDMappingCRUD;
import org.opendaylight.opendove.odmc.IfSBOpenDoveChangeVersionR;

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
        Object[] res = { OpenDoveNBInterfaces.class,
                OpenDoveSBInterfaces.class,
                OpenDoveBidirectionalInterfaces.class,
                OpenDoveNeutronCallbacks.class };
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
        if (imp.equals(OpenDoveNBInterfaces.class)) {
            c.setInterface(
                    new String[] { IfNBSystemRU.class.getName() }, null);
            Dictionary<String, String> props = new Hashtable<String, String>();
            props.put("salListenerName", "opendove");
            c.add(createContainerServiceDependency(containerName)
                    .setService(IClusterContainerServices.class)
                    .setCallbacks("setClusterContainerService",
                    "unsetClusterContainerService").setRequired(true));
        }
        if (imp.equals(OpenDoveNeutronCallbacks.class)) {
            c.setInterface(
                    new String[] { INeutronNetworkAware.class.getName(),
                            INeutronSubnetAware.class.getName(),
                            INeutronPortAware.class.getName(),
                            INeutronRouterAware.class.getName(),
                            INeutronFloatingIPAware.class.getName() }, null);
            Dictionary<String, String> props = new Hashtable<String, String>();
            props.put("salListenerName", "opendove");
            c.add(createContainerServiceDependency(containerName)
                    .setService(IClusterContainerServices.class)
                    .setCallbacks("setClusterContainerService",
                    "unsetClusterContainerService").setRequired(true));
        }
        if (imp.equals(OpenDoveBidirectionalInterfaces.class)) {
            c.setInterface(
                    new String[] { IfOpenDoveSwitchCRU.class.getName(),
                            IfOpenDoveNetworkCRU.class.getName(),
                            IfOpenDoveDomainCRU.class.getName(),
                            IfSBDoveVGWVNIDMappingCRUD.class.getName(),
                            IfOpenDoveServiceApplianceCRUD.class.getName() }, null);
            Dictionary<String, String> props = new Hashtable<String, String>();
            props.put("salListenerName", "opendove");
            c.add(createContainerServiceDependency(containerName)
                    .setService(IClusterContainerServices.class)
                    .setCallbacks("setClusterContainerService",
                    "unsetClusterContainerService").setRequired(true));
        }
        if (imp.equals(OpenDoveSBInterfaces.class)) {
            c.setInterface(
                    new String[] { IfSBDoveSubnetCRUD.class.getName(),
                            IfSBDovePolicyCRUD.class.getName(),
                            IfSBDoveGwIpv4CRUD.class.getName(),
                            IfSBDoveEGWSNATPoolCRUD.class.getName(),
                            IfSBDoveEGWFwdRuleCRUD.class.getName(),
                            IfSBOpenDoveChangeVersionR.class.getName(),
                            IfSBDoveNetworkSubnetAssociationCRUD.class.getName()
                            }, null);
            Dictionary<String, String> props = new Hashtable<String, String>();
            props.put("salListenerName", "opendove");
            c.add(createContainerServiceDependency(containerName)
                    .setService(IClusterContainerServices.class)
                    .setCallbacks("setClusterContainerService",
                    "unsetClusterContainerService").setRequired(true));
        }
    }
}
