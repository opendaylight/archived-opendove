/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.opendove.odmc.northbound;

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
import org.opendaylight.opendove.odmc.IfNBNetworkCRUD;
import org.opendaylight.opendove.odmc.IfNBPortCRUD;
import org.opendaylight.opendove.odmc.IfNBSubnetCRUD;
import org.opendaylight.opendove.odmc.OpenStackIPs;
import org.opendaylight.opendove.odmc.OpenStackPorts;
import org.opendaylight.opendove.odmc.OpenStackSubnets;

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

@Path("/ports")
public class OpenDovePortsNorthbound {

    private OpenStackPorts extractFields(OpenStackPorts o, List<String> fields) {
        return o.extractFields(fields);
    }

    /**
     * Returns a list of all Ports */

    @GET
    @Produces({ MediaType.APPLICATION_JSON })
    //@TypeHint(OpenStackPorts.class)
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 501, condition = "Not Implemented") })
    public Response listPorts(
            // return fields
            @QueryParam("fields") List<String> fields,
            // note: openstack isn't clear about filtering on lists, so we aren't handling them
            @QueryParam("id") String queryID,
            @QueryParam("network_id") String queryNetworkID,
            @QueryParam("name") String queryName,
            @QueryParam("admin_state_up") String queryAdminStateUp,
            @QueryParam("status") String queryStatus,
            @QueryParam("mac_address") String queryMACAddress,
            @QueryParam("device_id") String queryDeviceID,
            @QueryParam("device_owner") String queryDeviceOwner,
            @QueryParam("tenant_id") String queryTenantID,
            // pagination
            @QueryParam("limit") String limit,
            @QueryParam("marker") String marker,
            @QueryParam("page_reverse") String pageReverse
            // sorting not supported
            ) {
        IfNBPortCRUD portInterface = OpenDoveNBInterfaces.getIfNBPortCRUD("default",this);
        if (portInterface == null) {
            throw new ServiceUnavailableException("Port CRUD Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        List<OpenStackPorts> allPorts = portInterface.getAllPorts();
        List<OpenStackPorts> ans = new ArrayList<OpenStackPorts>();
        Iterator<OpenStackPorts> i = allPorts.iterator();
        while (i.hasNext()) {
            OpenStackPorts oSS = i.next();
            if ((queryID == null || queryID.equals(oSS.getID())) &&
                    (queryNetworkID == null || queryNetworkID.equals(oSS.getNetworkUUID())) &&
                    (queryName == null || queryName.equals(oSS.getName())) &&
                    (queryAdminStateUp == null || queryAdminStateUp.equals(oSS.getAdminStateUp())) &&
                    (queryStatus == null || queryStatus.equals(oSS.getStatus())) &&
                    (queryMACAddress == null || queryMACAddress.equals(oSS.getMacAddress())) &&
                    (queryDeviceID == null || queryDeviceID.equals(oSS.getDeviceID())) &&
                    (queryDeviceOwner == null || queryDeviceOwner.equals(oSS.getDeviceOwner())) &&
                    (queryTenantID == null || queryTenantID.equals(oSS.getTenantID()))) {
                if (fields.size() > 0)
                    ans.add(extractFields(oSS,fields));
                else
                    ans.add(oSS);
            }
        }
        //TODO: apply pagination to results
        //TODO: how to return proper empty set result: { "ports": [] }
        //TODO: how to return proper set with one element result: { "ports": [{}] }
        return Response.status(200).entity(
                new OpenStackPortRequest(ans)).build();
    }

    /**
     * Returns a specific Port */

    @Path("{portUUID}")
    @GET
    @Produces({ MediaType.APPLICATION_JSON })
    //@TypeHint(OpenStackPorts.class)
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 404, condition = "Not Found"),
            @ResponseCode(code = 501, condition = "Not Implemented") })
    public Response showPort(
            @PathParam("portUUID") String portUUID,
            // return fields
            @QueryParam("fields") List<String> fields ) {
        IfNBPortCRUD portInterface = OpenDoveNBInterfaces.getIfNBPortCRUD("default",this);
        if (portInterface == null) {
            throw new ServiceUnavailableException("Port CRUD Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        if (!portInterface.portExists(portUUID))
            return Response.status(404).build();
        if (fields.size() > 0) {
            OpenStackPorts ans = portInterface.getPort(portUUID);
            return Response.status(200).entity(
                    new OpenStackPortRequest(extractFields(ans, fields))).build();
        } else
            return Response.status(200).entity(
                    new OpenStackPortRequest(portInterface.getPort(portUUID))).build();
    }

    /**
     * Creates new Ports */

    @POST
    @Produces({ MediaType.APPLICATION_JSON })
    @Consumes({ MediaType.APPLICATION_JSON })
    //@TypeHint(OpenStackPorts.class)
    @StatusCodes({
            @ResponseCode(code = 201, condition = "Created"),
            @ResponseCode(code = 400, condition = "Bad Request"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 403, condition = "Forbidden"),
            @ResponseCode(code = 404, condition = "Not Found"),
            @ResponseCode(code = 409, condition = "Conflict"),
            @ResponseCode(code = 501, condition = "Not Implemented"),
            @ResponseCode(code = 503, condition = "MAC generation failure") })
    public Response createPorts(final OpenStackPortRequest input) {
        IfNBPortCRUD portInterface = OpenDoveNBInterfaces.getIfNBPortCRUD("default",this);
        if (portInterface == null) {
            throw new ServiceUnavailableException("Port CRUD Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        IfNBNetworkCRUD networkInterface = OpenDoveNBInterfaces.getIfNBNetworkCRUD("default", this);
        if (networkInterface == null) {
            throw new ServiceUnavailableException("Network CRUD Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        IfNBSubnetCRUD subnetInterface = OpenDoveNBInterfaces.getIfNBSubnetCRUD("default", this);
        if (subnetInterface == null) {
            throw new ServiceUnavailableException("Subnet CRUD Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        if (input.isSingleton()) {
            OpenStackPorts singleton = input.getSingleton();

            /*
             * the port must be part of an existing network, must not already exist,
             * have a valid MAC and the MAC not be in use
             */
            if (singleton.getNetworkUUID() == null)
                return Response.status(400).build();
            if (portInterface.portExists(singleton.getID()))
                return Response.status(400).build();
            if (!networkInterface.networkExists(singleton.getNetworkUUID()))
                return Response.status(404).build();
            if (singleton.getMacAddress() == null ||
                    !singleton.getMacAddress().matches("^([0-9A-F]{2}[:-]){5}([0-9A-F]{2})$"))
                return Response.status(400).build();
            if (portInterface.macInUse(singleton.getMacAddress()))
                return Response.status(409).build();

            // DOVE specific requirement on create
            if (!singleton.isAdminStateUp())
                return Response.status(400).build();

            /*
             * if fixed IPs are specified, each one has to have an existing subnet ID
             * that is in the same scoping network as the port.  In addition, if an IP
             * address is specified it has to be a valid address for the subnet and not
             * already in use
             */
            List<OpenStackIPs> fixedIPs = singleton.getFixedIPs();
            if (fixedIPs != null && fixedIPs.size() > 0) {
                Iterator<OpenStackIPs> fixedIPIterator = fixedIPs.iterator();
                while (fixedIPIterator.hasNext()) {
                    OpenStackIPs ip = fixedIPIterator.next();
                    if (ip.getSubnetUUID() == null)
                        return Response.status(400).build();
                    if (!subnetInterface.subnetExists(ip.getSubnetUUID()))
                        return Response.status(400).build();
                    OpenStackSubnets subnet = subnetInterface.getSubnet(ip.getSubnetUUID());
                    if (!singleton.getNetworkUUID().equalsIgnoreCase(subnet.getNetworkUUID()))
                        return Response.status(400).build();
                    if (ip.getIpAddress() != null) {
                        if (!subnet.isValidIP(ip.getIpAddress()))
                            return Response.status(400).build();
                        if (subnet.isIPInUse(ip.getIpAddress()))
                            return Response.status(409).build();
                    }
                }
            }

            // add the port to the cache
            singleton.initDefaults();
            portInterface.addPort(singleton);
        } else {
            List<OpenStackPorts> bulk = input.getBulk();
            Iterator<OpenStackPorts> i = bulk.iterator();
            HashMap<String, OpenStackPorts> testMap = new HashMap<String, OpenStackPorts>();
            while (i.hasNext()) {
                OpenStackPorts test = i.next();
                test.initDefaults();

                /*
                 * the port must be part of an existing network, must not already exist,
                 * have a valid MAC and the MAC not be in use.  Further the bulk request
                 * can't already contain a new port with the same UUID
                 */
                if (portInterface.portExists(test.getID()))
                    return Response.status(400).build();
                if (testMap.containsKey(test.getID()))
                    return Response.status(400).build();
                testMap.put(test.getID(), test);
                if (!networkInterface.networkExists(test.getNetworkUUID()))
                    return Response.status(404).build();
                if (!test.getMacAddress().matches("^([0-9A-F]{2}[:-]){5}([0-9A-F]{2})$"))
                    return Response.status(400).build();
                if (portInterface.macInUse(test.getMacAddress()))
                    return Response.status(409).build();

                // DOVE specific create requirement
                if (!test.isAdminStateUp())
                    return Response.status(400).build();

                /*
                 * if fixed IPs are specified, each one has to have an existing subnet ID
                 * that is in the same scoping network as the port.  In addition, if an IP
                 * address is specified it has to be a valid address for the subnet and not
                 * already in use
                 */
                List<OpenStackIPs> fixedIPs = test.getFixedIPs();
                if (fixedIPs != null && fixedIPs.size() > 0) {
                    Iterator<OpenStackIPs> fixedIPIterator = fixedIPs.iterator();
                    while (fixedIPIterator.hasNext()) {
                        OpenStackIPs ip = fixedIPIterator.next();
                        if (ip.getSubnetUUID() == null)
                            return Response.status(400).build();
                        if (!subnetInterface.subnetExists(ip.getSubnetUUID()))
                            return Response.status(400).build();
                        OpenStackSubnets subnet = subnetInterface.getSubnet(ip.getSubnetUUID());
                        if (!test.getNetworkUUID().equalsIgnoreCase(subnet.getNetworkUUID()))
                            return Response.status(400).build();
                        if (ip.getIpAddress() != null) {
                            if (!subnet.isValidIP(ip.getIpAddress()))
                                return Response.status(400).build();
                            if (!subnet.isIPInUse(ip.getIpAddress()))
                                return Response.status(409).build();
                        }
                    }
                }
            }

            //once everything has passed, then we can add to the cache
            i = bulk.iterator();
            while (i.hasNext()) {
                OpenStackPorts test = i.next();
                portInterface.addPort(test);
            }
        }
        return Response.status(201).entity(input).build();
    }

    /**
     * Updates a Port */

    @Path("{portUUID}")
    @PUT
    @Produces({ MediaType.APPLICATION_JSON })
    @Consumes({ MediaType.APPLICATION_JSON })
    //@TypeHint(OpenStackPorts.class)
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 400, condition = "Bad Request"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 403, condition = "Forbidden"),
            @ResponseCode(code = 404, condition = "Not Found"),
            @ResponseCode(code = 409, condition = "Conflict"),
            @ResponseCode(code = 501, condition = "Not Implemented") })
    public Response updatePort(
            @PathParam("portUUID") String portUUID,
            OpenStackPortRequest input
            ) {
        IfNBPortCRUD portInterface = OpenDoveNBInterfaces.getIfNBPortCRUD("default",this);
        if (portInterface == null) {
            throw new ServiceUnavailableException("Port CRUD Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        IfNBSubnetCRUD subnetInterface = OpenDoveNBInterfaces.getIfNBSubnetCRUD("default", this);
        if (subnetInterface == null) {
            throw new ServiceUnavailableException("Subnet CRUD Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }

        // port has to exist and only a single delta is supported
        if (!portInterface.portExists(portUUID))
            return Response.status(404).build();
        OpenStackPorts target = portInterface.getPort(portUUID);
        if (!input.isSingleton())
            return Response.status(400).build();
        OpenStackPorts singleton = input.getSingleton();

        // deltas restricted by Neutron
        if (singleton.getID() != null || singleton.getTenantID() != null ||
                singleton.getStatus() != null)
            return Response.status(400).build();

        // DOVE specific things that can't be changed
        if (singleton.getAdminStateUp() != null)
            return Response.status(403).build();

        // Verify the new fixed ips are valid
        List<OpenStackIPs> fixedIPs = singleton.getFixedIPs();
        if (fixedIPs != null && fixedIPs.size() > 0) {
            Iterator<OpenStackIPs> fixedIPIterator = fixedIPs.iterator();
            while (fixedIPIterator.hasNext()) {
                OpenStackIPs ip = fixedIPIterator.next();
                if (ip.getSubnetUUID() == null)
                    return Response.status(400).build();
                if (!subnetInterface.subnetExists(ip.getSubnetUUID()))
                    return Response.status(400).build();
                OpenStackSubnets subnet = subnetInterface.getSubnet(ip.getSubnetUUID());
                if (!target.getNetworkUUID().equalsIgnoreCase(subnet.getNetworkUUID()))
                    return Response.status(400).build();
                if (ip.getIpAddress() != null) {
                    if (!subnet.isValidIP(ip.getIpAddress()))
                        return Response.status(400).build();
                    if (subnet.isIPInUse(ip.getIpAddress()))
                        return Response.status(409).build();
                }
            }
        }

//        TODO: Support change of security groups
        // update the port and return the modified object
        portInterface.updatePort(portUUID, singleton);
        return Response.status(200).entity(
                new OpenStackPortRequest(portInterface.getPort(portUUID))).build();

    }

    /**
     * Deletes a Port */

    @Path("{portUUID}")
    @DELETE
    @StatusCodes({
        @ResponseCode(code = 204, condition = "No Content"),
        @ResponseCode(code = 401, condition = "Unauthorized"),
        @ResponseCode(code = 403, condition = "Forbidden"),
        @ResponseCode(code = 404, condition = "Not Found"),
        @ResponseCode(code = 501, condition = "Not Implemented") })
    public Response deletePort(
            @PathParam("portUUID") String portUUID) {
        IfNBPortCRUD portInterface = OpenDoveNBInterfaces.getIfNBPortCRUD("default",this);
        if (portInterface == null) {
            throw new ServiceUnavailableException("Port CRUD Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }

        // port has to exist and not be owned by anyone.  then it can be removed from the cache
        if (!portInterface.portExists(portUUID))
            return Response.status(404).build();
        OpenStackPorts port = portInterface.getPort(portUUID);
        if (port.getDeviceID() != null ||
                port.getDeviceOwner() != null)
            Response.status(403).build();
        portInterface.removePort(portUUID);
        return Response.status(204).build();
    }
}
