package org.opendaylight.opendove.odmc;

import javax.xml.bind.annotation.XmlAccessType;
import javax.xml.bind.annotation.XmlAccessorType;
import javax.xml.bind.annotation.XmlElement;
import javax.xml.bind.annotation.XmlRootElement;

@XmlRootElement
@XmlAccessorType(XmlAccessType.NONE)
public class OpenDoveGWSession {

    @XmlElement(name="net_id")
    String vnid;

    @XmlElement(name="age")
    String age;

    @XmlElement(name="ovl_sip")
    String overlaySourceIP;

    @XmlElement(name="ovl_dip")
    String overlayDestinationIP;

    @XmlElement(name="ovl_sport")
    Integer overlaySourcePort;

    @XmlElement(name="ovl_dport")
    Integer overlayDestinationPort;

    @XmlElement(name="orig_sip")
    String originalSourceIP;

    @XmlElement(name="orig_dip")
    String originalDestinationIP;

    @XmlElement(name="orig_sport")
    Integer originalSourcePort;

    @XmlElement(name="orig_dport")
    Integer originalDestinationPort;

    @XmlElement(name="sip")
    String sourceIP;

    @XmlElement(name="dip")
    String destinationIP;

    @XmlElement(name="sport")
    Integer sourcePort;

    @XmlElement(name="proto")
    Integer protocol;

    @XmlElement(name="dport")
    Integer destinationPort;

    @XmlElement(name="action")
    String action;

    @XmlElement(name="snat_ip")
    String sNATIP;

    @XmlElement(name="snat_sport")
    Integer sNATPort;

    public OpenDoveGWSession() { }

    public String getVnid() {
        return vnid;
    }

    public void setVnid(String vnid) {
        this.vnid = vnid;
    }

    public String getAge() {
        return age;
    }

    public void setAge(String age) {
        this.age = age;
    }

    public String getOverlaySourceIP() {
        return overlaySourceIP;
    }

    public void setOverlaySourceIP(String overlaySourceIP) {
        this.overlaySourceIP = overlaySourceIP;
    }

    public String getOverlayDestinationIP() {
        return overlayDestinationIP;
    }

    public void setOverlayDestinationIP(String overlayDestinationIP) {
        this.overlayDestinationIP = overlayDestinationIP;
    }

    public Integer getOverlaySourcePort() {
        return overlaySourcePort;
    }

    public void setOverlaySourcePort(Integer overlaySourcePort) {
        this.overlaySourcePort = overlaySourcePort;
    }

    public Integer getOverlayDestinationPort() {
        return overlayDestinationPort;
    }

    public void setOverlayDestinationPort(Integer overlayDestinationPort) {
        this.overlayDestinationPort = overlayDestinationPort;
    }

    public String getOriginalSourceIP() {
        return originalSourceIP;
    }

    public void setOriginalSourceIP(String originalSourceIP) {
        this.originalSourceIP = originalSourceIP;
    }

    public String getOriginalDestinationIP() {
        return originalDestinationIP;
    }

    public void setOriginalDestinationIP(String originalDestinationIP) {
        this.originalDestinationIP = originalDestinationIP;
    }

    public Integer getOriginalSourcePort() {
        return originalSourcePort;
    }

    public void setOriginalSourcePort(Integer originalSourcePort) {
        this.originalSourcePort = originalSourcePort;
    }

    public Integer getOriginalDestinationPort() {
        return originalDestinationPort;
    }

    public void setOriginalDestinationPort(Integer originalDestinationPort) {
        this.originalDestinationPort = originalDestinationPort;
    }

    public String getSourceIP() {
        return sourceIP;
    }

    public void setSourceIP(String sourceIP) {
        this.sourceIP = sourceIP;
    }

    public String getDestinationIP() {
        return destinationIP;
    }

    public void setDestinationIP(String destinationIP) {
        this.destinationIP = destinationIP;
    }

    public Integer getSourcePort() {
        return sourcePort;
    }

    public void setSourcePort(Integer sourcePort) {
        this.sourcePort = sourcePort;
    }

    public Integer getProtocol() {
        return protocol;
    }

    public void setProtocol(Integer protocol) {
        this.protocol = protocol;
    }

    public Integer getDestinationPort() {
        return destinationPort;
    }

    public void setDestinationPort(Integer destinationPort) {
        this.destinationPort = destinationPort;
    }

    public String getAction() {
        return action;
    }

    public void setAction(String action) {
        this.action = action;
    }

    public String getsNATIP() {
        return sNATIP;
    }

    public void setsNATIP(String sNATIP) {
        this.sNATIP = sNATIP;
    }

    public Integer getsNATPort() {
        return sNATPort;
    }

    public void setsNATPort(Integer sNATPort) {
        this.sNATPort = sNATPort;
    }
}
