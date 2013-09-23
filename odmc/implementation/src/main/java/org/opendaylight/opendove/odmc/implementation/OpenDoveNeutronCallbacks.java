/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.opendove.odmc.implementation;

import java.util.Iterator;

import javax.ws.rs.core.Response;

import org.opendaylight.controller.networkconfig.neutron.INeutronFloatingIPAware;
import org.opendaylight.controller.networkconfig.neutron.INeutronNetworkAware;
import org.opendaylight.controller.networkconfig.neutron.INeutronNetworkCRUD;
import org.opendaylight.controller.networkconfig.neutron.INeutronPortAware;
import org.opendaylight.controller.networkconfig.neutron.INeutronRouterAware;
import org.opendaylight.controller.networkconfig.neutron.INeutronSubnetAware;
import org.opendaylight.controller.networkconfig.neutron.NeutronCRUDInterfaces;
import org.opendaylight.controller.networkconfig.neutron.NeutronFloatingIP;
import org.opendaylight.controller.networkconfig.neutron.NeutronNetwork;
import org.opendaylight.controller.networkconfig.neutron.NeutronPort;
import org.opendaylight.controller.networkconfig.neutron.NeutronRouter;
import org.opendaylight.controller.networkconfig.neutron.NeutronRouter_Interface;
import org.opendaylight.controller.networkconfig.neutron.NeutronSubnet;
import org.opendaylight.opendove.odmc.IfNBSystemRU;
import org.opendaylight.opendove.odmc.IfSBDoveDomainCRU;
import org.opendaylight.opendove.odmc.IfSBDoveNetworkCRU;
import org.opendaylight.opendove.odmc.IfSBDoveNetworkSubnetAssociationCRUD;
import org.opendaylight.opendove.odmc.IfSBDoveSubnetCRUD;
import org.opendaylight.opendove.odmc.OpenDoveCRUDInterfaces;
import org.opendaylight.opendove.odmc.OpenDoveDomain;
import org.opendaylight.opendove.odmc.OpenDoveNetwork;
import org.opendaylight.opendove.odmc.OpenDoveNetworkSubnetAssociation;
import org.opendaylight.opendove.odmc.OpenDoveNeutronControlBlock;
import org.opendaylight.opendove.odmc.OpenDoveSubnet;

public class OpenDoveNeutronCallbacks implements INeutronNetworkAware, INeutronSubnetAware, INeutronPortAware,
INeutronRouterAware, INeutronFloatingIPAware {

    // callbacks for INeutronNetworkAware

    public int canCreateNetwork(NeutronNetwork network) {
        if (network.getAdminStateUp() != null && !network.isAdminStateUp())
            return 400;
        return 200;
    }
    public void neutronNetworkCreated(NeutronNetwork input) {
        IfNBSystemRU systemDB = OpenDoveCRUDInterfaces.getIfSystemRU(this);
        IfSBDoveDomainCRU domainDB = OpenDoveCRUDInterfaces.getIfDoveDomainCRU(this);
        IfSBDoveNetworkCRU doveNetworkDB = OpenDoveCRUDInterfaces.getIfDoveNetworkCRU(this);
        if (!input.isRouterExternal()) {            // don't map router external networks
            if (input.isShared()) {                    // map shared network
                OpenDoveNeutronControlBlock controlBlock = systemDB.getSystemBlock(); //get system block
                if (!controlBlock.getDomainSeparation()) { //if domain separation not supported, map to shared domain
                    OpenDoveDomain sharedDomain;
                    if (!domainDB.domainExistsByName("SharedDomain")) {// look up shared domain
                        sharedDomain = new OpenDoveDomain("SharedDomain"); // if doesn't exist, create
                        domainDB.addDomain(sharedDomain.getUUID(), sharedDomain);
                        //create EXT MCAST network
                        int vnid = doveNetworkDB.allocateVNID();
                        String networkName = "Ext_MCast_"+vnid;
                        OpenDoveNetwork extMCastNet = new OpenDoveNetwork(networkName, vnid, sharedDomain, 1, input.getID());
                        doveNetworkDB.addNetwork(extMCastNet.getUUID(), extMCastNet);
                    } else
                        sharedDomain = domainDB.getDomainByName("SharedDomain");
                    int vnid = doveNetworkDB.allocateVNID();
                    String networkName = "Neutron "+input.getID();
                    OpenDoveNetwork doveNetwork = new OpenDoveNetwork(networkName, vnid, sharedDomain, 0, input.getID());
                    doveNetworkDB.addNetwork(doveNetwork.getUUID(), doveNetwork);
                }
            } else {                                // map dedicated network
                String domainName = "Neutron "+input.getTenantID();
                OpenDoveDomain domain;
                if (!domainDB.domainExistsByName(domainName)) { // look up domain
                    domain = new OpenDoveDomain(domainName); // if doesn't exist, create
                    domainDB.addDomain(domain.getUUID(), domain);
                    //create EXT MCAST network
                    int vnid = doveNetworkDB.allocateVNID();
                    String networkName = "Ext_MCast_"+vnid;
                    OpenDoveNetwork extMCastNet = new OpenDoveNetwork(networkName, vnid, domain, 1, input.getID());
                    doveNetworkDB.addNetwork(extMCastNet.getUUID(), extMCastNet);
                } else
                    domain = domainDB.getDomainByName(domainName);
                int vnid = doveNetworkDB.allocateVNID();
                String networkName = "Neutron "+input.getID();
                OpenDoveNetwork doveNetwork = new OpenDoveNetwork(networkName, vnid, domain, 0, input.getID());
                doveNetworkDB.addNetwork(doveNetwork.getUUID(), doveNetwork);
            }
        }
    }

    public int canUpdateNetwork(NeutronNetwork delta, NeutronNetwork original) {
        /*
         * transitions forbidden by DOVE
         */
        if (delta.getNetworkName() != null || delta.getAdminStateUp() != null ||
                delta.getShared() != null || delta.getRouterExternal() != null)
            return 403;
        return 200;
    }

    public void neutronNetworkUpdated(NeutronNetwork network) {
        // TODO Auto-generated method stub

    }

    public int canDeleteNetwork(NeutronNetwork network) {
        return 200;
    }

    public void neutronNetworkDeleted(NeutronNetwork network) {
        IfSBDoveNetworkCRU doveNetworkDB = OpenDoveCRUDInterfaces.getIfDoveNetworkCRU(this);
        // mark open dove networks for deletion
        Iterator<OpenDoveNetwork> i = doveNetworkDB.getNetworks().iterator();
        while (i.hasNext()) {
            OpenDoveNetwork oDN = i.next();
            if (oDN.getNeutronNetwork().equalsIgnoreCase(network.getID()))
                oDN.setTombstoneFlag(true);
        }
    }

    // callbacks for INeutronSubnetAware

    public int canCreateSubnet(NeutronSubnet subnet) {
        /* if network is shared and domains are not separated,
         * then we need to configure an external gateway at this point.  If we don't have one
         * don't create the subnet
         */
        IfNBSystemRU systemDB = OpenDoveCRUDInterfaces.getIfSystemRU(this);
        INeutronNetworkCRUD neutronNetworkIf = NeutronCRUDInterfaces.getINeutronNetworkCRUD(this);
        NeutronNetwork neutronNetwork = neutronNetworkIf.getNetwork(subnet.getNetworkUUID());
        if (!neutronNetwork.isRouterExternal()) {
            if (neutronNetwork.isShared()) {
                OpenDoveNeutronControlBlock controlBlock = systemDB.getSystemBlock(); //get system block
                if (!controlBlock.getDomainSeparation()) {
                    //TODO: add external gateway code
                    return 404;
                }
            }
        }
        return 200;
    }

    public void neutronSubnetCreated(NeutronSubnet neutronSubnet) {
        IfNBSystemRU systemDB = OpenDoveCRUDInterfaces.getIfSystemRU(this);
        IfSBDoveDomainCRU domainDB = OpenDoveCRUDInterfaces.getIfDoveDomainCRU(this);
        IfSBDoveNetworkCRU networkDB = OpenDoveCRUDInterfaces.getIfDoveNetworkCRU(this);
        IfSBDoveSubnetCRUD subnetDB = OpenDoveCRUDInterfaces.getIfDoveSubnetCRUD(this);
        IfSBDoveNetworkSubnetAssociationCRUD networkSubnetAssociationDB =
        	OpenDoveCRUDInterfaces.getIfDoveNetworkSubnetAssociationCRUD(this);
        INeutronNetworkCRUD neutronNetworkIf = NeutronCRUDInterfaces.getINeutronNetworkCRUD(this);
        NeutronNetwork neutronNetwork = neutronNetworkIf.getNetwork(neutronSubnet.getNetworkUUID());
        if (!neutronNetwork.isRouterExternal()) {
            if (neutronNetwork.isShared()) {
                OpenDoveNeutronControlBlock controlBlock = systemDB.getSystemBlock(); //get system block
                if (!controlBlock.getDomainSeparation()) {
                    ; //TODO: configure external GW
                }
            } else {
                String domainName = "Neutron "+neutronSubnet.getTenantID();
                OpenDoveDomain domain = domainDB.getDomainByName(domainName);
                String networkName = "Neutron "+neutronSubnet.getNetworkUUID();
                OpenDoveNetwork network = networkDB.getNetworkByName(networkName);
                OpenDoveSubnet subnet = new OpenDoveSubnet(neutronSubnet, domain, neutronNetwork, network);
                // link subnet to network and track
                OpenDoveNetworkSubnetAssociation nSA = new OpenDoveNetworkSubnetAssociation();
                nSA.setOpenDoveNetworkVnid(network.getVnid());
                nSA.setOpenDoveNetworkSubnetUuid(subnet.getUUID());
                networkSubnetAssociationDB.addNetworkSubnetAssociation(nSA);
                subnetDB.addSubnet(subnet.getUUID(), subnet);
                domain.addSubnet(subnet);
            }
        }
    }

    public int canUpdateSubnet(NeutronSubnet delta, NeutronSubnet original) {
        /*
         * updates restricted by DOVE
         */
        if (delta.getGatewayIP() != null)
            return 403;
        return 200;
    }

    public void neutronSubnetUpdated(NeutronSubnet subnet) {
    	; //empty because there isn't anything that passes through to the subnet
    }

    public int canDeleteSubnet(NeutronSubnet subnet) {
        return 200;
    }

    public void neutronSubnetDeleted(NeutronSubnet subnet) {
        IfSBDoveDomainCRU domainDB = OpenDoveCRUDInterfaces.getIfDoveDomainCRU(this);
        IfSBDoveNetworkCRU doveNetworkDB = OpenDoveCRUDInterfaces.getIfDoveNetworkCRU(this);
        IfSBDoveSubnetCRUD doveSubnetDB = OpenDoveCRUDInterfaces.getIfDoveSubnetCRUD(this);
        // mark open dove networks for deletion
        Iterator<OpenDoveSubnet> i = doveSubnetDB.getSubnets().iterator();
        while (i.hasNext()) {
            OpenDoveSubnet oDS = i.next();
            if (oDS.getAssociatedOSSubnetUUID().equalsIgnoreCase(subnet.getID())) {
            	// need to remove from the domain
                String domainName = "Neutron "+subnet.getTenantID();
                OpenDoveDomain domain = domainDB.getDomainByName(domainName);
            	domain.removeSubnet(oDS);
            	// need to remove from the systemdb
            	doveSubnetDB.removeSubnet(oDS.getUUID());
            }
        }
    }


    // INeutronPortAware methods

    public int canCreatePort(NeutronPort port) {
        // DOVE specific requirement on create
        if (port.getAdminStateUp() != null && !port.isAdminStateUp())
            return 400;
        return 200;
    }

    public void neutronPortCreated(NeutronPort port) {
        // TODO Auto-generated method stub

    }
    public int canUpdatePort(NeutronPort delta, NeutronPort original) {
        // DOVE specific things that can't be changed
        if (delta.getAdminStateUp() != null)
            return 403;
        return 200;
    }
    public void neutronPortUpdated(NeutronPort port) {
        // TODO Auto-generated method stub

    }
    public int canDeletePort(NeutronPort port) {
        // TODO Auto-generated method stub
        return 200;
    }
    public void neutronPortDeleted(NeutronPort port) {
        // TODO Auto-generated method stub

    }

    // INeutronRouterAware methods
    public int canCreateRouter(NeutronRouter router) {
        /*
         * DOVE specific requirement on create
         */
        if (router.getAdminStateUp() != null && !router.isAdminStateUp())
            return 400;
        return 200;
    }
    public void neutronRouterCreated(NeutronRouter router) {
        // TODO Auto-generated method stub

    }
    public int canUpdateRouter(NeutronRouter delta, NeutronRouter original) {
        /*
         * attribute changes blocked by DOVE
         */
        if (delta.getAdminStateUp() != null)
            return 403;

        /* FIXME: if the external gateway info is being changed,
         * block with 403 if there is a router interface using it
         * (DOVE restriction)
         */
        return 200;
    }
    public void neutronRouterUpdated(NeutronRouter router) {
        // TODO Auto-generated method stub

    }
    public int canDeleteRouter(NeutronRouter router) {
        // TODO Auto-generated method stub
        return 200;
    }
    public void neutronRouterDeleted(NeutronRouter router) {
        // TODO Auto-generated method stub

    }
    public void neutronRouterInterfaceAttached(NeutronRouter router,
            NeutronRouter_Interface routerInterface) {
        // TODO Auto-generated method stub

    }
    public void neutronRouterInterfaceDetached(NeutronRouter router,
            NeutronRouter_Interface routerInterface) {
        // TODO Auto-generated method stub

    }

    // INeutronFloatingIPAware methods

    public int canCreateFloatingIP(NeutronFloatingIP floatingIP) {
        // TODO Auto-generated method stub
        return 200;
    }
    public void neutronFloatingIPCreated(NeutronFloatingIP floatingIP) {
        // TODO Auto-generated method stub

    }
    public int canUpdateFloatingIP(NeutronFloatingIP delta,
            NeutronFloatingIP original) {
        // TODO Auto-generated method stub
        return 200;
    }
    public void neutronFloatingIPUpdated(NeutronFloatingIP floatingIP) {
        // TODO Auto-generated method stub

    }
    public int canDeleteFloatingIP(NeutronFloatingIP floatingIP) {
        // TODO Auto-generated method stub
        return 200;
    }
    public void neutronFloatingIPDeleted(NeutronFloatingIP floatingIP) {
        // TODO Auto-generated method stub

    }
}
