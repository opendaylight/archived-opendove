/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.controller.odmc.northbound;


import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import javax.ws.rs.Consumes;
import javax.ws.rs.DELETE;
import javax.ws.rs.GET;
import javax.ws.rs.POST;
import javax.ws.rs.PUT;
import javax.ws.rs.Path;
import javax.ws.rs.PathParam;
import javax.ws.rs.Produces;
import javax.ws.rs.QueryParam;
import javax.ws.rs.core.MediaType;
import javax.ws.rs.core.Response;

import org.codehaus.enunciate.jaxrs.ResponseCode;
import org.codehaus.enunciate.jaxrs.StatusCodes;
import org.opendaylight.controller.northbound.commons.RestMessages;
import org.opendaylight.controller.northbound.commons.exception.ServiceUnavailableException;
import org.opendaylight.controller.odmc.IfNBNetworkCRUD;
import org.opendaylight.controller.odmc.IfNBSubnetCRUD;
import org.opendaylight.controller.odmc.OpenStackSubnets;

/**
 * Open DOVE Northbound REST APIs.<br>
 * This class provides REST APIs for managing open DOVE internals related to Subnets
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

@Path("/subnets")
public class OpenDoveSubnetsNorthbound {

    private OpenStackSubnets extractFields(OpenStackSubnets o, List<String> fields) {
        return o.extractFields(fields);
    }


    /**
     * Returns a list of all Subnets */
    @GET
    @Produces({ MediaType.APPLICATION_JSON })
    //@TypeHint(OpenStackSubnets.class)
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 501, condition = "Not Implemented") })
    public Response listSubnets(
            // return fields
            @QueryParam("fields") List<String> fields,
            // note: openstack isn't clear about filtering on lists, so we aren't handling them
            @QueryParam("id") String queryID,
            @QueryParam("network_id") String queryNetworkID,
            @QueryParam("name") String queryName,
            @QueryParam("ip_version") String queryIPVersion,
            @QueryParam("cidr") String queryCIDR,
            @QueryParam("gateway_ip") String queryGatewayIP,
            @QueryParam("enable_dhcp") String queryEnableDHCP,
            @QueryParam("tenant_id") String queryTenantID,
            // pagination
            @QueryParam("limit") String limit,
            @QueryParam("marker") String marker,
            @QueryParam("page_reverse") String pageReverse
            // sorting not supported
            ) {
        IfNBSubnetCRUD subnetInterface = OpenDoveNBInterfaces.getIfNBSubnetCRUD("default", this);
        if (subnetInterface == null) {
            throw new ServiceUnavailableException("Subnet CRUD Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        List<OpenStackSubnets> allNetworks = subnetInterface.getAllSubnets();
        List<OpenStackSubnets> ans = new ArrayList<OpenStackSubnets>();
        Iterator<OpenStackSubnets> i = allNetworks.iterator();
        while (i.hasNext()) {
            OpenStackSubnets oSS = i.next();
            if ((queryID == null || queryID.equals(oSS.getID())) &&
                    (queryNetworkID == null || queryNetworkID.equals(oSS.getNetworkUUID())) &&
                    (queryName == null || queryName.equals(oSS.getName())) &&
                    (queryIPVersion == null || queryIPVersion.equals(oSS.getIpVersion())) &&
                    (queryCIDR == null || queryCIDR.equals(oSS.getCidr())) &&
                    (queryGatewayIP == null || queryGatewayIP.equals(oSS.getGatewayIP())) &&
                    (queryEnableDHCP == null || queryEnableDHCP.equals(oSS.getEnableDHCP())) &&
                    (queryTenantID == null || queryTenantID.equals(oSS.getTenantID()))) {
                if (fields.size() > 0)
                    ans.add(extractFields(oSS,fields));
                else
                    ans.add(oSS);
            }
        }
        //TODO: apply pagination to results
        //TODO: how to return proper empty set result: { "subnets": [] }
        //TODO: how to return proper set with one element result: { "subnets": [{}] }
        return Response.status(200).entity(
                new OpenStackSubnetRequest(ans)).build();
    }

    /**
     * Returns a specific Subnet */

    @Path("{subnetUUID}")
    @GET
    @Produces({ MediaType.APPLICATION_JSON })
    //@TypeHint(OpenStackSubnets.class)
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 404, condition = "Not Found"),
            @ResponseCode(code = 501, condition = "Not Implemented") })
    public Response showSubnet(
            @PathParam("subnetUUID") String subnetUUID,
            // return fields
            @QueryParam("fields") List<String> fields) {
        IfNBSubnetCRUD subnetInterface = OpenDoveNBInterfaces.getIfNBSubnetCRUD("default",this);
        if (subnetInterface == null) {
            throw new ServiceUnavailableException("Subnet CRUD Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        if (!subnetInterface.subnetExists(subnetUUID))
            return Response.status(404).build();
        if (fields.size() > 0) {
            OpenStackSubnets ans = subnetInterface.getSubnet(subnetUUID);
            return Response.status(200).entity(
                    new OpenStackSubnetRequest(extractFields(ans, fields))).build();
        } else
            return Response.status(200).entity(
                    new OpenStackSubnetRequest(subnetInterface.getSubnet(subnetUUID))).build();
    }

    /**
     * Creates new Subnets */

    @POST
    @Produces({ MediaType.APPLICATION_JSON })
    @Consumes({ MediaType.APPLICATION_JSON })
    //@TypeHint(OpenStackSubnets.class)
    @StatusCodes({
            @ResponseCode(code = 201, condition = "Created"),
            @ResponseCode(code = 400, condition = "Bad Request"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 403, condition = "Forbidden"),
            @ResponseCode(code = 404, condition = "Not Found"),
            @ResponseCode(code = 409, condition = "Conflict"),
            @ResponseCode(code = 501, condition = "Not Implemented") })
    public Response createSubnets(final OpenStackSubnetRequest input) {
        IfNBSubnetCRUD subnetInterface = OpenDoveNBInterfaces.getIfNBSubnetCRUD("default",this);
        if (subnetInterface == null) {
            throw new ServiceUnavailableException("Subnet CRUD Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        IfNBNetworkCRUD networkInterface = OpenDoveNBInterfaces.getIfNBNetworkCRUD("default", this);
        if (networkInterface == null) {
            throw new ServiceUnavailableException("Network CRUD Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        if (input.isSingleton()) {
            OpenStackSubnets singleton = input.getSingleton();

            /*
             *  Verify that the subnet doesn't already exist (Issue: is a deeper check necessary?)
             *  the specified network exists, the subnet has a valid network address,
             *  and that the gateway IP doesn't overlap with the allocation pools
             *  *then* add the subnet to the cache
             */
            if (subnetInterface.subnetExists(singleton.getID()))
                return Response.status(400).build();
            if (!networkInterface.networkExists(singleton.getNetworkUUID()))
                return Response.status(404).build();
            if (!singleton.isValidCIDR())
                return Response.status(400).build();
            singleton.initDefaults();
            if (singleton.gatewayIP_Pool_overlap())
                return Response.status(409).build();
            subnetInterface.addSubnet(singleton);
        } else {
            List<OpenStackSubnets> bulk = input.getBulk();
            Iterator<OpenStackSubnets> i = bulk.iterator();
            HashMap<String, OpenStackSubnets> testMap = new HashMap<String, OpenStackSubnets>();
            while (i.hasNext()) {
                OpenStackSubnets test = i.next();

                /*
                 *  Verify that the subnet doesn't already exist (Issue: is a deeper check necessary?)
                 *  the specified network exists, the subnet has a valid network address,
                 *  and that the gateway IP doesn't overlap with the allocation pools,
                 *  and that the bulk request doesn't already contain a subnet with this id
                 */

                test.initDefaults();
                if (subnetInterface.subnetExists(test.getID()))
                    return Response.status(400).build();
                if (testMap.containsKey(test.getID()))
                    return Response.status(400).build();
                testMap.put(test.getID(), test);
                if (!networkInterface.networkExists(test.getNetworkUUID()))
                    return Response.status(404).build();
                if (!test.isValidCIDR())
                    return Response.status(400).build();
                if (test.gatewayIP_Pool_overlap())
                    return Response.status(409).build();
            }

            /*
             * now, each element of the bulk request can be added to the cache
             */
            i = bulk.iterator();
            while (i.hasNext()) {
                OpenStackSubnets test = i.next();
                subnetInterface.addSubnet(test);
            }
        }
        return Response.status(201).entity(input).build();
    }

    /**
     * Updates a Subnet */

    @Path("{subnetUUID}")
    @PUT
    @Produces({ MediaType.APPLICATION_JSON })
    @Consumes({ MediaType.APPLICATION_JSON })
    //@TypeHint(OpenStackSubnets.class)
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 400, condition = "Bad Request"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 403, condition = "Forbidden"),
            @ResponseCode(code = 404, condition = "Not Found"),
            @ResponseCode(code = 501, condition = "Not Implemented") })
    public Response updateSubnet(
            @PathParam("subnetUUID") String subnetUUID, final OpenStackSubnetRequest input
            ) {
        IfNBSubnetCRUD subnetInterface = OpenDoveNBInterfaces.getIfNBSubnetCRUD("default", this);
        if (subnetInterface == null) {
            throw new ServiceUnavailableException("Subnet CRUD Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }

        /*
         * verify the subnet exists and there is only one delta provided
         */
        if (!subnetInterface.subnetExists(subnetUUID))
            return Response.status(404).build();
        if (!input.isSingleton())
            return Response.status(400).build();
        OpenStackSubnets singleton = input.getSingleton();

        /*
         * updates restricted by Neutron
         */
        if (singleton.getID() != null || singleton.getTenantID() != null ||
                singleton.getIpVersion() != null || singleton.getCidr() != null ||
                singleton.getAllocationPools() != null)
            return Response.status(400).build();

        /*
         * updates restricted by DOVE
         */
        if (singleton.getGatewayIP() != null)
            return Response.status(403).build();

        /*
         * update the object and return it
         */
        subnetInterface.updateSubnet(subnetUUID, singleton);
        return Response.status(200).entity(
                new OpenStackSubnetRequest(subnetInterface.getSubnet(subnetUUID))).build();
    }

    /**
     * Deletes a Subnet */

    @Path("{subnetUUID}")
    @DELETE
    @StatusCodes({
            @ResponseCode(code = 204, condition = "No Content"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 404, condition = "Not Found"),
            @ResponseCode(code = 409, condition = "Conflict"),
            @ResponseCode(code = 501, condition = "Not Implemented") })
    public Response deleteSubnet(
            @PathParam("subnetUUID") String subnetUUID) {
        IfNBSubnetCRUD subnetInterface = OpenDoveNBInterfaces.getIfNBSubnetCRUD("default", this);
        if (subnetInterface == null) {
            throw new ServiceUnavailableException("Network CRUD Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }

        /*
         * verify the subnet exists and it isn't currently in use
         */
        if (!subnetInterface.subnetExists(subnetUUID))
            return Response.status(404).build();
        if (subnetInterface.subnetInUse(subnetUUID))
            return Response.status(409).build();

        /*
         * remove it and return 204 status
         */
        subnetInterface.removeSubnet(subnetUUID);
        return Response.status(204).build();
    }
}
