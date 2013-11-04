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
import javax.ws.rs.Produces;
import javax.ws.rs.core.MediaType;
import javax.ws.rs.core.Response;

import org.codehaus.enunciate.jaxrs.ResponseCode;
import org.codehaus.enunciate.jaxrs.StatusCodes;
import org.opendaylight.controller.northbound.commons.RestMessages;
import org.opendaylight.controller.northbound.commons.exception.ServiceUnavailableException;
import org.opendaylight.opendove.odmc.IfNBSystemRU;
import org.opendaylight.opendove.odmc.OpenDoveNeutronControlBlock;

/**
 * Open DOVE Northbound REST APIs.<br>
 * This class provides REST APIs for managing the open DOVE
 *
 * <br>
 * <br>
 * Authentication scheme : <b>HTTP Basic</b><br>
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


@Path("/system")
public class OpenDoveSystemNorthbound {

    /**
     * Returns the system control block
     *
     * @param none
     *            Identifier of the mapping
     * @return System Control Block
     *
     *         <pre>
     *
     * Example:
     *
     * Request URL:
     * http://localhost:8080/controller/nb/v2/opendove/odmc/system
     *
     * Response body in JSON:
     * {
     *    "domain_separation": false,
     *    "snat_pool_size": 1,
     *    "egw_replication_factor": 1
     * }
     * </pre>
     */
    @GET
    @Produces({ MediaType.APPLICATION_JSON })
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 501, condition = "Not Implemented") })
    public Response showSubnet() {
        IfNBSystemRU systemInterface = OpenDoveNBInterfaces.getIfNBSystemRU("default", this);
        if (systemInterface == null) {
            throw new ServiceUnavailableException("System RU Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        return Response.status(200).entity(
                systemInterface.getSystemBlock()).build();
    }

    /**
     * Updates the control block
     *
     * @param input
     *            system control block in patch format
     * @return Updated System Control Block
     *
     *         <pre>
     *
     * Example:
     *
     * Request URL:
     * http://localhost:8080/controller/nb/v2/opendove/odmc/system
     *
     * Request body in JSON:
     * {
     *    "domain_separation": true
     * }
     *
     * Response body in JSON:
     * {
     *    "domain_separation": true,
     *    "snat_pool_size": 1,
     *    "egw_replication_factor": 1
     * }
     * </pre>
     */
    @PUT
    @Produces({ MediaType.APPLICATION_JSON })
    @Consumes({ MediaType.APPLICATION_JSON })
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 400, condition = "Bad Request"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 501, condition = "Not Implemented") })
    public Response updateSubnet(final OpenDoveNeutronControlBlock input) {
        IfNBSystemRU systemInterface = OpenDoveNBInterfaces.getIfNBSystemRU("default", this);
        if (systemInterface == null) {
            throw new ServiceUnavailableException("System RU Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }

        // update network object and return the modified object
        systemInterface.updateControlBlock(input);
        return Response.status(200).entity(
                systemInterface.getSystemBlock()).build();
    }
}
