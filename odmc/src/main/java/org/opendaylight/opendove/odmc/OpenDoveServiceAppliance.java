/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.opendove.odmc;


import javax.xml.bind.annotation.XmlAccessType;
import javax.xml.bind.annotation.XmlAccessorType;
import javax.xml.bind.annotation.XmlElement;
import javax.xml.bind.annotation.XmlRootElement;

/*
        "uuid"  == PRIMARY KEY in the Cache

        {"ip_family",                 INT                 },
        {"ip",                        STRING              },
        {"uuid",                      STRING_PRIMARY_KEY  },
        {"dcs_rest_port",             INT                 },
        {"dgw_rest_port",             INT                 },
        {"dcs_raw_service_port",      INT                 },
        {"timestamp",                 STRING              },
        {"build_version",             STRING              },
        {"dcs_config_version",        INT                 },
        {"dgw_config_version",        INT                 },
        {"canBeDCS",                  BOOLEAN             },
        {"canBeDGW",                  BOOLEAN             },
        {"isDCS",                     BOOLEAN             },
        {"isDGW",                     BOOLEAN             },
*/


interface ServiceApplianceType
{
    public static final int NONE  = 0;
    public static final int DCS   = 1;
    public static final int DGW   = 2;
}

@XmlRootElement
@XmlAccessorType(XmlAccessType.NONE)
public class OpenDoveServiceAppliance  {


    @XmlElement (name="ip_family")
    Integer ip_family;

    @XmlElement (name="ip")
    String ip;

    @XmlElement (name="uuid")
    String uuid;

    @XmlElement (name="dcs_rest_service_port")
    Integer dcs_rest_service_port;

    @XmlElement (name="dgw_rest_service_port")
    Integer dgw_rest_service_port;

    @XmlElement (name="dcs_raw_service_port")
    Integer dcs_raw_service_port;

    @XmlElement (name="timestamp")
    String timestamp;

    @XmlElement (name="build_version")
    String build_version;

    @XmlElement (name="dcs_config_version")
    Integer dcs_config_version;

    @XmlElement (name="dgw_config_version")
    Integer dgw_config_version;

    @XmlElement (name="canBeDCS")
    Boolean canBeDCS;

    @XmlElement (name="canBeDGW")
    Boolean canBeDGW;

    @XmlElement (name="isDCS")
    Boolean isDCS;

    @XmlElement (name="isDGW")
    Boolean isDGW;

    public OpenDoveServiceAppliance () {
        this.ip_family                    = 0;
        this.ip                           = "";
        this.uuid                         = "";
        this.dcs_rest_service_port        = 0;
        this.dgw_rest_service_port        = 0;
        this.dcs_raw_service_port         = 0;
        this.timestamp                    = "";
        this.build_version                = "";
        this.dcs_config_version           = 0;
        this.dgw_config_version           = 0;
        this.canBeDCS                     = false;
        this.canBeDGW                     = false;
        this.isDCS                        = false;
        this.isDGW                        = false;
    }

    public Integer getIPFamily () {
        return ip_family;
    }

    public void setIPFamily (Integer ip_family) {
        this.ip_family = ip_family;
    }

    public String getIP() {
        return ip;
    }

    public void setIP(String ip) {
        this.ip = ip;
    }

    public String getUUID() {
        return uuid;
    }

    public String setUUID( String dsaUUID) {
        return this.uuid = dsaUUID;
    }

    public Integer getDcsRestServicePort () {
       return dcs_rest_service_port;
    } 

    public void setDcsRestServicePort (Integer dcs_rest_service_port) {
       this.dcs_rest_service_port = dcs_rest_service_port;
    } 

    public Integer getDgwRestServicePort () {
       return dgw_rest_service_port;
    } 

    public void setDgwRestServicePort (Integer dgw_rest_service_port) {
       this.dgw_rest_service_port = dgw_rest_service_port;
    } 

    public Integer getDcsRawServicePort () {
       return dcs_raw_service_port;
    } 

    public void setDcsRawServicePort (Integer dcs_raw_service_port) {
       this.dcs_raw_service_port = dcs_raw_service_port;
    } 

    public String getTimestamp() {
        return timestamp;
    }

    public void setTimestamp(String timestamp) {
       this.timestamp = timestamp;
    }
    public String get_build_version() {
       return build_version;
    }
    public void set_build_version(String build_version) {
       this.build_version =  build_version;
    }
  
    public Integer get_dcs_config_version () {
       return dcs_config_version;
    }

    public void set_dcs_config_version ( Integer dcs_config_version) {
       this.dcs_config_version = dcs_config_version;
    }

    public Integer get_dgw_config_version () {
       return dgw_config_version;
    }

    public void set_dgw_config_version ( Integer dgw_config_version) {
       this.dgw_config_version = dgw_config_version;
    }
    public Boolean get_canBeDCS () {
       return canBeDCS;
    }

    public void set_canBeDCS ( Boolean canBeDCS) {
       this.canBeDCS = canBeDCS;
    }
    public Boolean get_canBeDGW () {
       return canBeDGW;
    }

    public void set_canBeDGW ( Boolean canBeDGW) {
       this.canBeDCS = canBeDGW;
    }
    public Boolean get_isDCS () {
       return isDCS;
    }

    public void set_isDCS ( Boolean isDCS) {
       this.isDCS = isDCS;
    }
    public Boolean get_isDGW () {
       return isDGW;
    }

    public void set_isDGW ( Boolean isDGW) {
       this.isDGW = isDGW;
    }
    public void initDefaults() {
        // TODO Auto-generated method stub
    }
}