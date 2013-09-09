package org.opendaylight.opendove.odmc.internal;

import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Dictionary;
import java.util.EnumSet;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.Map.Entry;
import java.util.concurrent.ConcurrentMap;

import org.apache.felix.dm.Component;
import org.opendaylight.controller.clustering.services.CacheConfigException;
import org.opendaylight.controller.clustering.services.CacheExistException;
import org.opendaylight.controller.clustering.services.IClusterContainerServices;
import org.opendaylight.controller.clustering.services.IClusterServices;
import org.opendaylight.opendove.odmc.IfNBNetworkCRUD;
import org.opendaylight.opendove.odmc.IfNBSystemRU;
import org.opendaylight.opendove.odmc.IfSBDoveDomainCRU;
import org.opendaylight.opendove.odmc.IfSBDoveNetworkCRU;
import org.opendaylight.opendove.odmc.OpenDoveCRUDInterfaces;
import org.opendaylight.opendove.odmc.OpenDoveDomain;
import org.opendaylight.opendove.odmc.OpenDoveNeutronControlBlock;
import org.opendaylight.opendove.odmc.OpenStackNetworks;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class OpenDoveNeutronNetworkInterface implements IfNBNetworkCRUD {
    private static final Logger logger = LoggerFactory
    .getLogger(OpenDoveNeutronNBInterfaces.class);
    private String containerName = null;

    private ConcurrentMap<String, OpenStackNetworks> networkDB;
    private IClusterContainerServices clusterContainerService = null;

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
        logger.debug("Creating Cache for OpenDOVE Neutron Networks");
        try {
            // neutron caches
            this.clusterContainerService.createCache("openDoveNeutronNetworks",
                    EnumSet.of(IClusterServices.cacheMode.NON_TRANSACTIONAL));
        } catch (CacheConfigException cce) {
            logger.error("Cache couldn't be created for OpenDOVE Neutron Networks -  check cache mode");
        } catch (CacheExistException cce) {
            logger.error("Cache for OpenDOVE Neutron Networks already exists, destroy and recreate");
        }
        logger.debug("Cache successfully created for OpenDOVE Neutron Networks");
    }

    @SuppressWarnings({ "unchecked", "deprecation" })
    private void retrieveCache() {
        if (this.clusterContainerService == null) {
            logger.error("un-initialized clusterContainerService, can't retrieve cache");
            return;
        }
        logger.debug("Retrieving cache for openDoveNeutronNetworks");
        networkDB = (ConcurrentMap<String, OpenStackNetworks>) this.clusterContainerService.getCache("openDoveNeutronNetworks");
        if (networkDB == null) {
            logger.error("Cache couldn't be retrieved for openDOVENeutronNetworks");
        }
        logger.debug("Cache was successfully retrieved for openDOVENeutronNetworks");
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

    @SuppressWarnings("deprecation")
    private void destroyCache() {
        if (this.clusterContainerService == null) {
            logger.error("un-initialized clusterMger, can't destroy cache");
            return;
        }
        logger.debug("Destroying Cache for OpenDove Neutron Networks");
        this.clusterContainerService.destroyCache("openDoveNeutronNetworks");
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

    // IfNBNetworkCRUD methods

    public boolean networkExists(String uuid) {
        return networkDB.containsKey(uuid);
    }

    public OpenStackNetworks getNetwork(String uuid) {
        if (!networkExists(uuid))
            return null;
        return networkDB.get(uuid);
    }

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

    public boolean addNetwork(OpenStackNetworks input) {
        IfNBSystemRU systemDB = OpenDoveCRUDInterfaces.getIfSystemRU(this);
        IfSBDoveDomainCRU domainDB = OpenDoveCRUDInterfaces.getIfDoveDomainCRU(this);
        IfSBDoveNetworkCRU doveNetworkDB = OpenDoveCRUDInterfaces.getIfDoveNetworkCRU(this);
        if (networkExists(input.getID()))
            return false;
        networkDB.putIfAbsent(input.getID(), input);
        if (!input.isRouterExternal()) {            // don't map router external networks
            if (input.isShared()) {                    // map shared network
                OpenDoveNeutronControlBlock controlBlock = systemDB.getSystemBlock(); //get system block
                if (!controlBlock.getDomainSeparation()) { //if domain separation not supported, map to shared domain
                    if (!domainDB.domainExists("SharedDomain")) {// look up shared domain
                        OpenDoveDomain sharedDomain = new OpenDoveDomain("SharedDomain", "SharedDomain"); // if doesn't exist, create
                        domainDB.addDomain("SharedDomain", sharedDomain); 
                    }
                      // create DOVE network
                }
            } else {                                // map dedicated network
            	String domainUUID = input.getTenantID();
            	if (!domainDB.domainExists(domainUUID)) { // look up domain
            		OpenDoveDomain domain = new OpenDoveDomain(domainUUID, "Neutron "+domainUUID); // if doesn't exist, create
            		domainDB.addDomain(domainUUID, domain);
            	}
                // create DOVE network
            }
        }
        return true;
    }

    public boolean removeNetwork(String uuid) {
        if (!networkExists(uuid))
            return false;
        networkDB.remove(uuid);
        return true;
    }

    public boolean updateNetwork(String uuid, OpenStackNetworks delta) {
        if (!networkExists(uuid))
            return false;
        OpenStackNetworks target = networkDB.get(uuid);
        return overwrite(target, delta);
    }

    public boolean networkInUse(String netUUID) {
        if (!networkExists(netUUID))
            return true;
        OpenStackNetworks target = networkDB.get(netUUID);
        return (target.getPortsOnNetwork().size() > 0);
    }
}
