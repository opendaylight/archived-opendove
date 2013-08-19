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
import org.codehaus.enunciate.jaxrs.TypeHint;
import org.opendaylight.controller.northbound.commons.RestMessages;
import org.opendaylight.controller.northbound.commons.exception.ServiceUnavailableException;
import org.opendaylight.controller.odmc.IfNBNetworkCRUD;
import org.opendaylight.controller.odmc.OpenStackNetworks;

/**
 * Open DOVE Northbound REST APIs for Network.<br>
 * This class provides REST APIs for managing open DOVE internals related to Networks
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

@Path("/networks")
public class OpenDoveNetworksNorthbound {

    private OpenStackNetworks extractFields(OpenStackNetworks o, List<String> fields) {
        return o.extractFields(fields);
    }

    /**
     * Returns a list of all Networks */

    @GET
    @Produces({ MediaType.APPLICATION_JSON })
    //@TypeHint(OpenStackNetworks.class)
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 401, condition = "Unauthorized") })
    public Response listNetworks(
            // return fields
            @QueryParam("fields") List<String> fields,
            // note: openstack isn't clear about filtering on lists, so we aren't handling them
            @QueryParam("id") String queryID,
            @QueryParam("name") String queryName,
            @QueryParam("admin_state_up") String queryAdminStateUp,
            @QueryParam("status") String queryStatus,
            @QueryParam("shared") String queryShared,
            @QueryParam("tenant_id") String queryTenantID,
            // TODO: fix decoration for router extension
            @QueryParam("router_external") String queryRouterExternal,
            // TODO: fix decorations for provider extension
            @QueryParam("provider_network_type") String queryProviderNetworkType,
            @QueryParam("provider_physical_network") String queryProviderPhysicalNetwork,
            @QueryParam("provider_segmentation_id") String queryProviderSegmentationID,
            // pagination
            @QueryParam("limit") String limit,
            @QueryParam("marker") String marker,
            @QueryParam("page_reverse") String pageReverse
            // sorting not supported
        ) {
        IfNBNetworkCRUD networkInterface = OpenDoveNBInterfaces.getIfNBNetworkCRUD("default", this);
        if (networkInterface == null) {
            throw new ServiceUnavailableException("Network CRUD Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        List<OpenStackNetworks> allNetworks = networkInterface.getAllNetworks();
        List<OpenStackNetworks> ans = new ArrayList<OpenStackNetworks>();
        Iterator<OpenStackNetworks> i = allNetworks.iterator();
        while (i.hasNext()) {
            OpenStackNetworks oSN = i.next();
            //match filters: TODO provider extension and router extension
            Boolean bAdminStateUp = null;
            Boolean bShared = null;
            if (queryAdminStateUp != null)
                bAdminStateUp = new Boolean(queryAdminStateUp);
            if (queryShared != null)
                bShared = new Boolean(queryShared);
            if ((queryID == null || queryID.equals(oSN.getID())) &&
                    (queryName == null || queryName.equals(oSN.getNetworkName())) &&
                    (bAdminStateUp == null || bAdminStateUp.booleanValue() == oSN.isAdminStateUp()) &&
                    (queryStatus == null || queryStatus.equals(oSN.getStatus())) &&
                    (bShared == null || bShared.booleanValue() == oSN.isShared()) &&
                    (queryTenantID == null || queryTenantID.equals(oSN.getTenantID()))) {
                if (fields.size() > 0)
                    ans.add(extractFields(oSN,fields));
                else
                    ans.add(oSN);
            }
        }
        //TODO: apply pagination to results
        //TODO: how to return proper empty set result: { "networks": [] }
        //TODO: how to return proper set with one element result: { "networks": [{}] }
        return Response.status(200).entity(
                new OpenStackNetworkRequest(ans)).build();
    }

    /**
     * Returns a specific Network */

    @Path("{netUUID}")
    @GET
    @Produces({ MediaType.APPLICATION_JSON })
    //@TypeHint(OpenStackNetworks.class)
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 404, condition = "Not Found") })
    public Response showNetwork(
            @PathParam("netUUID") String netUUID,
            // return fields
            @QueryParam("fields") List<String> fields
            ) {
        IfNBNetworkCRUD networkInterface = OpenDoveNBInterfaces.getIfNBNetworkCRUD("default", this);
        if (networkInterface == null) {
            throw new ServiceUnavailableException("Network CRUD Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        if (!networkInterface.networkExists(netUUID))
            return Response.status(404).build();
        if (fields.size() > 0) {
            OpenStackNetworks ans = networkInterface.getNetwork(netUUID);
            return Response.status(200).entity(
                    new OpenStackNetworkRequest(extractFields(ans, fields))).build();
        } else
            return Response.status(200).entity(
                    new OpenStackNetworkRequest(networkInterface.getNetwork(netUUID))).build();
    }

    /**
     * Creates new Networks */
    @POST
    @Produces({ MediaType.APPLICATION_JSON })
    @Consumes({ MediaType.APPLICATION_JSON })
    @TypeHint(OpenStackNetworks.class)
    @StatusCodes({
            @ResponseCode(code = 201, condition = "Created"),
            @ResponseCode(code = 400, condition = "Bad Request"),
            @ResponseCode(code = 401, condition = "Unauthorized") })
    public Response createNetworks(final OpenStackNetworkRequest input) {
        IfNBNetworkCRUD networkInterface = OpenDoveNBInterfaces.getIfNBNetworkCRUD("default", this);
        if (networkInterface == null) {
            throw new ServiceUnavailableException("Network CRUD Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        if (input.isSingleton()) {
            OpenStackNetworks singleton = input.getSingleton();

            /*
             * network ID can't already exist
             */
            if (networkInterface.networkExists(singleton.getID()))
                return Response.status(400).build();

            /*
             * DOVE requirements on create. TODO: check existance of network name?
             */
            if (!singleton.isAdminStateUp())
                return Response.status(400).build();

            // add network to cache
            singleton.initDefaults();
            networkInterface.addNetwork(singleton);
        } else {
            List<OpenStackNetworks> bulk = input.getBulk();
            Iterator<OpenStackNetworks> i = bulk.iterator();
            HashMap<String, OpenStackNetworks> testMap = new HashMap<String, OpenStackNetworks>();
            while (i.hasNext()) {
                OpenStackNetworks test = i.next();

                /*
                 * network ID can't already exist, nor can there be an entry for this UUID
                 * already in this bulk request
                 */
                if (networkInterface.networkExists(test.getID()))
                    return Response.status(400).build();
                if (testMap.containsKey(test.getID()))
                    return Response.status(400).build();

                /*
                 * DOVE requirements on create. TODO: check existance of network name?
                 */
                if (!test.isAdminStateUp())
                    return Response.status(400).build();
                testMap.put(test.getID(),test);
            }

            // now that everything passed, add items to the cache
            i = bulk.iterator();
            while (i.hasNext()) {
                OpenStackNetworks test = i.next();
                test.initDefaults();
                networkInterface.addNetwork(test);
            }
        }
        return Response.status(201).entity(input).build();
    }

    /**
     * Updates a Network */
    @Path("{netUUID}")
    @PUT
    @Produces({ MediaType.APPLICATION_JSON })
    @Consumes({ MediaType.APPLICATION_JSON })
    //@TypeHint(OpenStackNetworks.class)
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 400, condition = "Bad Request"),
            @ResponseCode(code = 403, condition = "Forbidden"),
            @ResponseCode(code = 404, condition = "Not Found"), })
    public Response updateNetwork(
            @PathParam("netUUID") String netUUID, final OpenStackNetworkRequest input
            ) {
        IfNBNetworkCRUD networkInterface = OpenDoveNBInterfaces.getIfNBNetworkCRUD("default", this);
        if (networkInterface == null) {
            throw new ServiceUnavailableException("Network CRUD Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }

        /*
         * network has to exist and only a single delta is supported
         */
        if (!networkInterface.networkExists(netUUID))
            return Response.status(404).build();
        if (!input.isSingleton())
            return Response.status(400).build();
        OpenStackNetworks singleton = input.getSingleton();

        /*
         * transitions forbidden by Neutron
         */
        if (singleton.getID() != null || singleton.getTenantID() != null ||
                singleton.getStatus() != null)
            return Response.status(400).build();

        /*
         * transitions forbidden by DOVE
         */
        if (singleton.getNetworkName() != null || singleton.getAdminStateUp() != null ||
                singleton.getShared() != null || singleton.getRouterExternal() != null)
            return Response.status(403).build();

        // update network object and return the modified object
        networkInterface.updateNetwork(netUUID, singleton);
        return Response.status(200).entity(
                new OpenStackNetworkRequest(networkInterface.getNetwork(netUUID))).build();
    }

    /**
     * Deletes a Network */

    @Path("{netUUID}")
    @DELETE
    @StatusCodes({
            @ResponseCode(code = 204, condition = "No Content"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 404, condition = "Not Found"),
            @ResponseCode(code = 409, condition = "Network In Use") })
    public Response deleteNetwork(
            @PathParam("netUUID") String netUUID) {
        IfNBNetworkCRUD networkInterface = OpenDoveNBInterfaces.getIfNBNetworkCRUD("default", this);
        if (networkInterface == null) {
            throw new ServiceUnavailableException("Network CRUD Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }

        /*
         * network has to exist and not be in use before it can be removed
         */
        if (!networkInterface.networkExists(netUUID))
            return Response.status(404).build();
        if (networkInterface.networkInUse(netUUID))
            return Response.status(409).build();
        networkInterface.removeNetwork(netUUID);
        return Response.status(204).build();
    }
}
