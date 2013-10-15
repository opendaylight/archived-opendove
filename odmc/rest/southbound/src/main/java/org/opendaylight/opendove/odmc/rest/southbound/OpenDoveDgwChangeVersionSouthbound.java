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
import org.opendaylight.opendove.odmc.IfSBOpenDoveChangeVersionR;
import org.opendaylight.opendove.odmc.OpenDoveCRUDInterfaces;

/**
 * Open DOVE Southbound REST APIs for DGW Change Version.<br>
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

@Path("/odgw/changeversion")
public class OpenDoveDgwChangeVersionSouthbound {

    @Path("/{changeVersion}")
    @GET
    @Produces({ MediaType.APPLICATION_JSON })
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 204, condition = "No content") })
    public Response getOdcsChange(
            @PathParam("changeVersion") String changeVersion
            ) {
        IfSBOpenDoveChangeVersionR sbInterface = OpenDoveCRUDInterfaces.getIfSBOpenDoveChangeVersionR(this);

        System.out.println("********************** Inside getOdgwChange  ^^^^^^^^ ");
        if (sbInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        int i_changeVersion;
        try {
            i_changeVersion = Integer.parseInt(changeVersion);
        } catch (Exception e) {
            return Response.status(500).build();
        }
        if (sbInterface.versionExists(i_changeVersion) == 204 )
            return Response.status(204).build();
        return Response.status(200).entity(sbInterface.getNextOdcsChange(i_changeVersion)).build();
    }

}

