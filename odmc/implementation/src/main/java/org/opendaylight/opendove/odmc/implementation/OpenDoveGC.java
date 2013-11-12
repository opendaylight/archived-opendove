package org.opendaylight.opendove.odmc.implementation;

import java.util.List;
import java.util.concurrent.ConcurrentMap;
import java.util.concurrent.TimeUnit;

import org.opendaylight.controller.sal.utils.ServiceHelper;
import org.opendaylight.opendove.odmc.OpenDoveConcurrentBackedMap;
import org.opendaylight.opendove.odmc.OpenDoveDomain;
import org.opendaylight.opendove.odmc.OpenDoveEGWFwdRule;
import org.opendaylight.opendove.odmc.OpenDoveEGWSNATPool;
import org.opendaylight.opendove.odmc.OpenDoveGwIpv4;
import org.opendaylight.opendove.odmc.OpenDoveNetwork;
import org.opendaylight.opendove.odmc.OpenDoveNetworkSubnetAssociation;
import org.opendaylight.opendove.odmc.OpenDoveObject;
import org.opendaylight.opendove.odmc.OpenDovePolicy;
import org.opendaylight.opendove.odmc.OpenDoveServiceAppliance;
import org.opendaylight.opendove.odmc.OpenDoveSubnet;
import org.opendaylight.opendove.odmc.OpenDoveSwitch;
import org.opendaylight.opendove.odmc.OpenDoveVGWVNIDMapping;

import io.netty.util.Timeout;
import io.netty.util.Timer;
import io.netty.util.TimerTask;

public class OpenDoveGC implements TimerTask {
    private Timer timer;
    private final OpenDoveBidirectionalInterfaces openDoveBidirectionalInterfaces;
    private final OpenDoveSBInterfaces openDoveSBInterfaces;

    public OpenDoveGC() {
        openDoveBidirectionalInterfaces = (OpenDoveBidirectionalInterfaces) ServiceHelper.getGlobalInstance(
                OpenDoveBidirectionalInterfaces.class, this);
        openDoveSBInterfaces = (OpenDoveSBInterfaces) ServiceHelper.getGlobalInstance(
                OpenDoveSBInterfaces.class, this);
    }

    public void setTimer(Timer t) {
        timer = t;
    }

    public void run(Timeout arg0) throws Exception {
        if (openDoveSBInterfaces != null && openDoveBidirectionalInterfaces != null) {
            ConcurrentMap<Integer, OpenDoveObject> internalCache =
                    openDoveSBInterfaces.getObjectDB();
            List<OpenDoveServiceAppliance> oDSAs = openDoveBidirectionalInterfaces.getAppliances();
            for (Integer i: OpenDoveConcurrentBackedMap.getOrderedBackingKeys(internalCache)) {
                OpenDoveObject o = internalCache.get(i);
                if (o != null) {
                    if (canBeRemoved(o)) {
                        boolean purge = true;
                        for (OpenDoveServiceAppliance oDSA: oDSAs) {
                            if (oDSA.get_isDCS()) {
                                if (oDSA.get_dcs_config_version() < o.getLastChangeVersion()) {
                                    purge = false;
                                }
                            }
                            if (oDSA.get_isDGW()) {
                                if (oDSA.get_dgw_config_version() < o.getLastChangeVersion()) {
                                    purge = false;
                                }
                            }
                        }
                        if (purge) {
                            cleanObjectReferences(o);
                            internalCache.remove(i);
                        }
                    }
                }
            }
        }
        timer.newTimeout(this, 5000, TimeUnit.MILLISECONDS);
    }

    private boolean canBeRemoved(OpenDoveObject o) {
        if (o instanceof OpenDovePolicy) {
            if (((OpenDovePolicy) o).getPolicyAction() == 0) {
                return true;
            } else {
                return false;
            }
        }
        return o.getTombstoneFlag();
    }

    /* NOTE: if code in this block looks for dependent objects based on the parameters of the passed in object, please add
     * tests for checking the dependent object is not null, as this method *will* be called multiple times on an object
     * when removing that object
     */
    private void cleanObjectReferences(OpenDoveObject o) {
        if (o instanceof OpenDoveDomain) {
            for (OpenDoveNetwork oDN: openDoveBidirectionalInterfaces.getNetworks()) {
                oDN.removeScopingDomain((OpenDoveDomain) o);
            }
            openDoveBidirectionalInterfaces.removeDomain(o.getUUID());
        }
        if (o instanceof OpenDoveEGWFwdRule) {
            openDoveSBInterfaces.removeEgwFwdRule(o.getUUID());
        }
        if (o instanceof OpenDoveEGWSNATPool) {
            openDoveSBInterfaces.removeEgwSNATPool(o.getUUID());
        }
        if (o instanceof OpenDoveGwIpv4) {
            openDoveSBInterfaces.removeGwIpv4(o.getUUID());
            OpenDoveGwIpv4 gwIP = (OpenDoveGwIpv4) o;
            if (gwIP.getNeutronSubnet() != null) {
                gwIP.getNeutronSubnet().releaseIP(gwIP.getIP());
                gwIP.setNeutronSubnet(null);
            }
        }
        if (o instanceof OpenDoveNetwork) {
            for (OpenDoveDomain oDD: openDoveBidirectionalInterfaces.getDomains()) {
                oDD.removeNetwork((OpenDoveNetwork) o);
                if (oDD.getAssociatedNetworks().size() == 1) {
                    // if domain only has one network left, tombstone it
                    OpenDoveNetwork last = oDD.getAssociatedNetworks().get(0);
                    last.setTombstoneFlag(true);
                    openDoveBidirectionalInterfaces.updateNetwork(last.getUUID());
                }
                if (oDD.getAssociatedNetworks().size() == 0) {
                    // if domain has no networks left, tombstone it
                    oDD.setTombstoneFlag(true);
                    openDoveBidirectionalInterfaces.updateDomain(oDD.getUUID(), oDD);
                }
            }
            openDoveBidirectionalInterfaces.removeNetwork(o.getUUID());
        }
        if (o instanceof OpenDoveNetworkSubnetAssociation) {
            OpenDoveNetworkSubnetAssociation oDNSA = (OpenDoveNetworkSubnetAssociation) o;
            OpenDoveSubnet oDS = openDoveSBInterfaces.getSubnet(oDNSA.getOpenDoveNetworkSubnetUuid());
            if (oDS != null) {
                oDS.removeNetwork(openDoveBidirectionalInterfaces.getNetworkByVnid(oDNSA.getOpenDoveNetworkVnid()).getUUID());
                if (oDS.getNetworkUUIDs().size() == 0) {
                    oDS.setTombstoneFlag(true);
                    openDoveSBInterfaces.updateSubnet(oDS);
                }
            }
            openDoveSBInterfaces.removeNetworkSubnetAssociation(o.getUUID());
        }
        if (o instanceof OpenDovePolicy) {
            openDoveSBInterfaces.removePolicy(o.getUUID());
        }
        if (o instanceof OpenDoveSubnet) {
            for (OpenDoveDomain oDD: openDoveBidirectionalInterfaces.getDomains()) {
                oDD.removeSubnet((OpenDoveSubnet) o);
            }
            openDoveSBInterfaces.removeSubnet(o.getUUID());
        }
        if (o instanceof OpenDoveSwitch) {
            for (OpenDoveNetwork oDN: openDoveBidirectionalInterfaces.getNetworks()) {
                oDN.removeHostingSwitch((OpenDoveSwitch) o);
            }
            openDoveBidirectionalInterfaces.removeSwitch(o.getUUID());
        }
        if (o instanceof OpenDoveVGWVNIDMapping) {
            openDoveBidirectionalInterfaces.removeVgwVNIDMapping(o.getUUID());
        }
    }

}
