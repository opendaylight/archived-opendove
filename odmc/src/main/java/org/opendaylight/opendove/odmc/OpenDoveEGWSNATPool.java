/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.opendove.odmc;

import java.util.Iterator;
import javax.xml.bind.annotation.XmlAccessType;
import javax.xml.bind.annotation.XmlAccessorType;
import javax.xml.bind.annotation.XmlElement;
import javax.xml.bind.annotation.XmlRootElement;

import org.opendaylight.controller.networkconfig.neutron.NeutronSubnet;

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

    @XmlElement(name="domain_uuid")
    String domainUUID;

    @XmlElement(name="domain_id")
    Integer domainID;

    @XmlElement(name="ext_ip")
    OpenDoveGwIpv4 externalIP;

    public OpenDoveEGWSNATPool() {
        uuid = java.util.UUID.randomUUID().toString();
        tombstoneFlag = false;
    }

    public OpenDoveEGWSNATPool(String uuid2, String ip_low, String ip_high,
            String domain_uuid, Integer domain_id, Integer ext_mcast_vnid,
            Integer vnid2, OpenDoveGwIpv4 egwExtIP, int i, int j) {
        uuid = java.util.UUID.randomUUID().toString();
        gatewayUUID = uuid2;
        minIP = ip_low;
        maxIP = ip_high;
        domainUUID = domain_uuid;
        domainID = domain_id;
        externalMulticastVNID = ext_mcast_vnid;
        vnid = vnid2;
        externalIP = egwExtIP;
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

    public OpenDoveGwIpv4 getExternalIP() {
        return externalIP;
    }

    public void setExternalIP(OpenDoveGwIpv4 externalIP) {
        this.externalIP = externalIP;
    }

    public boolean isTrackedByDGW() {
        return true;
    }

    public String getSBDgwUri() {
        return "/controller/sb/v2/opendove/odmc/ext-gws/"+uuid;
    }

    static public void configureEGWSNATPool(String ip_low, String ip_high,
            IfSBDoveEGWSNATPoolCRUD snatPoolDB, OpenDoveNetwork oldODN,
            OpenDoveServiceAppliance target) {
        OpenDoveDomain d = oldODN.getScopingDomain();
        OpenDoveNetwork extMCastNet = d.getExtMCastNetwork();
        Integer domainID = d.getDomainId();
        OpenDoveGwIpv4  egwExternalIP = target.getEGWExtIP();
        oldODN.addEGW(target);
        OpenDoveEGWSNATPool snatPool = new OpenDoveEGWSNATPool(target.getUUID(), ip_low, ip_high,
                oldODN.getDomain_uuid(), domainID,
                extMCastNet.getVnid(), oldODN.getVnid(), egwExternalIP,
                8000, 9000);//TODO add control for low and high
        snatPoolDB.addEgwSNATPool(snatPool.getUUID(), snatPool);
    }

    public static void removeEGWSNATPool(NeutronSubnet neutronSubnet,
            IfSBDoveEGWSNATPoolCRUD snatPoolDB,
            OpenDoveNeutronControlBlock controlBlock, OpenDoveNetwork oldODN) {
        Iterator<OpenDoveEGWSNATPool> iterator = snatPoolDB.getEgwSNATPools().iterator();
        while (iterator.hasNext()) {
            OpenDoveEGWSNATPool snatPool = iterator.next();
            if (oldODN.networkUsesEGW(snatPool.getGatewayUUID()) &&
                    snatPool.getVnid() == oldODN.getVnid()) {
                snatPool.setTombstoneFlag(true);
                snatPoolDB.updateSNATPool(snatPool.getUUID(), snatPool);
            }
        }
    }
}
