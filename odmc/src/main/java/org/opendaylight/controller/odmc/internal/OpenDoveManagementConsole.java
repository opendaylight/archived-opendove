/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.controller.odmc.internal;

import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Dictionary;
import java.util.EnumSet;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Set;
import java.util.Map.Entry;
import java.util.concurrent.ConcurrentMap;

import org.apache.felix.dm.Component;
import org.opendaylight.controller.odmc.*;

import org.opendaylight.controller.clustering.services.CacheConfigException;
import org.opendaylight.controller.clustering.services.CacheExistException;
import org.opendaylight.controller.clustering.services.IClusterContainerServices;
import org.opendaylight.controller.clustering.services.IClusterServices;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class OpenDoveManagementConsole implements IfNBNetworkCRUD, IfNBSubnetCRUD,
         IfNBPortCRUD, IfNBRouterCRUD, IfNBFloatingIPCRUD {
    private static final Logger logger = LoggerFactory
            .getLogger(OpenDoveManagementConsole.class);
    private String containerName = null;

    private IClusterContainerServices clusterContainerService = null;
    private ConcurrentMap<String, OpenStackNetworks> networkDB;
    private ConcurrentMap<String, OpenStackSubnets> subnetDB;
    private ConcurrentMap<String, OpenStackPorts> portDB;
    private ConcurrentMap<String, OpenStackRouters> routerDB;
    private ConcurrentMap<String, OpenStackFloatingIPs> floatingIPDB;

    // methods needed for creating caches

    void setClusterContainerService(IClusterContainerServices s) {
        logger.debug("Cluster Service set");
        this.clusterContainerService = s;
    }

    void unsetClusterContainerService(IClusterContainerServices s) {
        if (this.clusterContainerService == s) {
            logger.debug("Cluster Service removed!");
            this.clusterContainerService = null;
        }
    }

    @SuppressWarnings("deprecation")
    private void allocateCache() {
        if (this.clusterContainerService == null) {
            logger.error("un-initialized clusterContainerService, can't create cache");
            return;
        }
        logger.debug("Creating Cache for OpenDOVE");
        try {
            // neutron caches
            this.clusterContainerService.createCache("openDoveNeutronNetworks",
                    EnumSet.of(IClusterServices.cacheMode.NON_TRANSACTIONAL));
            this.clusterContainerService.createCache("openDoveNeutronSubnets",
                    EnumSet.of(IClusterServices.cacheMode.NON_TRANSACTIONAL));
            this.clusterContainerService.createCache("openDoveNeutronPorts",
                    EnumSet.of(IClusterServices.cacheMode.NON_TRANSACTIONAL));
            this.clusterContainerService.createCache("openDoveNeutronRouters",
                    EnumSet.of(IClusterServices.cacheMode.NON_TRANSACTIONAL));
            this.clusterContainerService.createCache("openDoveNeutronFloatingIPs",
                    EnumSet.of(IClusterServices.cacheMode.NON_TRANSACTIONAL));
        } catch (CacheConfigException cce) {
            logger.error("Cache couldn't be created for OpenDOVE -  check cache mode");
        } catch (CacheExistException cce) {
            logger.error("Cache for OpenDOVE already exists, destroy and recreate");
        }
        logger.debug("Cache successfully created for OpenDOVE");
    }

    @SuppressWarnings({ "unchecked", "deprecation" })
    private void retrieveCache() {
        if (this.clusterContainerService == null) {
            logger.error("un-initialized clusterContainerService, can't retrieve cache");
            return;
        }
        logger.debug("Retrieving cache for openDoveNeutronNetworks");
        networkDB = (ConcurrentMap<String, OpenStackNetworks>) this.clusterContainerService
                .getCache("openDoveNeutronNetworks");
        if (networkDB == null) {
            logger.error("Cache couldn't be retrieved for openDOVENeutronNetworks");
        }
        logger.debug("Cache was successfully retrieved for openDOVENeutronNetworks");
        logger.debug("Retrieving cache for openDoveNeutronSubnets");
        subnetDB = (ConcurrentMap<String, OpenStackSubnets>) this.clusterContainerService
                .getCache("openDoveNeutronSubnets");
        if (subnetDB == null) {
            logger.error("Cache couldn't be retrieved for openDOVENeutronSubnets");
        }
        logger.debug("Cache was successfully retrieved for openDOVENeutronSubnets");
        logger.debug("Retrieving cache for openDoveNeutronPorts");
        portDB = (ConcurrentMap<String, OpenStackPorts>) this.clusterContainerService
                .getCache("openDoveNeutronPorts");
        if (portDB == null) {
            logger.error("Cache couldn't be retrieved for openDOVENeutronPorts");
        }
        logger.debug("Cache was successfully retrieved for openDOVENeutronPorts");
        logger.debug("Retrieving cache for openDoveNeutronRouters");
        routerDB = (ConcurrentMap<String, OpenStackRouters>) this.clusterContainerService
                .getCache("openDoveNeutronRouters");
        if (routerDB == null) {
            logger.error("Cache couldn't be retrieved for openDOVENeutronRouters");
        }
        logger.debug("Cache was successfully retrieved for openDOVENeutronRouters");
        logger.debug("Retrieving cache for openDoveNeutronFloatingIPs");
        floatingIPDB = (ConcurrentMap<String, OpenStackFloatingIPs>) this.clusterContainerService
                .getCache("openDoveNeutronFloatingIPs");
        if (floatingIPDB == null) {
            logger.error("Cache couldn't be retrieved for openDOVENeutronFloatingIPs");
        }
        logger.debug("Cache was successfully retrieved for openDOVENeutronFloatingIPs");
    }

    private void destroyCache() {
/*        if (this.clusterContainerService == null) {
            logger.error("un-initialized clusterMger, can't destroy cache");
            return;
        }
        logger.debug("Destroying Cache for HostTracker");
        this.clusterContainerService.destroyCache("hostTrackerAH");
        this.clusterContainerService.destroyCache("hostTrackerIH");
        nonClusterObjectCreate(); */
    }

    private void startUp() {
        allocateCache();
        retrieveCache();
    }

    /**
     * Function called by the dependency manager when all the required
     * dependencies are satisfied
     *
     */
    void init(Component c) {
        Dictionary<?, ?> props = c.getServiceProperties();
        if (props != null) {
            this.containerName = (String) props.get("containerName");
            logger.debug("Running containerName: {}", this.containerName);
        } else {
            // In the Global instance case the containerName is empty
            this.containerName = "";
        }
        startUp();
    }

    /**
     * Function called by the dependency manager when at least one dependency
     * become unsatisfied or when the component is shutting down because for
     * example bundle is being stopped.
     *
     */
    void destroy() {
        destroyCache();
    }

    /**
     * Function called by dependency manager after "init ()" is called and after
     * the services provided by the class are registered in the service registry
     *
     */
    void start() {
    }

    /**
     * Function called by the dependency manager before the services exported by
     * the component are unregistered, this will be followed by a "destroy ()"
     * calls
     *
     */
    void stop() {
    }

    // this method uses reflection to update an object from it's delta.

    private boolean overwrite(Object target, Object delta) {
        Method[] methods = target.getClass().getMethods();

        for(Method toMethod: methods){
            if(toMethod.getDeclaringClass().equals(target.getClass())
                    && toMethod.getName().startsWith("set")){

                String toName = toMethod.getName();
                String fromName = toName.replace("set", "get");

                try {
                    Method fromMethod = delta.getClass().getMethod(fromName);
                    Object value = fromMethod.invoke(delta, (Object[])null);
                    if(value != null){
                        toMethod.invoke(target, value);
                    }
                } catch (Exception e) {
                    e.printStackTrace();
                    return false;
                }
            }
        }
        return true;
    }

    // IfNBFloatingIPCRUD interface methods

    @Override
    public boolean floatingIPExists(String uuid) {
        return floatingIPDB.containsKey(uuid);
    }

    @Override
    public OpenStackFloatingIPs getFloatingIP(String uuid) {
        if (!floatingIPExists(uuid))
            return null;
        return floatingIPDB.get(uuid);
    }

    @Override
    public List<OpenStackFloatingIPs> getAllFloatingIPs() {
        Set<OpenStackFloatingIPs> allIPs = new HashSet<OpenStackFloatingIPs>();
        for (Entry<String, OpenStackFloatingIPs> entry : floatingIPDB.entrySet()) {
            OpenStackFloatingIPs floatingip = entry.getValue();
            allIPs.add(floatingip);
        }
        logger.debug("Exiting getAllFloatingIPs, Found {} FloatingIPs", allIPs.size());
        List<OpenStackFloatingIPs> ans = new ArrayList<OpenStackFloatingIPs>();
        ans.addAll(allIPs);
        return ans;
    }

    @Override
    public boolean addFloatingIP(OpenStackFloatingIPs input) {
        if (floatingIPExists(input.getID()))
            return false;
        //if floating_ip_address isn't there, allocate from the subnet pool
        OpenStackSubnets subnet = getSubnet(getNetwork(input.getFloatingNetworkUUID()).getSubnets().get(0));
        if (input.getFloatingIPAddress() == null)
            input.setFloatingIPAddress(subnet.getLowAddr());
        subnet.allocateIP(input.getFloatingIPAddress());

        //if port_id is there, bind port to this floating ip
        if (input.getPortUUID() != null) {
            OpenStackPorts port = getPort(input.getPortUUID());
            port.addFloatingIP(input.getFixedIPAddress(), input);
        }

        floatingIPDB.putIfAbsent(input.getID(), input);
        return true;
    }

    @Override
    public boolean removeFloatingIP(String uuid) {
        if (!floatingIPExists(uuid))
            return false;
        OpenStackFloatingIPs floatIP = getFloatingIP(uuid);
        //if floating_ip_address isn't there, allocate from the subnet pool
        OpenStackSubnets subnet = getSubnet(getNetwork(floatIP.getFloatingNetworkUUID()).getSubnets().get(0));
        subnet.releaseIP(floatIP.getFloatingIPAddress());
        if (floatIP.getPortUUID() != null) {
            OpenStackPorts port = getPort(floatIP.getPortUUID());
            port.removeFloatingIP(floatIP.getFixedIPAddress());
        }
        floatingIPDB.remove(uuid);
        return true;
    }

    @Override
    public boolean updateFloatingIP(String uuid, OpenStackFloatingIPs delta) {
        if (!floatingIPExists(uuid))
            return false;
        OpenStackFloatingIPs target = floatingIPDB.get(uuid);
        if (target.getPortUUID() != null) {
            OpenStackPorts port = getPort(target.getPortUUID());
            port.removeFloatingIP(target.getFixedIPAddress());
        }

        //if port_id is there, bind port to this floating ip
        if (delta.getPortUUID() != null) {
            OpenStackPorts port = getPort(delta.getPortUUID());
            port.addFloatingIP(delta.getFixedIPAddress(), delta);
        }

        target.setPortUUID(delta.getPortUUID());
        target.setFixedIPAddress(delta.getFixedIPAddress());
        return true;
    }

    // IfNBRouterCRUD Interface methods

    @Override
    public boolean routerExists(String uuid) {
        return routerDB.containsKey(uuid);
    }

    @Override
    public OpenStackRouters getRouter(String uuid) {
        if (!routerExists(uuid))
            return null;
        return routerDB.get(uuid);
    }

    @Override
    public List<OpenStackRouters> getAllRouters() {
        Set<OpenStackRouters> allRouters = new HashSet<OpenStackRouters>();
        for (Entry<String, OpenStackRouters> entry : routerDB.entrySet()) {
            OpenStackRouters router = entry.getValue();
            allRouters.add(router);
        }
        logger.debug("Exiting getAllRouters, Found {} Routers", allRouters.size());
        List<OpenStackRouters> ans = new ArrayList<OpenStackRouters>();
        ans.addAll(allRouters);
        return ans;
    }

    @Override
    public boolean addRouter(OpenStackRouters input) {
        if (routerExists(input.getID()))
            return false;
        routerDB.putIfAbsent(input.getID(), input);
        return true;
    }

    @Override
    public boolean removeRouter(String uuid) {
        if (!routerExists(uuid))
            return false;
        routerDB.remove(uuid);
        return true;
    }

    @Override
    public boolean updateRouter(String uuid, OpenStackRouters delta) {
        if (!routerExists(uuid))
            return false;
        OpenStackRouters target = routerDB.get(uuid);
        return overwrite(target, delta);
    }

    @Override
    public boolean routerInUse(String routerUUID) {
        if (!routerExists(routerUUID))
            return true;
        OpenStackRouters target = routerDB.get(routerUUID);
        return (target.getInterfaces().size() > 0);
    }

    // IfNBPortCRUD methods

    @Override
    public boolean portExists(String uuid) {
        return portDB.containsKey(uuid);
    }

    @Override
    public OpenStackPorts getPort(String uuid) {
        if (!portExists(uuid))
            return null;
        return portDB.get(uuid);
    }

    @Override
    public List<OpenStackPorts> getAllPorts() {
        Set<OpenStackPorts> allPorts = new HashSet<OpenStackPorts>();
        for (Entry<String, OpenStackPorts> entry : portDB.entrySet()) {
            OpenStackPorts port = entry.getValue();
            allPorts.add(port);
        }
        logger.debug("Exiting getAllPorts, Found {} OpenStackPorts", allPorts.size());
        List<OpenStackPorts> ans = new ArrayList<OpenStackPorts>();
        ans.addAll(allPorts);
        return ans;
    }

    @Override
    public boolean addPort(OpenStackPorts input) {
        if (portExists(input.getID()))
            return false;
        portDB.putIfAbsent(input.getID(), input);
        // if there are no fixed IPs, allocate one for each subnet in the network
        if (input.getFixedIPs().size() == 0) {
            List<OpenStackIPs> list = input.getFixedIPs();
            Iterator<OpenStackSubnets> subnetIterator = getAllSubnets().iterator();
            while (subnetIterator.hasNext()) {
                OpenStackSubnets subnet = subnetIterator.next();
                if (subnet.getNetworkUUID().equals(input.getNetworkUUID()))
                    list.add(new OpenStackIPs(subnet.getID()));
            }
        }
        Iterator<OpenStackIPs> fixedIPIterator = input.getFixedIPs().iterator();
        while (fixedIPIterator.hasNext()) {
            OpenStackIPs ip = fixedIPIterator.next();
            OpenStackSubnets subnet = getSubnet(ip.getSubnetUUID());
            if (ip.getIpAddress() == null)
                ip.setIpAddress(subnet.getLowAddr());
            if (!ip.getIpAddress().equals(subnet.getGatewayIP()))
                subnet.allocateIP(ip.getIpAddress());
        }
        OpenStackNetworks network = getNetwork(input.getNetworkUUID());
        network.addPort(input);
        return true;
    }

    @Override
    public boolean removePort(String uuid) {
        if (!portExists(uuid))
            return false;
        OpenStackPorts port = getPort(uuid);
        portDB.remove(uuid);
        OpenStackNetworks network = getNetwork(port.getNetworkUUID());
        network.removePort(port);
        Iterator<OpenStackIPs> fixedIPIterator = port.getFixedIPs().iterator();
        while (fixedIPIterator.hasNext()) {
            OpenStackIPs ip = fixedIPIterator.next();
            OpenStackSubnets subnet = getSubnet(ip.getSubnetUUID());
            if (!ip.getIpAddress().equals(subnet.getGatewayIP()))
                subnet.releaseIP(ip.getIpAddress());
        }
        return true;
    }

    @Override
    public boolean updatePort(String uuid, OpenStackPorts delta) {
        if (!portExists(uuid))
            return false;
        OpenStackPorts target = portDB.get(uuid);
        // remove old Fixed_IPs
        OpenStackPorts port = getPort(uuid);
        Iterator<OpenStackIPs> fixedIPIterator = port.getFixedIPs().iterator();
        while (fixedIPIterator.hasNext()) {
            OpenStackIPs ip = fixedIPIterator.next();
            OpenStackSubnets subnet = getSubnet(ip.getSubnetUUID());
            subnet.releaseIP(ip.getIpAddress());
        }

        // allocate new Fixed_IPs
        fixedIPIterator = delta.getFixedIPs().iterator();
        while (fixedIPIterator.hasNext()) {
            OpenStackIPs ip = fixedIPIterator.next();
            OpenStackSubnets subnet = getSubnet(ip.getSubnetUUID());
            if (ip.getIpAddress() == null)
                ip.setIpAddress(subnet.getLowAddr());
            subnet.allocateIP(ip.getIpAddress());
        }
        return overwrite(target, delta);
    }

    @Override
    public boolean macInUse(String macAddress) {
        List<OpenStackPorts> ports = getAllPorts();
        Iterator<OpenStackPorts> portIterator = ports.iterator();
        while (portIterator.hasNext()) {
            OpenStackPorts port = portIterator.next();
            if (macAddress.equalsIgnoreCase(port.getMacAddress()))
                return true;
        }
        return false;
    }

    @Override
    public OpenStackPorts getGatewayPort(String subnetUUID) {
        OpenStackSubnets subnet = getSubnet(subnetUUID);
        Iterator<OpenStackPorts> portIterator = getAllPorts().iterator();
        while (portIterator.hasNext()) {
            OpenStackPorts port = portIterator.next();
            List<OpenStackIPs> fixedIPs = port.getFixedIPs();
            if (fixedIPs.size() == 1) {
                if (subnet.getGatewayIP().equals(fixedIPs.get(0).getIpAddress()))
                    return port;
            }
        }
        return null;
    }

    // IfNBSubnetCRUD methods

    @Override
    public boolean subnetExists(String uuid) {
        return subnetDB.containsKey(uuid);
    }

    @Override
    public OpenStackSubnets getSubnet(String uuid) {
        if (!subnetExists(uuid))
            return null;
        return subnetDB.get(uuid);
    }

    @Override
    public List<OpenStackSubnets> getAllSubnets() {
        Set<OpenStackSubnets> allSubnets = new HashSet<OpenStackSubnets>();
        for (Entry<String, OpenStackSubnets> entry : subnetDB.entrySet()) {
            OpenStackSubnets subnet = entry.getValue();
            allSubnets.add(subnet);
        }
        logger.debug("Exiting getAllSubnets, Found {} OpenStackSubnets", allSubnets.size());
        List<OpenStackSubnets> ans = new ArrayList<OpenStackSubnets>();
        ans.addAll(allSubnets);
        return ans;
    }

    @Override
    public boolean addSubnet(OpenStackSubnets input) {
        String id = input.getID();
        if (subnetExists(id))
            return false;
        subnetDB.putIfAbsent(id, input);
        OpenStackNetworks targetNet = networkDB.get(input.getNetworkUUID());
        targetNet.addSubnet(id);
        return true;
    }

    @Override
    public boolean removeSubnet(String uuid) {
        if (!subnetExists(uuid))
            return false;
        OpenStackSubnets target = subnetDB.get(uuid);
        OpenStackNetworks targetNet = networkDB.get(target.getNetworkUUID());
        targetNet.removeSubnet(uuid);
        subnetDB.remove(uuid);
        return true;
    }

    @Override
    public boolean updateSubnet(String uuid, OpenStackSubnets delta) {
        if (!subnetExists(uuid))
            return false;
        OpenStackSubnets target = subnetDB.get(uuid);
        return overwrite(target, delta);
    }

    @Override
    public boolean subnetInUse(String subnetUUID) {
        if (!subnetExists(subnetUUID))
            return true;
        OpenStackSubnets target = subnetDB.get(subnetUUID);
        return (target.getPortsInSubnet().size() > 0);
    }

    // IfNBNetworkCRUD methods

    @Override
    public boolean networkExists(String uuid) {
        return networkDB.containsKey(uuid);
    }

    @Override
    public OpenStackNetworks getNetwork(String uuid) {
        if (!networkExists(uuid))
            return null;
        return networkDB.get(uuid);
    }

    @Override
    public List<OpenStackNetworks> getAllNetworks() {
        Set<OpenStackNetworks> allNetworks = new HashSet<OpenStackNetworks>();
        for (Entry<String, OpenStackNetworks> entry : networkDB.entrySet()) {
            OpenStackNetworks network = entry.getValue();
            allNetworks.add(network);
        }
        logger.debug("Exiting getAllNetworks, Found {} OpenStackNetworks", allNetworks.size());
        List<OpenStackNetworks> ans = new ArrayList<OpenStackNetworks>();
        ans.addAll(allNetworks);
        return ans;
    }

    @Override
    public boolean addNetwork(OpenStackNetworks input) {
        if (networkExists(input.getID()))
            return false;
        networkDB.putIfAbsent(input.getID(), input);
        return true;
    }

    @Override
    public boolean removeNetwork(String uuid) {
        if (!networkExists(uuid))
            return false;
        networkDB.remove(uuid);
        return true;
    }

    @Override
    public boolean updateNetwork(String uuid, OpenStackNetworks delta) {
        if (!networkExists(uuid))
            return false;
        OpenStackNetworks target = networkDB.get(uuid);
        return overwrite(target, delta);
    }

    @Override
    public boolean networkInUse(String netUUID) {
        if (!networkExists(netUUID))
            return true;
        OpenStackNetworks target = networkDB.get(netUUID);
        return (target.getPortsOnNetwork().size() > 0);
    }
}
