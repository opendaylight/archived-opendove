/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.opendove.odmc;

import java.util.Iterator;
import java.util.List;

import javax.xml.bind.annotation.XmlAccessType;
import javax.xml.bind.annotation.XmlAccessorType;
import javax.xml.bind.annotation.XmlElement;
import javax.xml.bind.annotation.XmlRootElement;

import org.opendaylight.controller.networkconfig.neutron.NeutronSubnet;
import org.opendaylight.controller.networkconfig.neutron.NeutronSubnet_IPAllocationPool;

@XmlRootElement
@XmlAccessorType(XmlAccessType.NONE)
public class OpenDoveEGWSNATPool extends OpenDoveObject implements IfOpenDGWTrackedObject {

    @XmlElement(name="id")
    String uuid;

    @XmlElement(name="net_id")
    Integer vnid;

    @XmlElement(name="min_ip")
    String minIP;

    @XmlElement(name="max_ip")
    String maxIP;

    @XmlElement(name="min_port")
    Integer minPort;

    @XmlElement(name="max_port")
    Integer maxPort;

    @XmlElement(name="gatewayUUID")
    String gatewayUUID;

    @XmlElement(name="extmcastvnid")
    Integer externalMulticastVNID;

    @XmlElement(name="domain_id")
    String domainUUID;

    @XmlElement(name="ext_ip")
    String externalIP;

    public OpenDoveEGWSNATPool() {
        uuid = java.util.UUID.randomUUID().toString();
        tombstoneFlag = false;
    }

    public OpenDoveEGWSNATPool(String uuid2, String ip_low, String ip_high,
            String domain_uuid, Integer vnid2, int i, int j) {
        uuid = java.util.UUID.randomUUID().toString();
        gatewayUUID = uuid2;
        minIP = ip_low;
        maxIP = ip_high;
        domainUUID = domain_uuid;
        vnid = vnid2;
        minPort = i;
        maxPort = j;
        tombstoneFlag = false;
    }

    @Override
    public String getUUID() {
        return uuid;
    }

    public void setUUID(String uuid) {
        this.uuid = uuid;
    }

    public Integer getVnid() {
        return vnid;
    }

    public void setVnid(Integer vnid) {
        this.vnid = vnid;
    }

    public String getMinIP() {
        return minIP;
    }

    public void setMinIP(String minIP) {
        this.minIP = minIP;
    }

    public String getMaxIP() {
        return maxIP;
    }

    public void setMaxIP(String maxIP) {
        this.maxIP = maxIP;
    }

    public Integer getMinPort() {
        return minPort;
    }

    public void setMinPort(Integer minPort) {
        this.minPort = minPort;
    }

    public Integer getMaxPort() {
        return maxPort;
    }

    public void setMaxPort(Integer maxPort) {
        this.maxPort = maxPort;
    }

    public String getGatewayUUID() {
        return gatewayUUID;
    }

    public void setGatewayUUID(String gatewayUUID) {
        this.gatewayUUID = gatewayUUID;
    }

    public Integer getExternalMulticastVNID() {
        return externalMulticastVNID;
    }

    public void setExternalMulticastVNID(Integer externalMulticastVNID) {
        this.externalMulticastVNID = externalMulticastVNID;
    }

    public String getDomainUUID() {
        return domainUUID;
    }

    public void setDomainUUID(String domainUUID) {
        this.domainUUID = domainUUID;
    }

    public String getExternalIP() {
        return externalIP;
    }

    public void setExternalIP(String externalIP) {
        this.externalIP = externalIP;
    }

    public boolean isTrackedByDGW() {
        return true;
    }

    public String getSBDgwUri() {
        return "/controller/sb/v2/opendove/odmc/ext-gws/"+uuid;
    }

    static public void configureEGWSNATPool(NeutronSubnet neutronSubnet,
            IfSBDoveEGWSNATPoolCRUD snatPoolDB,
            OpenDoveNeutronControlBlock controlBlock, OpenDoveNetwork oldODN) {
        Iterator<OpenDoveServiceAppliance> oDSAIterator = oldODN.getEGWs().iterator();
        String ip_low = null, ip_high = null;
        while (oDSAIterator.hasNext()) {
            OpenDoveServiceAppliance oDSA = oDSAIterator.next();
            Integer snatPoolSize = controlBlock.getSnatPoolSize();
            Iterator<NeutronSubnet_IPAllocationPool> poolIterator = neutronSubnet.getAllocationPools().iterator();
            while (poolIterator.hasNext()) {
                NeutronSubnet_IPAllocationPool pool = poolIterator.next();
                if (poolSize(pool) >= snatPoolSize) {
                    int i1;
                    for (i1=1; i1<=snatPoolSize; i1++) {
                        String ipAddr = pool.getPoolStart();
                        if (i1 == 1) {
                            ip_low = ipAddr;
                        }
                        if (i1 == snatPoolSize) {
                            ip_high = ipAddr;
                        }
                        neutronSubnet.allocateIP(ipAddr);
                    }
                    break;
                }
            }
            OpenDoveEGWSNATPool snatPool = new OpenDoveEGWSNATPool(oDSA.getUUID(), ip_low,
                    ip_high, oldODN.getDomain_uuid(), oldODN.getVnid(), 8000, 9000); //TODO add control for low and high
            snatPoolDB.addEgwSNATPool(snatPool.getUUID(), snatPool);
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

    static private String convertLongToIPv4String(Long i) {
        StringBuilder ans = new StringBuilder();
        while (i > 255) {
            ans.insert(0, i % 256);
            i >>= 8;
            ans.insert(0, ".");
        }
        ans.insert(0, i);
        return ans.toString();
    }

    public static void removeEGWSNATPool(NeutronSubnet neutronSubnet,
            IfSBDoveEGWSNATPoolCRUD snatPoolDB,
            OpenDoveNeutronControlBlock controlBlock, OpenDoveNetwork oldODN) {
        Iterator<OpenDoveEGWSNATPool> iterator = snatPoolDB.getEgwSNATPools().iterator();
        while (iterator.hasNext()) {
            OpenDoveEGWSNATPool snatPool = iterator.next();
            if (oldODN.networkUsesEGW(snatPool.getGatewayUUID()) &&
                    snatPool.getVnid() == oldODN.getVnid()) {
                long minPool = convertIPv4StringToLong(snatPool.getMinIP());
                long maxPool = convertIPv4StringToLong(snatPool.getMaxIP());
                long i;
                for (i=minPool; i<=maxPool; i++) {
                    neutronSubnet.releaseIP(convertLongToIPv4String(i));
                }
                snatPool.setTombstoneFlag(true);
              snatPoolDB.updateSNATPool(snatPool.getUUID(), snatPool);
            }
        }
    }
}
