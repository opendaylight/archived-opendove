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
import org.opendaylight.opendove.odmc.IfSBDoveSubnetCRUD;
import org.opendaylight.opendove.odmc.OpenDoveCRUDInterfaces;
import org.opendaylight.opendove.odmc.rest.OpenDoveSubnetRequest;

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

@Path("/subnets")
public class OpenDoveSubnetSouthbound {

    /**
     * Show subnet
     *
     * @param subnetUUID
     *            uuid of subnet
     * @return subnet Information
     *
     *         <pre>
     *
     * Example:
     *
     * Request URL:
     * http://127.0.0.1:8080/controller/sb/v2/opendove/odmc/subnets/8f8226a9-1443-480f-abbc-43ba9f37d223
     *
     * Response body in JSON:
     *
     * {
     *   "subnet" : {
     *     "is_tombstone" : false,
     *     "change_version" : 8,
     *     "create_version" : 8,
     *     "id" : "8f8226a9-1443-480f-abbc-43ba9f37d223",
     *     "domain_id" : "4b5b6730-05a6-4ed8-b0d1-da3ddb9225e4",
     *     "subnet" : "10.1.2.0",
     *     "mask" : "255.255.255.0",
     *     "nexthop" : "10.1.2.1",
     *     "type" : "Dedicated",
     *     "network_ids" : [ "8f53683e-da75-486a-a53a-32c3804a282f" ]
     *   }
     * }
     *
     * </pre>
     */
    @Path("{subnetUUID}")
    @GET
    @Produces({ MediaType.APPLICATION_JSON })
    @TypeHint(OpenDoveSubnetRequest.class)
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 204, condition = "No content"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 404, condition = "Not Found"),
            @ResponseCode(code = 500, condition = "Internal Error") })
    public Response showSubnet(
            @PathParam("subnetUUID") String subnetUUID
            ) {
        IfSBDoveSubnetCRUD sbInterface = OpenDoveCRUDInterfaces.getIfDoveSubnetCRUD(this);
        if (sbInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        if (!sbInterface.subnetExists(subnetUUID))
            throw new ResourceNotFoundException("Subnet not found");
        return Response.status(200).entity(new OpenDoveSubnetRequest(sbInterface.getSubnet(subnetUUID))).build();
    }

    /**
     * List all subnets
     *
     * @param input
     *            none
     * @return list of all opendove sunets
     *
     *         <pre>
     *
     * Example:
     *
     * Request URL:
     * http://localhost:8080/controller/sb/v2/opendove/odmc/subnets
     *
     * Response body in JSON:
     *
     * {
     *   "subnets" : [ {
     *     "is_tombstone" : false,
     *     "change_version" : 6,
     *     "create_version" : 6,
     *     "id" : "3dc34bec-cc59-44ca-a946-721a2385b345",
     *     "domain_id" : "4b5b6730-05a6-4ed8-b0d1-da3ddb9225e4",
     *     "subnet" : "10.1.1.0",
     *     "mask" : "255.255.255.0",
     *     "nexthop" : "10.1.1.1",
     *     "type" : "Dedicated",
     *     "network_ids" : [ "87a95eaa-bd99-4c16-bdb2-536623fc91bc" ],
     *     "associatedOSSubnetUUID" : "13150214-0513-1400-0000-000000000001"
     *   }, {
     *     "is_tombstone" : false,
     *     "change_version" : 8,
     *     "create_version" : 8,
     *     "id" : "8f8226a9-1443-480f-abbc-43ba9f37d223",
     *     "domain_id" : "4b5b6730-05a6-4ed8-b0d1-da3ddb9225e4",
     *     "subnet" : "10.1.2.0",
     *     "mask" : "255.255.255.0",
     *     "nexthop" : "10.1.2.1",
     *     "type" : "Dedicated",
     *     "network_ids" : [ "8f53683e-da75-486a-a53a-32c3804a282f" ],
     *     "associatedOSSubnetUUID" : "13150214-0513-1400-0000-000000000002"
   } ]
}
     *
     * </pre>
     */
    @GET
    @Produces({ MediaType.APPLICATION_JSON })
    @TypeHint(OpenDoveSubnetRequest.class)
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 204, condition = "No content"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 500, condition = "Internal Error") })
    public Response listSubnets() {
        IfSBDoveSubnetCRUD sbInterface = OpenDoveCRUDInterfaces.getIfDoveSubnetCRUD(this);
        if (sbInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        return Response.status(200).entity(new OpenDoveSubnetRequest(sbInterface.getSubnets())).build();
    }
}

