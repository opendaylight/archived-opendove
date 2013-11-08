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

import org.apache.commons.net.util.SubnetUtils;
import org.apache.commons.net.util.SubnetUtils.SubnetInfo;
import org.opendaylight.controller.networkconfig.neutron.NeutronSubnet;

@XmlRootElement
@XmlAccessorType(XmlAccessType.NONE)
public class OpenDoveGwIpv4 extends OpenDoveObject implements IfOpenDGWTrackedObject {
    @XmlElement(name="id")
    String uuid;

    @XmlElement(name="ip")
    String ip;

    @XmlElement(name="mask")
    String mask;

    @XmlElement(name="nexthop")
    String nexthop;

    @XmlElement (name="type")
    String type;

    @XmlElement (name="gwUUID")
    String gwUUID;

    @XmlElement (name="vlan")
    Integer vlan;


    public OpenDoveGwIpv4() { }

    public OpenDoveGwIpv4(String ip, String mask, String nexthop, String type, String gwUUID, Integer vlan) {
        uuid = java.util.UUID.randomUUID().toString();
        this.ip = ip;
        this.mask = mask;
        this.nexthop = nexthop;
        this.type = type;
        this.gwUUID = gwUUID;
        this.vlan = vlan;
        tombstoneFlag = false;
    }

    @Override
    public String getUUID() {
        return uuid;
    }

    public void setUuid(String uuid) {
        this.uuid = uuid;
    }

    public String getIP() {
        return ip;
    }

    public void setIP(String ip) {
        this.ip = ip;
    }

    public String getMask() {
        return mask;
    }

    public void setMask(String mask) {
        this.mask = mask;
    }

    public String getNexthop() {
        return nexthop;
    }

    public void setNexthop(String nexthop) {
        this.nexthop = nexthop;
    }

    public String getType() {
        return type;
    }

    public void setType(String type) {
        this.type = type;
    }

    public String getGWIndex() {
        return gwUUID;
    }

    public void setGWIndex(String gwUUID) {
        this.gwUUID = gwUUID;
    }

    public Integer getVlan() {
        return vlan;
    }

    public void setVlan(Integer vlan) {
        this.vlan = vlan;
    }

    public boolean isTrackedByDGW() {
        return true;
    }

    public String getSBDgwUri() {
        return "/controller/sb/v2/opendove/odmc/odgw/ipv4/" + uuid;
    }

    public static void assignEGWs(IfOpenDoveServiceApplianceCRUD serviceApplianceDB, IfSBDoveGwIpv4CRUD gatewayIPDB,
            NeutronSubnet neutronSubnet, OpenDoveNeutronControlBlock controlBlock, OpenDoveNetwork network) {
        Integer replicationFactor = controlBlock.getEgwReplicationFactor();
        List<OpenDoveServiceAppliance> oDSAs = serviceApplianceDB.getAppliances();
        SubnetUtils util = new SubnetUtils(neutronSubnet.getCidr());
        SubnetInfo info = util.getInfo();
        while (replicationFactor > 0 && oDSAs.size() > 0) {
            Integer count = oDSAs.size();
            int index = Math.abs(OpenDoveUtils.getNextInt()) % count;
            OpenDoveServiceAppliance target = oDSAs.get(index);
            //if target doesn't have address in this subnet, assign one
            List<OpenDoveGwIpv4> gwIPs= gatewayIPDB.getGwIpv4Pool();
            Iterator<OpenDoveGwIpv4> ipIterator = gwIPs.iterator();
            boolean found = false;
            while (ipIterator.hasNext()) {
                OpenDoveGwIpv4 gwIP = ipIterator.next();
                if (gwIP.getGWIndex().equalsIgnoreCase(target.getUUID()) &&
                    info.isInRange(gwIP.getIP())) {
                    found = true;
                }
            }
            if (!found) {
                String gwAddress = neutronSubnet.getLowAddr();
                neutronSubnet.allocateIP(gwAddress);
                OpenDoveGwIpv4 newGWIP = new OpenDoveGwIpv4(gwAddress, OpenDoveSubnet.getIPMask(neutronSubnet.getCidr()), neutronSubnet.getGatewayIP(),
                        "external", target.getUUID(), 0);
                gatewayIPDB.addGwIpv4(newGWIP.getUUID(), newGWIP);
            }
            //link egw to dove network
            network.addEGW(target);
            //prepare for next assignment
            replicationFactor--;
            oDSAs.remove(index);
        }
    }
}
