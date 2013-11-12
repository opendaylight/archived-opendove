/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.opendove.odmc.rest.southbound;

import javax.ws.rs.Consumes;
import javax.ws.rs.GET;
import javax.ws.rs.POST;
import javax.ws.rs.PUT;
import javax.ws.rs.Path;
import javax.ws.rs.PathParam;
import javax.ws.rs.Produces;
import javax.ws.rs.core.MediaType;
import javax.ws.rs.core.Response;

import java.text.SimpleDateFormat;
import java.util.Calendar;


import org.codehaus.enunciate.jaxrs.ResponseCode;
import org.codehaus.enunciate.jaxrs.StatusCodes;
import org.codehaus.enunciate.jaxrs.TypeHint;
import org.opendaylight.controller.networkconfig.neutron.INeutronPortCRUD;
import org.opendaylight.controller.networkconfig.neutron.NeutronCRUDInterfaces;
import org.opendaylight.controller.networkconfig.neutron.NeutronPort;
import org.opendaylight.controller.northbound.commons.RestMessages;
import org.opendaylight.controller.northbound.commons.exception.ResourceConflictException;
import org.opendaylight.controller.northbound.commons.exception.ResourceNotFoundException;
import org.opendaylight.controller.northbound.commons.exception.ServiceUnavailableException;
import org.opendaylight.opendove.odmc.IfOpenDoveNetworkCRUD;
import org.opendaylight.opendove.odmc.IfOpenDoveSwitchCRUD;
import org.opendaylight.opendove.odmc.OpenDoveCRUDInterfaces;
import org.opendaylight.opendove.odmc.OpenDoveNetwork;
import org.opendaylight.opendove.odmc.OpenDoveSwitch;

/**
 * Open DOVE Southbound REST APIs for oDCS Switch.<br>
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

@Path("/switch")
public class OpenDoveSwitchSouthbound {

    /**
     * Registers an opendove switch with the oDMC
     *
     * @param input
     *            opendove switch information in JSON format
     * @return registered opendove switch information
     *
     *         <pre>
     *
     * Example:
     *
     * Request URL:
     * http://localhost:8080/controller/sb/v2/opendove/odmc/switch
     *
     * Request body in JSON:
     * {
     *   "name": "test_switch",
     *   "tunnelip": "5.5.5.5",
     *   "managementip": "6.6.6.6"
     * }
     *
     * Response body in JSON:
     * {
     *   "change_version" : 13,
     *   "create_version" : 13,
     *   "id" : "5086a907-3107-4cda-8e99-6b67675634b2",
     *   "name" : "test_switch",
     *   "tunnelip" : "5.5.5.5",
     *   "managementip" : "6.6.6.6",
     *   "timestamp" : "Sun Nov 03 09:03:51 CST 2013"
     * }
     * </pre>
     */
    @POST
    @Consumes({ MediaType.APPLICATION_JSON })
    @Produces({ MediaType.APPLICATION_JSON })
    @TypeHint(OpenDoveSwitch.class)
    @StatusCodes({
        @ResponseCode(code = 201, condition = "Registration Accepted"),
        @ResponseCode(code = 204, condition = "Switch previously registered, no endpoint registration needed"),
        @ResponseCode(code = 205, condition = "Switch previously registered, endpoint registration required"),
        @ResponseCode(code = 500, condition = "Internal Error") })
    public Response processRegistration (OpenDoveSwitch openDoveSwitch) {
        IfOpenDoveSwitchCRUD sbInterface = OpenDoveCRUDInterfaces.getIfOpenDoveSwitchCRU(this);

        if (sbInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }

        if (sbInterface.switchExists(openDoveSwitch.getUUID())) {
            sbInterface.updateSwitch(openDoveSwitch.getUUID(), openDoveSwitch);
            OpenDoveSwitch target = sbInterface.getSwitch(openDoveSwitch.getUUID());
            if (target.getReRegister()) {
                target.setReRegister(false);
                sbInterface.updateSwitch(openDoveSwitch.getUUID(), target);
                return Response.status(205).entity(openDoveSwitch).build();
            } else
                return Response.status(204).entity(openDoveSwitch).build();
        }

        // Set the Timestamp, UUID and Registration flag
        String timestamp = new SimpleDateFormat("EEE MMM dd HH:mm:ss zzz yyyy").format(Calendar.getInstance().getTime());
        openDoveSwitch.setTimestamp(timestamp);
        openDoveSwitch.setUUID(java.util.UUID.randomUUID().toString());
        openDoveSwitch.setTombstoneFlag(new Boolean(false));
        openDoveSwitch.setReRegister(new Boolean(false));
        sbInterface.addSwitch(openDoveSwitch.getUUID(), openDoveSwitch);

        return Response.status(201).entity(openDoveSwitch).build();
    }

    /**
     * Heartbeat method for an opendove switch
     *
     * @param input
     *            updated opendove switch information in JSON format
     * @return none
     *
     *         <pre>
     *
     * Example:
     *
     * Request URL:
     * http://localhost:8080/controller/sb/v2/opendove/odmc/switch/5086a907-3107-4cda-8e99-6b67675634b2
     *
     * Request body in JSON:
     * {
     *   "name": "test_switch",
     *   "tunnelip": "5.5.5.5",
     *   "managementip": "6.6.6.6"
     * }
     *
     * </pre>
     */
    @Path("/{switchUUID}")
    @PUT
    @StatusCodes({
        @ResponseCode(code = 204, condition = "No endpoint registration needed"),
        @ResponseCode(code = 205, condition = "Endpoint registration required"),
        @ResponseCode(code = 409, condition = "Switch not registered"),
        @ResponseCode(code = 500, condition = "Internal Error") })

    public Response procesHeartbeat (
            @PathParam("switchUUID") String switchUUID,
            OpenDoveSwitch openDoveSwitch) {

        IfOpenDoveSwitchCRUD sbInterface = OpenDoveCRUDInterfaces.getIfOpenDoveSwitchCRU(this);

        if (sbInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }

        /*
         *  Heart-Beat from Same UUID with a different IP will be accepted - It will be
         *  treated as a change in IP Address, Infinispan Cache will be updated in this
         *  case.
         *
         *  Heart-Beat from non-existent in DMC Cache will
         *  treated as a conflict
         */
        if (!sbInterface.switchExists(switchUUID))
            throw new ResourceConflictException("Heartbeat not accepted from unregistered switch");

        String timestamp = new SimpleDateFormat("EEE MMM dd HH:mm:ss zzz yyyy").format(Calendar.getInstance().getTime());
        openDoveSwitch.setTimestamp(timestamp);

        sbInterface.updateSwitch(switchUUID, openDoveSwitch);
        OpenDoveSwitch target = sbInterface.getSwitch(switchUUID);
        if (target.getReRegister()) {
            target.setReRegister(false);
            sbInterface.updateSwitch(openDoveSwitch.getUUID(), target);
            return Response.status(205).build();
        } else
            return Response.status(204).build();
    }

    /**
     * Map openstack port to opendove vnid
     *
     * @param switchUUID
     *            UUID of switch making request
     * @param portUUID
     *            openstack neutron port UUID
     * @return opendove network information
     *
     *         <pre>
     *
     * Example:
     *
     * Request URL:
     * http://127.0.0.1:8080/controller/sb/v2/opendove/odmc/switch/473931b4-0f79-4139-8af2-87dc100be0de/getVNIDbyPort/100f1214-0000-0000-0000-000000000001
     *
     * Response body in JSON
     * {
     *   "is_tombstone" : false,
     *   "change_version" : 5,
     *   "create_version" : 5,
     *   "id" : "892dab23-94c9-4676-94c9-89da84b8cdf1",
     *   "network_id" : 13586458,
     *   "name" : "Neutron 0d051418-0f12-0b00-0000-000000000001",
     *   "domain_uuid" : "e47a1688-ed1d-4163-a3c6-c8be30c714ea",
     *   "type" : 0
     * }
     *
     * </pre>
     */
    @Path("/{switchUUID}/getVNIDbyPort/{portUUID}")
    @GET
    @Produces({ MediaType.APPLICATION_JSON })
    @TypeHint(OpenDoveNetwork.class)
    @StatusCodes({
        @ResponseCode(code = 204, condition = "No content"),
        @ResponseCode(code = 401, condition = "Unauthorized"),
        @ResponseCode(code = 404, condition = "some Resource Not Found"),
        @ResponseCode(code = 500, condition = "Internal Error") })
    public Response getByPort(
            @PathParam("switchUUID") String switchUUID,
            @PathParam("portUUID") String portUUID) {
        IfOpenDoveSwitchCRUD sbInterface = OpenDoveCRUDInterfaces.getIfOpenDoveSwitchCRU(this);

        if (sbInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }

        if (!sbInterface.switchExists(switchUUID))
            throw new ResourceNotFoundException("Switch does not exist");
        OpenDoveSwitch oSwitch = sbInterface.getSwitch(switchUUID);

        INeutronPortCRUD sbNeutronPortInterface = NeutronCRUDInterfaces.getINeutronPortCRUD(this);
        if (!sbNeutronPortInterface.portExists(portUUID))
            throw new ResourceNotFoundException("Port does not exist");

        NeutronPort nPort = sbNeutronPortInterface.getPort(portUUID);

        IfOpenDoveNetworkCRUD sbNetworkInterface = OpenDoveCRUDInterfaces.getIfDoveNetworkCRU(this);
        if (sbNetworkInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        for (OpenDoveNetwork network : sbNetworkInterface.getNetworks()) {
            if (network.getNeutronNetwork().equalsIgnoreCase(nPort.getNetworkUUID()) &&
                    network.getScopingDomain().getAssociatedOSTenantUUID().equalsIgnoreCase(nPort.getTenantID())) {
                if (!network.getHostingSwitches().contains(oSwitch))
                    network.getHostingSwitches().add(oSwitch);
                return Response.status(200).entity(network).build();
            }
        }
        throw new ResourceNotFoundException("No OpenDove Network found for port");
    }
}

