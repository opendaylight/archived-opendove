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
import java.util.Random;
import java.util.concurrent.ConcurrentMap;

import org.apache.felix.dm.Component;
import org.opendaylight.controller.clustering.services.CacheConfigException;
import org.opendaylight.controller.clustering.services.CacheExistException;
import org.opendaylight.controller.clustering.services.IClusterContainerServices;
import org.opendaylight.controller.clustering.services.IClusterServices;
import org.opendaylight.opendove.odmc.IfOpenDCSTrackedObject;
import org.opendaylight.opendove.odmc.IfOpenDGWTrackedObject;
import org.opendaylight.opendove.odmc.IfSBDoveDomainCRU;
import org.opendaylight.opendove.odmc.IfSBDoveNetworkCRU;
import org.opendaylight.opendove.odmc.IfSBDoveNetworkSubnetAssociationCRUD;
import org.opendaylight.opendove.odmc.IfSBDoveSubnetCRUD;
import org.opendaylight.opendove.odmc.IfSBOpenDoveChangeVersionR;
import org.opendaylight.opendove.odmc.OpenDoveChange;
import org.opendaylight.opendove.odmc.OpenDoveConcurrentBackedMap;
import org.opendaylight.opendove.odmc.OpenDoveDomain;
import org.opendaylight.opendove.odmc.OpenDoveNetwork;
import org.opendaylight.opendove.odmc.OpenDoveNetworkSubnetAssociation;
import org.opendaylight.opendove.odmc.OpenDoveObject;
import org.opendaylight.opendove.odmc.OpenDoveSubnet;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class OpenDoveSBInterfaces implements IfSBDoveDomainCRU, IfSBDoveNetworkCRU, IfSBDoveSubnetCRUD,
	IfSBOpenDoveChangeVersionR, IfSBDoveNetworkSubnetAssociationCRUD {
    private static final Logger logger = LoggerFactory.getLogger(OpenDoveSBInterfaces.class);
    private String containerName = null;

    private IClusterContainerServices clusterContainerService = null;
    private OpenDoveConcurrentBackedMap domainMap, networkMap, subnetMap, networkSubnetAssociationMap;
    private ConcurrentMap<Integer, OpenDoveObject> objectDB;

    private static Random rng;

    private static void initRNG() {
        rng = new Random();    //TODO: need to seed this better
    }

    public static long getNext() {
        return rng.nextLong();
    }

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
        networkSubnetAssociationMap = new OpenDoveConcurrentBackedMap(objectDB, OpenDoveNetworkSubnetAssociation.class);
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
        OpenDoveSBInterfaces.initRNG();
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
            long candidateVNID = OpenDoveSBInterfaces.getNext() & 0x0000000000FFFFFF;
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

	public boolean versionExists(int version) {
		return objectDB.containsKey(version);
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
		boolean loop = true;
		while (loop) {
			changeIndex++;
			if (!objectDB.containsKey(changeIndex)) {
				ans.setNextChange(changeIndex);
				loop = false;
			} else {
				OpenDoveObject test = objectDB.get(changeIndex);
				try {
					IfOpenDGWTrackedObject t = (IfOpenDGWTrackedObject) test;
					ans.setNextChange(changeIndex);
					loop = false;
				} catch (Exception e) {
					;
				}
			}
		}
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
		boolean loop = true;
		while (loop) {
			changeIndex++;
			if (!objectDB.containsKey(changeIndex)) {
				ans.setNextChange(changeIndex);
				loop = false;
			} else {
				OpenDoveObject test = objectDB.get(changeIndex);
				try {
					IfOpenDCSTrackedObject t = (IfOpenDCSTrackedObject) test;
					ans.setNextChange(changeIndex);
					loop = false;
				} catch (Exception e) {
					;
				}
			}
		}
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
}
