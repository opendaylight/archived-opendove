/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.opendove.odmc.rest.northbound;

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
import org.opendaylight.controller.northbound.commons.exception.ServiceUnavailableException;
import org.opendaylight.opendove.odmc.IfOpenDoveNetworkCRUD;
import org.opendaylight.opendove.odmc.OpenDoveCRUDInterfaces;
import org.opendaylight.opendove.odmc.rest.OpenDoveEndpointRequest;
import org.opendaylight.opendove.odmc.rest.OpenDoveNetworkRequest;

/**
 * Open DOVE Northbound REST APIs for Networks.<br>
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

@Path("/networks")
public class OpenDoveNetworksNorthbound {

    /**
     * Returns a particular network
     *
     * @param networkUUID
     *            Identifier of the network
     * @return Data on that network
     *
     *         <pre>
     *
     * Example:
     *
     * Request URL:
     * http://localhost:8080/controller/nb/v2/opendove/odmc/networks/uuid
     *
     * Response body in JSON:
     * {
     *   "network": {
     *     "id": "uuid",
     *     "network_id": 102,
     *     "name": "Test Network",
     *     "domain_id": "domain_uuid",
     *     "type": 1,
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
    public Response showServiceAppliance(
            @PathParam("networkUUID") String networkUUID
            ) {
        IfOpenDoveNetworkCRUD sbInterface = OpenDoveCRUDInterfaces.getIfDoveNetworkCRU(this);
        if (sbInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        if (!sbInterface.networkExists(networkUUID))
            return Response.status(404).build();
        return Response.status(200).entity(new OpenDoveNetworkRequest(sbInterface.getNetwork(networkUUID))).build();
    }

    /**
     * List endpoints on a particular network
     *
     * @param networkUUID
     *            Identifier of the network
     * @return List of endpoints on that network
     *
     *         <pre>
     *
     * Example:
     *
     * Request URL:
     * http://localhost:8080/controller/nb/v2/opendove/odmc/networks/uuid/endpoints
     *
     * Response body in JSON:
     * {
     *   "endpoints":  [ {
     *   "Host IP": "100.100.100.100",
     *   "mac": "00:00:00:00:00:00",
     *   "virtual IPs": "10.10.10.1",
     *   "physical IPs": "1.2.3.4"
     *   } ]
     * }
     * </pre>
     */
    @Path("{networkUUID}/endpoints")
    @GET
    @Produces({ MediaType.APPLICATION_JSON })
    @TypeHint(OpenDoveEndpointRequest.class)
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 204, condition = "No content"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 404, condition = "Not Found"),
            @ResponseCode(code = 500, condition = "Internal Error") })
    public Response showOVSEndpoints(
            @PathParam("networkUUID") String networkUUID
            ) {
        IfOpenDoveNetworkCRUD sbInterface = OpenDoveCRUDInterfaces.getIfDoveNetworkCRU(this);
        if (sbInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        if (!sbInterface.networkExists(networkUUID))
            return Response.status(404).build();
        return Response.status(200).entity(new OpenDoveEndpointRequest(sbInterface.getEndpoints(networkUUID))).build();
    }

    /**
     * Lists networks
     *
     * @param none
     *
     * @return List of networks
     *
     *         <pre>
     *
     * Example:
     *
     * Request URL:
     * http://localhost:8080/controller/nb/v2/opendove/odmc/networks
     *
     * Response body in JSON:
     * {
     *   "networks": [ {
     *     "id": "uuid",
     *     "network_id": 102,
     *     "name": "Test Network",
     *     "domain_id": "domain_uuid",
     *     "type": 1,
     *   } ]
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
    public Response showServiceAppliances() {
        IfOpenDoveNetworkCRUD sbInterface = OpenDoveCRUDInterfaces.getIfDoveNetworkCRU(this);
        if (sbInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        return Response.status(200).entity(new OpenDoveNetworkRequest(sbInterface.getNetworks())).build();
    }
}

