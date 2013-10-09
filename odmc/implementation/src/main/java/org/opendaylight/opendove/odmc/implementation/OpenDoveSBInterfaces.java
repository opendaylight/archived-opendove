/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.opendove.odmc.implementation;

import java.util.ArrayList;
import java.util.Dictionary;
import java.util.EnumSet;
import java.util.Iterator;
import java.util.List;
import java.util.concurrent.ConcurrentMap;

import org.apache.felix.dm.Component;
import org.opendaylight.controller.clustering.services.CacheConfigException;
import org.opendaylight.controller.clustering.services.CacheExistException;
import org.opendaylight.controller.clustering.services.IClusterContainerServices;
import org.opendaylight.controller.clustering.services.IClusterServices;
import org.opendaylight.opendove.odmc.IfOpenDCSTrackedObject;
import org.opendaylight.opendove.odmc.IfOpenDGWTrackedObject;
import org.opendaylight.opendove.odmc.IfSBDoveDomainCRU;
import org.opendaylight.opendove.odmc.IfSBDoveEGWFwdRuleCRUD;
import org.opendaylight.opendove.odmc.IfSBDoveGwIpv4CRUD;
import org.opendaylight.opendove.odmc.IfSBDoveEGWSNATPoolCRUD;
import org.opendaylight.opendove.odmc.IfSBDoveNetworkCRU;
import org.opendaylight.opendove.odmc.IfSBDoveNetworkSubnetAssociationCRUD;
import org.opendaylight.opendove.odmc.IfSBDovePolicyCRUD;
import org.opendaylight.opendove.odmc.IfSBDoveSubnetCRUD;
import org.opendaylight.opendove.odmc.IfSBDoveVGWVNIDMappingCRUD;
import org.opendaylight.opendove.odmc.IfSBOpenDoveChangeVersionR;
import org.opendaylight.opendove.odmc.OpenDoveChange;
import org.opendaylight.opendove.odmc.OpenDoveConcurrentBackedMap;
import org.opendaylight.opendove.odmc.OpenDoveDomain;
import org.opendaylight.opendove.odmc.OpenDoveEGWFwdRule;
import org.opendaylight.opendove.odmc.OpenDoveEGWSNATPool;
import org.opendaylight.opendove.odmc.OpenDoveGwIpv4;
import org.opendaylight.opendove.odmc.OpenDoveNetwork;
import org.opendaylight.opendove.odmc.OpenDoveNetworkSubnetAssociation;
import org.opendaylight.opendove.odmc.OpenDoveObject;
import org.opendaylight.opendove.odmc.OpenDovePolicy;
import org.opendaylight.opendove.odmc.OpenDoveSubnet;
import org.opendaylight.opendove.odmc.OpenDoveVGWVNIDMapping;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class OpenDoveSBInterfaces implements IfSBDoveDomainCRU, IfSBDoveNetworkCRU, IfSBDoveSubnetCRUD,
    IfSBDovePolicyCRUD, IfSBDoveGwIpv4CRUD, IfSBDoveEGWSNATPoolCRUD, IfSBDoveEGWFwdRuleCRUD, IfSBDoveVGWVNIDMappingCRUD,
    IfSBOpenDoveChangeVersionR, IfSBDoveNetworkSubnetAssociationCRUD {
    private static final Logger logger = LoggerFactory.getLogger(OpenDoveSBInterfaces.class);
    private String containerName = null;

    private IClusterContainerServices clusterContainerService = null;
    private OpenDoveConcurrentBackedMap domainMap, networkMap, subnetMap, networkSubnetAssociationMap;
    private OpenDoveConcurrentBackedMap policyMap, gwIpv4Map, egwSNATPoolMap, egwFwdRuleMap, vgwVNIDMap;
    private ConcurrentMap<Integer, OpenDoveObject> objectDB;

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
            // DOVE caches
//            this.clusterContainerService.createCache("openDoveDomains",
//                   EnumSet.of(IClusterServices.cacheMode.NON_TRANSACTIONAL));
//            this.clusterContainerService.createCache("openDoveNetworks",
//                    EnumSet.of(IClusterServices.cacheMode.NON_TRANSACTIONAL));
//            this.clusterContainerService.createCache("openDoveSubnets",
//                    EnumSet.of(IClusterServices.cacheMode.NON_TRANSACTIONAL));
          this.clusterContainerService.createCache("openDoveObjects",
                  EnumSet.of(IClusterServices.cacheMode.NON_TRANSACTIONAL));
        } catch (CacheConfigException cce) {
            logger.error("Southbound Caches couldn't be created for OpenDOVE -  check cache mode");
        } catch (CacheExistException cce) {
            logger.error("Cache for OpenDOVE already exists, destroy and recreate");
        }
        logger.debug("Southbound Caches successfully created for OpenDOVE");
    }

    @SuppressWarnings({ "unchecked", "deprecation" })
    private void retrieveCache() {
        if (this.clusterContainerService == null) {
            logger.error("un-initialized clusterContainerService, can't retrieve cache");
            return;
        }
/*        logger.debug("Retrieving cache for openDoveDomains");
        domainDB = (ConcurrentMap<String, OpenDoveDomain>) this.clusterContainerService
                .getCache("openDoveDomains");
        if (domainDB == null) {
            logger.error("Cache couldn't be retrieved for openDoveDomains");
        }
        logger.debug("Cache was successfully retrieved for openDoveDomains");

        logger.debug("Retrieving cache for openDoveNetworks");
        networkDB = (ConcurrentMap<String, OpenDoveNetwork>) this.clusterContainerService
                .getCache("openDoveNetworks");
        if (networkDB == null) {
            logger.error("Cache couldn't be retrieved for openDoveNetworks");
        }
        logger.debug("Cache was successfully retrieved for openDoveNetworks");

        logger.debug("Retrieving cache for openDoveSubnets");
        subnetDB = (ConcurrentMap<String, OpenDoveSubnet>) this.clusterContainerService
                .getCache("openDoveSubnets");
        if (subnetDB == null) {
            logger.error("Cache couldn't be retrieved for openDoveSubnets");
        }
        logger.debug("Cache was successfully retrieved for openDoveSubnets");
        */
        logger.debug("Retrieving cache for openDoveObjects");
        objectDB = (ConcurrentMap<Integer, OpenDoveObject>) this.clusterContainerService
                .getCache("openDoveObjects");
        if (objectDB == null) {
            logger.error("Cache couldn't be retrieved for openDoveSubnets");
        }
        domainMap = new OpenDoveConcurrentBackedMap(objectDB, OpenDoveDomain.class);
        networkMap = new OpenDoveConcurrentBackedMap(objectDB, OpenDoveNetwork.class);
        subnetMap = new OpenDoveConcurrentBackedMap(objectDB, OpenDoveSubnet.class);
        policyMap = new OpenDoveConcurrentBackedMap(objectDB, OpenDovePolicy.class);
        networkSubnetAssociationMap = new OpenDoveConcurrentBackedMap(objectDB, OpenDoveNetworkSubnetAssociation.class);
        egwSNATPoolMap = new OpenDoveConcurrentBackedMap(objectDB, OpenDoveEGWSNATPool.class);
        gwIpv4Map = new OpenDoveConcurrentBackedMap(objectDB, OpenDoveGwIpv4.class);
        egwFwdRuleMap = new OpenDoveConcurrentBackedMap(objectDB, OpenDoveEGWFwdRule.class);
        vgwVNIDMap = new OpenDoveConcurrentBackedMap(objectDB, OpenDoveVGWVNIDMapping.class);
        logger.debug("Cache was successfully retrieved for openDoveSubnets");
    }

    @SuppressWarnings("deprecation")
    private void destroyCache() {
        if (this.clusterContainerService == null) {
            logger.error("un-initialized clusterMger, can't destroy cache");
            return;
        }
        logger.debug("Destroying Caches for OpenDove");
//        this.clusterContainerService.destroyCache("openDoveDomains");
//        this.clusterContainerService.destroyCache("openDoveNetworks");
//        this.clusterContainerService.destroyCache("openDoveSubnets");
        this.clusterContainerService.destroyCache("openDoveObjects");
    }

    private void startUp() {
        allocateCache();
        retrieveCache();
        OpenDoveNetwork.initRNG();
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

    // code to support SB domain interfaces (including URI)

    public boolean domainExists(String domainUUID) {
        return(domainMap.containsKey(domainUUID));
    }

    public boolean domainExistsByName(String name) {
        Iterator<OpenDoveObject> i = domainMap.values().iterator();
        while (i.hasNext()) {
            OpenDoveDomain d = (OpenDoveDomain) i.next();
            if (d.getName().compareTo(name) == 0)
                return true;
        }
        return false;
    }

    public OpenDoveDomain getDomain(String domainUUID) {
        return(OpenDoveDomain) (domainMap.get(domainUUID));
    }

    public OpenDoveDomain getDomainByName(String name) {
        Iterator<OpenDoveObject> i = domainMap.values().iterator();
        while (i.hasNext()) {
            OpenDoveDomain d = (OpenDoveDomain) i.next();
            if (d.getName().compareTo(name) == 0)
                return d;
        }
        return null;
    }

    public void addDomain(String domainUUID, OpenDoveDomain domain) {
        domainMap.putIfAbsent(domainUUID, domain);
    }

    public void addNetworkToDomain(String domainUUID, OpenDoveNetwork network) {
        if (domainExists(domainUUID)) {
            OpenDoveDomain domain = (OpenDoveDomain) domainMap.get(domainUUID);
            domain.addNetwork(network);
        }
    }

    public List<OpenDoveDomain> getDomains() {
        List<OpenDoveDomain> answer = new ArrayList<OpenDoveDomain>();
        Iterator<OpenDoveObject> i = domainMap.values().iterator();
        while (i.hasNext()) {
            answer.add((OpenDoveDomain) i.next());
        }
        return answer;
    }

    // code to support SB network interfaces (including URI)

    public boolean networkExists(String networkUUID) {
        return(networkMap.containsKey(networkUUID));
    }

    public boolean networkExistsByVnid(int vnid) {
        Iterator<OpenDoveObject> i = networkMap.values().iterator();
        while (i.hasNext()) {
            OpenDoveNetwork n = (OpenDoveNetwork) i.next();
            if (n.getVnid() == vnid)
                return true;
        }
        return false;
    }

    public OpenDoveNetwork getNetwork(String networkUUID) {
        return (OpenDoveNetwork) (networkMap.get(networkUUID));
    }

    public OpenDoveNetwork getNetworkByVnid(int vnid) {
        Iterator<OpenDoveObject> i = networkMap.values().iterator();
        while (i.hasNext()) {
            OpenDoveNetwork n = (OpenDoveNetwork) i.next();
            if (n.getVnid() == vnid)
                return n;
        }
        return null;
    }

    public OpenDoveNetwork getNetworkByName(String name) {
        Iterator<OpenDoveObject> i = networkMap.values().iterator();
        while (i.hasNext()) {
            OpenDoveNetwork n = (OpenDoveNetwork) i.next();
            if (n.getName().equals(name))
                return n;
        }
        return null;
    }

    public void addNetwork(String networkUUID, OpenDoveNetwork network) {
        networkMap.putIfAbsent(networkUUID, network);
        addNetworkToDomain(network.getDomain_uuid(), network);
    }

    public int allocateVNID() {
        boolean done = false;
        while (!done) {
            long candidateVNID = OpenDoveNetwork.getNext() & 0x0000000000FFFFFF;
            if (!networkExistsByVnid((int) candidateVNID))
                return (int) candidateVNID;
        }
        return 0;
    }

    public List<OpenDoveNetwork> getNetworks() {
        List<OpenDoveNetwork> answer = new ArrayList<OpenDoveNetwork>();
        Iterator<OpenDoveObject> i = networkMap.values().iterator();
        while (i.hasNext()) {
            answer.add((OpenDoveNetwork) i.next());
        }
        return answer;
    }

    // code to support SB subnet interfaces (including URI)

    public boolean subnetExists(String subnetUUID) {
        return(subnetMap.containsKey(subnetUUID));
    }

    public OpenDoveSubnet getSubnet(String subnetUUID) {
        return (OpenDoveSubnet) (subnetMap.get(subnetUUID));
    }

    public void addSubnet(String subnetUUID, OpenDoveSubnet subnet) {
        subnetMap.putIfAbsent(subnetUUID, subnet);
    }

    public List<OpenDoveSubnet> getSubnets() {
        List<OpenDoveSubnet> answer = new ArrayList<OpenDoveSubnet>();
        Iterator<OpenDoveObject> i = subnetMap.values().iterator();
        while (i.hasNext()) {
            answer.add((OpenDoveSubnet) i.next());
        }
        return answer;
    }

    public void removeSubnet(String subnetUUID) {
        subnetMap.remove(subnetUUID);
    }

    public int versionExists(int version) {
        List<Integer> orderedKeys = OpenDoveConcurrentBackedMap.getOrderedBackingKeys(objectDB);
        Iterator<Integer> iterator = orderedKeys.iterator();
        while (iterator.hasNext()) {
            Integer i = iterator.next();
            if (i >= version)
                return 200;
        }
        return 204;
    }

    public OpenDoveChange getNextOdgwChange(int version) {
        OpenDoveObject currentChange = objectDB.get(version);
        OpenDoveChange ans = new OpenDoveChange();
        if (currentChange == null) {
            ans.setMethod("");
            ans.setUri("");
        } else {
            try {
                if (currentChange.getTombstoneFlag())
                    ans.setMethod("DELETE");
                else
                    ans.setMethod("GET");
                IfOpenDGWTrackedObject t = (IfOpenDGWTrackedObject) currentChange;
                ans.setUri(t.getSBDgwUri());
            } catch (Exception e) {
                ans.setMethod("");
                ans.setUri("");
            }
        }
        int changeIndex = currentChange.getLastChangeVersion();
        List<Integer> orderedKeys = OpenDoveConcurrentBackedMap.getOrderedBackingKeys(objectDB);
        Iterator<Integer> iterator = orderedKeys.iterator();
        int lastChangeSeen = changeIndex;
        while (iterator.hasNext()) {
            Integer i = iterator.next();
            if (i > changeIndex) {
                OpenDoveObject test = objectDB.get(i);
                lastChangeSeen = i;
                try {
                    IfOpenDGWTrackedObject t = (IfOpenDGWTrackedObject) test;
                    ans.setNextChange(i);
                    return ans;
                } catch (Exception e) {
                    ;
                }
            }
        }
        ans.setNextChange(lastChangeSeen+1);
        return ans;
    }

    public OpenDoveChange getNextOdcsChange(int version) {
        OpenDoveObject currentChange = objectDB.get(version);
        OpenDoveChange ans = new OpenDoveChange();
        if (currentChange == null) {
            ans.setMethod("");
            ans.setUri("");
        } else {
            try {
                if (currentChange.getTombstoneFlag())
                    ans.setMethod("DELETE");
                else
                    ans.setMethod("GET");
                IfOpenDCSTrackedObject t = (IfOpenDCSTrackedObject) currentChange;
                ans.setUri(t.getSBDcsUri());
            } catch (Exception e) {
                ans.setMethod("");
                ans.setUri("");
            }
        }
        int changeIndex = currentChange.getLastChangeVersion();
        List<Integer> orderedKeys = OpenDoveConcurrentBackedMap.getOrderedBackingKeys(objectDB);
        Iterator<Integer> iterator = orderedKeys.iterator();
        int lastChangeSeen = changeIndex;
        while (iterator.hasNext()) {
            Integer i = iterator.next();
            if (i > changeIndex) {
                OpenDoveObject test = objectDB.get(i);
                lastChangeSeen = i;
                try {
                    IfOpenDCSTrackedObject t = (IfOpenDCSTrackedObject) test;
                    ans.setNextChange(i);
                    return ans;
                } catch (Exception e) {
                    ;
                }
            }
        }
        ans.setNextChange(lastChangeSeen+1);
        return ans;
    }

    public boolean associationExists(String uuid) {
        return(networkSubnetAssociationMap.containsKey(uuid));
    }

    public OpenDoveNetworkSubnetAssociation getAssociation(String uuid) {
        if (networkSubnetAssociationMap.containsKey(uuid))
            return (OpenDoveNetworkSubnetAssociation) networkSubnetAssociationMap.get(uuid);
        return null;
    }

    public void addNetworkSubnetAssociation(OpenDoveNetworkSubnetAssociation association) {
        networkSubnetAssociationMap.putIfAbsent(association.getUUID(), association);
    }

    public List<OpenDoveNetworkSubnetAssociation> getAssociations() {
        List<OpenDoveNetworkSubnetAssociation> answer = new ArrayList<OpenDoveNetworkSubnetAssociation>();
        Iterator<OpenDoveObject> i = networkSubnetAssociationMap.values().iterator();
        while (i.hasNext()) {
            answer.add((OpenDoveNetworkSubnetAssociation) i.next());
        }
        return answer;
    }

    public void removeNetworkSubnetAssociation(String uuid) {
        networkSubnetAssociationMap.remove(uuid);
    }

    // implementation of IfSBDovePolicyCRUD methods

    public boolean policyExists(String policyUUID) {
        return(policyMap.containsKey(policyUUID));
    }

    public OpenDovePolicy getPolicy(String policyUUID) {
        return (OpenDovePolicy) (policyMap.get(policyUUID));
    }

    public void addPolicy(String policyUUID, OpenDovePolicy policy) {
        policyMap.putIfAbsent(policyUUID, policy);
    }

    public List<OpenDovePolicy> getPolicies() {
        List<OpenDovePolicy> answer = new ArrayList<OpenDovePolicy>();
        Iterator<OpenDoveObject> i = policyMap.values().iterator();
        while (i.hasNext()) {
            answer.add((OpenDovePolicy) i.next());
        }
        return answer;
    }

    public void removePolicy(String policyUUID) {
        policyMap.remove(policyUUID);
    }

    public void updatePolicy(OpenDovePolicy policy) {
        policyMap.update(policy.getUUID(), policy);
    }

    // IfSBDoveEGWSNATPool CRUD Methods

    public boolean egwSNATPoolExists(String poolUUID) {
        return(egwSNATPoolMap.containsKey(poolUUID));
    }

    public OpenDoveEGWSNATPool getEgwSNATPool(String poolUUID) {
        return (OpenDoveEGWSNATPool) (egwSNATPoolMap.get(poolUUID));
    }

    public void addEgwSNATPool(String poolUUID, OpenDoveEGWSNATPool pool) {
        egwSNATPoolMap.putIfAbsent(poolUUID, pool);
    }

    public List<OpenDoveEGWSNATPool> getEgwSNATPools() {
        List<OpenDoveEGWSNATPool> answer = new ArrayList<OpenDoveEGWSNATPool>();
        Iterator<OpenDoveObject> i = egwSNATPoolMap.values().iterator();
        while (i.hasNext()) {
            answer.add((OpenDoveEGWSNATPool) i.next());
        }
        return answer;
    }

    public void updateSNATPool(String key, OpenDoveEGWSNATPool pool) {
        egwSNATPoolMap.update(key, pool);
    }

    public void removeEgwSNATPool(String poolUUID) {
        egwSNATPoolMap.remove(poolUUID);
    }

    // IfSBDoveGwIpv4 CRUD Methods

    public boolean gwIpv4Exists(String ipv4UUID) {
        return(gwIpv4Map.containsKey(ipv4UUID));
    }

    public OpenDoveGwIpv4 getGwIpv4(String ipv4UUID) {
        return (OpenDoveGwIpv4) (gwIpv4Map.get(ipv4UUID));
    }

    public void addGwIpv4(String ipv4UUID, OpenDoveGwIpv4 ipv4) {
        gwIpv4Map.putIfAbsent(ipv4UUID, ipv4);
    }

    public List<OpenDoveGwIpv4> getGwIpv4Pool() {
        List<OpenDoveGwIpv4> answer = new ArrayList<OpenDoveGwIpv4>();
        Iterator<OpenDoveObject> i = gwIpv4Map.values().iterator();
        while (i.hasNext()) {
            answer.add((OpenDoveGwIpv4) i.next());
        }
        return answer;
    }

    public void removeGwIpv4(String ipv4UUID) {
        gwIpv4Map.remove(ipv4UUID);
    }

    // Implementation of IfSBDoveEGWFwdRuleCRUD methods

    public boolean egwFwdRuleExists(String ruleUUID) {
        return(egwFwdRuleMap.containsKey(ruleUUID));
    }

    public OpenDoveEGWFwdRule getEgwFwdRule(String ruleUUID) {
        return (OpenDoveEGWFwdRule) (egwFwdRuleMap.get(ruleUUID));
    }

    public void addEgwFwdRule(String ruleUUID, OpenDoveEGWFwdRule rule) {
        egwFwdRuleMap.putIfAbsent(ruleUUID, rule);
    }

    public List<OpenDoveEGWFwdRule> getEgwFwdRules() {
        List<OpenDoveEGWFwdRule> answer = new ArrayList<OpenDoveEGWFwdRule>();
        Iterator<OpenDoveObject> i = egwFwdRuleMap.values().iterator();
        while (i.hasNext()) {
            answer.add((OpenDoveEGWFwdRule) i.next());
        }
        return answer;
    }

    public void removeEgwFwdRule(String ruleUUID) {
        egwFwdRuleMap.remove(ruleUUID);
    }

    public void updateEgwFwdRule(String ruleUUID, OpenDoveEGWFwdRule rule) {
        egwFwdRuleMap.update(ruleUUID, rule);
    }

    // IfSBDoveVGWVNIDMappingCRUD methods

    public boolean vgwVNIDMappingExists(String mappingUUID) {
        return(vgwVNIDMap.containsKey(mappingUUID));
    }

    public OpenDoveVGWVNIDMapping getVgwVNIDMapping(String mappingUUID) {
        return (OpenDoveVGWVNIDMapping) (vgwVNIDMap.get(mappingUUID));
    }

    public void addVgwVNIDMapping(String mappingUUID,
            OpenDoveVGWVNIDMapping mapping) {
        vgwVNIDMap.putIfAbsent(mappingUUID, mapping);
    }

    public List<OpenDoveVGWVNIDMapping> getVgwVNIDMappings() {
        List<OpenDoveVGWVNIDMapping> answer = new ArrayList<OpenDoveVGWVNIDMapping>();
        Iterator<OpenDoveObject> i = vgwVNIDMap.values().iterator();
        while (i.hasNext()) {
            answer.add((OpenDoveVGWVNIDMapping) i.next());
        }
        return answer;
    }

    public void removeVgwVNIDMapping(String mappingUUID) {
        vgwVNIDMap.remove(mappingUUID);
    }
}
