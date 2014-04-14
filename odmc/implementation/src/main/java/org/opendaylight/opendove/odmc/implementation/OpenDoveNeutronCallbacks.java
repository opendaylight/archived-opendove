/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.opendove.odmc.implementation;

import java.util.Iterator;

import org.opendaylight.controller.networkconfig.neutron.INeutronFloatingIPAware;
import org.opendaylight.controller.networkconfig.neutron.INeutronNetworkAware;
import org.opendaylight.controller.networkconfig.neutron.INeutronNetworkCRUD;
import org.opendaylight.controller.networkconfig.neutron.INeutronPortAware;
import org.opendaylight.controller.networkconfig.neutron.INeutronPortCRUD;
import org.opendaylight.controller.networkconfig.neutron.INeutronRouterAware;
import org.opendaylight.controller.networkconfig.neutron.INeutronSubnetAware;
import org.opendaylight.controller.networkconfig.neutron.INeutronSubnetCRUD;
import org.opendaylight.controller.networkconfig.neutron.NeutronCRUDInterfaces;
import org.opendaylight.controller.networkconfig.neutron.NeutronFloatingIP;
import org.opendaylight.controller.networkconfig.neutron.NeutronNetwork;
import org.opendaylight.controller.networkconfig.neutron.NeutronPort;
import org.opendaylight.controller.networkconfig.neutron.NeutronRouter;
import org.opendaylight.controller.networkconfig.neutron.NeutronRouter_Interface;
import org.opendaylight.controller.networkconfig.neutron.NeutronSubnet;
import org.opendaylight.controller.networkconfig.neutron.NeutronSubnet_IPAllocationPool;
import org.opendaylight.opendove.odmc.IfNBSystemRU;
import org.opendaylight.opendove.odmc.IfOpenDoveServiceApplianceCRUD;
import org.opendaylight.opendove.odmc.IfOpenDoveDomainCRUD;
import org.opendaylight.opendove.odmc.IfSBDoveEGWSNATPoolCRUD;
import org.opendaylight.opendove.odmc.IfSBDoveGwIpv4CRUD;
import org.opendaylight.opendove.odmc.IfOpenDoveNetworkCRUD;
import org.opendaylight.opendove.odmc.IfSBDoveNetworkSubnetAssociationCRUD;
import org.opendaylight.opendove.odmc.IfSBDovePolicyCRUD;
import org.opendaylight.opendove.odmc.IfSBDoveSubnetCRUD;
import org.opendaylight.opendove.odmc.OpenDoveCRUDInterfaces;
import org.opendaylight.opendove.odmc.OpenDoveDomain;
import org.opendaylight.opendove.odmc.OpenDoveEGWFwdRule;
import org.opendaylight.opendove.odmc.OpenDoveEGWSNATPool;
import org.opendaylight.opendove.odmc.OpenDoveGwIpv4;
import org.opendaylight.opendove.odmc.OpenDoveNetwork;
import org.opendaylight.opendove.odmc.OpenDoveNetworkSubnetAssociation;
import org.opendaylight.opendove.odmc.OpenDoveNeutronControlBlock;
import org.opendaylight.opendove.odmc.OpenDovePolicy;
import org.opendaylight.opendove.odmc.OpenDoveServiceAppliance;
import org.opendaylight.opendove.odmc.OpenDoveSubnet;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class OpenDoveNeutronCallbacks implements INeutronNetworkAware, INeutronSubnetAware, INeutronPortAware,
INeutronRouterAware, INeutronFloatingIPAware {
    protected static final Logger logger = LoggerFactory
            .getLogger(Activator.class);

    // callbacks for INeutronNetworkAware

    public int canCreateNetwork(NeutronNetwork network) {
        if (network.getAdminStateUp() != null && !network.isAdminStateUp()) {
            return 400;
        }
        return 200;
    }
    public void neutronNetworkCreated(NeutronNetwork input) {
        IfNBSystemRU systemDB = OpenDoveCRUDInterfaces.getIfSystemRU(this);
        IfOpenDoveDomainCRUD domainDB = OpenDoveCRUDInterfaces.getIfDoveDomainCRU(this);
        IfOpenDoveNetworkCRUD doveNetworkDB = OpenDoveCRUDInterfaces.getIfDoveNetworkCRU(this);
        if (!input.isRouterExternal()) {            // don't map router external networks
            if (input.isShared()) {                    // map shared network
                OpenDoveNeutronControlBlock controlBlock = systemDB.getSystemBlock(); //get system block
                if (!controlBlock.getDomainSeparation()) { //if domain separation not supported, map to shared domain
                    OpenDoveDomain sharedDomain = createDoveDomain("SharedDomain", input.getID(), domainDB, doveNetworkDB);
                    int vnid = doveNetworkDB.allocateVNID();
                    String networkName = "Neutron "+input.getID();
                    OpenDoveNetwork doveNetwork = new OpenDoveNetwork(networkName, vnid, sharedDomain, 0, input.getID());
                    doveNetworkDB.addNetwork(doveNetwork.getUUID(), doveNetwork);
                }
            } else {                                // map dedicated network
                String domainName = "Neutron "+input.getTenantID();
                OpenDoveDomain domain = createDoveDomain(domainName, input.getID(), domainDB, doveNetworkDB);
                domain.setAssociatedOSTenantUUID(input.getTenantID());
                int vnid = doveNetworkDB.allocateVNID();
                String networkName = "Neutron "+input.getID();
                OpenDoveNetwork doveNetwork = new OpenDoveNetwork(networkName, vnid, domain, 0, input.getID());
                doveNetworkDB.addNetwork(doveNetwork.getUUID(), doveNetwork);
            }
        }
    }

    private OpenDoveDomain createDoveDomain(String domainName, String netUUID,
            IfOpenDoveDomainCRUD domainDB, IfOpenDoveNetworkCRUD doveNetworkDB) {
        OpenDoveDomain domain;
        if (!domainDB.domainExistsByName(domainName)) { // look up domain
            domain = new OpenDoveDomain(domainName); // if doesn't exist, create
            domainDB.addDomain(domain.getUUID(), domain);
            //create EXT MCAST network
            int vnid = doveNetworkDB.allocateVNID();
            String networkName = "Ext_MCast_"+vnid;
            OpenDoveNetwork extMCastNet = new OpenDoveNetwork(networkName, vnid, domain, 1, "");
            doveNetworkDB.addNetwork(extMCastNet.getUUID(), extMCastNet);

            /*
             *  Set the Ext Mcast Network for the Domain Here.
             *  This is needed for EXT-GW SNAT Pool Creation
             */
            domain.setExtMCastNetwork(extMCastNet);
        } else {
            domain = domainDB.getDomainByName(domainName);
        }
        return domain;
    }

    public int canUpdateNetwork(NeutronNetwork delta, NeutronNetwork original) {
        /*
         * transitions forbidden by openDove
         */
        if (delta.getNetworkName() != null || delta.getAdminStateUp() != null ||
                delta.getShared() != null || delta.getRouterExternal() != null) {
            return 403;
        }
        return 200;
    }

    public void neutronNetworkUpdated(NeutronNetwork network) {
        // openDove doesn't do anything here
        ;
    }

    public int canDeleteNetwork(NeutronNetwork network) {
        return 200;
    }

    public void neutronNetworkDeleted(NeutronNetwork network) {
        IfOpenDoveNetworkCRUD doveNetworkDB = OpenDoveCRUDInterfaces.getIfDoveNetworkCRU(this);
        // mark open dove networks for deletion
        // no need to release EGW and SNAT pools for networks here - before the network can be removed,
        // the router interface has to be removed (which tears down policies and SNAT pools) and
        // then the port has to be removed.
        Iterator<OpenDoveNetwork> i = doveNetworkDB.getNetworks().iterator();
        while (i.hasNext()) {
            OpenDoveNetwork oDN = i.next();
            if (oDN.getNeutronNetwork().equalsIgnoreCase(network.getID())) {
                oDN.setTombstoneFlag(true);
                doveNetworkDB.updateNetwork(oDN.getUUID());
            }
        }
    }

    // callbacks for INeutronSubnetAware

    public int canCreateSubnet(NeutronSubnet subnet) {
        INeutronNetworkCRUD neutronNetworkIf = NeutronCRUDInterfaces.getINeutronNetworkCRUD(this);
        NeutronNetwork neutronNetwork = neutronNetworkIf.getNetwork(subnet.getNetworkUUID());
        if (!neutronNetwork.isRouterExternal()) {
            // not external, so check conditions on shared network
            return canAllocateEGW(subnet.getNetworkUUID(), true);
        }
        // external first need to check if this network has no subnets
        if (neutronNetwork.getSubnets().size() > 0) {
            return 403;
        }
        return 200;
    }

    public void neutronSubnetCreated(NeutronSubnet neutronSubnet) {
        IfNBSystemRU systemDB = OpenDoveCRUDInterfaces.getIfSystemRU(this);
        IfOpenDoveDomainCRUD domainDB = OpenDoveCRUDInterfaces.getIfDoveDomainCRU(this);
        IfOpenDoveNetworkCRUD networkDB = OpenDoveCRUDInterfaces.getIfDoveNetworkCRU(this);
        IfSBDoveSubnetCRUD subnetDB = OpenDoveCRUDInterfaces.getIfDoveSubnetCRUD(this);
        IfSBDoveNetworkSubnetAssociationCRUD networkSubnetAssociationDB =
                OpenDoveCRUDInterfaces.getIfDoveNetworkSubnetAssociationCRUD(this);
        INeutronNetworkCRUD neutronNetworkIf = NeutronCRUDInterfaces.getINeutronNetworkCRUD(this);
        NeutronNetwork neutronNetwork = neutronNetworkIf.getNetwork(neutronSubnet.getNetworkUUID());
        OpenDoveNeutronControlBlock controlBlock = systemDB.getSystemBlock(); //get system block
        if (!neutronNetwork.isRouterExternal()) {
            if (neutronNetwork.isShared()) {
                if (!controlBlock.getDomainSeparation()) {
                    OpenDoveDomain sharedDomain = domainDB.getDomainByName("SharedDomain");
                    String networkName = "Neutron "+neutronSubnet.getNetworkUUID();
                    OpenDoveNetwork network = networkDB.getNetworkByName(networkName);
                    OpenDoveSubnet subnet = createDoveSubnet(neutronSubnet,
                            subnetDB, networkSubnetAssociationDB,
                            neutronNetwork, sharedDomain, network);
                    subnetDB.addSubnet(subnet.getUUID(), subnet);
                    //assign egws and 0 size SNAT pool
                    Integer replicationFactor = controlBlock.getEgwReplicationFactor();
                    IfSBDoveEGWSNATPoolCRUD snatPoolDB = OpenDoveCRUDInterfaces.getIfDoveEGWSNATPoolCRUD(this);
                    while (replicationFactor > 0) {
                        String gwAddress = selectAddress(neutronNetwork, neutronSubnet, null);
                        OpenDoveServiceAppliance target = OpenDoveServiceAppliance.selectDGW(this);
                        if (target != null) {
                            OpenDoveGwIpv4.assignEGWs(this, target, neutronSubnet.getCidr(),
                                    neutronSubnet.getGatewayIP(), gwAddress, null);
                            OpenDoveEGWSNATPool.configureEGWSNATPool("0.0.0.0", "0.0.0.0", snatPoolDB, network, target);
                        }
                        replicationFactor--;
                    }
                }
            } else {
                String domainName = "Neutron "+neutronSubnet.getTenantID();
                OpenDoveDomain domain = domainDB.getDomainByName(domainName);
                String networkName = "Neutron "+neutronSubnet.getNetworkUUID();
                OpenDoveNetwork network = networkDB.getNetworkByName(networkName);
                OpenDoveSubnet subnet = createDoveSubnet(neutronSubnet,
                        subnetDB, networkSubnetAssociationDB, neutronNetwork,
                        domain, network);
                subnetDB.addSubnet(subnet.getUUID(), subnet);
            }
        }
    }

    private OpenDoveSubnet createDoveSubnet(NeutronSubnet neutronSubnet,
            IfSBDoveSubnetCRUD subnetDB,
            IfSBDoveNetworkSubnetAssociationCRUD networkSubnetAssociationDB,
            NeutronNetwork neutronNetwork, OpenDoveDomain sharedDomain,
            OpenDoveNetwork network) {
        OpenDoveSubnet subnet = new OpenDoveSubnet(neutronSubnet, sharedDomain, neutronNetwork, network);
        // link subnet to network and track
        OpenDoveNetworkSubnetAssociation nSA = new OpenDoveNetworkSubnetAssociation();
        nSA.setOpenDoveNetworkVnid(network.getVnid());
        nSA.setOpenDoveNetworkSubnetUuid(subnet.getUUID());
        networkSubnetAssociationDB.addNetworkSubnetAssociation(nSA);
        subnetDB.addSubnet(subnet.getUUID(), subnet);
        sharedDomain.addSubnet(subnet);
        return subnet;
    }

    public int canUpdateSubnet(NeutronSubnet delta, NeutronSubnet original) {
        /*
         * updates restricted by openDove
         */
        if (delta.getGatewayIP() != null) {
            return 403;
        }
        return 200;
    }

    public void neutronSubnetUpdated(NeutronSubnet subnet) {
        ; //empty because there isn't anything that passes through to the subnet
    }

    public int canDeleteSubnet(NeutronSubnet subnet) {
        /* check if router external - if so, check for EGW assignments or SNAT pool assignments
         * if either exist, can't delete router external subnet
         */
        INeutronNetworkCRUD neutronNetworkIf = NeutronCRUDInterfaces.getINeutronNetworkCRUD(this);
        NeutronNetwork neutronNetwork = neutronNetworkIf.getNetwork(subnet.getNetworkUUID());
        if (neutronNetwork.isRouterExternal()) {
            IfSBDoveEGWSNATPoolCRUD snatPoolDB = OpenDoveCRUDInterfaces.getIfDoveEGWSNATPoolCRUD(this);
            if (snatPoolDB.getEgwSNATPools().size() > 0 ) {
                return 409;
            }
            IfSBDoveGwIpv4CRUD gatewayIPDB = OpenDoveCRUDInterfaces.getIfSBDoveGwIpv4CRUD(this);
            for (OpenDoveGwIpv4 ipv4 : gatewayIPDB.getGwIpv4Pool()) {
                if (ipv4.getType().equalsIgnoreCase("external")) {
                    return 409;
                }
            }
        }
        return 200;
    }

    public void neutronSubnetDeleted(NeutronSubnet subnet) {
        IfNBSystemRU systemDB = OpenDoveCRUDInterfaces.getIfSystemRU(this);
        OpenDoveNeutronControlBlock controlBlock = systemDB.getSystemBlock();

        OpenDoveCRUDInterfaces.getIfDoveNetworkCRU(this);
        IfSBDoveSubnetCRUD subnetDB = OpenDoveCRUDInterfaces.getIfDoveSubnetCRUD(this);
        IfSBDoveNetworkSubnetAssociationCRUD networkSubnetAssociationDB =
                OpenDoveCRUDInterfaces.getIfDoveNetworkSubnetAssociationCRUD(this);
        INeutronNetworkCRUD neutronNetworkIf = NeutronCRUDInterfaces.getINeutronNetworkCRUD(this);
        NeutronNetwork neutronNetwork = neutronNetworkIf.getNetwork(subnet.getNetworkUUID());
        if (!neutronNetwork.isRouterExternal()) {
            if (neutronNetwork.isShared()) {
                if (!controlBlock.getDomainSeparation()) {
                    // tombstone EGW IPv4 information
                    IfOpenDoveNetworkCRUD networkDB = OpenDoveCRUDInterfaces.getIfDoveNetworkCRU(this);
                    IfOpenDoveServiceApplianceCRUD serviceApplianceDB = OpenDoveCRUDInterfaces.getIfDoveServiceApplianceCRUD(this);
                    IfSBDoveGwIpv4CRUD gatewayIPDB = OpenDoveCRUDInterfaces.getIfSBDoveGwIpv4CRUD(this);
                    String networkName = "Neutron "+subnet.getNetworkUUID();
                    OpenDoveNetwork network = networkDB.getNetworkByName(networkName);
                    OpenDoveGwIpv4.tombstoneEGWs(serviceApplianceDB, gatewayIPDB, subnet, network);
                }
            }
            // tombstone networkSubnetAssociation
            for (OpenDoveSubnet oDS: subnetDB.getSubnets()) {
                if (oDS.getAssociatedOSSubnetUUID().equalsIgnoreCase(subnet.getID())) {
                    for (OpenDoveNetworkSubnetAssociation oDNSA : networkSubnetAssociationDB.getAssociations()) {
                        if (oDNSA.getOpenDoveNetworkSubnetUuid().equalsIgnoreCase(oDS.getUUID())) {
                            oDNSA.setTombstoneFlag(true);
                            networkSubnetAssociationDB.updateAssociation(oDNSA);
                        }
                    }
                }
            }
        }
    }

    // INeutronPortAware methods

    public int canCreatePort(NeutronPort port) {
        // openDove specific requirement on create
        if (port.getAdminStateUp() != null && !port.isAdminStateUp()) {
            return 400;
        }
        return canAllocateEGW(port.getNetworkUUID(), false);
    }

    public void neutronPortCreated(NeutronPort port) {
        IfNBSystemRU systemDB = OpenDoveCRUDInterfaces.getIfSystemRU(this);
        IfOpenDoveDomainCRUD domainDB = OpenDoveCRUDInterfaces.getIfDoveDomainCRU(this);
        IfOpenDoveNetworkCRUD networkDB = OpenDoveCRUDInterfaces.getIfDoveNetworkCRU(this);
        IfSBDoveSubnetCRUD subnetDB = OpenDoveCRUDInterfaces.getIfDoveSubnetCRUD(this);
        IfSBDoveNetworkSubnetAssociationCRUD networkSubnetAssociationDB =
                OpenDoveCRUDInterfaces.getIfDoveNetworkSubnetAssociationCRUD(this);
        INeutronNetworkCRUD neutronNetworkIf = NeutronCRUDInterfaces.getINeutronNetworkCRUD(this);
        NeutronNetwork neutronNetwork = neutronNetworkIf.getNetwork(port.getNetworkUUID());
        INeutronSubnetCRUD neutronSubnetIf = NeutronCRUDInterfaces.getINeutronSubnetCRUD(this);
        NeutronSubnet neutronSubnet = neutronSubnetIf.getSubnet(port.getFixedIPs().get(0).getSubnetUUID());
        if (!neutronNetwork.isRouterExternal()) {
            if (neutronNetwork.isShared()) {
                OpenDoveNeutronControlBlock controlBlock = systemDB.getSystemBlock(); //get system block
                if (controlBlock.getDomainSeparation()) {
                    //create dove tenant
                    String domainName = "Neutron "+neutronNetwork.getTenantID();
                    OpenDoveDomain domain = createDoveDomain(domainName, neutronNetwork.getID(), domainDB, networkDB);
                    domain.setAssociatedOSTenantUUID(neutronNetwork.getTenantID());
                    int vnid = networkDB.allocateVNID();
                    //create dove network
                    String networkName = "Neutron "+neutronNetwork.getID();
                    OpenDoveNetwork doveNetwork = new OpenDoveNetwork(networkName, vnid, domain, 0, neutronNetwork.getID());
                    networkDB.addNetwork(doveNetwork.getUUID(), doveNetwork);
                    //create dove subnet
                    OpenDoveSubnet subnet = createDoveSubnet(neutronSubnet,
                            subnetDB, networkSubnetAssociationDB, neutronNetwork,
                            domain, doveNetwork);
                    subnetDB.addSubnet(subnet.getUUID(), subnet);
                    //assign egw to network
                    Integer replicationFactor = controlBlock.getEgwReplicationFactor();
                    IfSBDoveEGWSNATPoolCRUD snatPoolDB = OpenDoveCRUDInterfaces.getIfDoveEGWSNATPoolCRUD(this);
                    while (replicationFactor > 0) {
                        String gwAddress = selectAddress(neutronNetwork, neutronSubnet, null);
                        OpenDoveServiceAppliance target = OpenDoveServiceAppliance.selectDGW(this);
                        if (target != null) {
                            OpenDoveGwIpv4.assignEGWs(this, target, neutronSubnet.getCidr(),
                                    neutronSubnet.getGatewayIP(), gwAddress, null);
                            OpenDoveEGWSNATPool.configureEGWSNATPool("0.0.0.0", "0.0.0.0", snatPoolDB, doveNetwork, target);
                        }
                        replicationFactor--;
                    }
                }
            }
        }
    }

    public int canUpdatePort(NeutronPort delta, NeutronPort original) {
        // openDove specific things that can't be changed
        if (delta.getAdminStateUp() != null) {
            return 403;
        }
        return 200;
    }
    public void neutronPortUpdated(NeutronPort port) {
        // nothing openDove related changes

    }
    public int canDeletePort(NeutronPort port) {
        // nothing openDove related changes
        return 200;
    }
    public void neutronPortDeleted(NeutronPort port) {
        IfNBSystemRU systemDB = OpenDoveCRUDInterfaces.getIfSystemRU(this);
        IfSBDoveSubnetCRUD subnetDB = OpenDoveCRUDInterfaces.getIfDoveSubnetCRUD(this);
        IfSBDoveNetworkSubnetAssociationCRUD networkSubnetAssociationDB =
                OpenDoveCRUDInterfaces.getIfDoveNetworkSubnetAssociationCRUD(this);
        INeutronNetworkCRUD neutronNetworkIf = NeutronCRUDInterfaces.getINeutronNetworkCRUD(this);
        NeutronNetwork neutronNetwork = neutronNetworkIf.getNetwork(port.getNetworkUUID());
        INeutronSubnetCRUD neutronSubnetIf = NeutronCRUDInterfaces.getINeutronSubnetCRUD(this);
        NeutronSubnet neutronSubnet = neutronSubnetIf.getSubnet(port.getFixedIPs().get(0).getSubnetUUID());
        OpenDoveNeutronControlBlock controlBlock = systemDB.getSystemBlock(); //get system block
        if (!neutronNetwork.isRouterExternal()) {
            if (neutronNetwork.isShared()) {
                if (controlBlock.getDomainSeparation()) {
                    // tombstone EGW IPv4 information
                    IfOpenDoveNetworkCRUD networkDB = OpenDoveCRUDInterfaces.getIfDoveNetworkCRU(this);
                    IfOpenDoveServiceApplianceCRUD serviceApplianceDB = OpenDoveCRUDInterfaces.getIfDoveServiceApplianceCRUD(this);
                    IfSBDoveGwIpv4CRUD gatewayIPDB = OpenDoveCRUDInterfaces.getIfSBDoveGwIpv4CRUD(this);
                    String networkName = "Neutron "+neutronSubnet.getNetworkUUID();
                    OpenDoveNetwork network = networkDB.getNetworkByName(networkName);
                    OpenDoveGwIpv4.tombstoneEGWs(serviceApplianceDB, gatewayIPDB, neutronSubnet, network);
                }
            }
            // tombstone networkSubnetAssociation
            for (OpenDoveSubnet oDS: subnetDB.getSubnets()) {
                if (oDS.getAssociatedOSSubnetUUID().equalsIgnoreCase(neutronSubnet.getID())) {
                    for (OpenDoveNetworkSubnetAssociation oDNSA : networkSubnetAssociationDB.getAssociations()) {
                        if (oDNSA.getOpenDoveNetworkSubnetUuid().equalsIgnoreCase(oDS.getUUID())) {
                            oDNSA.setTombstoneFlag(true);
                            networkSubnetAssociationDB.updateAssociation(oDNSA);
                        }
                    }
                }
            }
        }
    }

    // INeutronRouterAware methods
    public int canCreateRouter(NeutronRouter router) {
        /*
         * openDove specific requirement on create
         */
        if (router.getAdminStateUp() != null && !router.isAdminStateUp()) {
            return 400;
        }
        // requirement that external gateway info exist and have one and only one subnet
        if (router.getExternalGatewayInfo() != null) {
            INeutronNetworkCRUD neutronNetworkIf = NeutronCRUDInterfaces.getINeutronNetworkCRUD(this);
            String neutronNetworkUUID = router.getExternalGatewayInfo().getNetworkID();
            NeutronNetwork neutronNetwork = neutronNetworkIf.getNetwork(neutronNetworkUUID);
            if (neutronNetwork == null) {
                return 400;
            }
            if (!neutronNetwork.isRouterExternal()) {
                return 400;
            }
            if (neutronNetwork.getSubnets().size() != 1) {
                return 400;
            }
            // do we have an oDGW to configure?
                    IfOpenDoveServiceApplianceCRUD serviceApplianceDB = OpenDoveCRUDInterfaces.getIfDoveServiceApplianceCRUD(this);
                    for (OpenDoveServiceAppliance oDSA : serviceApplianceDB.getAppliances()) {
                        if (oDSA.get_isDGW() && oDSA.getDoveTunnel() != null){
                            return 200;
                        }
                    }
                    return 400;
        }
        return 200;
    }

    public void neutronRouterCreated(NeutronRouter router) {
        // if there is an external gateway, then select and assign EGWs for this external space
        if (router.getExternalGatewayInfo() != null) {
            INeutronNetworkCRUD neutronNetworkIf = NeutronCRUDInterfaces.getINeutronNetworkCRUD(this);
            NeutronNetwork neutronNetwork = neutronNetworkIf.getNetwork(router.getExternalGatewayInfo().getNetworkID());
            INeutronSubnetCRUD neutronSubnetIf = NeutronCRUDInterfaces.getINeutronSubnetCRUD(this);
            NeutronSubnet neutronSubnet = neutronSubnetIf.getSubnet(neutronNetwork.getSubnets().get(0));
            IfNBSystemRU systemDB = OpenDoveCRUDInterfaces.getIfSystemRU(this);
            OpenDoveNeutronControlBlock controlBlock = systemDB.getSystemBlock(); //get system block
            Integer replicationFactor = controlBlock.getEgwReplicationFactor();
            while (replicationFactor > 0) {
                String gwAddress = selectAddress(neutronNetwork, neutronSubnet, router);
                OpenDoveServiceAppliance target = OpenDoveServiceAppliance.selectDGW(this);
                if (target != null) {
                    OpenDoveGwIpv4.assignEGWs(this, target, neutronSubnet.getCidr(), neutronSubnet.getGatewayIP(), gwAddress, router);
                }
                replicationFactor--;
            }
        }
    }

    public int canUpdateRouter(NeutronRouter delta, NeutronRouter original) {
        /*
         * attribute changes blocked by openDove
         */

        return 200;
    }

    public void neutronRouterUpdated(NeutronRouter router) {
        if (router.getExternalGatewayInfo() != null) {
            // remove old address: this is a blantent hack, but we don't have a way to tell of the router's
            // external gw info has changed at this point.
            IfSBDoveGwIpv4CRUD gatewayIPDB = OpenDoveCRUDInterfaces.getIfSBDoveGwIpv4CRUD(this);
            for (OpenDoveGwIpv4 test: gatewayIPDB.getGwIpv4Pool()) {
                if (test.getNeutronRouter() != null && router.getID().equalsIgnoreCase(test.getNeutronRouter().getID())) {
                    test.setTombstoneFlag(true);
                }
            }
            INeutronNetworkCRUD neutronNetworkIf = NeutronCRUDInterfaces.getINeutronNetworkCRUD(this);
            NeutronNetwork neutronNetwork = neutronNetworkIf.getNetwork(router.getExternalGatewayInfo().getNetworkID());
            INeutronSubnetCRUD neutronSubnetIf = NeutronCRUDInterfaces.getINeutronSubnetCRUD(this);
            NeutronSubnet neutronSubnet = neutronSubnetIf.getSubnet(neutronNetwork.getSubnets().get(0));
            IfNBSystemRU systemDB = OpenDoveCRUDInterfaces.getIfSystemRU(this);
            OpenDoveNeutronControlBlock controlBlock = systemDB.getSystemBlock(); //get system block
            Integer replicationFactor = controlBlock.getEgwReplicationFactor();
            while (replicationFactor > 0) {
                String gwAddress = selectAddress(neutronNetwork, neutronSubnet, router);
                OpenDoveServiceAppliance target = OpenDoveServiceAppliance.selectDGW(this);
                if (target != null) {
                    OpenDoveGwIpv4.assignEGWs(this, target, neutronSubnet.getCidr(), neutronSubnet.getGatewayIP(), gwAddress, router);
                }
                replicationFactor--;
            }
        } else {
            IfSBDoveGwIpv4CRUD gatewayIPDB = OpenDoveCRUDInterfaces.getIfSBDoveGwIpv4CRUD(this);
            for (OpenDoveGwIpv4 test: gatewayIPDB.getGwIpv4Pool()) {
                if (test.getNeutronRouter() != null && router.getID().equalsIgnoreCase(test.getNeutronRouter().getID())) {
                    test.setTombstoneFlag(true);
                }
            }
        }
    }

    public int canDeleteRouter(NeutronRouter router) {
        //openDove doesn't block router deletes
        return 200;
    }

    public void neutronRouterDeleted(NeutronRouter router) {
        if (router.getExternalGatewayInfo() != null) {
            IfSBDoveGwIpv4CRUD gatewayIPDB = OpenDoveCRUDInterfaces.getIfSBDoveGwIpv4CRUD(this);
            for (OpenDoveGwIpv4 test: gatewayIPDB.getGwIpv4Pool()) {
                if (test.getNeutronRouter() != null && router.getID().equalsIgnoreCase(test.getNeutronRouter().getID())) {
                    test.setTombstoneFlag(true);
                }
            }
        }
    }

    public int canAttachInterface(NeutronRouter router,
            NeutronRouter_Interface routerInterface) {
        //EGW is given an external address when the router is created
        //so here, all we need to do is check to see if there are enough addresses in the external
        //space to allow us to add another interface.  If not, return BADREQUEST with a message
        if (router.getExternalGatewayInfo() != null) {
            INeutronNetworkCRUD neutronNetworkIf = NeutronCRUDInterfaces.getINeutronNetworkCRUD(this);
            NeutronNetwork externalNetwork = neutronNetworkIf.getNetwork(router.getExternalGatewayInfo().getNetworkID());
            INeutronSubnetCRUD neutronSubnetIf = NeutronCRUDInterfaces.getINeutronSubnetCRUD(this);
            NeutronSubnet neutronSubnet = neutronSubnetIf.getSubnet(externalNetwork.getSubnets().get(0));
            IfNBSystemRU systemDB = OpenDoveCRUDInterfaces.getIfSystemRU(this);
            OpenDoveNeutronControlBlock controlBlock = systemDB.getSystemBlock(); //get system block
            Integer snatPoolSize = controlBlock.getSnatPoolSize();
            boolean havePool = false;
            for (NeutronSubnet_IPAllocationPool pool: neutronSubnet.getAllocationPools()) {
                if (poolSize(pool) >= snatPoolSize) {
                    havePool = true;
                }
            }
            if (!havePool) {
                return 400;
            }
        }
        return 200;
    }

    public void neutronRouterInterfaceAttached(NeutronRouter router,
            NeutronRouter_Interface routerInterface) {
        INeutronSubnetCRUD neutronSubnetIf = NeutronCRUDInterfaces.getINeutronSubnetCRUD(this);
        NeutronSubnet neutronSubnet = neutronSubnetIf.getSubnet(routerInterface.getSubnetUUID());
        INeutronNetworkCRUD neutronNetworkIf = NeutronCRUDInterfaces.getINeutronNetworkCRUD(this);
        NeutronNetwork neutronNetwork = neutronNetworkIf.getNetwork(neutronSubnet.getNetworkUUID());
        String networkName = "Neutron " + neutronNetwork.getID();
        IfOpenDoveNetworkCRUD doveNetworkDB = OpenDoveCRUDInterfaces.getIfDoveNetworkCRU(this);
        OpenDoveNetwork newODN = doveNetworkDB.getNetworkByName(networkName);
        IfSBDovePolicyCRUD dovePolicyDB = OpenDoveCRUDInterfaces.getIfDovePolicyCRUD(this);
        IfSBDoveEGWSNATPoolCRUD snatPoolDB = OpenDoveCRUDInterfaces.getIfDoveEGWSNATPoolCRUD(this);
        IfNBSystemRU systemDB = OpenDoveCRUDInterfaces.getIfSystemRU(this);
        OpenDoveNeutronControlBlock controlBlock = systemDB.getSystemBlock(); //get system block

        for (NeutronRouter_Interface oldInterface: router.getInterfaces().values()) {
            NeutronSubnet oldNeutronSubnet = neutronSubnetIf.getSubnet(oldInterface.getSubnetUUID());
            NeutronNetwork oldNeutronNetwork = neutronNetworkIf.getNetwork(oldNeutronSubnet.getNetworkUUID());
            String oldNetworkUUID = "Neutron " + oldNeutronNetwork.getID();
            OpenDoveNetwork oldODN = doveNetworkDB.getNetworkByName(oldNetworkUUID);

            if (!oldInterface.equals(routerInterface)) {
                logger.debug("Adding policies between "+oldInterface+" and "+routerInterface);
                if (newODN.getDomain_uuid().equalsIgnoreCase(oldODN.getDomain_uuid())) {
                    OpenDovePolicy.createAllowPolicies(dovePolicyDB, newODN, oldODN);
                }
            }
        }
        // Fix Bug 516 to use external gateway info information
        if (router.getExternalGatewayInfo() != null) {
            // change this to get the neutronNetwork from external gateway info
            // then pull the subnet from that object (see how ext address gets assigned
            NeutronNetwork externalNetwork = neutronNetworkIf.getNetwork(router.getExternalGatewayInfo().getNetworkID());
            NeutronSubnet externalNeutronSubnet = neutronSubnetIf.getSubnet(externalNetwork.getSubnets().get(0));
            Integer replicationFactor = controlBlock.getEgwReplicationFactor();
            Integer snatPoolSize = controlBlock.getSnatPoolSize();
            while (replicationFactor > 0) {
                OpenDoveServiceAppliance target = OpenDoveServiceAppliance.selectAssignedDGW(this, externalNeutronSubnet.getCidr());
                if (target != null) {
                    NeutronSubnet_IPAllocationPool pool = selectPool(externalNeutronSubnet, snatPoolSize);
                    if (pool != null) {
                        OpenDoveEGWSNATPool.configureEGWSNATPool(pool.getPoolStart(), pool.getPoolEnd(), snatPoolDB, newODN, target);
                    } else {
                        //TODO: log error
                        ;
                    }
                } else {
                    //TODO: log error
                    ;
                }
                replicationFactor--;
            }
        }
    }

    public int canDetachInterface(NeutronRouter router,
            NeutronRouter_Interface routerInterface) {
        // openDove doesn't limit this
        return 200;
    }

    public void neutronRouterInterfaceDetached(NeutronRouter router,
            NeutronRouter_Interface routerInterface) {
        INeutronSubnetCRUD neutronSubnetIf = NeutronCRUDInterfaces.getINeutronSubnetCRUD(this);
        NeutronSubnet neutronSubnet = neutronSubnetIf.getSubnet(routerInterface.getSubnetUUID());
        INeutronNetworkCRUD neutronNetworkIf = NeutronCRUDInterfaces.getINeutronNetworkCRUD(this);
        NeutronNetwork neutronNetwork = neutronNetworkIf.getNetwork(neutronSubnet.getNetworkUUID());
        String networkUUID = "Neutron " + neutronNetwork.getID();
        IfSBDoveSubnetCRUD doveSubnetDB = OpenDoveCRUDInterfaces.getIfDoveSubnetCRUD(this);
        IfOpenDoveNetworkCRUD doveNetworkDB = OpenDoveCRUDInterfaces.getIfDoveNetworkCRU(this);
        OpenDoveNetwork newODN = doveNetworkDB.getNetworkByName(networkUUID);
        IfSBDovePolicyCRUD dovePolicyDB = OpenDoveCRUDInterfaces.getIfDovePolicyCRUD(this);
        IfSBDoveEGWSNATPoolCRUD snatPoolDB = OpenDoveCRUDInterfaces.getIfDoveEGWSNATPoolCRUD(this);
        IfNBSystemRU systemDB = OpenDoveCRUDInterfaces.getIfSystemRU(this);
        OpenDoveNeutronControlBlock controlBlock = systemDB.getSystemBlock(); //get system block

        for ( NeutronRouter_Interface oldInterface: router.getInterfaces().values()) {
            NeutronSubnet oldNeutronSubnet = neutronSubnetIf.getSubnet(oldInterface.getSubnetUUID());
            NeutronNetwork oldNeutronNetwork = neutronNetworkIf.getNetwork(oldNeutronSubnet.getNetworkUUID());
            String oldNetworkUUID = "Neutron " + oldNeutronNetwork.getID();
            OpenDoveNetwork oldODN = doveNetworkDB.getNetworkByName(oldNetworkUUID);

            if (!oldInterface.equals(routerInterface)) {
                logger.debug("Removing connection between "+oldInterface+" and "+routerInterface);
                if (neutronNetwork.isRouterExternal() || oldNeutronNetwork.isRouterExternal()) {
                    if (neutronNetwork.isRouterExternal()) {
                        Iterator<OpenDoveSubnet> i = doveSubnetDB.getSubnets().iterator();
                        while (i.hasNext()) {
                            OpenDoveSubnet oDS = i.next();
                            if (oDS.getAssociatedOSSubnetUUID().equalsIgnoreCase(oldNeutronSubnet.getID()) &&
                                    oDS.getDomainUUID() == oldODN.getDomain_uuid()) {
                                OpenDoveEGWSNATPool.removeEGWSNATPool(neutronSubnet, snatPoolDB,
                                        controlBlock, oldODN);
                            }
                        }
                    } else {
                        for ( OpenDoveSubnet oDS: doveSubnetDB.getSubnets()) {
                            if (oDS.getAssociatedOSSubnetUUID().equalsIgnoreCase(neutronSubnet.getID()) &&
                                    oDS.getDomainUUID() == newODN.getDomain_uuid()) {
                                OpenDoveEGWSNATPool.removeEGWSNATPool(oldNeutronSubnet, snatPoolDB,
                                        controlBlock, newODN);
                            }
                        }
                    }
                } else {
                    if (newODN.getDomain_uuid().equalsIgnoreCase(oldODN.getDomain_uuid())) {
                        OpenDovePolicy.removeAllowPolicies(dovePolicyDB, newODN, oldODN);
                    }
                }
            }
        }
    }

    // INeutronFloatingIPAware methods

    public int canCreateFloatingIP(NeutronFloatingIP floatingIP) {
        // opendove doesn't block anything here
        return 200;
    }

    public void neutronFloatingIPCreated(NeutronFloatingIP floatingIP) {
        OpenDoveEGWFwdRule.mapFloatingIPtoEGWFwdRule(floatingIP, this);
    }

    public int canUpdateFloatingIP(NeutronFloatingIP delta,
            NeutronFloatingIP original) {
        // opendove doesn't block anything here
        return 200;
    }
    public void neutronFloatingIPUpdated(NeutronFloatingIP floatingIP) {
        // if Port-ID is null, look through all EGWFwdRules and set tombstone flag for each
        // if Port-ID is not null, repeat create steps (refactor)
        if (floatingIP.getPortUUID() == null) {
            OpenDoveEGWFwdRule.removeEgwFwdRulesForFloatingIP(floatingIP, this);
        } else {
            OpenDoveEGWFwdRule.mapFloatingIPtoEGWFwdRule(floatingIP, this);
        }
    }
    public int canDeleteFloatingIP(NeutronFloatingIP floatingIP) {
        // opendove doesn't block anything here
        return 200;
    }
    public void neutronFloatingIPDeleted(NeutronFloatingIP floatingIP) {
        // look through all EGWFwdRules and set tombstone flag for each (refactor)
        OpenDoveEGWFwdRule.removeEgwFwdRulesForFloatingIP(floatingIP, this);
    }

    private int canAllocateEGW(String uuid, boolean negate) {
        /* if object is shared or external
         * then we need to configure an external gateway at this point.  If we don't have one
         * don't create the port
         */
        IfNBSystemRU systemDB = OpenDoveCRUDInterfaces.getIfSystemRU(this);
        INeutronNetworkCRUD neutronNetworkIf = NeutronCRUDInterfaces.getINeutronNetworkCRUD(this);
        NeutronNetwork neutronNetwork = neutronNetworkIf.getNetwork(uuid);
        if (neutronNetwork.isShared()) {
            OpenDoveNeutronControlBlock controlBlock = systemDB.getSystemBlock(); //get system block
            if ((!negate && controlBlock.getDomainSeparation()) ||
                    (negate && !controlBlock.getDomainSeparation())) {
                IfOpenDoveServiceApplianceCRUD serviceApplianceDB = OpenDoveCRUDInterfaces.getIfDoveServiceApplianceCRUD(this);
                for (OpenDoveServiceAppliance oDSA : serviceApplianceDB.getAppliances()) {
                    if (oDSA.get_isDGW() && oDSA.getDoveTunnel() != null){
                        return 200;
                    }
                }
                return 400;
            }
        }
        return 200;
    }

    private String selectAddress(NeutronNetwork neutronNetwork, NeutronSubnet neutronSubnet, NeutronRouter neutronRouter) {
        String address = null;
        if (neutronNetwork.isRouterExternal()) {
            // need to get external address for router and determine if it is used
            INeutronPortCRUD neutronPortIf = NeutronCRUDInterfaces.getINeutronPortCRUD(this);
            String portAddress = null;
            for (NeutronPort port: neutronPortIf.getAllPorts()) {
                if (neutronRouter.getID().equals(port.getDeviceID())) {
                    portAddress = port.getFixedIPs().get(0).getIpAddress();
                }
            }
            boolean used = false;
            IfSBDoveGwIpv4CRUD gatewayIPDB = OpenDoveCRUDInterfaces.getIfSBDoveGwIpv4CRUD(this);
            for (OpenDoveGwIpv4 ipv4 : gatewayIPDB.getGwIpv4Pool()) {
                if (ipv4.getType().equalsIgnoreCase("external") && ipv4.getIP().equals(portAddress)) {
                    used = true;
                }
            }
            if (used || portAddress == null) {
                // If so, allocate a new one
                NeutronSubnet_IPAllocationPool ans = selectPool(neutronSubnet, 1);
                address = ans.getPoolStart();
            } else {
                //If not, use it
                address = portAddress;
            }
        } else {
            //shared network, just grab next address
            NeutronSubnet_IPAllocationPool ans = selectPool(neutronSubnet, 1);
            address = ans.getPoolStart();
        }
        return address;
    }

    private NeutronSubnet_IPAllocationPool selectPool(NeutronSubnet subnet, Integer size) {
        String ip_low = null, ip_high = null;
        if (size > 0) {
            for (NeutronSubnet_IPAllocationPool pool: subnet.getAllocationPools()) {
                if (poolSize(pool) >= size) {
                    int i1;
                    for (i1=1; i1<=size; i1++) {
                        String ipAddr = pool.getPoolStart();
                        if (i1 == 1) {
                            ip_low = ipAddr;
                        }
                        if (i1 == size) {
                            ip_high = ipAddr;
                        }
                        subnet.allocateIP(ipAddr);
                        //TODO: close loop by allocating Neutron port
                    }
                    break;
                }
            }
        }
        if (ip_low != null && ip_high != null) {
            return new NeutronSubnet_IPAllocationPool(ip_low, ip_high);
        } else {
            return null;
        }
    }

    static private Integer poolSize(NeutronSubnet_IPAllocationPool pool) {
        long bottom = convertIPv4StringToLong(pool.getPoolStart());
        long top = convertIPv4StringToLong(pool.getPoolEnd());
        int ans = (int) (top-bottom)+1;
        return new Integer(ans);
    }

    //FIXME: make this method in NeutronSubnet_IPAllocationPool public and replace
    static private long convertIPv4StringToLong(String inputString) {
        long ans = 0;
        String[] parts = inputString.split("\\.");
        for (String part: parts) {
            ans <<= 8;
            ans |= Integer.parseInt(part);
        }
        return ans;
    }

}

