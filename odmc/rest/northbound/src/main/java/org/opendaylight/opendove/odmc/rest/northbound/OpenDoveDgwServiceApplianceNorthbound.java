/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.opendove.odmc.rest.northbound;

import javax.ws.rs.GET;
import javax.ws.rs.PUT;
import javax.ws.rs.POST;
import javax.ws.rs.Path;
import javax.ws.rs.PathParam;
import javax.ws.rs.Produces;
import javax.ws.rs.core.MediaType;
import javax.ws.rs.core.Response;

import org.codehaus.enunciate.jaxrs.ResponseCode;
import org.codehaus.enunciate.jaxrs.StatusCodes;
import org.opendaylight.controller.northbound.commons.RestMessages;
import org.opendaylight.controller.northbound.commons.exception.ServiceUnavailableException;
import org.opendaylight.opendove.odmc.IfOpenDoveServiceApplianceCRU;
import org.opendaylight.opendove.odmc.OpenDoveCRUDInterfaces;
import org.opendaylight.opendove.odmc.rest.northbound.OpenDoveSBRestClient;
import org.opendaylight.opendove.odmc.OpenDoveServiceAppliance;

/**
 * Open DOVE Northbound REST APIs for DGW Service Appliance.<br>
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

@Path("/odgw")
public class OpenDoveDgwServiceApplianceNorthbound {

    /*
     *  NB REST(PUT) Handler Function for "DGW service-appliance Role Assignment"
     */

    @Path("/role/{saUUID}")
    @PUT
    @Produces({ MediaType.APPLICATION_JSON })
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 204, condition = "No content"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 409, condition = "DCS is in a Conflicted State"), 
            @ResponseCode(code = 404, condition = "Not Found"),
            @ResponseCode(code = 500, condition = "Internal Error") 
            })
    public Response nbAssignDgwServiceApplianceRole(
            @PathParam("saUUID") String dsaUUID
            ) {
        IfOpenDoveServiceApplianceCRU sbInterface = OpenDoveCRUDInterfaces.getIfDoveServiceApplianceCRU(this);
        if (sbInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        if (!sbInterface.applianceExists(dsaUUID))
            return Response.status(404).build();

        OpenDoveServiceAppliance dcsAppliance = sbInterface.getDoveServiceAppliance(dsaUUID);

        OpenDoveSBRestClient sbRestClient =    new OpenDoveSBRestClient();
        sbRestClient.assignDcsServiceApplianceRole(dcsAppliance);

        return Response.status(200).entity(new OpenDoveServiceApplianceRequest(sbInterface.getDoveServiceAppliance(dsaUUID))).build();
    }

}

