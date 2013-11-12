/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.opendove.odmc.rest.northbound;

import javax.ws.rs.Consumes;
import javax.ws.rs.GET;
import javax.ws.rs.PUT;
import javax.ws.rs.Path;
import javax.ws.rs.PathParam;
import javax.ws.rs.Produces;
import javax.ws.rs.core.MediaType;
import javax.ws.rs.core.Response;

import org.codehaus.enunciate.jaxrs.ResponseCode;
import org.codehaus.enunciate.jaxrs.StatusCodes;
import org.codehaus.enunciate.jaxrs.TypeHint;
import org.opendaylight.controller.northbound.commons.RestMessages;
import org.opendaylight.controller.northbound.commons.exception.ServiceUnavailableException;
import org.opendaylight.opendove.odmc.IfOpenDoveServiceApplianceCRUD;
import org.opendaylight.opendove.odmc.OpenDoveCRUDInterfaces;
import org.opendaylight.opendove.odmc.rest.OpenDoveGWSessionStatsRequest;
import org.opendaylight.opendove.odmc.rest.OpenDoveGWStats;
import org.opendaylight.opendove.odmc.rest.OpenDoveRestClient;
import org.opendaylight.opendove.odmc.rest.OpenDoveServiceApplianceRequest;
import org.opendaylight.opendove.odmc.rest.OpenDoveVNIDStats;
import org.opendaylight.opendove.odmc.OpenDoveServiceAppliance;
import org.opendaylight.opendove.odmc.OpenDoveGwIpv4;
import org.opendaylight.opendove.odmc.rest.OpenDoveGwIpv4Request;
import org.opendaylight.opendove.odmc.IfSBDoveGwIpv4CRUD;

/**
 * Open DOVE Northbound REST APIs for DGW Service Appliance.<br>
 *
 * <br>
 * <br>
 * Authentication scheme [for now]: <b>HTTP Basic</b><br>
 * Authentication realm : <b>opendaylight</b><br>
 * Transport : <b>HTTP and HTTPS</b><br>
 * <br>
 * HTTPS Authentication is disabled by default. Administrator can enable it in
 * tomcat-server.xml after adding a proper keystore / SSL certificate from a
 * trusted authority.<br>
 * More info :
 * http://tomcat.apache.org/tomcat-7.0-doc/ssl-howto.html#Configuration
 *
 */

@Path("/odgw")
public class OpenDoveDgwServiceApplianceNorthbound {

    /**
     * Sets/Resets the odgw service role on a service appliance
     *
     * @param odgwUUID
     *            serivce appliance UUID to modify
     * @return Updated service appliance information
     *
     *         <pre>
     *
     * Example:
     *
     * Request URL:
     * http://localhost:8080/controller/nb/v2/opendove/odgw/uuid/role
     *
     * Request body in JSON:
     * {
     *    "service_appliance": {
     *      "isDGW": true
     *    }
     * }
     *
     * Response body in JSON:
     * {
     *   "service_appliance": {
     *     "ip_family": 4,
     *     "ip": "10.10.10.1",
     *     "uuid": "uuid",
     *     "dcs_rest_service_port": 1888,
     *     "dgw_rest_service_port": 1888,
     *     "dcs_raw_service_port": 932,
     *     "timestamp": "now",
     *     "build_version": "openDSA-1",
     *     "dcs_config_version": 60,
     *     "canBeDCS": true,
     *     "canBeDGW": true,
     *     "isDCS": false,
     *     "isDGW": true
     *   }
     * }
     * </pre>
     */
    @Path("/{odgwUUID}/role")
    @PUT
    @Consumes({ MediaType.APPLICATION_JSON })
    @Produces({ MediaType.APPLICATION_JSON })
    @TypeHint(OpenDoveServiceApplianceRequest.class)
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 204, condition = "No content"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 409, condition = "DCS is in a Conflicted State"),
            @ResponseCode(code = 404, condition = "Not Found"),
            @ResponseCode(code = 500, condition = "Internal Error")
            })
    public Response nbAssignDgwServiceApplianceRole(
            @PathParam("odgwUUID") String odgwUUID,
            OpenDoveServiceApplianceRequest request
            ) {
        IfOpenDoveServiceApplianceCRUD sbInterface = OpenDoveCRUDInterfaces.getIfDoveServiceApplianceCRUD(this);
        if (sbInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        if (!sbInterface.applianceExists(odgwUUID)) {
            return Response.status(404).build();
        }
        if (!request.isSingleton()) {
            return Response.status(400).build();
        }
        OpenDoveServiceAppliance delta = request.getSingleton();
        if (delta.get_isDGW() == null) {
            return Response.status(400).build();
        }

        OpenDoveServiceAppliance dgwAppliance = sbInterface.getDoveServiceAppliance(odgwUUID);
        if (!dgwAppliance.get_canBeDGW())
         {
            return Response.status(400).build();
        //dgwAppliance.set_isDGW(delta.get_isDGW());
        }

        /*
         *   No Need to send the REST Request to O-DGW, Just update the isDGW field in the Cache.
         *   OpenDoveRestClient sbRestClient = new OpenDoveRestClient();
         */
         //   sbRestClient.assignDgwServiceApplianceRole(dgwAppliance);
        sbInterface.updateDoveServiceAppliance(odgwUUID, delta);
        return Response.status(200).entity(new OpenDoveServiceApplianceRequest(sbInterface.getDoveServiceAppliance(odgwUUID))).build();
    }

    /**
     * Creates odgw IPv4 Interface ("dovetunnel"/"external")  on a service appliance
     *
     * @param odgwUUID
     *            serivce appliance UUID to modify
     * @return Updated DGW IPv4 Interface information
     *
     *         <pre>
     *
     * Example:
     *
     * Request URL:
     * http://localhost:8080/controller/nb/v2/opendove/odgw/uuid/ipv4
     *
     * Request body in JSON:
     * {
     *    "gw_ipv4_assignment": {
     *      "id"         : STRING
     *      "ip"         : STRING
     *      "mask"       : STRING
     *      "nexthop"    : STRING
     *      "intf_type"  : STRING  ("dovetunnel")
     *      "vlan"       : INTEGER
     *    }
     * }
     *
     * Response body in JSON:
     * {
     *   "gw_ipv4_assignment": {
     *      "id"         : STRING
     *      "ip"         : STRING
     *      "nexthop"    : STRING
     *      "mask"       : STRING
     *      "intf_type"  : STRING  ("dovetunnel")
     *      "gwUUID"     : STRING  ("dovetunnel")
     *      "vlan"       : INTEGER
     *   }
     * }
     * </pre>
     */
    @Path("/{odgwUUID}/ipv4")
    @PUT
    @Consumes({ MediaType.APPLICATION_JSON })
    @Produces({ MediaType.APPLICATION_JSON })
    @TypeHint(OpenDoveServiceApplianceRequest.class)
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 204, condition = "No content"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 409, condition = "DGW is in a Conflicted State"),
            @ResponseCode(code = 404, condition = "Not Found"),
            @ResponseCode(code = 500, condition = "Internal Error")
            })
    public Response nbCreateDgwIPv4Interface(
            @PathParam("odgwUUID") String odgwUUID,
            OpenDoveGwIpv4Request request
            ) {
        IfOpenDoveServiceApplianceCRUD sbInterface = OpenDoveCRUDInterfaces.getIfDoveServiceApplianceCRUD(this);
        IfSBDoveGwIpv4CRUD sbGWIpv4Interface = OpenDoveCRUDInterfaces.getIfSBDoveGwIpv4CRUD(this);
        if (sbInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        if (!sbInterface.applianceExists(odgwUUID)) {
            return Response.status(404).build();
        }
        if (!request.isSingleton()) {
            return Response.status(400).build();
        }

        OpenDoveGwIpv4 gwIpv4 = request.getSingleton();
        if (gwIpv4.getIP() == null) {
            return Response.status(400).build();
        }

        OpenDoveServiceAppliance dgwAppliance = sbInterface.getDoveServiceAppliance(odgwUUID);
        if (!dgwAppliance.get_canBeDGW()) {
            return Response.status(400).build();
        }

        String ipv4UUID;
        if (!dgwAppliance.get_isDGW()) {
            return Response.status(400).build();
        } else {
            IfSBDoveGwIpv4CRUD gatewayIPDB = OpenDoveCRUDInterfaces.getIfSBDoveGwIpv4CRUD(this);
            OpenDoveGwIpv4 newGWIP = new OpenDoveGwIpv4(gwIpv4.getIP(), gwIpv4.getMask(), gwIpv4.getNexthop(), gwIpv4.getType(), odgwUUID, 0);
            gatewayIPDB.addGwIpv4(newGWIP.getUUID(), newGWIP);
            dgwAppliance.setDoveTunnel(newGWIP);
            ipv4UUID = newGWIP.getUUID();
        }

        // return Response.status(200).entity(new OpenDoveServiceApplianceRequest(sbInterface.getDoveServiceAppliance(odgwUUID))).build();
        return Response.status(200).entity(new OpenDoveGwIpv4Request(sbGWIpv4Interface.getGwIpv4(ipv4UUID))).build();
    }

    /**
     * Gets statistics for a gateway
     *
     * @param odgwUUID
     *            gateway UUID to interrogate
     * @return statistics
     *
     *         <pre>
     *
     * Example:
     *
     * Request URL:
     * http://localhost:8080/controller/nb/v2/opendove/odgw/uuid/allstats
     *
     * Response body in JSON:
     * {
     *     "ovl_to_ext_leave_bytes": "count",
     *     "ovl_to_ext_leave_pkts": "count",
     *     "ovl_to_ext_leave_bps": "count",
     *     "ovl_to_ext_leave_pps": "count",
     *     "ext_to_ovl_enter_bytes": "count",
     *     "ext_to_ovl_enter_pkts": "count",
     *     "ext_to_ovl_enter_bps": "count",
     *     "ext_to_ovl_enter_pps": "count",
     *     "ovl_to_vlan_leave_bytes": "count",
     *     "ovl_to_vlan_leave_pkts": "count",
     *     "ovl_to_vlan_leave_bps": "count",
     *     "ovl_to_vlan_leave_pps": "count",
     *     "vlan_to_ovl_enter_bytes": "count",
     *     "vlan_to_ovl_enter_pkts": "count",
     *     "vlan_to_ovl_enter_bps": "count",
     *     "vlan_to_ovl_enter_pps": "count"
     * }
     * </pre>
     */
    @Path("/{odgwUUID}/allstats")
    @GET
    @Produces({ MediaType.APPLICATION_JSON })
    @TypeHint(OpenDoveGWStats.class)
    @StatusCodes({
        @ResponseCode(code = 200, condition = "Operation successful"),
        @ResponseCode(code = 204, condition = "No content"),
        @ResponseCode(code = 404, condition = "Not Found"),
        })
    public Response getAllStats(
            @PathParam("odgwUUID") String odgwUUID
            ) {


//        IfOpenDoveDgwStatsCRU sbInterface = OpenDoveCRUDInterfaces.getIfOpenDoveDgwCRU(this);
        IfOpenDoveServiceApplianceCRUD sbInterface = OpenDoveCRUDInterfaces.getIfDoveServiceApplianceCRUD(this);        if (sbInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        OpenDoveServiceAppliance dgwAppliance = sbInterface.getDoveServiceAppliance(odgwUUID);

        if (!sbInterface.applianceExists(odgwUUID)) {
            return Response.status(404).build();
        }

        OpenDoveRestClient client = new OpenDoveRestClient();
        OpenDoveGWStats answer = client.getDgwAllStats(dgwAppliance, odgwUUID);
        if (answer == null) {
            return Response.status(204).build();
        }
        return Response.status(200).entity(answer).build();
    }

    /**
     * Gets session statistics for a gateway
     *
     * @param odgwUUID
     *            gateway UUID to interrogate
     * @return statistics
     *
     *         <pre>
     *
     * Example:
     *
     * Request URL:
     * http://localhost:8080/controller/nb/v2/opendove/odgw/uuid/session_stats
     *
     * Response body in JSON:
     * {
     *   "sessions": [ {
     *     "net_id": "%s",
     *     "age": "%s",
     *     "ovl_sip": "%s",
     *     "ovl_dip": "%s",
     *     "ovl_sport": %d,
     *     "ovl_dport": %d,
     *     "orig_sip": "%s",
     *     "orig_dip": "%s",
     *     "orig_sport": %d,
     *     "orig_dport": %d,
     *     "sip": "%s",
     *     "dip": "%s",
     *     "proto": %d,
     *     "sport": %d,
     *     "dport": %d,
     *     "action": "%s",
     *     "snat_ip": "%s",
     *     "snat_port": %d
     *   } ]
     * }
     * </pre>
     */
    @Path("/{odgwUUID}/session_stats")
    @GET
    @Produces({ MediaType.APPLICATION_JSON })
    @TypeHint(OpenDoveGWSessionStatsRequest.class)
    @StatusCodes({
        @ResponseCode(code = 200, condition = "Operation successful"),
        @ResponseCode(code = 204, condition = "No content"),
        @ResponseCode(code = 404, condition = "Not Found"),
        })

    public Response getSessionStats(
            @PathParam("odgwUUID") String odgwUUID) {
        //return Response.status(501).build();


        //IfOpenDoveDgwStatsCRU sbInterface = OpenDoveCRUDInterfaces.getIfOpenDoveDgwCRU(this);
        IfOpenDoveServiceApplianceCRUD sbInterface = OpenDoveCRUDInterfaces.getIfDoveServiceApplianceCRUD(this);
        if (sbInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }

        OpenDoveServiceAppliance dgwAppliance = sbInterface.getDoveServiceAppliance(odgwUUID);
        if (!sbInterface.applianceExists(odgwUUID)) {
            return Response.status(404).build();
        }

        OpenDoveRestClient client = new OpenDoveRestClient();

        OpenDoveGWSessionStatsRequest answer = client.getDgwSessionStats(dgwAppliance, odgwUUID);
        if (answer == null) {
            return Response.status(204).build();
        }
        return Response.status(200).entity(answer).build();
    }

    /**
     * Gets vnid statistics for a gateway
     *
     * @param odgwUUID
     *            gateway UUID to interrogate
     * @param vnid
     *            VNID to get statistics on
     * @return statistics
     *
     *         <pre>
     *
     * Example:
     *
     * Request URL:
     * http://localhost:8080/controller/nb/v2/opendove/odgw/uuid/allstats
     *
     * Response body in JSON:
     * {
     *     "net_id": "vnid",
     *     "ovl_to_ext_leave_bytes": "count",
     *     "ovl_to_ext_leave_pkts": "count",
     *     "ovl_to_ext_leave_bps": "count",
     *     "ovl_to_ext_leave_pps": "count",
     *     "ext_to_ovl_enter_bytes": "count",
     *     "ext_to_ovl_enter_pkts": "count",
     *     "ext_to_ovl_enter_bps": "count",
     *     "ext_to_ovl_enter_pps": "count",
     *     "ovl_to_vlan_leave_bytes": "count",
     *     "ovl_to_vlan_leave_pkts": "count",
     *     "ovl_to_vlan_leave_bps": "count",
     *     "ovl_to_vlan_leave_pps": "count",
     *     "vlan_to_ovl_enter_bytes": "count",
     *     "vlan_to_ovl_enter_pkts": "count",
     *     "vlan_to_ovl_enter_bps": "count",
     *     "vlan_to_ovl_enter_pps": "count"
     * }
     * </pre>
     */
    @Path("/{odgwUUID}/vnid_stats/{vnid}")
    @GET
    @Produces({ MediaType.APPLICATION_JSON })
    @TypeHint(OpenDoveVNIDStats.class)
    @StatusCodes({
        @ResponseCode(code = 200, condition = "Operation successful"),
        @ResponseCode(code = 204, condition = "No content"),
        })
    public Response getVNIDStats(
            @PathParam("odgwUUID") String odgwUUID,
            @PathParam("vnid") String vnid ) {
        // return Response.status(501).build();

        //IfOpenDoveDgwStatsCRU sbInterface = OpenDoveCRUDInterfaces.getIfOpenDoveDgwCRU(this);
        IfOpenDoveServiceApplianceCRUD sbInterface = OpenDoveCRUDInterfaces.getIfDoveServiceApplianceCRUD(this);
        if (sbInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        OpenDoveServiceAppliance dgwAppliance = sbInterface.getDoveServiceAppliance(odgwUUID);

        if (!sbInterface.applianceExists(odgwUUID)) {
            return Response.status(404).build();
        }

        OpenDoveRestClient client = new OpenDoveRestClient();

        OpenDoveVNIDStats answer = client.getDgwVNIDStats(dgwAppliance, odgwUUID, vnid);
        if (answer == null) {
            return Response.status(204).build();
        }
        return Response.status(200).entity(answer).build();
    }

}

