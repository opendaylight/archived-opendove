/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.opendove.odmc.northbound;

import java.util.ArrayList;
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
import org.opendaylight.opendove.odmc.IfNBNetworkCRUD;
import org.opendaylight.opendove.odmc.IfNBPortCRUD;
import org.opendaylight.opendove.odmc.IfNBRouterCRUD;
import org.opendaylight.opendove.odmc.IfNBSubnetCRUD;
import org.opendaylight.opendove.odmc.OpenStackIPs;
import org.opendaylight.opendove.odmc.OpenStackNetworks;
import org.opendaylight.opendove.odmc.OpenStackPorts;
import org.opendaylight.opendove.odmc.OpenStackRouterInterfaces;
import org.opendaylight.opendove.odmc.OpenStackRouters;
import org.opendaylight.opendove.odmc.OpenStackSubnets;
import org.opendaylight.controller.northbound.commons.RestMessages;
import org.opendaylight.controller.northbound.commons.exception.ServiceUnavailableException;


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

@Path("/routers")
public class OpenDoveRoutersNorthbound {

    private OpenStackRouters extractFields(OpenStackRouters o, List<String> fields) {
        return o.extractFields(fields);
    }

    /**
     * Returns a list of all Routers */

    @GET
    @Produces({ MediaType.APPLICATION_JSON })
    //@TypeHint(OpenStackRouters.class)
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 501, condition = "Not Implemented") })
    public Response listRouters(
            // return fields
            @QueryParam("fields") List<String> fields,
            // note: openstack isn't clear about filtering on lists, so we aren't handling them
            @QueryParam("id") String queryID,
            @QueryParam("name") String queryName,
            @QueryParam("admin_state_up") String queryAdminStateUp,
            @QueryParam("status") String queryStatus,
            @QueryParam("tenant_id") String queryTenantID,
            @QueryParam("external_gateway_info") String queryExternalGatewayInfo,
            // pagination
            @QueryParam("limit") String limit,
            @QueryParam("marker") String marker,
            @QueryParam("page_reverse") String pageReverse
            // sorting not supported
            ) {
        IfNBRouterCRUD routerInterface = OpenDoveNBInterfaces.getIfNBRouterCRUD("default",this);
        if (routerInterface == null) {
            throw new ServiceUnavailableException("Router CRUD Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        List<OpenStackRouters> allRouters = routerInterface.getAllRouters();
        List<OpenStackRouters> ans = new ArrayList<OpenStackRouters>();
        Iterator<OpenStackRouters> i = allRouters.iterator();
        while (i.hasNext()) {
            OpenStackRouters oSS = i.next();
            if ((queryID == null || queryID.equals(oSS.getID())) &&
                    (queryName == null || queryName.equals(oSS.getName())) &&
                    (queryAdminStateUp == null || queryAdminStateUp.equals(oSS.getAdminStateUp())) &&
                    (queryStatus == null || queryStatus.equals(oSS.getStatus())) &&
                    (queryExternalGatewayInfo == null || queryExternalGatewayInfo.equals(oSS.getExternalGatewayInfo())) &&
                    (queryTenantID == null || queryTenantID.equals(oSS.getTenantID()))) {
                if (fields.size() > 0)
                    ans.add(extractFields(oSS,fields));
                else
                    ans.add(oSS);
            }
        }
        //TODO: apply pagination to results
        //TODO: how to return proper empty set result: { "routers": [] }
        //TODO: how to return proper set with one element result: { "routers": [{}] }
        return Response.status(200).entity(
                new OpenStackRouterRequest(ans)).build();
    }

    /**
     * Returns a specific Router */

    @Path("{routerUUID}")
    @GET
    @Produces({ MediaType.APPLICATION_JSON })
    //@TypeHint(OpenStackRouters.class)
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 403, condition = "Forbidden"),
            @ResponseCode(code = 404, condition = "Not Found"),
            @ResponseCode(code = 501, condition = "Not Implemented") })
    public Response showRouter(
            @PathParam("routerUUID") String routerUUID,
            // return fields
            @QueryParam("fields") List<String> fields) {
        IfNBRouterCRUD routerInterface = OpenDoveNBInterfaces.getIfNBRouterCRUD("default",this);
        if (routerInterface == null) {
            throw new ServiceUnavailableException("Router CRUD Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        if (!routerInterface.routerExists(routerUUID))
            return Response.status(404).build();
        if (fields.size() > 0) {
            OpenStackRouters ans = routerInterface.getRouter(routerUUID);
            return Response.status(200).entity(
                    new OpenStackRouterRequest(extractFields(ans, fields))).build();
        } else
            return Response.status(200).entity(
                    new OpenStackRouterRequest(routerInterface.getRouter(routerUUID))).build();
    }

    /**
     * Creates new Routers */

    @POST
    @Produces({ MediaType.APPLICATION_JSON })
    @Consumes({ MediaType.APPLICATION_JSON })
    //@TypeHint(OpenStackRouters.class)
    @StatusCodes({
            @ResponseCode(code = 201, condition = "Created"),
            @ResponseCode(code = 400, condition = "Bad Request"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 501, condition = "Not Implemented") })
    public Response createRouters(final OpenStackRouterRequest input) {
        IfNBRouterCRUD routerInterface = OpenDoveNBInterfaces.getIfNBRouterCRUD("default",this);
        if (routerInterface == null) {
            throw new ServiceUnavailableException("Router CRUD Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        IfNBNetworkCRUD networkInterface = OpenDoveNBInterfaces.getIfNBNetworkCRUD("default", this);
        if (networkInterface == null) {
            throw new ServiceUnavailableException("Network CRUD Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        if (input.isSingleton()) {
            OpenStackRouters singleton = input.getSingleton();

            /*
             * verify that the router doesn't already exist (issue: is deeper inspection necessary?)
             * if there is external gateway information provided, verify that the specified network
             * exists and has been designated as "router:external"
             */
            if (routerInterface.routerExists(singleton.getID()))
                return Response.status(400).build();
            if (singleton.getExternalGatewayInfo() != null) {
                String externNetworkPtr = singleton.getExternalGatewayInfo().getNetworkID();
                if (!networkInterface.networkExists(externNetworkPtr))
                    return Response.status(400).build();
                OpenStackNetworks externNetwork = networkInterface.getNetwork(externNetworkPtr);
                if (!externNetwork.isRouterExternal())
                    return Response.status(400).build();
            }

            /*
             * DOVE specific requirement on create
             */
            if (!singleton.isAdminStateUp())
                return Response.status(400).build();
            singleton.initDefaults();

            /*
             * add router to the cache
             */
            routerInterface.addRouter(singleton);
        } else {

            /*
             * only singleton router creates supported
             */
            return Response.status(400).build();
        }
        return Response.status(201).entity(input).build();
    }

    /**
     * Updates a Router */

    @Path("{routerUUID}")
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
            @PathParam("routerUUID") String routerUUID,
            OpenStackRouterRequest input
            ) {
        IfNBRouterCRUD routerInterface = OpenDoveNBInterfaces.getIfNBRouterCRUD("default",this);
        if (routerInterface == null) {
            throw new ServiceUnavailableException("Router CRUD Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        IfNBNetworkCRUD networkInterface = OpenDoveNBInterfaces.getIfNBNetworkCRUD("default", this);
        if (networkInterface == null) {
            throw new ServiceUnavailableException("Network CRUD Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }

        /*
         * router has to exist and only a single delta can be supplied
         */
        if (!routerInterface.routerExists(routerUUID))
            return Response.status(404).build();
        if (!input.isSingleton())
            return Response.status(400).build();
        OpenStackRouters singleton = input.getSingleton();

        /*
         * attribute changes blocked by Neutron
         */
        if (singleton.getID() != null || singleton.getTenantID() != null ||
                singleton.getStatus() != null)
            return Response.status(400).build();

        /*
         * attribute changes blocked by DOVE
         */
        if (singleton.getAdminStateUp() != null)
            return Response.status(403).build();

        /* FIXME: if the external gateway info is being changed,
         * block with 403 if there is a router interface using it
         * (DOVE restriction)
         */
        /*
         * if the external gateway info is being changed, verify that the new network
         * exists and has been designated as an external network
         */
        if (singleton.getExternalGatewayInfo() != null) {
            String externNetworkPtr = singleton.getExternalGatewayInfo().getNetworkID();
            if (!networkInterface.networkExists(externNetworkPtr))
                return Response.status(400).build();
            OpenStackNetworks externNetwork = networkInterface.getNetwork(externNetworkPtr);
            if (!externNetwork.isRouterExternal())
                return Response.status(400).build();
        }

        /*
         * update the router entry and return the modified object
         */
        routerInterface.updateRouter(routerUUID, singleton);
        return Response.status(200).entity(
                new OpenStackRouterRequest(routerInterface.getRouter(routerUUID))).build();

    }

    /**
     * Deletes a Router */

    @Path("{routerUUID}")
    @DELETE
    @StatusCodes({
            @ResponseCode(code = 204, condition = "No Content"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 404, condition = "Not Found"),
            @ResponseCode(code = 409, condition = "Conflict"),
            @ResponseCode(code = 501, condition = "Not Implemented") })
    public Response deleteRouter(
            @PathParam("routerUUID") String routerUUID) {
        IfNBRouterCRUD routerInterface = OpenDoveNBInterfaces.getIfNBRouterCRUD("default",this);
        if (routerInterface == null) {
            throw new ServiceUnavailableException("Router CRUD Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }

        /*
         * verify that the router exists and is not in use before removing it
         */
        if (!routerInterface.routerExists(routerUUID))
            return Response.status(404).build();
        if (routerInterface.routerInUse(routerUUID))
            return Response.status(409).build();
        routerInterface.removeRouter(routerUUID);
        return Response.status(204).build();
    }

    /**
     * Adds an interface to a router */

    @Path("{routerUUID}/add_router_interface")
    @PUT
    @Produces({ MediaType.APPLICATION_JSON })
    @Consumes({ MediaType.APPLICATION_JSON })
    //@TypeHint(OpenStackRouterInterfaces.class)
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 400, condition = "Bad Request"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 404, condition = "Not Found"),
            @ResponseCode(code = 409, condition = "Conflict"),
            @ResponseCode(code = 501, condition = "Not Implemented") })
    public Response addRouterInterface(
            @PathParam("routerUUID") String routerUUID,
            OpenStackRouterInterfaces input
            ) {
        IfNBRouterCRUD routerInterface = OpenDoveNBInterfaces.getIfNBRouterCRUD("default",this);
        if (routerInterface == null) {
            throw new ServiceUnavailableException("Router CRUD Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        IfNBPortCRUD portInterface = OpenDoveNBInterfaces.getIfNBPortCRUD("default",this);
        if (portInterface == null) {
            throw new ServiceUnavailableException("Port CRUD Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        IfNBSubnetCRUD subnetInterface = OpenDoveNBInterfaces.getIfNBSubnetCRUD("default",this);
        if (subnetInterface == null) {
            throw new ServiceUnavailableException("Subnet CRUD Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }

        /*
         *  the router has to exist and the input can only specify either a subnet id
         *  or a port id, but not both
         */
        if (!routerInterface.routerExists(routerUUID))
            return Response.status(400).build();
        OpenStackRouters target = routerInterface.getRouter(routerUUID);
        if (input.getSubnetUUID() != null &&
                input.getPortUUID() != null)
            return Response.status(400).build();
        if (input.getSubnetUUID() == null &&
                    input.getPortUUID() == null)
                return Response.status(400).build();

        /*
         * add an interface by subnet id
         */
        if (input.getSubnetUUID() != null) {

            //check for a port for the gateway ip of the subnet
            OpenStackPorts port = portInterface.getGatewayPort(input.getSubnetUUID());
            if (port != null) {

                //if exists, is it in use? 409 if it is
                if (port.getDeviceID() != null ||
                        port.getDeviceOwner() != null)
                    return Response.status(409).build();

                //assign the port to the router
                input.setPortUUID(port.getID());
                target.addInterface(input.getPortUUID(), input);

                //mark the port device id and device owner fields
                port.setDeviceOwner("network:router_interface");
                port.setDeviceID(routerUUID);
            } else {

                //if not create one and assign the gateway IP of the
                //subnet to it
                OpenStackPorts newPort = new OpenStackPorts();
                List<OpenStackIPs> newIPList = new ArrayList<OpenStackIPs>();
                OpenStackIPs newIP = new OpenStackIPs();
                newIP.setSubnetUUID(input.getSubnetUUID());
                OpenStackSubnets subnet = subnetInterface.getSubnet(input.getSubnetUUID());
                String gwAddr = subnet.getGatewayIP();
                newIP.setIpAddress(gwAddr);
                newIPList.add(newIP);
                newPort.setFixedIPs(newIPList);

                //TODO: how to assign this uuid?
                newPort.setPortUUID(gwAddr);
                newPort.setNetworkUUID(subnet.getNetworkUUID());
                portInterface.addPort(newPort);

                //assign the port to the router
                input.setPortUUID(newPort.getID());
                target.addInterface(input.getPortUUID(), input);

                //mark the port device id and device owner fields
                newPort.setDeviceOwner("network:router_interface");
                newPort.setDeviceID(routerUUID);
            }
        }

        /*
         * add interface by port id
         */
        if (input.getPortUUID() != null) {

            /*
             * verify that the port exists, has only a single fixed IP and that it is
             * not currently in use
             */
            if (!portInterface.portExists(input.getPortUUID()))
                return Response.status(400).build();
            OpenStackPorts port = portInterface.getPort(input.getPortUUID());
            if (port.getFixedIPs().size() != 1)
                return Response.status(400).build();
            if (port.getDeviceID() != null ||
                    port.getDeviceOwner() != null)
                return Response.status(409).build();

            //add the interface to the router
            input.setSubnetUUID(port.getFixedIPs().get(0).getSubnetUUID());
            target.addInterface(input.getPortUUID(), input);

            //mark the port device id and device owner fields
            port.setDeviceOwner("network:router_interface");
            port.setDeviceID(routerUUID);
        }
        return Response.status(200).entity(input).build();
    }

    /**
     * Removes an interface to a router */

    @Path("{routerUUID}/remove_router_interface")
    @PUT
    @Produces({ MediaType.APPLICATION_JSON })
    @Consumes({ MediaType.APPLICATION_JSON })
    //@TypeHint(OpenStackRouterInterfaces.class)
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 400, condition = "Bad Request"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 404, condition = "Not Found"),
            @ResponseCode(code = 409, condition = "Conflict"),
            @ResponseCode(code = 501, condition = "Not Implemented") })
    public Response removeRouterInterface(
            @PathParam("routerUUID") String routerUUID,
            OpenStackRouterInterfaces input
            ) {
        IfNBRouterCRUD routerInterface = OpenDoveNBInterfaces.getIfNBRouterCRUD("default",this);
        if (routerInterface == null) {
            throw new ServiceUnavailableException("Router CRUD Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        IfNBPortCRUD portInterface = OpenDoveNBInterfaces.getIfNBPortCRUD("default",this);
        if (portInterface == null) {
            throw new ServiceUnavailableException("Port CRUD Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        IfNBSubnetCRUD subnetInterface = OpenDoveNBInterfaces.getIfNBSubnetCRUD("default",this);
        if (subnetInterface == null) {
            throw new ServiceUnavailableException("Subnet CRUD Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }

        // verify the router exists
        if (!routerInterface.routerExists(routerUUID))
            return Response.status(400).build();
        OpenStackRouters target = routerInterface.getRouter(routerUUID);

        /*
         * remove by subnet id.  Collect information about the impacted router for the response and
         * remove the port corresponding to the gateway IP address of the subnet
         */
        if (input.getPortUUID() == null &&
                input.getSubnetUUID() != null) {
            OpenStackPorts port = portInterface.getGatewayPort(input.getSubnetUUID());
            if (port == null)
                return Response.status(404).build();
            input.setPortUUID(port.getID());
            target.removeInterface(input.getPortUUID());
            input.setID(target.getID());
            input.setTenantID(target.getTenantID());

            // reset the port ownership
            port.setDeviceID(null);
            port.setDeviceOwner(null);
            return Response.status(200).entity(input).build();
        }

        /*
         * remove by port id. collect information about the impacted router for the response
         * remove the interface and reset the port ownership
         */
        if (input.getPortUUID() != null &&
                input.getSubnetUUID() == null) {
            OpenStackRouterInterfaces targetInterface = target.getInterfaces().get(input.getPortUUID());
            target.removeInterface(input.getPortUUID());
            input.setSubnetUUID(targetInterface.getSubnetUUID());
            input.setID(target.getID());
            input.setTenantID(target.getTenantID());
            OpenStackPorts port = portInterface.getPort(input.getPortUUID());
            port.setDeviceID(null);
            port.setDeviceOwner(null);
            return Response.status(200).entity(input).build();
        }

        /*
         * remove by both port and subnet ID.  Verify that the first fixed IP of the port is a valid
         * IP address for the subnet, and then remove the interface, collecting information about the
         * impacted router for the response and reset port ownership
         */
        if (input.getPortUUID() != null &&
                input.getSubnetUUID() != null) {
            OpenStackPorts port = portInterface.getPort(input.getPortUUID());
            OpenStackSubnets subnet = subnetInterface.getSubnet(input.getSubnetUUID());
            if (!subnet.isValidIP(port.getFixedIPs().get(0).getIpAddress()))
                return Response.status(409).build();
            target.removeInterface(input.getPortUUID());
            input.setID(target.getID());
            input.setTenantID(target.getTenantID());
            port.setDeviceID(null);
            port.setDeviceOwner(null);
            return Response.status(200).entity(input).build();
        }

        // have to specify either a port ID or a subnet ID
        return Response.status(400).build();
    }
}
