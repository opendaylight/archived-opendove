/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.opendove.odmc.rest.southbound;

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
import org.opendaylight.opendove.odmc.IfSBDovePolicyCRUD;
import org.opendaylight.opendove.odmc.OpenDoveCRUDInterfaces;
import org.opendaylight.opendove.odmc.rest.OpenDovePolicyRequest;

/**
 * Open DOVE Southbound REST APIs for Subnets.<br>
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

@Path("/policies")
public class OpenDovePolicySouthbound {

    /**
     * Show policy
     *
     * @param policyUUID
     *            uuid of policy
     * @return policy Information
     *
     *         <pre>
     *
     * Example:
     *
     * Request URL:
     * http://127.0.0.1:8080/controller/sb/v2/opendove/odmc/policies/72787da8-f332-4f70-9221-e676cb0e09f6
     *
     * Response body in JSON:
     *
     * {
     *   "policy" : {
     *     "is_tombstone" : false,
     *     "change_version" : 9,
     *     "create_version" : 9,
     *     "id" : "72787da8-f332-4f70-9221-e676cb0e09f6",
     *     "type" : 1,
     *     "src_network" : 8293070,
     *     "dst_network" : 9725207,
     *     "ttl" : 1000,
     *     "action" : 1,
     *     "domain_id" : "4b5b6730-05a6-4ed8-b0d1-da3ddb9225e4",
     *     "traffic_type" : 0
     *   }
     * }
     *
     * </pre>
     */
    @Path("{policyUUID}")
    @GET
    @Produces({ MediaType.APPLICATION_JSON })
    @TypeHint(OpenDovePolicyRequest.class)
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 204, condition = "No content"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 404, condition = "Not Found"),
            @ResponseCode(code = 500, condition = "Internal Error") })
    public Response showSubnet(
            @PathParam("policyUUID") String policyUUID
            ) {
        IfSBDovePolicyCRUD sbInterface = OpenDoveCRUDInterfaces.getIfDovePolicyCRUD(this);
        if (sbInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        if (!sbInterface.policyExists(policyUUID))
            throw new ResourceNotFoundException("Network not found");
        return Response.status(200).entity(new OpenDovePolicyRequest(sbInterface.getPolicy(policyUUID))).build();
    }

    /**
     * List all policies
     *
     * @param input
     *            none
     * @return list of all policies
     *
     *         <pre>
     *
     * Example:
     *
     * Request URL:
     * http://localhost:8080/controller/sb/v2/opendove/odmc/policies
     *
     * Response body in JSON:
     * {
     *    "policies" : [ {
     *      "is_tombstone" : false,
     *      "change_version" : 10,
     *      "create_version" : 10,
     *      "id" : "496c2591-3b57-4a2f-9ea6-c902768c8601",
     *      "type" : 1,
     *      "src_network" : 8293070,
     *      "dst_network" : 9725207,
     *      "ttl" : 1000,
     *      "action" : 1,
     *      "domain_id" : "4b5b6730-05a6-4ed8-b0d1-da3ddb9225e4",
     *      "traffic_type" : 1
     *    }, {
     *      "is_tombstone" : false,
     *      "change_version" : 12,
     *      "create_version" : 12,
     *      "id" : "f5e8dc57-e1cd-42b7-a796-5ccd82f4fe1b",
     *      "type" : 1,
     *      "src_network" : 9725207,
     *      "dst_network" : 8293070,
     *      "ttl" : 1000,
     *      "action" : 1,
     *      "domain_id" : "4b5b6730-05a6-4ed8-b0d1-da3ddb9225e4",
     *      "traffic_type" : 1
     *    }, {
     *      "is_tombstone" : false,
     *      "change_version" : 11,
     *      "create_version" : 11,
     *      "id" : "e9aa8058-62cb-4fff-87a5-e0372711c61a",
     *      "type" : 1,
     *      "src_network" : 9725207,
     *      "dst_network" : 8293070,
     *      "ttl" : 1000,
     *      "action" : 1,
     *      "domain_id" : "4b5b6730-05a6-4ed8-b0d1-da3ddb9225e4",
     *      "traffic_type" : 0
     *    }, {
     *      "is_tombstone" : false,
     *      "change_version" : 9,
     *      "create_version" : 9,
     *      "id" : "72787da8-f332-4f70-9221-e676cb0e09f6",
     *      "type" : 1,
     *      "src_network" : 8293070,
     *      "dst_network" : 9725207,
     *      "ttl" : 1000,
     *      "action" : 1,
     *      "domain_id" : "4b5b6730-05a6-4ed8-b0d1-da3ddb9225e4",
     *      "traffic_type" : 0
     *    } ]
     * }
     * </pre>
     */
    @GET
    @Produces({ MediaType.APPLICATION_JSON })
    @TypeHint(OpenDovePolicyRequest.class)
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 204, condition = "No content"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 500, condition = "Internal Error") })
    public Response listPolicies() {
        IfSBDovePolicyCRUD sbInterface = OpenDoveCRUDInterfaces.getIfDovePolicyCRUD(this);
        if (sbInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        return Response.status(200).entity(new OpenDovePolicyRequest(sbInterface.getPolicies())).build();
    }
}

