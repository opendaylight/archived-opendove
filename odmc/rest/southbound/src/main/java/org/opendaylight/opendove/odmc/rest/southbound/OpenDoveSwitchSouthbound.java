/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.opendove.odmc.rest.southbound;

import javax.ws.rs.GET;
import javax.ws.rs.POST;
import javax.ws.rs.PUT;
import javax.ws.rs.Path;
import javax.ws.rs.PathParam;
import javax.ws.rs.Produces;
import javax.ws.rs.core.MediaType;
import javax.ws.rs.core.Response;

import java.text.SimpleDateFormat;
import java.util.Calendar;


import org.codehaus.enunciate.jaxrs.ResponseCode;
import org.codehaus.enunciate.jaxrs.StatusCodes;
import org.opendaylight.controller.networkconfig.neutron.INeutronPortCRUD;
import org.opendaylight.controller.networkconfig.neutron.NeutronCRUDInterfaces;
import org.opendaylight.controller.networkconfig.neutron.NeutronPort;
import org.opendaylight.controller.northbound.commons.RestMessages;
import org.opendaylight.controller.northbound.commons.exception.ServiceUnavailableException;
import org.opendaylight.opendove.odmc.IfOpenDoveNetworkCRUD;
import org.opendaylight.opendove.odmc.IfOpenDoveSwitchCRUD;
import org.opendaylight.opendove.odmc.OpenDoveCRUDInterfaces;
import org.opendaylight.opendove.odmc.OpenDoveNetwork;
import org.opendaylight.opendove.odmc.OpenDoveSwitch;

/**
 * Open DOVE Southbound REST APIs for oDCS Switch.<br>
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

@Path("/switch")
public class OpenDoveSwitchSouthbound {

    /*
     *  REST Handler Function for oVS <==> DMC Registration
     */
    @POST
    @Produces({ MediaType.APPLICATION_JSON })
    @StatusCodes({
        @ResponseCode(code = 200, condition = "Operation successful"),
        @ResponseCode(code = 201, condition = "Registration Accepted"),
        @ResponseCode(code = 409, condition = "Service Appliance IP Address Conflict"),
        @ResponseCode(code = 500, condition = "Internal Error") })
    public Response processRegistration (OpenDoveSwitch openDoveSwitch) {
        IfOpenDoveSwitchCRUD sbInterface = OpenDoveCRUDInterfaces.getIfOpenDoveSwitchCRU(this);

        if (sbInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }

        if (sbInterface.switchExists(openDoveSwitch.getUUID())) {
            sbInterface.updateSwitch(openDoveSwitch.getUUID(), openDoveSwitch);
            OpenDoveSwitch target = sbInterface.getSwitch(openDoveSwitch.getUUID());
            if (target.getReRegister()) {
                target.setReRegister(false);
                sbInterface.updateSwitch(openDoveSwitch.getUUID(), target);
                return Response.status(205).entity(openDoveSwitch).build();
            } else
                return Response.status(204).entity(openDoveSwitch).build();
        }
        
        // Set the Timestamp, UUID and Registration flag
        String timestamp = new SimpleDateFormat("EEE MMM dd HH:mm:ss zzz yyyy").format(Calendar.getInstance().getTime());
        openDoveSwitch.setTimestamp(timestamp);
        openDoveSwitch.setUUID(java.util.UUID.randomUUID().toString());
        openDoveSwitch.setReRegister(new Boolean(false));
        sbInterface.addSwitch(openDoveSwitch.getUUID(), openDoveSwitch);

        return Response.status(201).entity(openDoveSwitch).build();
    }

    /*
     *  REST Handler Function for DCS Heart-Beat
     */
    @Path("/{switchUUID}")
    @PUT
    @Produces({ MediaType.APPLICATION_JSON })
    @StatusCodes({
        @ResponseCode(code = 200, condition = "Operation successful"),
        @ResponseCode(code = 409, condition = "Service Appliance IP Address Conflict"),
        @ResponseCode(code = 500, condition = "Internal Error") })

    public Response procesHeartbeat (
            @PathParam("switchUUID") String switchUUID,
            OpenDoveSwitch openDoveSwitch) {

        IfOpenDoveSwitchCRUD sbInterface = OpenDoveCRUDInterfaces.getIfOpenDoveSwitchCRU(this);

        if (sbInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }

        /*
         *  Heart-Beat from Same UUID with a different IP will be accepted - It will be
         *  treated as a change in IP Address, Infinispan Cache will be updated in this
         *  case.
         *
         *  Heart-Beat from different UUID with an  IP that already exists in DMC Cache will
         *  treated as a conflict
         */
        if (!sbInterface.switchExists(switchUUID))
            return Response.status(409).build();

        String timestamp = new SimpleDateFormat("EEE MMM dd HH:mm:ss zzz yyyy").format(Calendar.getInstance().getTime());
        openDoveSwitch.setTimestamp(timestamp);

        sbInterface.updateSwitch(switchUUID, openDoveSwitch);
        OpenDoveSwitch target = sbInterface.getSwitch(openDoveSwitch.getUUID());
        if (target.getReRegister()) {
            target.setReRegister(false);
            sbInterface.updateSwitch(openDoveSwitch.getUUID(), target);
            return Response.status(205).build();
        } else
            return Response.status(204).build();
    }
     
    @Path("/{switchUUID}/getVNIDbyPort/{portUUID}")
    @GET
    @Produces({ MediaType.APPLICATION_JSON })
    @StatusCodes({
        @ResponseCode(code = 204, condition = "No content"),
        @ResponseCode(code = 401, condition = "Unauthorized"),
        @ResponseCode(code = 404, condition = "Not Found"),
        @ResponseCode(code = 500, condition = "Internal Error") })
    public Response getByPort(
    		@PathParam("switchUUID") String switchUUID,
    		@PathParam("portUUID") String portUUID) {
        IfOpenDoveSwitchCRUD sbInterface = OpenDoveCRUDInterfaces.getIfOpenDoveSwitchCRU(this);

        if (sbInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }

        if (!sbInterface.switchExists(switchUUID))
            return Response.status(404).build();
        OpenDoveSwitch oSwitch = sbInterface.getSwitch(switchUUID);
    	
    	INeutronPortCRUD sbNeutronPortInterface = NeutronCRUDInterfaces.getINeutronPortCRUD(this);
    	if (!sbNeutronPortInterface.portExists(portUUID))
    		return Response.status(404).build();
    	
    	NeutronPort nPort = sbNeutronPortInterface.getPort(portUUID);
    	
        IfOpenDoveNetworkCRUD sbNetworkInterface = OpenDoveCRUDInterfaces.getIfDoveNetworkCRU(this);
        if (sbNetworkInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
    	for (OpenDoveNetwork network : sbNetworkInterface.getNetworks()) {
    		if (network.getNeutronNetwork().equalsIgnoreCase(nPort.getNetworkUUID()) &&
    				network.getScopingDomain().getAssociatedOSTenantUUID().equalsIgnoreCase(nPort.getTenantID())) {
    			if (!network.getHostingSwitches().contains(oSwitch))
    				network.getHostingSwitches().add(oSwitch);
    			return Response.status(200).entity(network).build();
    		}
        }
    	return Response.status(404).build();
    }
}

