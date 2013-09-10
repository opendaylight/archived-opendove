/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.opendove.odmc.internal;

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
import org.opendaylight.opendove.odmc.IfSBDoveDomainCRU;
import org.opendaylight.opendove.odmc.IfSBDoveNetworkCRU;
import org.opendaylight.opendove.odmc.OpenDoveDomain;
import org.opendaylight.opendove.odmc.OpenDoveNetwork;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class OpenDoveSBInterfaces implements IfSBDoveDomainCRU, IfSBDoveNetworkCRU {
    private static final Logger logger = LoggerFactory
            .getLogger(OpenDoveNeutronNBInterfaces.class);
    private String containerName = null;

    private IClusterContainerServices clusterContainerService = null;
    private ConcurrentMap<String, OpenDoveDomain> domainDB;
    private ConcurrentMap<String, OpenDoveNetwork> networkDB;

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
            this.clusterContainerService.createCache("openDoveDomains",
                    EnumSet.of(IClusterServices.cacheMode.NON_TRANSACTIONAL));
            this.clusterContainerService.createCache("openDoveNetworks",
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
        logger.debug("Retrieving cache for openDoveDomains");
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
            logger.error("Cache couldn't be retrieved for openDoveDomains");
        }
        logger.debug("Cache was successfully retrieved for openDoveDomains");
    }

    @SuppressWarnings("deprecation")
    private void destroyCache() {
        if (this.clusterContainerService == null) {
            logger.error("un-initialized clusterMger, can't destroy cache");
            return;
        }
        logger.debug("Destroying Caches for OpenDove");
        this.clusterContainerService.destroyCache("openDoveDomains");
        this.clusterContainerService.destroyCache("openDoveNetworks");
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
        return(domainDB.containsKey(domainUUID));
    }

    public boolean domainExistsByName(String name) {
        Iterator<OpenDoveDomain> i = domainDB.values().iterator();
        while (i.hasNext()) {
            OpenDoveDomain d = i.next();
            if (d.getName().compareTo(name) == 0)
                return true;
        }
        return false;
    }

    public OpenDoveDomain getDomain(String domainUUID) {
        return(domainDB.get(domainUUID));
    }

    public OpenDoveDomain getDomainByName(String name) {
        Iterator<OpenDoveDomain> i = domainDB.values().iterator();
        while (i.hasNext()) {
            OpenDoveDomain d = i.next();
            if (d.getName().compareTo(name) == 0)
                return d;
        }
        return null;
    }

    public void addDomain(String domainUUID, OpenDoveDomain domain) {
        domainDB.putIfAbsent(domainUUID, domain);
    }

    public void addNetworkToDomain(String domainUUID, OpenDoveNetwork network) {
        if (domainExists(domainUUID)) {
            OpenDoveDomain domain = domainDB.get(domainUUID);
            domain.addNetwork(network);
        }
    }

    public List<OpenDoveDomain> getDomains() {
        List<OpenDoveDomain> answer = new ArrayList<OpenDoveDomain>();
        answer.addAll(domainDB.values());
        return answer;
    }

    // code to support SB network interfaces (including URI)

    public boolean networkExists(String networkUUID) {
        return(networkDB.containsKey(networkUUID));
    }
    
    public boolean networkExistsByVnid(int vnid) {
        Iterator<OpenDoveNetwork> i = networkDB.values().iterator();
        while (i.hasNext()) {
            OpenDoveNetwork n = i.next();
            if (n.getVnid() == vnid)
                return true;
        }
        return false;
    }

    public OpenDoveNetwork getNetwork(String networkUUID) {
        return(networkDB.get(networkUUID));
    }

    public void addNetwork(String networkUUID, OpenDoveNetwork network) {
        networkDB.putIfAbsent(networkUUID, network);
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
        answer.addAll(networkDB.values());
        return answer;
	}
}
