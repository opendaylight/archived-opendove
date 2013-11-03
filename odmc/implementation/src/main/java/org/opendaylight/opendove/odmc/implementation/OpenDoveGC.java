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
	private OpenDoveBidirectionalInterfaces openDoveBidirectionalInterfaces;
	private OpenDoveSBInterfaces openDoveSBInterfaces;

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
                    if (o.getTombstoneFlag()) {
                        boolean purge = true;
                        for (OpenDoveServiceAppliance oDSA: oDSAs) {
                            if (oDSA.get_isDCS()) {
                                if (oDSA.get_dcs_config_version() < i)
                                    purge = false;
                            }
                            if (oDSA.get_isDGW()) {
                                if (oDSA.get_dgw_config_version() < i)
                                    purge = false;
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
		}
		if (o instanceof OpenDoveNetwork) {
			for (OpenDoveDomain oDD: openDoveBidirectionalInterfaces.getDomains()) {
				oDD.removeNetwork((OpenDoveNetwork) o);
				//TODO: if domain only has one EXT_MCAST network left, tombstone the EXT_MCAST network
				//TODO: if domain has no networks left, tombstone it
			}
			openDoveBidirectionalInterfaces.removeNetwork(o.getUUID());
		}
		if (o instanceof OpenDoveNetworkSubnetAssociation) {
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
