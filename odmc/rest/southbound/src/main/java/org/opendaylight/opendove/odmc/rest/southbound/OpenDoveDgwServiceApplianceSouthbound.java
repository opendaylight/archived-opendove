/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.opendove.odmc.rest.southbound;

import javax.ws.rs.Consumes;
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
import org.opendaylight.controller.northbound.commons.RestMessages;
import org.opendaylight.controller.northbound.commons.exception.ResourceConflictException;
import org.opendaylight.controller.northbound.commons.exception.ServiceUnavailableException;
import org.opendaylight.opendove.odmc.IfOpenDoveServiceApplianceCRUD;
import org.opendaylight.opendove.odmc.OpenDoveCRUDInterfaces;
import org.opendaylight.opendove.odmc.OpenDoveServiceAppliance;

/**
 * Open DOVE Southbound REST APIs for DGW Service Appliance.<br>
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
public class OpenDoveDgwServiceApplianceSouthbound {

    /**
     * Registers an oDGW with the oDMC
     *
     * @param input
     *            oDGW information in JSON format
     * @return registered oDGW information
     *
     *         <pre>
     *
     * Example:
     *
     * Request URL:
     * http://localhost:8080/controller/sb/v2/opendove/odmc/odgw
     *
     * Request body in JSON:
     * {
     *   "ip_family": 0,
     *   "ip": "1.1.1.1",
     *   "uuid": "1",
     *   "dcs_rest_service_port": 0,
     *   "dgw_rest_service_port": 0,
     *   "dcs_raw_service_port": 0,
     *   "build_version": "",
     *   "dcs_config_version": 0,
     *   "dgw_config_version": 0,
     *   "canBeDCS": false,
     *   "canBeDGW": true,
     *   "isDCS": false,
     *   "isDGW": false
     * }
     *
     * Response body in JSON:
     * {
     *   "ip_family": 0,
     *   "ip": "1.1.1.1",
     *   "uuid": "1",
     *   "dcs_rest_service_port": 0,
     *   "dgw_rest_service_port": 0,
     *   "dcs_raw_service_port": 0,
     *   "timestamp": "Thu Oct 03 13:50:01 PDT 2013",
     *   "build_version": "",
     *   "dcs_config_version": 0,
     *   "dgw_config_version": 0,
     *   "canBeDCS": false,
     *   "canBeDGW": true,
     *   "isDCS": false,
     *   "isDGW": false
     * }
     * </pre>
     */
    @POST
    @Consumes({ MediaType.APPLICATION_JSON })
    @Produces({ MediaType.APPLICATION_JSON })
    @TypeHint(OpenDoveServiceAppliance.class)
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 201, condition = "Registration Accepted"),
            @ResponseCode(code = 409, condition = "Service Appliance IP Address Conflict"),
            @ResponseCode(code = 500, condition = "Internal Error") })
    public Response processDgwRegistration (OpenDoveServiceAppliance appliance) {
        String dsaIP   = appliance.getIP();
        String dsaUUID = appliance.getUUID();

        IfOpenDoveServiceApplianceCRUD sbInterface = OpenDoveCRUDInterfaces.getIfDoveServiceApplianceCRUD(this);

        if (sbInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }


        /*
         *  Registration from Same UUID with a different IP will be accepted - It will be
         *  treated as a change in IP Address, Infinispan Cache will be updated in this
         *  case.
         *
         *  Registration from different UUID with an  IP that already exists in DMC Cache will
         *  treated as a conflict, Registration will be rejected in this case.
         */
        if (sbInterface.dsaIPConflict(dsaIP, dsaUUID)) {
            throw new ResourceConflictException("Another device already is registered at that IP, remove that device first");
        }

        // Set the Timestamp
        String timestamp = new SimpleDateFormat("EEE MMM dd HH:mm:ss zzz yyyy").format(Calendar.getInstance().getTime());
        appliance.setTimestamp(timestamp);
        appliance.set_canBeDCS(false);
        appliance.set_isDCS(false);

        if (sbInterface.applianceExists(dsaUUID) ) {

             sbInterface.updateDoveServiceAppliance(dsaUUID, appliance);

             /* Service Appliance Exists in Cache, Just Return HTTP_OK(200) in this Case */
             return Response.status(200).entity(sbInterface.getDoveServiceAppliance(dsaUUID)).build();
        } else {
             sbInterface.addDoveServiceAppliance(dsaUUID, appliance);
        }

        return Response.status(201).entity(appliance).build();
    }

    /**
     * oDGW Heartbeat method to oDMC
     *
     * @param input
     *            updated oDGW information in JSON format (uses patch semantics)
     * @return updated oDGW information
     *
     *         <pre>
     *
     * Example:
     *
     * Request URL:
     * http://localhost:8080/controller/sb/v2/opendove/odmc/odgw/1
     *
     * Request body in JSON:
     * {
     *   "ip_family": 0,
     *   "ip": "1.1.1.1",
     *   "dgw_config_version": 1
     *   "canBeDGW": true,
     *   "isDGW": true|false
     *
     * }
     *
     * Response body in JSON:
     * {
     *   "ip_family": 0,
     *   "ip": "1.1.1.1",
     *   "uuid": "1",
     *   "dcs_rest_service_port": 0,
     *   "dgw_rest_service_port": 0,
     *   "dcs_raw_service_port": 0,
     *   "timestamp": "Thu Oct 03 14:00:01 PDT 2013",
     *   "build_version": "",
     *   "dcs_config_version": 0,
     *   "dgw_config_version": 1,
     *   "canBeDCS": false,
     *   "canBeDGW": true,
     *   "isDCS": false,
     *   "isDGW": true
     * }
     * </pre>
     */
    @Path("/{dsaUUID}")
    @PUT
    @Consumes({ MediaType.APPLICATION_JSON })
    @Produces({ MediaType.APPLICATION_JSON })
    @TypeHint(OpenDoveServiceAppliance.class)
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 409, condition = "Service Appliance IP Address Conflict"),
            @ResponseCode(code = 500, condition = "Internal Error") })

    public Response procesDgwHeartbeat (
                                 @PathParam("dsaUUID") String dsaUUID,
                                 OpenDoveServiceAppliance appliance) {
        String dsaIP   = appliance.getIP();
        appliance.setUUID(dsaUUID);

        IfOpenDoveServiceApplianceCRUD sbInterface = OpenDoveCRUDInterfaces.getIfDoveServiceApplianceCRUD(this);

        if (sbInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }

        /*
         *  Heart-Beat from Same UUID with a different IP will be accepted - It will be
         *  treated as a change in IP Address, Infinispan Cache will be updated in this
         *  case.
         *
         *  Heart-Beat from different UUID with an  IP that already exists in DMC Cache will
         *  treated as a conflict
         */
        if (sbInterface.dsaIPConflict(dsaIP, dsaUUID)) {
            throw new ResourceConflictException("Another device already is registered at that IP, remove that device first");
        }

        // Set the Timestamp
        String timestamp = new SimpleDateFormat("EEE MMM dd HH:mm:ss zzz yyyy").format(Calendar.getInstance().getTime());
        appliance.setTimestamp(timestamp);
        appliance.set_canBeDCS(false);
        appliance.set_isDCS(false);

        if (sbInterface.applianceExists(dsaUUID) ) {

            //  Get the Service Appliance from the Infinispan Cache if the Appliance Already exists.
            OpenDoveServiceAppliance  dgwNode = sbInterface.getDoveServiceAppliance(dsaUUID);
            /*
             * if isDGW field is 'true' in Cache but the Received Heart-Beat message from oDGW has
             * isDGW field set to 'false', Preserve the State that is in local Cache.
             */
             Boolean isDGW  = dgwNode.get_isDGW();
             if (isDGW == true ) {
                appliance.set_isDGW(true);
             }
            sbInterface.updateDoveServiceAppliance(dsaUUID, appliance);
        } else {
            /*
             * Heart-Beat will be accepted only for Registered Appliances
             */
            throw new ResourceConflictException("Heartbeat only accepted from Registered Appliances");
        }

        return Response.status(200).entity(sbInterface.getDoveServiceAppliance(dsaUUID)).build();
    }
}

