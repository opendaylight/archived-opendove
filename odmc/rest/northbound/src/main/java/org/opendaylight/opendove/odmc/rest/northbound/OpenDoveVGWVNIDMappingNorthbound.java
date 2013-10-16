/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.opendove.odmc.rest.northbound;

import javax.ws.rs.Consumes;
import javax.ws.rs.DELETE;
import javax.ws.rs.GET;
import javax.ws.rs.POST;
import javax.ws.rs.PUT;
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
import org.opendaylight.opendove.odmc.IfSBDoveVGWVNIDMappingCRUD;
import org.opendaylight.opendove.odmc.OpenDoveCRUDInterfaces;
import org.opendaylight.opendove.odmc.OpenDoveVGWVNIDMapping;
import org.opendaylight.opendove.odmc.rest.OpenDoveVGWVNIDMappingRequest;

/**
 * Open DOVE Southbound REST APIs for VGW VNID Mappings.<br>
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

@Path("/vlan-gws")
public class OpenDoveVGWVNIDMappingNorthbound {

    /**
     * Returns a particular VNID to VLAN mapping
     *
     * @param mappingUUID
     *            Identifier of the mapping
     * @return Data on that mapping
     *
     *         <pre>
     *
     * Example:
     *
     * Request URL:
     * http://localhost:8080/controller/nb/v2/opendove/odmc/vlan-gws/uuid
     *
     * Response body in JSON:
     * {
     *   "vnid_mapping_rule": {
     *     "id": "uuid",
     *     "net_id": 100,
     *     "gatewayUUID": "uuid2"
     *   }
     * }
     * </pre>
     */
    @Path("{mappingUUID}")
    @GET
    @Produces({ MediaType.APPLICATION_JSON })
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 204, condition = "No content"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 404, condition = "Not Found"),
            @ResponseCode(code = 500, condition = "Internal Error") })
    public Response showRule(
            @PathParam("mappingUUID") String mappingUUID
            ) {
        IfSBDoveVGWVNIDMappingCRUD sbInterface = OpenDoveCRUDInterfaces.getIfSBDoveVGWVNIDMappingCRUD(this);
        if (sbInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        if (!sbInterface.vgwVNIDMappingExists(mappingUUID))
            return Response.status(404).build();
        return Response.status(200).entity(sbInterface.getVgwVNIDMapping(mappingUUID)).build();
    }
    
    /**
     * Returns the list of VNID to VLAN mappings
     *
     * @param none
     * @return Mapping data
     *
     *         <pre>
     *
     * Example:
     *
     * Request URL:
     * http://localhost:8080/controller/nb/v2/opendove/odmc/vlan-gws/
     *
     * Response body in JSON:
     * {
     *   "vnid_mapping_rules": [ {
     *     "id": "uuid",
     *     "net_id": 100,
     *     "gatewayUUID": "uuid2"
     *   } ... ]
     * }
     * </pre>
     */
    @GET
    @Produces({ MediaType.APPLICATION_JSON })
    @TypeHint(OpenDoveVGWVNIDMappingRequest.class)
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 204, condition = "No content"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 500, condition = "Internal Error") })
    public Response listRules() {
        IfSBDoveVGWVNIDMappingCRUD sbInterface = OpenDoveCRUDInterfaces.getIfSBDoveVGWVNIDMappingCRUD(this);
        if (sbInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        return Response.status(200).entity(new OpenDoveVGWVNIDMappingRequest(sbInterface.getVgwVNIDMappings())).build();
    }
    
    /**
     * Creates a new VNID to VLAN mappings
     *
     * @param input
     *            Mapping request data
     * @return Created mapping
     *
     *         <pre>
     *
     * Example:
     *
     * Request URL:
     * http://localhost:8080/controller/nb/v2/opendove/odmc/vlan-gws/
     *
     * Request body in JSON:
     * {
     *   "vnid_mapping_rule": {
     *     "id": "uuid",
     *     "net_id": 100,
     *     "gatewayUUID": "uuid2"
     *   }
     * }
     * 
     * Response body in JSON:
     * {
     *   "vnid_mapping_rule": {
     *     "id": "uuid",
     *     "net_id": 100,
     *     "gatewayUUID": "uuid2"
     *   }
     * }
     * </pre>
     */
    @POST
    @Produces({ MediaType.APPLICATION_JSON })
    @Consumes({ MediaType.APPLICATION_JSON })
    //@TypeHint(OpenStackRouters.class)
    @StatusCodes({
            @ResponseCode(code = 201, condition = "Created"),
            @ResponseCode(code = 400, condition = "Bad Request"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 501, condition = "Not Implemented") })
    public Response createRule(final OpenDoveVGWVNIDMappingRequest input) {
        IfSBDoveVGWVNIDMappingCRUD sbInterface = OpenDoveCRUDInterfaces.getIfSBDoveVGWVNIDMappingCRUD(this);
        if (sbInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        if (input.isSingleton()) {
        	OpenDoveVGWVNIDMapping singleton = input.getSingleton();
        	singleton.setUUID(java.util.UUID.randomUUID().toString());
            sbInterface.addVgwVNIDMapping(singleton.getUUID(), singleton);
        } else {
            /*
             * only singleton rule creates supported
             */
            return Response.status(400).build();
        }
        return Response.status(201).entity(input).build();
    }

    /**
     * Updates a rule
     *
     * @param mappingUUID
     *            Identifier of the mapping
     * @param input
     *            Mapping request delta
     * @return Modified mapping
     *
     *         <pre>
     *
     * Example:
     *
     * Request URL:
     * http://localhost:8080/controller/nb/v2/opendove/odmc/vlan-gws/uuid
     *
     * Request body in JSON:
     * {
     *   "vnid_mapping_rule": {
     *     "net_id": 110200,
     *   }
     * }
     * 
     * Response body in JSON:
     * {
     *   "vnid_mapping_rule": {
     *     "id": "uuid",
     *     "net_id": 110200,
     *     "gatewayUUID": "uuid2"
     *   }
     * }
     * </pre>
     */
    @Path("{mappingUUID}")
    @PUT
    @Produces({ MediaType.APPLICATION_JSON })
    @Consumes({ MediaType.APPLICATION_JSON })
    //@TypeHint(OpenStackRouters.class)
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 400, condition = "Bad Request"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 404, condition = "Not Found"),
            @ResponseCode(code = 501, condition = "Not Implemented") })
    public Response updateRouter(
            @PathParam("mappingUUID") String mappingUUID,
            OpenDoveVGWVNIDMappingRequest input
            ) {
        IfSBDoveVGWVNIDMappingCRUD sbInterface = OpenDoveCRUDInterfaces.getIfSBDoveVGWVNIDMappingCRUD(this);
        if (sbInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }

        /*
         * rule has to exist and only a single delta can be supplied
         */
        if (!sbInterface.vgwVNIDMappingExists(mappingUUID))
            return Response.status(404).build();
        if (!input.isSingleton())
            return Response.status(400).build();
        OpenDoveVGWVNIDMapping delta = input.getSingleton();

        /*
         * update the rule and return the modified object
         */
        sbInterface.updateRule(mappingUUID, delta);
        return Response.status(200).entity(
                new OpenDoveVGWVNIDMappingRequest(sbInterface.getVgwVNIDMapping(mappingUUID))).build();

    }

    /**
     * Deletes a mapping
     *
     * @param mappingUUID
     *            Identifier of the mapping
     * @return nothing
     *
     *         <pre>
     *
     * Example:
     *
     * Request URL:
     * http://localhost:8080/controller/nb/v2/opendove/odmc/vlan-gws/uuid
     *
     * Request body in JSON: none
     * 
     * Response body in JSON: none
     * </pre>
     */
    @Path("{mappingUUID}")
    @DELETE
    @Produces({})
    @Consumes({})
    @StatusCodes({
            @ResponseCode(code = 204, condition = "No Content"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 404, condition = "Not Found"),
            @ResponseCode(code = 409, condition = "Conflict"),
            @ResponseCode(code = 501, condition = "Not Implemented") })
    public Response deleteRouter(
            @PathParam("mappingUUID") String mappingUUID) {
        IfSBDoveVGWVNIDMappingCRUD sbInterface = OpenDoveCRUDInterfaces.getIfSBDoveVGWVNIDMappingCRUD(this);
        if (sbInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }

        /*
         * verify that the rule exists and is not in use before removing it
         */
        if (!sbInterface.vgwVNIDMappingExists(mappingUUID))
            return Response.status(404).build();
        OpenDoveVGWVNIDMapping mapping = sbInterface.getVgwVNIDMapping(mappingUUID);
        mapping.setTombstoneFlag(true);
        sbInterface.updateRule(mappingUUID, mapping);
        return Response.status(204).build();
    }

}

