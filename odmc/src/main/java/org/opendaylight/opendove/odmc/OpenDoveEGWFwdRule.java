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

import org.opendaylight.controller.networkconfig.neutron.INeutronNetworkCRUD;
import org.opendaylight.controller.networkconfig.neutron.INeutronPortCRUD;
import org.opendaylight.controller.networkconfig.neutron.NeutronCRUDInterfaces;
import org.opendaylight.controller.networkconfig.neutron.NeutronFloatingIP;
import org.opendaylight.controller.networkconfig.neutron.NeutronNetwork;
import org.opendaylight.controller.networkconfig.neutron.NeutronPort;

@XmlRootElement
@XmlAccessorType(XmlAccessType.NONE)
public class OpenDoveEGWFwdRule extends OpenDoveObject implements IfOpenDGWTrackedObject {

    @XmlElement(name="id")
    String uuid;

    @XmlElement(name="net_id")
    Integer vnid;

    @XmlElement(name="ip")
    String externalIP;

    @XmlElement(name="port")
    Integer externalPort;

    @XmlElement(name="real_ip")
    String internalIP;

    @XmlElement(name="real_port")
    Integer internalPort;

    @XmlElement(name="gatewayUUID")
    String gatewayUUID;

    @XmlElement(name="pip_min")
    String minProxyIP;

    @XmlElement(name="pip_max")
    String maxProxyIP;

    String associatedNeutronFloatingIPUUID;

    public OpenDoveEGWFwdRule() {
        uuid = java.util.UUID.randomUUID().toString();
        tombstoneFlag = false;
    }

    public OpenDoveEGWFwdRule(NeutronFloatingIP floatingIP, String uuid2,
            String minProxyIP2, String maxProxyIP2, Integer vnid2) {
        uuid = java.util.UUID.randomUUID().toString();
        tombstoneFlag = false;
        externalIP = floatingIP.getFloatingIPAddress();
        gatewayUUID = uuid2;
        internalIP = floatingIP.getFixedIPAddress();
        externalPort = internalPort = 0;
        minProxyIP = minProxyIP2;
        maxProxyIP = maxProxyIP2;
        vnid = vnid2;
        associatedNeutronFloatingIPUUID = floatingIP.getID();
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


    public String getGatewayUUID() {
        return gatewayUUID;
    }

    public void setGatewayUUID(String gatewayUUID) {
        this.gatewayUUID = gatewayUUID;
    }


    public String getExternalIP() {
        return externalIP;
    }

    public void setExternalIP(String externalIP) {
        this.externalIP = externalIP;
    }

    public Integer getExternalPort() {
        return externalPort;
    }

    public void setExternalPort(Integer externalPort) {
        this.externalPort = externalPort;
    }

    public String getInternalIP() {
        return internalIP;
    }

    public void setInternalIP(String internalIP) {
        this.internalIP = internalIP;
    }

    public Integer getInternalPort() {
        return internalPort;
    }

    public void setInternalPort(Integer internalPort) {
        this.internalPort = internalPort;
    }

    public String getMinProxyIP() {
        return minProxyIP;
    }

    public void setMinProxyIP(String minProxyIP) {
        this.minProxyIP = minProxyIP;
    }

    public String getMaxProxyIP() {
        return maxProxyIP;
    }

    public void setMaxProxyIP(String maxProxyIP) {
        this.maxProxyIP = maxProxyIP;
    }

    public String getAssociatedNeutronFloatingIPUUID() {
        return associatedNeutronFloatingIPUUID;
    }

    public void setAssociatedNeutronFloatingIPUUID(
            String associatedNeutronFloatingIPUUID) {
        this.associatedNeutronFloatingIPUUID = associatedNeutronFloatingIPUUID;
    }

    public boolean isTrackedByDGW() {
        return true;
    }

    public String getSBDgwUri() {
        return "/controller/sb/v2/opendove/odmc/egw-fwd-rules/"+uuid;
    }

    static public void mapFloatingIPtoEGWFwdRule(NeutronFloatingIP floatingIP, Object o) {
        // map from port back to vnid
        IfSBDoveEGWFwdRuleCRUD egwFwdRuleCRUDif = OpenDoveCRUDInterfaces.getIfSBDoveEGWFwdRuleCRUD(o);
        INeutronPortCRUD neutronPortCRUDif = NeutronCRUDInterfaces.getINeutronPortCRUD(o);
        NeutronPort port = neutronPortCRUDif.getPort(floatingIP.getPortUUID());
        String neutronNetwork = port.getNetworkUUID();
        INeutronNetworkCRUD neutronNetworkCRUDif = NeutronCRUDInterfaces.getINeutronNetworkCRUD(o);
        NeutronNetwork network = neutronNetworkCRUDif.getNetwork(neutronNetwork);
        String doveDomainName = "Neutron "+port.getTenantID();
        String doveNetworkName = "Neutron "+network.getID();
        IfNBSystemRU systemDB = OpenDoveCRUDInterfaces.getIfSystemRU(o);
        OpenDoveNeutronControlBlock controlBlock = systemDB.getSystemBlock(); //get system block
        if (network.isShared() && !controlBlock.getDomainSeparation())
            doveDomainName = "SharedDomain";
        IfSBDoveDomainCRU domainDB = OpenDoveCRUDInterfaces.getIfDoveDomainCRU(o);
        String doveDomainUUID = domainDB.getDomainByName(doveDomainName).getUUID();
        IfSBDoveNetworkCRU doveNetworkDB = OpenDoveCRUDInterfaces.getIfDoveNetworkCRU(o);
        Iterator<OpenDoveNetwork> doveNetworkIterator = doveNetworkDB.getNetworks().iterator();
        while (doveNetworkIterator.hasNext()) {
            OpenDoveNetwork doveNetwork = doveNetworkIterator.next();
            if (doveNetwork.getName().equalsIgnoreCase(doveNetworkName) &&
                    doveNetwork.getDomain_uuid().equalsIgnoreCase(doveDomainUUID)) {
                // get gateway list from vnid
                Integer vnid = doveNetwork.getVnid();
                Iterator<OpenDoveServiceAppliance> egwIterator = doveNetwork.getEGWs().iterator();
                while (egwIterator.hasNext()) {
                    OpenDoveServiceAppliance egw = egwIterator.next();
                    // create EGWFwdRule object for each gateway, link to floatingIP and add
                    // FIXME: pip_max_str to what?
                    // FIXME: pip_min_str to what?
                    OpenDoveEGWFwdRule egwFwdRule = new OpenDoveEGWFwdRule(floatingIP, egw.getUUID(),
                            "0.0.0.0", "0.0.0.0", vnid);
                    egwFwdRuleCRUDif.addEgwFwdRule(egwFwdRule.getUUID(), egwFwdRule);
                }
            }
        }
    }

    static public void removeEgwFwdRulesForFloatingIP(NeutronFloatingIP floatingIP, Object o) {
        IfSBDoveEGWFwdRuleCRUD egwFwdRuleCRUDif = OpenDoveCRUDInterfaces.getIfSBDoveEGWFwdRuleCRUD(o);
        Iterator<OpenDoveEGWFwdRule> iterator = egwFwdRuleCRUDif.getEgwFwdRules().iterator();
        while (iterator.hasNext()) {
            OpenDoveEGWFwdRule egwFwdRule = iterator.next();
            if (egwFwdRule.getAssociatedNeutronFloatingIPUUID().equalsIgnoreCase(floatingIP.getID())) {
                egwFwdRule.setTombstoneFlag(true);
                egwFwdRuleCRUDif.updateEgwFwdRule(egwFwdRule.getUUID(), egwFwdRule);
            }
        }
    }
}
