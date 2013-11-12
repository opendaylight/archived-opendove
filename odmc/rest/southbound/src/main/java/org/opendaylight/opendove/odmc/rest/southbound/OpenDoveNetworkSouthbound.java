/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.opendove.odmc.rest.southbound;

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
import org.opendaylight.controller.northbound.commons.exception.ResourceNotFoundException;
import org.opendaylight.controller.northbound.commons.exception.ServiceUnavailableException;
import org.opendaylight.opendove.odmc.IfOpenDoveNetworkCRUD;
import org.opendaylight.opendove.odmc.IfSBDoveSubnetCRUD;
import org.opendaylight.opendove.odmc.OpenDoveCRUDInterfaces;
import org.opendaylight.opendove.odmc.OpenDoveNetwork;
import org.opendaylight.opendove.odmc.OpenDoveSubnet;
import org.opendaylight.opendove.odmc.OpenDoveSwitch;
import org.opendaylight.opendove.odmc.rest.OpenDoveNetworkRequest;
import org.opendaylight.opendove.odmc.rest.OpenDoveSubnetRequest;

/**
 * Open DOVE Southbound REST APIs for Networks.<br>
 *
 * <br>
 * <br>
 * Authentication scheme [for now]: <b>HTTP Basic</b><br>
 * Authentication realm: <b>opendaylight</b><br>
 * Transport: <b>HTTP and HTTPS</b><br>
 * <br>
 * HTTPS Authentication is disabled by default. Administrator can enable it in
 * tomcat-server.xml after adding a proper keystore / SSL certificate from a
 * trusted authority.<br>
 * More info:
 * http://tomcat.apache.org/tomcat-7.0-doc/ssl-howto.html#Configuration
 *
 */

@Path("/networks")
public class OpenDoveNetworkSouthbound {

    /**
     * Show network
     *
     * @param networkUUID
     *            uuid of network
     * @return network Information
     *
     *         <pre>
     *
     * Example:
     *
     * Request URL:
     * http://127.0.0.1:8080/controller/sb/v2/opendove/odmc/networks/419fb938-26bf-459e-83b8-5a5700deec52
     *
     * Response body in JSON:
     * {
     *   "network": {
     *     "is_tombstone": false,
     *     "change_version": 2,
     *     "create_version": 2,
     *     "id": "419fb938-26bf-459e-83b8-5a5700deec52",
     *     "network_id": 9725101,
     *     "name": "Ext_MCast_9725101",
     *     "domain_uuid": "7c1a671d-f2fd-4c3e-ad7b-097263abc3ff",
     *     "type": 1
     *   }
     * }
     * </pre>
     */
    @Path("{networkUUID}")
    @GET
    @Produces({ MediaType.APPLICATION_JSON })
    @TypeHint(OpenDoveNetworkRequest.class)
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 204, condition = "No content"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 404, condition = "Not Found"),
            @ResponseCode(code = 500, condition = "Internal Error") })
    public Response showNetwork(
            @PathParam("networkUUID") String networkUUID
            ) {
        IfOpenDoveNetworkCRUD sbInterface = OpenDoveCRUDInterfaces.getIfDoveNetworkCRU(this);
        if (sbInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        if (!sbInterface.networkExists(networkUUID)) {
            throw new ResourceNotFoundException("Network not found");
        }
        return Response.status(200).entity(new OpenDoveNetworkRequest(sbInterface.getNetwork(networkUUID))).build();
    }

    /**
     * List all networks
     *
     * @param input
     *            none
     * @return list of all networks
     *
     *         <pre>
     *
     * Example:
     *
     * Request URL:
     * http://localhost:8080/controller/sb/v2/opendove/odmc/networks
     *
     * Response body in JSON:
     * {
     *    "networks": [ {
     *        "is_tombstone": false,
     *        "change_version": 2,
     *        "create_version": 2,
     *        "id": "419fb938-26bf-459e-83b8-5a5700deec52",
     *        "network_id": 9725101,
     *        "name": "Ext_MCast_9725101",
     *        "domain_uuid": "7c1a671d-f2fd-4c3e-ad7b-097263abc3ff",
     *        "type": 1
     *     }, {
     *        "is_tombstone": false,
     *        "change_version": 4,
     *        "create_version": 4,
     *        "id": "db256e1a-6601-4001-8ece-0ca2d1c75609",
     *        "network_id": 11099999,
     *        "name": "Neutron 0d051418-0f12-0b00-0000-000000000002",
     *        "domain_uuid": "7c1a671d-f2fd-4c3e-ad7b-097263abc3ff",
     *        "type": 0
     *     }, {
     *        "is_tombstone": false,
     *        "change_version": 3,
     *        "create_version": 3,
     *        "id": "11051cde-73e2-4193-8c94-bc45eca8022d",
     *        "network_id": 9220207,
     *        "name": "Neutron 0d051418-0f12-0b00-0000-000000000001",
     *        "domain_uuid": "7c1a671d-f2fd-4c3e-ad7b-097263abc3ff",
     *        "type": 0
     *     } ]
     * }
     * </pre>
     */
    @GET
    @Produces({ MediaType.APPLICATION_JSON })
    @TypeHint(OpenDoveNetworkRequest.class)
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 204, condition = "No content"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 500, condition = "Internal Error") })
    public Response listNetworks() {
        IfOpenDoveNetworkCRUD sbInterface = OpenDoveCRUDInterfaces.getIfDoveNetworkCRU(this);
        if (sbInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        return Response.status(200).entity(new OpenDoveNetworkRequest(sbInterface.getNetworks())).build();
    }

    /**
     * Signal endpoints attached to a network to re-register with the oDCS
     *
     * @param vnid
     *            virtual network identifier
     * @return none
     *
     *         <pre>
     *
     * Example:
     *
     * Request URL:
     * http://localhost:8080/controller/sb/v2/opendove/odmc/networks/9220207/endpoint-register
     * </pre>
     */
    @Path("{vnid}/endpoint-register")
    @PUT
    @StatusCodes({
        @ResponseCode(code = 204, condition = "No content"),
        @ResponseCode(code = 401, condition = "Unauthorized"),
        @ResponseCode(code = 404, condition = "Not Found"),
        @ResponseCode(code = 500, condition = "Internal Error") })
    public Response askReRegister(
            @PathParam("vnid") String vnid) {
        IfOpenDoveNetworkCRUD sbNetworkInterface = OpenDoveCRUDInterfaces.getIfDoveNetworkCRU(this);
        if (sbNetworkInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        if (!sbNetworkInterface.networkExistsByVnid(Integer.parseInt(vnid))) {
            throw new ResourceNotFoundException("Network not found");
        }
        for (OpenDoveSwitch oSwitch: sbNetworkInterface.getNetworkByVnid(Integer.parseInt(vnid)).getHostingSwitches()) {
            oSwitch.setReRegister(true);
        }
        return Response.status(204).build();

    }

    /**
     * Show subnet associated with a network (identified by vnid)
     *
     * @param vnid
     *            virtual network identifier
     * @param subnetUUID
     *            subnet UUID
     * @return subnet information
     *
     *         <pre>
     *
     * Example:
     *
     * Request URL:
     * http://127.0.0.1:8080/controller/sb/v2/opendove/odmc/networks/11099999/subnets/7787235a-4452-4107-9dfd-6731df5f9799
     *
     * Response body in JSON:
     * {
     *   "is_tombstone": false,
     *   "change_version": 8,
     *   "create_version": 8,
     *   "id": "7787235a-4452-4107-9dfd-6731df5f9799",
     *   "domain_id": "7c1a671d-f2fd-4c3e-ad7b-097263abc3ff",
     *   "subnet": "10.1.2.0",
     *   "mask": "255.255.255.0",
     *   "nexthop": "10.1.2.1",
     *   "type": "Dedicated",
     *   "network_ids": [ "db256e1a-6601-4001-8ece-0ca2d1c75609" ]
     * }
     * </pre>
     */
    @Path("{vnid}/subnets/{subnetUUID}")
    @GET
    @Produces({ MediaType.APPLICATION_JSON })
    @TypeHint(OpenDoveSubnet.class)
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 204, condition = "No content"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 404, condition = "Not Found"),
            @ResponseCode(code = 500, condition = "Internal Error") })
    public Response showNetwork(
            @PathParam("subnetUUID") String subnetUUID,
            @PathParam("vnid") String vnid
            ) {
        IfSBDoveSubnetCRUD sbSubnetInterface = OpenDoveCRUDInterfaces.getIfDoveSubnetCRUD(this);
        if (sbSubnetInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        IfOpenDoveNetworkCRUD sbNetworkInterface = OpenDoveCRUDInterfaces.getIfDoveNetworkCRU(this);
        if (sbNetworkInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        if (!sbSubnetInterface.subnetExists(subnetUUID)) {
            throw new ResourceNotFoundException("Subnet not found");
        }
        if (!sbNetworkInterface.networkExistsByVnid(Integer.parseInt(vnid))) {
            throw new ResourceNotFoundException("Network not found");
        }
        OpenDoveSubnet oDS = sbSubnetInterface.getSubnet(subnetUUID);
        OpenDoveNetwork oDN = sbNetworkInterface.getNetworkByVnid(Integer.parseInt(vnid));
        if (!oDS.getNetworkUUIDs().contains(oDN.getUUID())) {
            throw new ResourceNotFoundException("Subnet not associated with network");
        }
        return Response.status(200).entity(oDS).build();
    }

    /**
     * Show subnet associated with a network (identified by vnid)
     *
     * @param vnid
     *            virtual network identifier
     * @param subnetUUID
     *            subnet UUID
     * @return subnet information
     *
     *         <pre>
     *
     * Example:
     * Used for DGW
     * Request URL:
     * http://127.0.0.1:8080/controller/sb/v2/opendove/odmc/networks/11099999/networkSubnets/7787235a-4452-4107-9dfd-6731df5f9799
     *
     * Response body in JSON:
     * subnet
     * {
     *   "is_tombstone": false,
     *   "change_version": 8,
     *   "create_version": 8,
     *   "id": "7787235a-4452-4107-9dfd-6731df5f9799",
     *   "domain_id": "7c1a671d-f2fd-4c3e-ad7b-097263abc3ff",
     *   "subnet": "10.1.2.0",
     *   "mask": "255.255.255.0",
     *   "nexthop": "10.1.2.1",
     *   "type": "Dedicated",
     *   "network_ids": [ "db256e1a-6601-4001-8ece-0ca2d1c75609" ]
     * }
     * </pre>
     */
    @Path("{vnid}/networkSubnets/{subnetUUID}")
    @GET
    @Produces({ MediaType.APPLICATION_JSON })
    @TypeHint(OpenDoveSubnet.class)
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 204, condition = "No content"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 404, condition = "Not Found"),
            @ResponseCode(code = 500, condition = "Internal Error") })
    public Response showNetworkSubnets(
            @PathParam("subnetUUID") String subnetUUID,
            @PathParam("vnid") String vnid
            ) {
        IfSBDoveSubnetCRUD sbSubnetInterface = OpenDoveCRUDInterfaces.getIfDoveSubnetCRUD(this);
        if (sbSubnetInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        IfOpenDoveNetworkCRUD sbNetworkInterface = OpenDoveCRUDInterfaces.getIfDoveNetworkCRU(this);
        if (sbNetworkInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        if (!sbSubnetInterface.subnetExists(subnetUUID)) {
            throw new ResourceNotFoundException("Subnet not found");
        }
        if (!sbNetworkInterface.networkExistsByVnid(Integer.parseInt(vnid))) {
            throw new ResourceNotFoundException("Network not found");
        }
        OpenDoveSubnet oDS = sbSubnetInterface.getSubnet(subnetUUID);
        OpenDoveNetwork oDN = sbNetworkInterface.getNetworkByVnid(Integer.parseInt(vnid));
        if (!oDS.getNetworkUUIDs().contains(oDN.getUUID())) {
            throw new ResourceNotFoundException("Subnet not associated with network");
        }
        // return Response.status(200).entity(oDS).build();
        return Response.status(200).entity(new OpenDoveSubnetRequest(sbSubnetInterface.getSubnet(subnetUUID))).build();
    }
}

