/*
 * Copyright IBM Corporation, 2013.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

package org.opendaylight.opendove.odmc.rest;

import java.io.StringReader;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Iterator;

import javax.xml.bind.JAXBContext;
import javax.xml.bind.Unmarshaller;
import javax.xml.transform.stream.StreamSource;

import org.codehaus.jettison.json.JSONException;
import org.codehaus.jettison.json.JSONObject;
import org.codehaus.jettison.json.JSONArray;
import org.codehaus.jettison.json.JSONTokener;

import org.opendaylight.controller.commons.httpclient.HTTPClient;
import org.opendaylight.controller.commons.httpclient.HTTPRequest;
import org.opendaylight.controller.commons.httpclient.HTTPResponse;
import org.opendaylight.controller.northbound.commons.exception.BadRequestException;
import org.opendaylight.controller.northbound.commons.exception.InternalServerErrorException;
import org.opendaylight.opendove.odmc.OpenDoveDomain;
import org.opendaylight.opendove.odmc.OpenDoveServiceAppliance;
import org.opendaylight.opendove.odmc.IfOpenDoveServiceApplianceCRUD;
import org.opendaylight.opendove.odmc.IfOpenDoveDomainCRUD;
import org.eclipse.persistence.jaxb.UnmarshallerProperties;

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

public class OpenDoveRestClient {
    IfOpenDoveServiceApplianceCRUD sbDSAInterface;
    IfOpenDoveDomainCRUD           sbDomainInterface;

    public OpenDoveRestClient () {
    }

    public OpenDoveRestClient(IfOpenDoveServiceApplianceCRUD iface) {
        sbDSAInterface = iface;
    }

    public OpenDoveRestClient(IfOpenDoveServiceApplianceCRUD iface1, IfOpenDoveDomainCRUD iface2) {
        sbDSAInterface = iface1;
        sbDomainInterface = iface2;
    }

    /*
     *  REST Client Methods for "DCS service-appliance Role Assignment"
     */

    public Integer assignDcsServiceApplianceRole(OpenDoveServiceAppliance appliance) {
        JSONObject jo;
        try {
            jo = new JSONObject().put("action", "start");
            return sendJSONObject(jo, appliance);
        } catch (JSONException e) {
            throw new InternalServerErrorException("Could not set up JSON object for southbound communication");
        }
    }

    public Integer unassignDcsServiceApplianceRole(OpenDoveServiceAppliance appliance) {
        JSONObject jo;
        try {
            jo = new JSONObject().put("action", "stop");
            return sendJSONObject(jo, appliance);
        } catch (JSONException e) {
            throw new InternalServerErrorException("Could not set up JSON object for southbound communication");
        }
    }

    public Integer sendJSONObject(JSONObject jo, OpenDoveServiceAppliance appliance) {
        String  dsaIP   = appliance.getIP();
        Integer dcs_rest_service_port = appliance.getDcsRestServicePort();


        try {
            // execute HTTP request and verify response code
            String uri = "http://" + dsaIP + ":" + dcs_rest_service_port + "/controller/sb/v2/opendove/odcs/role";
            HTTPRequest request = new HTTPRequest();
            request.setMethod("PUT");
            request.setUri(uri);
            request.setEntity(jo.toString());

            Map<String, List<String>> headers = new HashMap<String, List<String>>();
            //  String authString = "admin:admin";
            //  byte[] authEncBytes = Base64.encodeBase64(authString.getBytes());
            //  String authStringEnc = new String(authEncBytes);
            List<String> header = new ArrayList<String>();
            //  header.add("Basic "+authStringEnc);
            //  headers.put("Authorization", header);
            //  header = new ArrayList<String>();
            header.add("application/json");
            headers.put("Content-Type", header);
            headers.put("Accept", header);
            request.setHeaders(headers);
            HTTPResponse response = HTTPClient.sendRequest(request);
            return response.getStatus();
        } catch (Exception e) {
            throw new BadRequestException("Could not complete SB request");
        }
    }
    /*
     *  REST Client Method for Providing DCS Cluster Nodes Details to All DCS Nodes that are
     *  in Role Assigned State
     *  Return:
     *    1: for Success, if o-DMC receives HTTP_OK(200) from all the Nodes
     *   -1: failure for All other cases.
     */

    public Integer sendDcsClusterInfo() {

        Integer dcs_rest_service_port;
        Integer dcs_raw_service_port;
        String  uuid;
        Integer ip_family;
        String  dsaIP;

        Integer  retVal = 1; // Success

        List<OpenDoveServiceAppliance> oDCSs = sbDSAInterface.getRoleAssignedDcsAppliances();

        Iterator<OpenDoveServiceAppliance> iterator = oDCSs.iterator();

        JSONArray dcs_list = new JSONArray();
        while (iterator.hasNext()) {
            OpenDoveServiceAppliance appliance =  iterator.next();
            ip_family = appliance.getIPFamily ();
            dsaIP     = appliance.getIP();
            uuid      = appliance.getUUID();
            dcs_rest_service_port = appliance.getDcsRestServicePort();
            dcs_raw_service_port = appliance.getDcsRawServicePort();

            JSONObject dcs = new JSONObject();
            try {
                  dcs.put("uuid", uuid);
                  dcs.put("ip_family", ip_family);
                  dcs.put("ip", dsaIP);
                  dcs.put("dcs_rest_service_port", dcs_rest_service_port);
                  dcs.put("dcs_raw_service_port",  dcs_raw_service_port);
            } catch ( Exception e ) {
               retVal = -1;
               return retVal;
            }
            dcs_list.put(dcs);
        }

        JSONObject jo;
        try {
              jo = new JSONObject().put("dps", dcs_list);
        } catch ( Exception e ) {
              retVal = -1;
              return retVal;
        }

        iterator = oDCSs.iterator();

        while (iterator.hasNext()) {
             try {
                 OpenDoveServiceAppliance appliance =  iterator.next();
                 dsaIP     = appliance.getIP();
                 dcs_rest_service_port = appliance.getDcsRestServicePort();
                 // execute HTTP request and verify response code
                 String uri = "http://" + dsaIP + ":" + dcs_rest_service_port + "/controller/sb/v2/opendove/odcs/cluster";
                 HTTPRequest request = new HTTPRequest();
                 request.setMethod("PUT");
                 request.setUri(uri);
                 request.setEntity(jo.toString());

                 Map<String, List<String>> headers = new HashMap<String, List<String>>();
                 // String authString = "admin:admin";
                 // byte[] authEncBytes = Base64.encodeBase64(authString.getBytes());
                 // String authStringEnc = new String(authEncBytes);
                 List<String> header = new ArrayList<String>();
                 // header.add("Basic "+authStringEnc);
                 // headers.put("Authorization", header);
                 // header = new ArrayList<String>();
                 header.add("application/json");
                 headers.put("Content-Type", header);
                 headers.put("Accept", header);
                 request.setHeaders(headers);
                 HTTPResponse response = HTTPClient.sendRequest(request);

                 if ( (response.getStatus() != 200)  || (response.getStatus() != 204)) {
                     retVal = -1;
                 }
             } catch (Exception e) {
                 retVal =  -1;
                 return retVal;
             }
        } // end of while.
        return retVal;
    }
    /*
     *  REST Client Method for Providing DCS List for a Given Domain
     *  Return:
     */

    public List<OpenDoveServiceAppliance>  getDomainDCSList(String domainUUID) {

        List<OpenDoveServiceAppliance> answer = new ArrayList<OpenDoveServiceAppliance>();
        Integer dcs_rest_service_port = 0;
        String  dsaIP = "";

        /*
         * Any DCS Node that has the Role Assigned flag set, will be able to answer this.
         * Just pick up one Node and Send the REST Request.
         * Sample Output from DCS
         * {
         *   dcslist:
         *   [
         *       {
         *          ip_family                : INTEGER (ex: 2)
         *          ip                       : STRING  (ex: '1.2.3.4'
         *          uuid                     : STRING  (ex: "aa-bb-ccccc")
         *          dcs_rest_service_port    : INTEGER (ex: 1888)
         *          dcs_raw_service_port     : INTEGER (ex: 902)
         *      }
         *   ]
         *  }
         */

        /* Get Domain_id for domain_uuid  */
        OpenDoveDomain domain = sbDomainInterface.getDomain(domainUUID);
        if (domain == null) {
            return answer;
        }
        Integer domain_id = domain.getDomainId();
        List<OpenDoveServiceAppliance> oDCSs = sbDSAInterface.getRoleAssignedDcsAppliances();

        Iterator<OpenDoveServiceAppliance> iterator = oDCSs.iterator();

        while (iterator.hasNext()) {
            OpenDoveServiceAppliance appliance =  iterator.next();
            dsaIP     = appliance.getIP();
            dcs_rest_service_port = appliance.getDcsRestServicePort();

            if ( dsaIP != null && dcs_rest_service_port != 0 ) {
               break;
            }
        }
        try {
             // execute HTTP request and verify response code
             //String uri = "http://" + dsaIP + ":" + dcs_rest_service_port + "/controller/sb/v2/opendove/odmc/odcs/odcslist?domain=" + domain_id;
             String uri = "http://" + dsaIP + ":" + dcs_rest_service_port + "/controller/sb/v2/opendove/odmc/odcs/domains/bynumber/" + domain_id + "/dcslist";
             HTTPRequest request = new HTTPRequest();
             request.setMethod("GET");
             request.setUri(uri);
             System.out.println("4. *********** REST_CLIENT show-dcs list URI" + uri);
             Map<String, List<String>> headers = new HashMap<String, List<String>>();
             // String authString = "admin:admin";
             // byte[] authEncBytes = Base64.encodeBase64(authString.getBytes());
             // String authStringEnc = new String(authEncBytes);
             List<String> header = new ArrayList<String>();
             // header.add("Basic "+authStringEnc);
             // headers.put("Authorization", header);
             // header = new ArrayList<String>();
             //header.add("application/json");
             headers.put("Content-Type", header);
             headers.put("Accept", header);
             request.setHeaders(headers);
             HTTPResponse response = HTTPClient.sendRequest(request);
             String dcsJsonList = response.getEntity();
             JSONTokener jt = new JSONTokener(dcsJsonList);
             JSONObject  json = new JSONObject(jt);
             JSONArray arr = json.getJSONArray("dcslist");
             for (int i = 0; i < arr.length(); i++)
             {
                OpenDoveServiceAppliance dcs  = new OpenDoveServiceAppliance();

                Integer ip_family             = arr.getJSONObject(i).getInt("ip_family");
                String ip                     = arr.getJSONObject(i).getString("ip");
                String uuid                   = arr.getJSONObject(i).getString("uuid");
                dcs_rest_service_port         = arr.getJSONObject(i).getInt("dcs_rest_service_port");
                Integer dcs_raw_service_port  = arr.getJSONObject(i).getInt("dcs_raw_service_port");
                dcs.setIPFamily(ip_family);
                dcs.setIP(ip);
                dcs.setUUID(uuid);
                dcs.setDcsRestServicePort(dcs_rest_service_port);
                dcs.setDcsRawServicePort(dcs_raw_service_port);
                answer.add(dcs);
             }
        } catch (Exception e) {
             return answer;
        }
        return answer;
    }
    /*
     *  REST Client Method for "DGW service-appliance Role Assignment"
     */

    public Integer assignDgwServiceApplianceRole(OpenDoveServiceAppliance appliance) {


        String  dsaIP   = appliance.getIP();
        Integer dgw_rest_service_port = appliance.getDgwRestServicePort();


        try {
            String action = "start";
            JSONObject jo = new JSONObject().put("action", action);

            // execute HTTP request and verify response code
            String uri = "http://" + dsaIP + ":" + dgw_rest_service_port + "/controller/sb/v2/opendove/odgw/role";
            HTTPRequest request = new HTTPRequest();
            request.setMethod("PUT");
            request.setUri(uri);
            request.setEntity(jo.toString());

            Map<String, List<String>> headers = new HashMap<String, List<String>>();
            //  String authString = "admin:admin";
            //  byte[] authEncBytes = Base64.encodeBase64(authString.getBytes());
            //  String authStringEnc = new String(authEncBytes);
            List<String> header = new ArrayList<String>();
            //  header.add("Basic "+authStringEnc);
            //  headers.put("Authorization", header);
            //  header = new ArrayList<String>();
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

    public OpenDoveSwitchStatsRequest getSwitchStats(String queryIPAddr, String queryVNID,
            String queryMAC) {
        HTTPRequest request = new HTTPRequest();
        StringBuilder uri = new StringBuilder();
        uri.append("http://");
        uri.append(queryIPAddr);
        uri.append(":1999/oVS/stats?vnid=");
        uri.append(queryVNID);
        if (queryMAC != null) {
            uri.append("&mac=");
            uri.append(queryMAC);
        }
        request.setUri(uri.toString());
        request.setMethod("GET");
        Map<String, List<String>> headers = new HashMap<String, List<String>>();
        //  String authString = "admin:admin";
        //  byte[] authEncBytes = Base64.encodeBase64(authString.getBytes());
        //  String authStringEnc = new String(authEncBytes);
        List<String> header = new ArrayList<String>();
        //  header.add("Basic "+authStringEnc);
        //  headers.put("Authorization", header);
        //  header = new ArrayList<String>();
        header.add("application/json");
        headers.put("Accept", header);
        request.setHeaders(headers);
        try {
            HTTPResponse response = HTTPClient.sendRequest(request);
            if (response.getStatus() < 200 && response.getStatus() > 299) {
                return null;
            }
            JAXBContext jc = JAXBContext.newInstance(OpenDoveSwitchStatsRequest.class);
            Unmarshaller unmarshaller = jc.createUnmarshaller();
            unmarshaller.setProperty(UnmarshallerProperties.MEDIA_TYPE, "application/json");
            unmarshaller.setProperty(UnmarshallerProperties.JSON_INCLUDE_ROOT, false);
            OpenDoveSwitchStatsRequest stats = (OpenDoveSwitchStatsRequest) unmarshaller.unmarshal(new StreamSource(new StringReader( response.getEntity())));
            return stats;
        } catch (Exception e) {
            return null;
        }
    }

    public Integer deleteSwitchStats(String queryIPAddr, String queryVNID,
            String queryMAC) {
        HTTPRequest request = new HTTPRequest();
        StringBuilder uri = new StringBuilder();
        uri.append("http://");
        uri.append(queryIPAddr);
        uri.append(":1999/oVS/stats?vnid=");
        uri.append(queryVNID);
        if (queryMAC != null) {
            uri.append("&mac=");
            uri.append(queryMAC);
        }
        request.setUri(uri.toString());
        request.setMethod("DELETE");
        Map<String, List<String>> headers = new HashMap<String, List<String>>();
        //  String authString = "admin:admin";
        //  byte[] authEncBytes = Base64.encodeBase64(authString.getBytes());
        //  String authStringEnc = new String(authEncBytes);
        // List<String> header = new ArrayList<String>();
        //  header.add("Basic "+authStringEnc);
        //  headers.put("Authorization", header);
        //  header = new ArrayList<String>();
        request.setHeaders(headers);
        try {
            HTTPResponse response = HTTPClient.sendRequest(request);
            if (response != null) {
                return response.getStatus();
            }
            return 404;  // nothing found
        }  catch (Exception e) {
            return 504;
        }
    }

    /*
     *  REST Client Method for "DGW All stats"
     */

    public OpenDoveGWStats getDgwAllStats(OpenDoveServiceAppliance appliance, String odgwUUID)
    {

        String  dsaIP   = appliance.getIP();
        Integer dgw_rest_service_port = appliance.getDgwRestServicePort();

            //String action = "start";
            //JSONObject jo = new JSONObject().put("action", action);

            // execute HTTP request and verify response code
            String uri = "http://" + dsaIP + ":" + dgw_rest_service_port + "/controller/sb/v2/opendove/" + odgwUUID + "/allstats";
            HTTPRequest request = new HTTPRequest();
            request.setMethod("GET");
            request.setUri(uri);
            //    request.setEntity(jo.toString());

            Map<String, List<String>> headers = new HashMap<String, List<String>>();
            //  String authString = "admin:admin";
            //  byte[] authEncBytes = Base64.encodeBase64(authString.getBytes());
            //  String authStringEnc = new String(authEncBytes);
            List<String> header = new ArrayList<String>();
            //  header.add("Basic "+authStringEnc);
            //  headers.put("Authorization", header);
            //  header = new ArrayList<String>();

            //header.add("application/json");

            headers.put("Content-Type", header);
            headers.put("Accept", header);
            request.setHeaders(headers);

          try {
              HTTPResponse response = HTTPClient.sendRequest(request);
              if (response.getStatus() < 200 && response.getStatus() > 299) {
                return null;
            }
              JAXBContext jc = JAXBContext.newInstance(OpenDoveGWStats.class);
              Unmarshaller unmarshaller = jc.createUnmarshaller();
              unmarshaller.setProperty(UnmarshallerProperties.MEDIA_TYPE, "application/json");
              unmarshaller.setProperty(UnmarshallerProperties.JSON_INCLUDE_ROOT, false);
              OpenDoveGWStats stats = (OpenDoveGWStats) unmarshaller.unmarshal(new StreamSource(new StringReader( response.getEntity())));
              return stats;
          } catch (Exception e) {
              return null;
          }

    }


    /*
         *    REST Client Method for "DGW Session stats"
        */

    public OpenDoveGWSessionStatsRequest getDgwSessionStats(OpenDoveServiceAppliance appliance, String odgwUUID)
    {

        String  dsaIP = appliance.getIP();
        Integer dgw_rest_service_port = appliance.getDgwRestServicePort();

        //String action = "start";
        //JSONObject jo = new JSONObject().put("action", action);

       // execute HTTP request and verify response code
       String uri = "http://" + dsaIP + ":" + dgw_rest_service_port + "/controller/sb/v2/opendove/" + odgwUUID + "/session_stats";
       HTTPRequest request = new HTTPRequest();
       request.setMethod("GET");
       request.setUri(uri);
       //  request.setEntity(jo.toString());

       Map<String, List<String>> headers = new HashMap<String, List<String>>();
      //String authString = "admin:admin";
      //byte[] authEncBytes = Base64.encodeBase64(authString.getBytes());
      //String authStringEnc = new String(authEncBytes);
       List<String> header = new ArrayList<String>();
      //header.add("Basic "+authStringEnc);
      //headers.put("Authorization", header);
      //header = new ArrayList<String>();

     //header.add("application/json");

       headers.put("Content-Type", header);
       headers.put("Accept", header);
       request.setHeaders(headers);

       try {
          HTTPResponse response = HTTPClient.sendRequest(request);
          if (response.getStatus() < 200 && response.getStatus() > 299) {
            return null;
        }
          JAXBContext jc = JAXBContext.newInstance(OpenDoveGWSessionStatsRequest.class);
          Unmarshaller unmarshaller = jc.createUnmarshaller();
          unmarshaller.setProperty(UnmarshallerProperties.MEDIA_TYPE, "application/json");
          unmarshaller.setProperty(UnmarshallerProperties.JSON_INCLUDE_ROOT, false);
          OpenDoveGWSessionStatsRequest stats = (OpenDoveGWSessionStatsRequest) unmarshaller.unmarshal(new StreamSource(new StringReader( response.getEntity())));
             return stats;
       } catch (Exception e) {
            return null;
       }

}


/*
 *  REST Client Method for "DGW Session stats"
 */
//* http://localhost:8080/controller/nb/v2/opendove/odgw/uuid/vnid_stats/vnid
//    @Path("/{odgwUUID}/vnid_stats/{vnid}")

public OpenDoveVNIDStats getDgwVNIDStats(OpenDoveServiceAppliance appliance, String odgwUUID, String queryVNID)
{
    String  dsaIP = appliance.getIP();
    Integer dgw_rest_service_port = appliance.getDgwRestServicePort();

    //String action = "start";
    //JSONObject jo = new JSONObject().put("action", action);
    // execute HTTP request and verify response code
    String uri = "http://" + dsaIP + ":" + dgw_rest_service_port + "/controller/sb/v2/opendove/" + odgwUUID +
                "/vnid_stats/" + queryVNID;

    HTTPRequest request = new HTTPRequest();
    request.setMethod("GET");
    request.setUri(uri);
    //  request.setEntity(jo.toString());

    Map<String, List<String>> headers = new HashMap<String, List<String>>();
    //String authString = "admin:admin";
    //byte[] authEncBytes = Base64.encodeBase64(authString.getBytes());
    //String authStringEnc = new String(authEncBytes);
    List<String> header = new ArrayList<String>();
    //header.add("Basic "+authStringEnc);
    //headers.put("Authorization", header);
    //header = new ArrayList<String>();

    //header.add("application/json");

    headers.put("Content-Type", header);
    headers.put("Accept", header);
    request.setHeaders(headers);

    try {
       HTTPResponse response = HTTPClient.sendRequest(request);
       if (response.getStatus() < 200 && response.getStatus() > 299) {
        return null;
    }
       JAXBContext jc = JAXBContext.newInstance(OpenDoveVNIDStats.class);
       Unmarshaller unmarshaller = jc.createUnmarshaller();
       unmarshaller.setProperty(UnmarshallerProperties.MEDIA_TYPE, "application/json");
       unmarshaller.setProperty(UnmarshallerProperties.JSON_INCLUDE_ROOT, false);
       OpenDoveVNIDStats stats = (OpenDoveVNIDStats) unmarshaller.unmarshal(new StreamSource(new StringReader( response.getEntity())));
       return stats;
     } catch (Exception e) {
         return null;
    }

}


}

