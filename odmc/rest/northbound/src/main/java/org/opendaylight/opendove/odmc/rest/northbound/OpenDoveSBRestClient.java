/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.opendove.odmc.rest.northbound;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.codehaus.jettison.json.JSONObject;
import org.opendaylight.controller.commons.httpclient.*;
import org.opendaylight.opendove.odmc.OpenDoveServiceAppliance;

/**
 * Open DOVE REST Client Interface Class for  Service Appliances (DCS and DGW etc.).<br>
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

public class OpenDoveSBRestClient {

    public OpenDoveSBRestClient () {
    }

    /*
     *  REST Client Method for "DCS service-appliance Role Assignment"
     */

    public Integer assignDcsServiceApplianceRole(OpenDoveServiceAppliance appliance) {

        String  dsaIP   = appliance.getIP();
        Integer dcs_rest_service_port = appliance.getDcsRestServicePort();


        // Test PUT subnet2
        try {
            String action = "start";
            JSONObject jo = new JSONObject().put("action", action);

            // execute HTTP request and verify response code
            String uri = "http://" + dsaIP + ":" + dcs_rest_service_port + "/controller/sb/v2/opendove/odcs/role";
            HTTPRequest request = new HTTPRequest();
            request.setMethod("PUT");
            request.setUri(uri);
            request.setEntity(jo.toString());

            Map<String, List<String>> headers = new HashMap<String, List<String>>();
//            String authString = "admin:admin";
//           byte[] authEncBytes = Base64.encodeBase64(authString.getBytes());
//            String authStringEnc = new String(authEncBytes);
            List<String> header = new ArrayList<String>();
//            header.add("Basic "+authStringEnc);
//            headers.put("Authorization", header);
//            header = new ArrayList<String>();
            header.add("application/json");
            headers.put("Content-Type", header);
            headers.put("Accept", header);
            request.setHeaders(headers);
            HTTPResponse response = HTTPClient.sendRequest(request);
            return response.getStatus();
        } catch (Exception e) {
            return 400;
        }
    }
}

