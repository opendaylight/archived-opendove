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
import javax.ws.rs.QueryParam;
import javax.ws.rs.core.MediaType;
import javax.ws.rs.core.Response;

import org.codehaus.enunciate.jaxrs.ResponseCode;
import org.codehaus.enunciate.jaxrs.StatusCodes;
import org.codehaus.enunciate.jaxrs.TypeHint;
import org.opendaylight.controller.northbound.commons.RestMessages;
import org.opendaylight.controller.northbound.commons.exception.ServiceUnavailableException;
import org.opendaylight.opendove.odmc.IfOpenDoveSwitchCRUD;
import org.opendaylight.opendove.odmc.OpenDoveCRUDInterfaces;
import org.opendaylight.opendove.odmc.rest.OpenDoveSwitchStatsRequest;

/**
 * Open DOVE Northbound REST APIs for Switch Stats.<br>
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

@Path("/switch_stats")
public class OpenDoveSwitchStatsNorthbound {

   /**
    * Returns switch statistics
    *
    * @param ip
    *            switches' IPv4 address
    * @param vnid
    *            Vitrual Network Identifier to query
    * @param mac
    *            optional MAC to query
    * @return statistics
    *
    *         <pre>
    *
    * Example:
    *
    * Request URL:
    * http://localhost:8080/controller/nb/v2/opendove/odmc/switch_stats?ip=1.1.1.1&vnid=100[&mac=000000000001]
    *
    * Response body in JSON:
    * {
    *   "stats": [ { "Name": "<name>", "Value": <value>" } ]
    * }
    * </pre>
    */

    @GET
    @Produces({ MediaType.APPLICATION_JSON })
    @TypeHint(OpenDoveSwitchStatsRequest.class)
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 204, condition = "No content"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 500, condition = "Internal Error") })
    public Response showStats(
            @QueryParam("ip") String queryIPAddr,
            @QueryParam("vnid") String queryVNID,
            @QueryParam("mac") String queryMAC
            ) {
        IfOpenDoveSwitchCRUD sbInterface = OpenDoveCRUDInterfaces.getIfOpenDoveSwitchCRU(this);
        if (sbInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        OpenDoveSwitchStatsRequest answer = (OpenDoveSwitchStatsRequest)
                sbInterface.getStats(queryIPAddr, queryVNID, queryMAC);
        if (answer == null) {
        	return Response.status(204).build();
        }
        return Response.status(200).entity(answer).build();
    }

    /**
     * Returns switch statistics
     *
     * @param ip
     *            switches' IPv4 address
     * @param vnid
     *            Vitrual Network Identifier to query
     * @param mac
     *            optional MAC to query
     *
     *         <pre>
     *
     * Example:
     *
     * Request URL:
     * http://localhost:8080/controller/nb/v2/opendove/odmc/1.1.1.1/100/000000000001
     *
     * Response body in JSON:
     *          none
     * </pre>
     */    
//    @Path("/ip/{ip}/vnid/{vnid}/mac/{mac}")
    @DELETE
    @Produces({ MediaType.APPLICATION_JSON })
    @TypeHint(OpenDoveSwitchStatsRequest.class)
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 204, condition = "No content"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 500, condition = "Internal Error") })
    public Response deleteStats(
            @QueryParam("ip") String queryIPAddr,
            @QueryParam("vnid") String queryVNID,
            @QueryParam("mac") String queryMAC
            ) {
        IfOpenDoveSwitchCRUD sbInterface = OpenDoveCRUDInterfaces.getIfOpenDoveSwitchCRU(this);
        if (sbInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        return Response.status(sbInterface.deleteStats(queryIPAddr, queryVNID, queryMAC)).build();
//        return Response.status(204).build();
    }
}

