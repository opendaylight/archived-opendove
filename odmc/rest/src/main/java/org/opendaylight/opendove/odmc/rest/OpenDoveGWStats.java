/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.opendove.odmc.rest;

import javax.xml.bind.annotation.XmlAccessType;
import javax.xml.bind.annotation.XmlAccessorType;
import javax.xml.bind.annotation.XmlElement;
import javax.xml.bind.annotation.XmlRootElement;

@XmlRootElement
@XmlAccessorType(XmlAccessType.NONE)
public class OpenDoveGWStats {

    @XmlElement(name="ovl_to_ext_leave_bytes")
    String ovlToExtLeaveBytes;

    @XmlElement(name="ovl_to_ext_leave_pkts")
    String ovlToExtLeavePkts;

    @XmlElement(name="ovl_to_ext_leave_bps")
    String ovlToExtLeaveBps;

    @XmlElement(name="ovl_to_ext_leave_pps")
    String ovlToExtLeavePps;

    @XmlElement(name="ext_to_ovl_enter_bytes")
    String extToOvlEnterBytes;

    @XmlElement(name="ext_to_ovl_enter_pkts")
    String extToOvlEnterPkts;

    @XmlElement(name="ext_to_ovl_enter_bps")
    String extToOvlEnterBps;

    @XmlElement(name="ext_to_ovl_enter_pps")
    String extToOvlEnterPps;

    @XmlElement(name="ovl_to_vlan_leave_bytes")
    String ovlToVlanLeaveBytes;

    @XmlElement(name="ovl_to_vlan_leave_pkts")
    String ovlToVlanLeavePkts;

    @XmlElement(name="ovl_to_vlan_leave_bps")
    String ovlToVlanLeaveBps;

    @XmlElement(name="ovl_to_vlan_leave_pps")
    String ovlToVlanLeavePps;

    @XmlElement(name="vlan_to_ovl_enter_bytes")
    String vlanToOvlEnterBytes;

    @XmlElement(name="vlan_to_ovl_enter_pkts")
    String vlanToOvlEnterPkts;

    @XmlElement(name="vlan_to_ovl_enter_bps")
    String vlanToOvlEnterBps;

    @XmlElement(name="vlan_to_ovl_enter_pps")
    String vlanToOvlEnterPps;

    public OpenDoveGWStats() { }

    public String getOvlToExtLeaveBytes() {
        return ovlToExtLeaveBytes;
    }

    public void setOvlToExtLeaveBytes(String ovlToExtLeaveBytes) {
        this.ovlToExtLeaveBytes = ovlToExtLeaveBytes;
    }

    public String getOvlToExtLeavePkts() {
        return ovlToExtLeavePkts;
    }

    public void setOvlToExtLeavePkts(String ovlToExtLeavePkts) {
        this.ovlToExtLeavePkts = ovlToExtLeavePkts;
    }

    public String getOvlToExtLeaveBps() {
        return ovlToExtLeaveBps;
    }

    public void setOvlToExtLeaveBps(String ovlToExtLeaveBps) {
        this.ovlToExtLeaveBps = ovlToExtLeaveBps;
    }

    public String getOvlToExtLeavePps() {
        return ovlToExtLeavePps;
    }

    public void setOvlToExtLeavePps(String ovlToExtLeavePps) {
        this.ovlToExtLeavePps = ovlToExtLeavePps;
    }

    public String getOvlToExtEnterBytes() {
        return extToOvlEnterBytes;
    }

    public void setOvlToExtEnterBytes(String extToOvlEnterBytes) {
        this.extToOvlEnterBytes = extToOvlEnterBytes;
    }

    public String getOvlToExtEnterPkts() {
        return extToOvlEnterPkts;
    }

    public void setOvlToExtEnterPkts(String extToOvlEnterPkts) {
        this.extToOvlEnterPkts = extToOvlEnterPkts;
    }

    public String getOvlToExtEnterBps() {
        return extToOvlEnterBps;
    }

    public void setOvlToExtEnterBps(String extToOvlEnterBps) {
        this.extToOvlEnterBps = extToOvlEnterBps;
    }

    public String getOvlToExtEnterPps() {
        return extToOvlEnterPps;
    }

    public void setOvlToExtEnterPps(String extToOvlEnterPps) {
        this.extToOvlEnterPps = extToOvlEnterPps;
    }

    public String getOvlToVlanLeaveBytes() {
        return ovlToVlanLeaveBytes;
    }

    public void setOvlToVlanLeaveBytes(String ovlToVlanLeaveBytes) {
        this.ovlToVlanLeaveBytes = ovlToVlanLeaveBytes;
    }

    public String getOvlToVlanLeavePkts() {
        return ovlToVlanLeavePkts;
    }

    public void setOvlToVlanLeavePkts(String ovlToVlanLeavePkts) {
        this.ovlToVlanLeavePkts = ovlToVlanLeavePkts;
    }

    public String getOvlToVlanLeaveBps() {
        return ovlToVlanLeaveBps;
    }

    public void setOvlToVlanLeaveBps(String ovlToVlanLeaveBps) {
        this.ovlToVlanLeaveBps = ovlToVlanLeaveBps;
    }

    public String getOvlToVlanLeavePps() {
        return ovlToVlanLeavePps;
    }

    public void setOvlToVlanLeavePps(String ovlToVlanLeavePps) {
        this.ovlToVlanLeavePps = ovlToVlanLeavePps;
    }

    public String getOvlToVlanEnterBytes() {
        return vlanToOvlEnterBytes;
    }

    public void setOvlToVlanEnterBytes(String vlanToOvlEnterBytes) {
        this.vlanToOvlEnterBytes = vlanToOvlEnterBytes;
    }

    public String getOvlToVlanEnterPkts() {
        return vlanToOvlEnterPkts;
    }

    public void setOvlToVlanEnterPkts(String vlanToOvlEnterPkts) {
        this.vlanToOvlEnterPkts = vlanToOvlEnterPkts;
    }

    public String getOvlToVlanEnterBps() {
        return vlanToOvlEnterBps;
    }

    public void setOvlToVlanEnterBps(String vlanToOvlEnterBps) {
        this.vlanToOvlEnterBps = vlanToOvlEnterBps;
    }

    public String getOvlToVlanEnterPps() {
        return vlanToOvlEnterPps;
    }

    public void setOvlToVlanEnterPps(String vlanToOvlEnterPps) {
        this.vlanToOvlEnterPps = vlanToOvlEnterPps;
    }


}
