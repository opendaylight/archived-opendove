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
import org.opendaylight.controller.northbound.commons.RestMessages;
import org.opendaylight.controller.northbound.commons.exception.ServiceUnavailableException;
import org.opendaylight.opendove.odmc.IfOpenDoveNetworkCRU;
import org.opendaylight.opendove.odmc.IfSBDoveSubnetCRUD;
import org.opendaylight.opendove.odmc.OpenDoveCRUDInterfaces;
import org.opendaylight.opendove.odmc.OpenDoveNetwork;
import org.opendaylight.opendove.odmc.OpenDoveSubnet;
import org.opendaylight.opendove.odmc.rest.OpenDoveNetworkRequest;

/**
 * Open DOVE Southbound REST APIs for Networks.<br>
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

@Path("/networks")
public class OpenDoveNetworkSouthbound {

    @Path("{networkUUID}")
    @GET
    @Produces({ MediaType.APPLICATION_JSON })
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 204, condition = "No content"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 404, condition = "Not Found"),
            @ResponseCode(code = 500, condition = "Internal Error") })
    public Response showNetwork(
            @PathParam("networkUUID") String networkUUID
            ) {
        IfOpenDoveNetworkCRU sbInterface = OpenDoveCRUDInterfaces.getIfDoveNetworkCRU(this);
        if (sbInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        if (!sbInterface.networkExists(networkUUID))
            return Response.status(404).build();
        return Response.status(200).entity(new OpenDoveNetworkRequest(sbInterface.getNetwork(networkUUID))).build();
    }

    @GET
    @Produces({ MediaType.APPLICATION_JSON })
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 204, condition = "No content"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 500, condition = "Internal Error") })
    public Response listNetworks() {
        IfOpenDoveNetworkCRU sbInterface = OpenDoveCRUDInterfaces.getIfDoveNetworkCRU(this);
        if (sbInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        return Response.status(200).entity(new OpenDoveNetworkRequest(sbInterface.getNetworks())).build();
    }

    @Path("{vnid}/subnets/{subnetUUID}")
    @GET
    @Produces({ MediaType.APPLICATION_JSON })
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 204, condition = "No content"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 404, condition = "Not Found"),
            @ResponseCode(code = 500, condition = "Internal Error") })
    public Response showNetwork(
            @PathParam("subnetUUID") String subnetUUID,
            @PathParam("vnid") String vnid
            ) {
        IfSBDoveSubnetCRUD sbSubnetInterface = OpenDoveCRUDInterfaces.getIfDoveSubnetCRUD(this);
        if (sbSubnetInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        IfOpenDoveNetworkCRU sbNetworkInterface = OpenDoveCRUDInterfaces.getIfDoveNetworkCRU(this);
        if (sbNetworkInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        if (!sbSubnetInterface.subnetExists(subnetUUID))
            return Response.status(404).build();
        if (!sbNetworkInterface.networkExistsByVnid(Integer.parseInt(vnid)))
            return Response.status(404).build();
        OpenDoveSubnet oDS = sbSubnetInterface.getSubnet(subnetUUID);
        OpenDoveNetwork oDN = sbNetworkInterface.getNetworkByVnid(Integer.parseInt(vnid));
        if (!oDS.getNetworkUUIDs().contains(oDN.getUUID()))
            return Response.status(404).build();
        return Response.status(200).entity(oDS).build();
    }

}

