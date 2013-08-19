/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.controller.odmc;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import javax.xml.bind.annotation.XmlAccessType;
import javax.xml.bind.annotation.XmlAccessorType;
import javax.xml.bind.annotation.XmlElement;
import javax.xml.bind.annotation.XmlRootElement;

import org.apache.commons.net.util.SubnetUtils;
import org.apache.commons.net.util.SubnetUtils.SubnetInfo;

@XmlRootElement
@XmlAccessorType(XmlAccessType.NONE)

public class OpenStackSubnets {
    // See OpenStack Network API v2.0 Reference for description of
    // annotated attributes

    @XmlElement (name="id")
    String subnetUUID;

    @XmlElement (name="network_id")
    String networkUUID;

    @XmlElement (name="name")
    String name;

    @XmlElement (defaultValue="4", name="ip_version")
    String ipVersion;

    @XmlElement (name="cidr")
    String cidr;

    @XmlElement (name="gateway_ip")
    String gatewayIP;

    @XmlElement (name="dns_nameservers")
    List<String> dnsNameservers;

    @XmlElement (name="allocation_pools")
    List<OpenStackIPAllocationPool> allocationPools;

    @XmlElement (name="host_routes")
    List<OpenStackHostRoute> hostRoutes;

    @XmlElement (defaultValue="true", name="enable_dhcp")
    String enableDHCP;

    @XmlElement (name="tenant_id")
    String tenantID;

    /* stores the OpenStackPorts associated with an instance
     * used to determine if that instance can be deleted.
     */
    List<OpenStackPorts> myPorts;

    public OpenStackSubnets() {
        myPorts = new ArrayList<OpenStackPorts>();
    }

    public String getID() { return subnetUUID; }

    public String getSubnetUUID() {
        return subnetUUID;
    }

    public void setSubnetUUID(String subnetUUID) {
        this.subnetUUID = subnetUUID;
    }

    public String getNetworkUUID() {
        return networkUUID;
    }

    public void setNetworkUUID(String networkUUID) {
        this.networkUUID = networkUUID;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public String getIpVersion() {
        return ipVersion;
    }

    public void setIpVersion(String ipVersion) {
        this.ipVersion = ipVersion;
    }

    public String getCidr() {
        return cidr;
    }

    public void setCidr(String cidr) {
        this.cidr = cidr;
    }

    public String getGatewayIP() {
        return gatewayIP;
    }

    public void setGatewayIP(String gatewayIP) {
        this.gatewayIP = gatewayIP;
    }

    public List<String> getDnsNameservers() {
        return dnsNameservers;
    }

    public void setDnsNameservers(List<String> dnsNameservers) {
        this.dnsNameservers = dnsNameservers;
    }

    public List<OpenStackIPAllocationPool> getAllocationPools() {
        return allocationPools;
    }

    public void setAllocationPools(List<OpenStackIPAllocationPool> allocationPools) {
        this.allocationPools = allocationPools;
    }

    public List<OpenStackHostRoute> getHostRoutes() {
        return hostRoutes;
    }

    public void setHostRoutes(List<OpenStackHostRoute> hostRoutes) {
        this.hostRoutes = hostRoutes;
    }

    public boolean isEnableDHCP() {
        if (enableDHCP == null || enableDHCP.equalsIgnoreCase("true"))
            return true;
        return false;
    }

    public String getEnableDHCP() { return enableDHCP; }

    public void setEnableDHCP(String newValue) {
        if (newValue.equalsIgnoreCase("true") ||
                newValue.equalsIgnoreCase("false"))
            this.enableDHCP = newValue;
    }

    public String getTenantID() {
        return tenantID;
    }

    public void setTenantID(String tenantID) {
        this.tenantID = tenantID;
    }

    /**
     * This method copies selected fields from the object and returns them
     * as a new object, suitable for marshaling.
     *
     * @param fields
     *            List of attributes to be extracted
     * @return an OpenStackSubnets object with only the selected fields
     * populated
     */

    public OpenStackSubnets extractFields(List<String> fields) {
        OpenStackSubnets ans = new OpenStackSubnets();
        Iterator<String> i = fields.iterator();
        while (i.hasNext()) {
            String s = i.next();
            if (s.equals("id"))
                ans.setSubnetUUID(this.getSubnetUUID());
            if (s.equals("network_id"))
                ans.setNetworkUUID(this.getNetworkUUID());
            if (s.equals("name"))
                ans.setName(this.getName());
            if (s.equals("ip_version"))
                ans.setIpVersion(this.getIpVersion());
            if (s.equals("cidr"))
                ans.setCidr(this.getCidr());
            if (s.equals("gateway_ip"))
                ans.setGatewayIP(this.getGatewayIP());
            if (s.equals("dns_nameservers")) {
                List<String> nsList = new ArrayList<String>();
                nsList.addAll(this.getDnsNameservers());
                ans.setDnsNameservers(nsList);
            }
            if (s.equals("allocation_pools")) {
                List<OpenStackIPAllocationPool> aPools = new ArrayList<OpenStackIPAllocationPool>();
                aPools.addAll(this.getAllocationPools());
                ans.setAllocationPools(aPools);
            }
            if (s.equals("host_routes")) {
                List<OpenStackHostRoute> hRoutes = new ArrayList<OpenStackHostRoute>();
                hRoutes.addAll(this.getHostRoutes());
                ans.setHostRoutes(hRoutes);
            }
            if (s.equals("enable_dhcp"))
                ans.setEnableDHCP(this.getEnableDHCP());
            if (s.equals("tenant_id"))
                ans.setTenantID(this.getTenantID());
        }
        return ans;
    }

    /* test to see if the cidr address used to define this subnet
     * is a valid network address (an necessary condition when creating
     * a new subnet)
     */
    public boolean isValidCIDR() {
        try {
            SubnetUtils util = new SubnetUtils(cidr);
            SubnetInfo info = util.getInfo();
            if (!info.getNetworkAddress().equals(info.getAddress()))
                return false;
        } catch (Exception e) {
            return false;
        }
        return true;
    }

    /* test to see if the gateway IP specified overlaps with specified
     * allocation pools (an error condition when creating a new subnet
     * or assigning a gateway IP)
     */
    public boolean gatewayIP_Pool_overlap() {
        Iterator<OpenStackIPAllocationPool> i = allocationPools.iterator();
        while (i.hasNext()) {
            OpenStackIPAllocationPool pool = i.next();
            if (pool.contains(gatewayIP))
                return true;
        }
        return false;
    }

    public void initDefaults() {
        dnsNameservers = new ArrayList<String>();
        allocationPools = new ArrayList<OpenStackIPAllocationPool>();
        hostRoutes = new ArrayList<OpenStackHostRoute>();
        try {
            SubnetUtils util = new SubnetUtils(cidr);
            SubnetInfo info = util.getInfo();
            if (gatewayIP == null)
                gatewayIP = info.getLowAddress();
            if (allocationPools.size() < 1) {
                OpenStackIPAllocationPool source =
                    new OpenStackIPAllocationPool(info.getLowAddress(),
                            info.getHighAddress());
                allocationPools = source.splitPool(gatewayIP);
            }
        } catch (Exception e) {
            ;
        }
    }

    public List<OpenStackPorts> getPortsInSubnet() {
        return myPorts;
    }

    public void addPort(OpenStackPorts port) {
        myPorts.add(port);
    }

    public void removePort(OpenStackPorts port) {
        myPorts.remove(port);
    }

    /* this method tests to see if the supplied IPv4 address
     * is valid for this subnet or not
     */
    public boolean isValidIP(String ipAddress) {
        try {
            SubnetUtils util = new SubnetUtils(cidr);
            SubnetInfo info = util.getInfo();
            return info.isInRange(ipAddress);
        } catch (Exception e) {
            return false;
        }
    }

    /* test to see if the supplied IPv4 address is part of one of the
     * available allocation pools or not
     */
    public boolean isIPInUse(String ipAddress) {
        Iterator<OpenStackIPAllocationPool> i = allocationPools.iterator();
        while (i.hasNext()) {
            OpenStackIPAllocationPool pool = i.next();
            if (pool.contains(ipAddress))
                return false;
        }
        return true;
    }

    /* method to get the lowest available address of the subnet.
     * go through all the allocation pools and keep the lowest of their
     * low addresses.
     */
    public String getLowAddr() {
        String ans = null;
        Iterator<OpenStackIPAllocationPool> i = allocationPools.iterator();
        while (i.hasNext()) {
            OpenStackIPAllocationPool pool = i.next();
            if (ans == null)
                ans = pool.getPoolStart();
            else
                if (OpenStackIPAllocationPool.convert(pool.getPoolStart()) <
                        OpenStackIPAllocationPool.convert(ans))
                    ans = pool.getPoolStart();
        }
        return ans;
    }

    /*
     * allocate the parameter address.  Because this uses an iterator to
     * check the instance's list of allocation pools and we want to modify
     * pools while the iterator is being used, it is necessary to
     * build a new list of allocation pools and replace the list when
     * finished (otherwise a split will cause undefined iterator behavior.
     */
    public void allocateIP(String ipAddress) {
        Iterator<OpenStackIPAllocationPool> i = allocationPools.iterator();
        List<OpenStackIPAllocationPool> newList = new ArrayList<OpenStackIPAllocationPool>();    // we have to modify a separate list
        while (i.hasNext()) {
            OpenStackIPAllocationPool pool = i.next();
            if (pool.getPoolEnd().equalsIgnoreCase(ipAddress) &&
                    pool.getPoolStart().equalsIgnoreCase(ipAddress))
                ; // do nothing, i.e. don't add the current pool to the new list
            else
                if (pool.contains(ipAddress)) {
                    List<OpenStackIPAllocationPool> pools = pool.splitPool(ipAddress);
                    newList.addAll(pools);
                } else
                    newList.add(pool);
        }
        allocationPools = newList;
    }

    /*
     * release an IP address back to the subnet.  Although an iterator
     * is used, the list is not modified until the iterator is complete, so
     * an extra list is not necessary.
     */
    public void releaseIP(String ipAddress) {
        OpenStackIPAllocationPool lPool = null;
        OpenStackIPAllocationPool hPool = null;
        Iterator<OpenStackIPAllocationPool> i = allocationPools.iterator();
        long sIP = OpenStackIPAllocationPool.convert(ipAddress);
        //look for lPool where ipAddr - 1 is high address
        //look for hPool where ipAddr + 1 is low address
        while (i.hasNext()) {
            OpenStackIPAllocationPool pool = i.next();
            long lIP = OpenStackIPAllocationPool.convert(pool.getPoolStart());
            long hIP = OpenStackIPAllocationPool.convert(pool.getPoolEnd());
            if (sIP+1 == lIP)
                hPool = pool;
            if (sIP-1 == hIP)
                lPool = pool;
        }
        //if (lPool == NULL and hPool == NULL) create new pool where low = ip = high
        if (lPool == null && hPool == null)
            allocationPools.add(new OpenStackIPAllocationPool(ipAddress,ipAddress));
        //if (lPool == NULL and hPool != NULL) change low address of hPool to ipAddr
        if (lPool == null && hPool != null)
            hPool.setPoolStart(ipAddress);
        //if (lPool != NULL and hPool == NULL) change high address of lPool to ipAddr
        if (lPool != null && hPool == null)
            lPool.setPoolEnd(ipAddress);
        //if (lPool != NULL and hPool != NULL) remove lPool and hPool and create new pool
        //        where low address = lPool.low address and high address = hPool.high Address
        if (lPool != null && hPool != null) {
            allocationPools.remove(lPool);
            allocationPools.remove(hPool);
            allocationPools.add(new OpenStackIPAllocationPool(
                    lPool.getPoolStart(), hPool.getPoolEnd()));
        }
    }
}
