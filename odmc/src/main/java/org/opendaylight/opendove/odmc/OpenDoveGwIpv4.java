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

    @XmlElement (name="intf_type")
    String type;

    @XmlElement (name="gwUUID")
    String gwUUID;

    @XmlElement (name="vlan")
    Integer vlan;

    NeutronSubnet neutronSubnet;
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

    public String getGWUUID() {
        return gwUUID;
    }

    public void setGWUUID(String gwUUID) {
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

    public void setNeutronSubnet(NeutronSubnet subnet) {
        neutronSubnet = subnet;
    }

    public NeutronSubnet getNeutronSubnet() {
        return neutronSubnet;
    }

    public String getSBDgwUri() {
        return "/controller/sb/v2/opendove/odmc/odgw/ipv4/" + uuid;
    }

    public static OpenDoveGwIpv4 assignEGWs(Object o, OpenDoveServiceAppliance target,
           String subnetCIDR, String subnetGatewayIP, String gwAddress) {
        IfSBDoveGwIpv4CRUD gatewayIPDB = OpenDoveCRUDInterfaces.getIfSBDoveGwIpv4CRUD(o);
        SubnetUtils util = new SubnetUtils(subnetCIDR);
        SubnetInfo info = util.getInfo();
        OpenDoveGwIpv4 newGWIP = null;
            List<OpenDoveGwIpv4> gwIPs= gatewayIPDB.getGwIpv4Pool();
            Iterator<OpenDoveGwIpv4> ipIterator = gwIPs.iterator();
            boolean found = false;

            while (ipIterator.hasNext()) {
                OpenDoveGwIpv4 gwIP = ipIterator.next();

                if (gwIP.getGWUUID().equalsIgnoreCase(target.getUUID()) &&
                    info.isInRange(gwIP.getIP()) && gwIP.getType().equalsIgnoreCase("external")) {
                    found = true;
                }
            }

            if (!found) {
                newGWIP = new OpenDoveGwIpv4(gwAddress, OpenDoveSubnet.getIPMask(subnetCIDR), subnetGatewayIP,
                        "external", target.getUUID(), 0);
                //TODO: this needs to be outside
                //newGWIP.setNeutronSubnet(neutronSubnet);
                gatewayIPDB.addGwIpv4(newGWIP.getUUID(), newGWIP);
                // Set the External IP for EGW, will be used by SNAT Pool Configuration
                target.setEGWExtIP(newGWIP);
            }

            return newGWIP;
    }

    public static void tombstoneEGWs(
            IfOpenDoveServiceApplianceCRUD serviceApplianceDB,
            IfSBDoveGwIpv4CRUD gatewayIPDB, NeutronSubnet subnet,
            OpenDoveNetwork network) {
        for (OpenDoveGwIpv4 gwIP: gatewayIPDB.getGwIpv4Pool()) {
            if (subnet.isValidIP(gwIP.getIP())) {
                subnet.releaseIP(gwIP.getIP());
                gwIP.setTombstoneFlag(true);
                gatewayIPDB.updateGwIpv4(gwIP);
                network.removeEGW(serviceApplianceDB.getDoveServiceAppliance(gwIP.getGWUUID()));
            }
        }
    }
}
