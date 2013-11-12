package org.opendaylight.opendove.odmc;

import java.util.ArrayList;
import java.util.List;

import javax.xml.bind.annotation.XmlAccessType;
import javax.xml.bind.annotation.XmlAccessorType;
import javax.xml.bind.annotation.XmlElement;
import javax.xml.bind.annotation.XmlRootElement;

import org.apache.commons.net.util.SubnetUtils;
import org.apache.commons.net.util.SubnetUtils.SubnetInfo;
import org.opendaylight.controller.networkconfig.neutron.NeutronNetwork;
import org.opendaylight.controller.networkconfig.neutron.NeutronSubnet;

@XmlRootElement
@XmlAccessorType(XmlAccessType.NONE)
public class OpenDoveSubnet extends OpenDoveObject {
    @XmlElement(name="id")
    String uuid;

    @XmlElement(name="domain_id")
    String domainUUID;

    @XmlElement(name="subnet")
    String subnet;

    @XmlElement(name="mask")
    String mask;

    @XmlElement(name="nexthop")
    String nexthop;

    @XmlElement (name="type")
    String subnetType;

    @XmlElement (name="network_ids")
    List<String> networkUUIDs;

    String associatedOSSubnetUUID;

    public OpenDoveSubnet() {
        networkUUIDs = new ArrayList<String>();
    }

    public static String getIPNetwork(String cidr) {
        String[] parts = cidr.split("/");
        return parts[0];
    }

    public static String getIPMask(String cidr) {
        String[] parts = cidr.split("/");
        String[] residual = {"0", "128", "192", "224", "240", "248", "252", "254", "255" };
        int length = Integer.parseInt(parts[1]);
        StringBuffer ans = new StringBuffer();
        int octet = 0;
        while (length >= 8) {
            ans.append("255.");
            length -= 8;
            octet++;
        }
        if (length > 0) {
            ans.append(residual[length]);
            octet++;
        }
        while (octet < 4) {
            ans.append("0");
            if (octet < 3) {
                ans.append(".");
            }
            octet++;
        }
        return ans.toString();
    }

    public OpenDoveSubnet(NeutronSubnet neutronSubnet, OpenDoveDomain domain,
            NeutronNetwork neutronNetwork, OpenDoveNetwork network) {
        networkUUIDs = new ArrayList<String>();
        uuid = java.util.UUID.randomUUID().toString();
        domainUUID = domain.getUUID();
        subnet = getIPNetwork(neutronSubnet.getCidr());
        mask = getIPMask(neutronSubnet.getCidr());
        nexthop = neutronSubnet.getGatewayIP();
        if (neutronNetwork.isShared()) {
            subnetType = "Shared";
        } else {
            subnetType = "Dedicated";
        }
        // link to network
        networkUUIDs.add(network.getUUID());
        tombstoneFlag = false;
        associatedOSSubnetUUID = neutronSubnet.getID();
    }

    @Override
    public String getUUID() {
        return uuid;
    }

    public void setUuid(String uuid) {
        this.uuid = uuid;
    }

    public String getDomainUUID() {
        return domainUUID;
    }

    public void setDomainUUID(String domainUUID) {
        this.domainUUID = domainUUID;
    }

    public String getSubnet() {
        return subnet;
    }

    public void setSubnet(String subnet) {
        this.subnet = subnet;
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

    public String getSubnetType() {
        return subnetType;
    }

    public void setSubnetType(String subnetType) {
        this.subnetType = subnetType;
    }

    public List<String> getNetworkUUIDs() {
        return networkUUIDs;
    }

    public void setNetworkUUIDs(List<String> networkUUIDs) {
        this.networkUUIDs = networkUUIDs;
    }

    public void removeNetwork(String uuid) {
        networkUUIDs.remove(uuid);
    }

    public String getAssociatedOSSubnetUUID() {
        return associatedOSSubnetUUID;
    }

    public void setAssociatedOSSubnetUUID(String associatedOSSubnetUUID) {
        this.associatedOSSubnetUUID = associatedOSSubnetUUID;
    }

    public boolean isTrackedByDCS() {
        return true;
    }

    public boolean containsAddress(String ip) {
        try {
            SubnetUtils util = new SubnetUtils(subnet, mask);
            SubnetInfo info = util.getInfo();
            return info.isInRange(ip);
        } catch (Exception e) {
            return false;
        }
    }
}
