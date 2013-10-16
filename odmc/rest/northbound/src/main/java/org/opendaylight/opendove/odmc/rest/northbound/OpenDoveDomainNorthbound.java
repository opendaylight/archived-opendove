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
import org.opendaylight.opendove.odmc.IfOpenDoveDomainCRU;
import org.opendaylight.opendove.odmc.OpenDoveCRUDInterfaces;
import org.opendaylight.opendove.odmc.rest.OpenDoveDCSList;
import org.opendaylight.opendove.odmc.rest.OpenDoveDomainRequest;

/**
 * Open DOVE Northbound REST APIs for Domains.<br>
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

@Path("/domains")
public class OpenDoveDomainNorthbound {

    /**
     * Returns a particular domain
     *
     * @param domainUUID
     *            Identifier of the domain
     * @return Data on that domain
     *
     *         <pre>
     *
     * Example:
     *
     * Request URL:
     * http://localhost:8080/controller/nb/v2/opendove/odmc/domains/uuid
     *
     * Response body in JSON:
     * {
     *   "domain": {
     *     "id": "uuid",
     *     "name": "domain_name",
     *     "replication_factor": 2
     *   }
     * }
     * </pre>
     */
	@Path("{domainUUID}")
    @GET
    @Produces({ MediaType.APPLICATION_JSON })
    @TypeHint(OpenDoveDomainRequest.class)
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 204, condition = "No content"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 404, condition = "Not Found"),
            @ResponseCode(code = 500, condition = "Internal Error") })
    public Response showServiceAppliance(
            @PathParam("domainUUID") String domainUUID
            ) {
        IfOpenDoveDomainCRU sbInterface = OpenDoveCRUDInterfaces.getIfDoveDomainCRU(this);
        if (sbInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        if (!sbInterface.domainExists(domainUUID))
            return Response.status(404).build();
        return Response.status(200).entity(new OpenDoveDomainRequest(sbInterface.getDomain(domainUUID))).build();
    }

    /**
     * List service appliances hosting a particular domain
     *
     * @param domainUUID
     *            Identifier of the network
     * @return List of service appliances hosting that domain
     *
     *         <pre>
     *
     * Example:
     *
     * Request URL:
     * http://localhost:8080/controller/nb/v2/opendove/odmc/networks/uuid/domains/uuid/odcs-list
     *
     * Response body in JSON:
     * {
     *   "odcs-list":  [ {
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
     *     "isDCS": true,
     *     "isDGW": false
     *   } ]
     * }
     * </pre>
     */
	@Path("{domainUUID}/odcs-list")
    @GET
    @Produces({ MediaType.APPLICATION_JSON })
    @TypeHint(OpenDoveDCSList.class)
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 204, condition = "No content"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 404, condition = "Not Found"),
            @ResponseCode(code = 500, condition = "Internal Error") })
    public Response showODCSList(
            @PathParam("domainUUID") String domainUUID
            ) {
        IfOpenDoveDomainCRU sbInterface = OpenDoveCRUDInterfaces.getIfDoveDomainCRU(this);
        if (sbInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        if (!sbInterface.domainExists(domainUUID))
            return Response.status(404).build();
        return Response.status(200).entity(new OpenDoveDCSList(sbInterface.getDCSList(domainUUID))).build();
    }

    /**
     * Lists domains
     *
     * @param none
     * 
     * @return List of domains
     *
     *         <pre>
     *
     * Example:
     *
     * Request URL:
     * http://localhost:8080/controller/nb/v2/opendove/odmc/domains
     *
     * Response body in JSON:
     * {
     *   "domains": [ {
     *     "id": "uuid",
     *     "name": "domain_name",
     *     "replication_factor": 2
     *   } ]
     * }
     * </pre>
     */@GET
    @Produces({ MediaType.APPLICATION_JSON })
    @TypeHint(OpenDoveDomainRequest.class)
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 204, condition = "No content"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 500, condition = "Internal Error") })
    public Response showServiceAppliances() {
        IfOpenDoveDomainCRU sbInterface = OpenDoveCRUDInterfaces.getIfDoveDomainCRU(this);
        if (sbInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        return Response.status(200).entity(new OpenDoveDomainRequest(sbInterface.getDomains())).build();
    }
}

