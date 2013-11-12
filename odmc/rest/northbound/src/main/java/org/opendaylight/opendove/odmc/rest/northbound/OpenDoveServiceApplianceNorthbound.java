/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.opendove.odmc.rest.northbound;

import javax.ws.rs.DELETE;
import javax.ws.rs.GET;
import javax.ws.rs.Path;
import javax.ws.rs.PathParam;
import javax.ws.rs.Produces;
import javax.ws.rs.core.MediaType;
import javax.ws.rs.core.Response;

import org.codehaus.enunciate.jaxrs.ResponseCode;
import org.codehaus.enunciate.jaxrs.StatusCodes;
import org.codehaus.enunciate.jaxrs.TypeHint;
import org.opendaylight.controller.northbound.commons.RestMessages;
import org.opendaylight.controller.northbound.commons.exception.ResourceConflictException;
import org.opendaylight.controller.northbound.commons.exception.ServiceUnavailableException;
import org.opendaylight.opendove.odmc.IfOpenDoveServiceApplianceCRUD;
import org.opendaylight.opendove.odmc.OpenDoveCRUDInterfaces;
import org.opendaylight.opendove.odmc.OpenDoveServiceAppliance;
import org.opendaylight.opendove.odmc.rest.OpenDoveServiceApplianceRequest;

/**
 * Open DOVE Northbound REST APIs for Service Appliance.<br>
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

@Path("/serviceAppliances")
public class OpenDoveServiceApplianceNorthbound {

    /**
     * Returns all service appliances
     *
     * @param none
     *
     * @return List of all service appliances
     *
     *         <pre>
     *
     * Example:
     *
     * Request URL:
     * http://localhost:8080/controller/nb/v2/opendove/odmc/serviceAppliances
     *
     * Response body in JSON:
     * {
     *   "service_appliances":  [ {
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
     *     "isDGW": false
     *   } ]
     * }
     * </pre>
     */
    @GET
    @Produces({ MediaType.APPLICATION_JSON })
    @TypeHint(OpenDoveServiceApplianceRequest.class)
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 204, condition = "No content"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 500, condition = "Internal Error") })
    public Response showServiceAppliances() {
        IfOpenDoveServiceApplianceCRUD sbInterface = OpenDoveCRUDInterfaces.getIfDoveServiceApplianceCRUD(this);
        if (sbInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        return Response.status(200).entity(new OpenDoveServiceApplianceRequest(sbInterface.getAppliances())).build();
    }

    /**
     * Deletes a particular service appliance
     *
     * @param saUUID
     *            Identifier of the service appliance
     *
     * @return none
     *         </pre>
     * Example:
     *
     * Request URL:
     * http://localhost:8080/controller/nb/v2/opendove/odmc/serviceAppliances/uuid
     *
     * </pre>
     */
    @Path("{saUUID}")
    @DELETE
    @StatusCodes({
            @ResponseCode(code = 204, condition = "No content"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 404, condition = "Not Found"),
            @ResponseCode(code = 500, condition = "Internal Error") })
    public Response removeServiceAppliance(
            @PathParam("saUUID") String saUUID
            ) {
        IfOpenDoveServiceApplianceCRUD sbInterface = OpenDoveCRUDInterfaces.getIfDoveServiceApplianceCRUD(this);
        if (sbInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        if (!sbInterface.applianceExists(saUUID))
            return Response.status(404).build();
        OpenDoveServiceAppliance appliance = sbInterface.getDoveServiceAppliance(saUUID);
        if (appliance.get_isDCS() || appliance.get_isDGW())
            throw new ResourceConflictException("cannot delete role assigned service appliance");
        sbInterface.deleteServiceAppliance(saUUID);
        return Response.status(204).build();
    }

    /**
     * Returns a particular service appliance
     *
     * @param saUUID
     *            Identifier of the service appliance
     * @return Data on that service appliance
     *
     *         <pre>
     *
     * Example:
     *
     * Request URL:
     * http://localhost:8080/controller/nb/v2/opendove/odmc/serviceAppliances/uuid
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
     *     "isDGW": false
     *   }
     * }
     * </pre>
     */
    @Path("{saUUID}")
    @GET
    @Produces({ MediaType.APPLICATION_JSON })
    @TypeHint(OpenDoveServiceApplianceRequest.class)
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 204, condition = "No content"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 404, condition = "Not Found"),
            @ResponseCode(code = 500, condition = "Internal Error") })
    public Response showServiceAppliance(
            @PathParam("saUUID") String saUUID
            ) {
        IfOpenDoveServiceApplianceCRUD sbInterface = OpenDoveCRUDInterfaces.getIfDoveServiceApplianceCRUD(this);
        if (sbInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        if (!sbInterface.applianceExists(saUUID))
            return Response.status(404).build();
        return Response.status(200).entity(new OpenDoveServiceApplianceRequest(sbInterface.getDoveServiceAppliance(saUUID))).build();
    }

}

