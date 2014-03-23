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
import org.opendaylight.controller.northbound.commons.exception.ResourceNotFoundException;
import org.opendaylight.controller.northbound.commons.exception.ServiceUnavailableException;
import org.opendaylight.opendove.odmc.IfOpenDoveSwitchCRUD;
import org.opendaylight.opendove.odmc.OpenDoveCRUDInterfaces;
import org.opendaylight.opendove.odmc.OpenDoveSwitch;
import org.opendaylight.opendove.odmc.rest.OpenDoveSwitchRequest;

/**
 * Open DOVE Northbound REST APIs for Switch.<br>
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

@Path("/switches")
public class OpenDoveSwitchNorthbound {

    /**
     * Returns a particular switch
     *
     * @param switchUUID
     *            Identifier of the switch
     * @return Data on that switch
     *
     *         <pre>
     *
     * Example:
     *
     * Request URL:
     * http://localhost:8080/controller/nb/v2/opendove/odmc/switches/uuid
     *
     * Response body in JSON:
     * {
     *   "switch": {
     *     "id": "uuid",
     *     "name": "switch name",
     *     "tunnelip": "10.10.10.1",
     *     "mgmtip": "20.20.20.2",
     *     "timestamp": "now"
     *   }
     * }
     * </pre>
     */
    @Path("{switchUUID}")
    @GET
    @Produces({ MediaType.APPLICATION_JSON })
    @TypeHint(OpenDoveSwitchRequest.class)
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 204, condition = "No content"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 404, condition = "Not Found"),
            @ResponseCode(code = 500, condition = "Internal Error") })
    public Response showSwtich(
            @PathParam("switchUUID") String switchUUID
            ) {
        IfOpenDoveSwitchCRUD sbInterface = OpenDoveCRUDInterfaces.getIfOpenDoveSwitchCRU(this);
        if (sbInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        if (!sbInterface.switchExists(switchUUID)) {
            throw new ResourceNotFoundException("Switch doesn't exist");
        }
        return Response.status(200).entity(new OpenDoveSwitchRequest(sbInterface.getSwitch(switchUUID))).build();
    }

    /**
     * Removes a particular switch
     *
     * @param switchUUID
     *            Identifier of the switch
     * @return none
     *
     *         <pre>
     *
     * Example:
     *
     * Request URL:
     * http://localhost:8080/controller/nb/v2/opendove/odmc/switches/uuid
     *
     * Response body in JSON:
     * none
     * </pre>
     */
    @Path("{switchUUID}")
    @DELETE
    @Produces({ MediaType.APPLICATION_JSON })
    @TypeHint(OpenDoveSwitchRequest.class)
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 204, condition = "No content"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 404, condition = "Not Found"),
            @ResponseCode(code = 500, condition = "Internal Error") })
    public Response deleteSwtich(
            @PathParam("switchUUID") String switchUUID
            ) {
        IfOpenDoveSwitchCRUD sbInterface = OpenDoveCRUDInterfaces.getIfOpenDoveSwitchCRU(this);
        if (sbInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        if (!sbInterface.switchExists(switchUUID)) {
            throw new ResourceNotFoundException("Switch doesn't exist");
        }
        OpenDoveSwitch target = sbInterface.getSwitch(switchUUID);
        target.setTombstoneFlag(true);
        return Response.status(202).build();
    }

    /**
     * Returns all switches
     *
     * @param none
     *
     * @return List of all switches
     *
     *         <pre>
     *
     * Example:
     *
     * Request URL:
     * http://localhost:8080/controller/nb/v2/opendove/odmc/switches
     *
     * Response body in JSON:
     * {
     *   "switches":  [ {
     *     "id": "uuid",
     *     "name": "switch name",
     *     "tunnelip": "10.10.10.1",
     *     "mgmtip": "20.20.20.2",
     *     "timestamp": "now"
     *   } ]
     * }
     * </pre>
     */
    @GET
    @Produces({ MediaType.APPLICATION_JSON })
    @TypeHint(OpenDoveSwitchRequest.class)
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 204, condition = "No content"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 500, condition = "Internal Error") })
    public Response showServiceAppliances() {
        IfOpenDoveSwitchCRUD sbInterface = OpenDoveCRUDInterfaces.getIfOpenDoveSwitchCRU(this);
        if (sbInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        return Response.status(200).entity(new OpenDoveSwitchRequest(sbInterface.getSwitches())).build();
    }
}

