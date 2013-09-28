/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.opendove.odmc;


import javax.xml.bind.annotation.XmlElement;
import java.util.Date;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Calendar;

/*
        "uuid"  == PRIMARY KEY in the Cache

        {"ip_family",                 INT                 },
        {"ip",                        STRING              },
        {"uuid",                      STRING_PRIMARY_KEY  },
        {"dcs_rest_port",             INT                 },
        {"dgw_rest_port",             INT                 },
        {"dcs_raw_service_port",      INT                 },
        {"is_dcs_leader",             INT                 },
        {"timestamp",                 STRING              },
        {"build_version",             STRING              },
        {"dcs_config_version",        INT                 },
        {"dgw_config_version",        INT                 },
        {"create_version",            INT                 },
        {"dcs_change_version",        INT                 },
        {"dgw_change_version",        INT                 },
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


public class OpenDoveServiceAppliance extends OpenDoveObject {


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

    @XmlElement (name="is_dcs_leader")
    Integer is_dcs_leader;

    @XmlElement (name="timestamp")
    String timestamp;

    @XmlElement (name="build_version")
    String build_version;

    @XmlElement (name="dcs_config_version")
    Integer dcs_config_version;

    @XmlElement (name="create_version")
    Integer create_version;

    @XmlElement (name="dgw_config_version")
    Integer dgw_config_version;

    @XmlElement (name="dcs_change_version")
    Integer dcs_change_version;

    @XmlElement (name="dgw_change_version")
    Integer dgw_change_version;

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
        this.is_dcs_leader                = 0;
        this.timestamp                    = "";
        this.build_version                = "";
        this.dcs_config_version           = 0;
        this.dgw_config_version           = 0;
        this.create_version               = 0;
        this.dcs_change_version           = 0;
        this.dgw_change_version           = 0;
        this.canBeDCS                     = false;
        this.canBeDGW                     = false;
        this.isDCS                        = false;
        this.isDGW                        = false;
    }

    public OpenDoveServiceAppliance (Integer ip_family, String ip, String uuid, Integer dcs_rest_service_port,
                                     Integer dcs_raw_service_port, Integer is_dcs_leader, Boolean canBeDCS,
                                     String build_version) {
        this.ip_family             = ip_family;
        this.ip                    = ip;
        this.uuid                  = uuid;
        this.dcs_rest_service_port = dcs_rest_service_port;
        this.dcs_raw_service_port  = dcs_raw_service_port;
        this.is_dcs_leader         = is_dcs_leader;
        this.build_version         = build_version;
        this.canBeDCS              = canBeDCS;
    }

    public String getIP() {
        return ip;
    }

    public String getUUID() {
        return uuid;
    }

    public String setUUID( String dsaUUID) {
        return this.uuid = dsaUUID;
    }

    public void setTimestamp() {
       this.timestamp = new SimpleDateFormat("EEE MMM dd HH:mm:ss zzz yyyy").format(Calendar.getInstance().getTime());
    }

    public void initDefaults() {
        // TODO Auto-generated method stub
    }
}
