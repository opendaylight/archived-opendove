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
import org.codehaus.enunciate.jaxrs.TypeHint;
import org.opendaylight.controller.northbound.commons.RestMessages;
import org.opendaylight.controller.northbound.commons.exception.ResourceNotFoundException;
import org.opendaylight.controller.northbound.commons.exception.ServiceUnavailableException;
import org.opendaylight.opendove.odmc.IfOpenDoveDomainCRUD;
import org.opendaylight.opendove.odmc.IfOpenDoveNetworkCRUD;
import org.opendaylight.opendove.odmc.IfSBDovePolicyCRUD;
import org.opendaylight.opendove.odmc.OpenDoveCRUDInterfaces;
import org.opendaylight.opendove.odmc.OpenDoveDomain;
import org.opendaylight.opendove.odmc.OpenDoveNetwork;
import org.opendaylight.opendove.odmc.OpenDovePolicy;
import org.opendaylight.opendove.odmc.rest.OpenDoveDomainRequest;

/**
 * Open DOVE Southbound REST APIs for Domains.<br>
 *
 * <br>
 * <br>
 * Authentication scheme [for now]: <b>HTTP Basic</b><br>
 * Authentication realm: <b>opendaylight</b><br>
 * Transport: <b>HTTP and HTTPS</b><br>
 * <br>
 * HTTPS Authentication is disabled by default. Administrator can enable it in
 * tomcat-server.xml after adding a proper keystore / SSL certificate from a
 * trusted authority.<br>
 * More info:
 * http://tomcat.apache.org/tomcat-7.0-doc/ssl-howto.html#Configuration
 *
 */

@Path("/domains")
public class OpenDoveDomainSouthbound {
    /**
     * Show domain by uuid
     *
     * @param domainUUID
     *            uuid of domain
     * @return domain Information
     *
     *         <pre>
     *
     * Example:
     *
     * Request URL:
     * http://localhost:8080/controller/sb/v2/opendove/odmc/domains/3f749e3e-a7cc-4e55-bb05-eb9cde963a0c
     *
     * Response body in JSON:
     * {
     *    "domain": [ {
     *       "is_tombstone": false,
     *       "change_version": 1,
     *       "create_version": 1,
     *       "id": "3f749e3e-a7cc-4e55-bb05-eb9cde963a0c",
     *       "name": "Neutron 14050e01-0e14-0000-0000-000000000001",
     *       "replication_factor": 2
     *    } ]
     * }
     * </pre>
     */
    @Path("/{domainUUID}")
    @GET
    @Produces({ MediaType.APPLICATION_JSON })
    @TypeHint(OpenDoveDomainRequest.class)
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 204, condition = "No content"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 404, condition = "Not Found"),
            @ResponseCode(code = 500, condition = "Internal Error") })
    public Response showDomain(
            @PathParam("domainUUID") String domainUUID
            ) {
        IfOpenDoveDomainCRUD sbInterface = OpenDoveCRUDInterfaces.getIfDoveDomainCRU(this);
        if (sbInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        if (!sbInterface.domainExists(domainUUID)) {
            throw new ResourceNotFoundException("Domain not found");
        }
        return Response.status(200).entity(new OpenDoveDomainRequest(sbInterface.getDomain(domainUUID))).build();
    }

    /**
     * Show network scoped by domain
     *
     * @param domainUUID
     *            uuid of domain
     * @param vnid
     *            virtual network ID of network
     * @return network Information
     *
     *         <pre>
     *
     * Example:
     *
     * Request URL:
     * http://127.0.0.1:8080/controller/sb/v2/opendove/odmc/domains/7c1a671d-f2fd-4c3e-ad7b-097263abc3ff/networks/11099999
     *
     * Response body in JSON:
     * {
     *   "is_tombstone": false,
     *   "change_version": 4,
     *   "create_version": 4,
     *   "id": "db256e1a-6601-4001-8ece-0ca2d1c75609",
     *   "network_id": 11099999,
     *   "name": "Neutron 0d051418-0f12-0b00-0000-000000000002",
     *   "domain_uuid": "7c1a671d-f2fd-4c3e-ad7b-097263abc3ff",
     *   "type": 0
     * }
     * </pre>
     */
    @Path("{domainUUID}/networks/{vnid}")
    @GET
    @Produces({ MediaType.APPLICATION_JSON })
    @TypeHint(OpenDoveNetwork.class)
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 204, condition = "No content"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 404, condition = "Not Found"),
            @ResponseCode(code = 500, condition = "Internal Error") })
    public Response showNetwork(
            @PathParam("domainUUID") String domainUUID,
            @PathParam("vnid") String vnid
            ) {
        IfOpenDoveDomainCRUD sbDomainInterface = OpenDoveCRUDInterfaces.getIfDoveDomainCRU(this);
        if (sbDomainInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        IfOpenDoveNetworkCRUD sbNetworkInterface = OpenDoveCRUDInterfaces.getIfDoveNetworkCRU(this);
        if (sbNetworkInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        if (!sbDomainInterface.domainExists(domainUUID)) {
            throw new ResourceNotFoundException("Domain not found");
        }
        if (!sbNetworkInterface.networkExistsByVnid(Integer.parseInt(vnid))) {
            throw new ResourceNotFoundException("Network not found");
        }
        OpenDoveNetwork oDN = sbNetworkInterface.getNetworkByVnid(Integer.parseInt(vnid));
        if (!domainUUID.equalsIgnoreCase(oDN.getDomain_uuid())) {
            throw new ResourceNotFoundException("network not scoped by domain");
        }
        return Response.status(200).entity(oDN).build();
    }

    /**
     * Show network scoped by domain (using its createVersion number)
     *
     * @param domainId
     *            createVersion # of domain
     * @param vnid
     *            virtual network ID of network
     * @return network Information
     *
     *         <pre>
     *
     * Example:
     *
     * Request URL:
     * http://127.0.0.1:8080/controller/sb/v2/opendove/odmc/domains/bynumber/1/networks/11099999
     *
     * Response body in JSON:
     * {
     *   "is_tombstone": false,
     *   "change_version": 4,
     *   "create_version": 4,
     *   "id": "db256e1a-6601-4001-8ece-0ca2d1c75609",
     *   "network_id": 11099999,
     *   "name": "Neutron 0d051418-0f12-0b00-0000-000000000002",
     *   "domain_uuid": "7c1a671d-f2fd-4c3e-ad7b-097263abc3ff",
     *   "type": 0
     * }
     * </pre>
     */
    @Path("/bynumber/{domainID}/networks/{vnid}")
    @GET
    @Produces({ MediaType.APPLICATION_JSON })
    @TypeHint(OpenDoveNetwork.class)
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 204, condition = "No content"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 404, condition = "Not Found"),
            @ResponseCode(code = 500, condition = "Internal Error") })
    public Response showNetworkGivenDomainByNumber(
            @PathParam("domainID") String domainID,
            @PathParam("vnid") String vnid
            ) {
        IfOpenDoveDomainCRUD sbDomainInterface = OpenDoveCRUDInterfaces.getIfDoveDomainCRU(this);
        if (sbDomainInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        IfOpenDoveNetworkCRUD sbNetworkInterface = OpenDoveCRUDInterfaces.getIfDoveNetworkCRU(this);
        if (sbNetworkInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        if (!sbDomainInterface.domainExistsByNumber(domainID)) {
            throw new ResourceNotFoundException("Domain not found");
        }
        if (!sbNetworkInterface.networkExistsByVnid(Integer.parseInt(vnid))) {
            throw new ResourceNotFoundException("Network not found");
        }
        OpenDoveDomain oDD = sbDomainInterface.getDomainByNumber(domainID);
        OpenDoveNetwork oDN = sbNetworkInterface.getNetworkByVnid(Integer.parseInt(vnid));
        if (!oDD.getUUID().equalsIgnoreCase(oDN.getDomain_uuid())) {
            throw new ResourceNotFoundException("Network not scoped by domain");
        }
        return Response.status(200).entity(oDN).build();
    }

    /**
     * Show policy scoped by domain
     *
     * @param domainUUID
     *            UUID of domain
     * @param policyUUID
     *            UUID of policy
     * @return policy Information
     *
     *         <pre>
     *
     * Example:
     *
     * Request URL:
     * http://127.0.0.1:8080/controller/sb/v2/opendove/odmc/domains/7c1a671d-f2fd-4c3e-ad7b-097263abc3ff/policy/b68088f9-ce4a-48fc-839e-b376f81aac70
     *
     * Response body in JSON:
     * {
     *   "is_tombstone": false,
     *   "change_version": 9,
     *   "create_version": 9,
     *   "id": "b68088f9-ce4a-48fc-839e-b376f81aac70",
     *   "type": 1,
     *   "src_network": 11099999,
     *   "dst_network": 9220207,
     *   "ttl": 1000,
     *   "action": 1,
     *   "domain_id": "7c1a671d-f2fd-4c3e-ad7b-097263abc3ff",
     *   "traffic_type": 0
     * }
     * </pre>
     */
    @Path("{domainUUID}/policy/{policyUUID}")
    @GET
    @Produces({ MediaType.APPLICATION_JSON })
    @TypeHint(OpenDovePolicy.class)
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 204, condition = "No content"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 404, condition = "Not Found"),
            @ResponseCode(code = 500, condition = "Internal Error") })
    public Response showPolicy(
            @PathParam("domainUUID") String domainUUID,
            @PathParam("policyUUID") String policyUUID
            ) {
        IfOpenDoveDomainCRUD sbDomainInterface = OpenDoveCRUDInterfaces.getIfDoveDomainCRU(this);
        if (sbDomainInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        IfSBDovePolicyCRUD sbPolicyInterface = OpenDoveCRUDInterfaces.getIfDovePolicyCRUD(this);
        if (sbPolicyInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        if (!sbDomainInterface.domainExists(domainUUID)) {
            throw new ResourceNotFoundException("Domain not found");
        }
        if (!sbPolicyInterface.policyExists(policyUUID)) {
            throw new ResourceNotFoundException("Policy not found");
        }
        OpenDovePolicy oDP = sbPolicyInterface.getPolicy(policyUUID);
        if (!domainUUID.equalsIgnoreCase(oDP.getDomainUUID())) {
            throw new ResourceNotFoundException("Policy not scoped by domain");
        }
        return Response.status(200).entity(oDP).build();
    }

    /**
     * List all domains
     *
     * @param input
     *            none
     * @return list of all domains
     *
     *         <pre>
     *
     * Example:
     *
     * Request URL:
     * http://localhost:8080/controller/sb/v2/opendove/odmc/domains
     *
     * Response body in JSON:
     * {
     *    "domains": [ {
     *       "is_tombstone": false,
     *       "change_version": 1,
     *       "create_version": 1,
     *       "id": "3f749e3e-a7cc-4e55-bb05-eb9cde963a0c",
     *       "name": "Neutron 14050e01-0e14-0000-0000-000000000001",
     *       "replication_factor": 2
     *    } ]
     * }
     * </pre>
     */
    @GET
    @Produces({ MediaType.APPLICATION_JSON })
    @TypeHint(OpenDoveDomainRequest.class)
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 204, condition = "No content"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 500, condition = "Internal Error") })
    public Response listDomains() {
        IfOpenDoveDomainCRUD sbInterface = OpenDoveCRUDInterfaces.getIfDoveDomainCRU(this);
        if (sbInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        return Response.status(200).entity(new OpenDoveDomainRequest(sbInterface.getDomains())).build();
    }

    /**
     * Show domain by createVersion number
     *
     * @param domainNumber
     *            createVersion number of domain
     * @return domain Information
     *
     *         <pre>
     *
     * Example:
     *
     * Request URL:
     * http://localhost:8080/controller/sb/v2/opendove/odmc/domains/bynumber/1
     *
     * Response body in JSON:
     * {
     *    "domain": [ {
     *       "is_tombstone": false,
     *       "change_version": 1,
     *       "create_version": 1,
     *       "id": "3f749e3e-a7cc-4e55-bb05-eb9cde963a0c",
     *       "name": "Neutron 14050e01-0e14-0000-0000-000000000001",
     *       "replication_factor": 2
     *    } ]
     * }
     * </pre>
     */
    @Path("/bynumber/{domainNumber}")
    @GET
    @Produces({ MediaType.APPLICATION_JSON })
    @TypeHint(OpenDoveDomainRequest.class)
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 204, condition = "No content"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 404, condition = "Not Found"),
            @ResponseCode(code = 500, condition = "Internal Error") })
    public Response showDomainByNumber(
            @PathParam("domainNumber") String domainID
            ) {
        IfOpenDoveDomainCRUD sbInterface = OpenDoveCRUDInterfaces.getIfDoveDomainCRU(this);
        if (sbInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        if (!sbInterface.domainExistsByNumber(domainID)) {
            throw new ResourceNotFoundException("Domain not found");
        }
        return Response.status(200).entity(new OpenDoveDomainRequest(sbInterface.getDomainByNumber(domainID))).build();
    }

    /**
     * Show policy scoped by domain (byNumber)
     *
     * @param domainID
     *            ID of domain
     * @param policyUUID
     *            UUID of policy
     * @return policy Information
     *
     *         <pre>
     *
     * Example:
     *
     * Request URL:
     * http://127.0.0.1:8080/controller/sb/v2/opendove/odmc/domains/bynumber/domainID/policy/b68088f9-ce4a-48fc-839e-b376f81aac70
     *
     * Response body in JSON:
     * {
     *   "is_tombstone": false,
     *   "change_version": 9,
     *   "create_version": 9,
     *   "id": "b68088f9-ce4a-48fc-839e-b376f81aac70",
     *   "type": 1,
     *   "src_network": 11099999,
     *   "dst_network": 9220207,
     *   "ttl": 1000,
     *   "action": 1,
     *   "domain_id": "7c1a671d-f2fd-4c3e-ad7b-097263abc3ff",
     *   "traffic_type": 0
     * }
     * </pre>
     */
    @Path("/bynumber/{domainID}/policy/{policyUUID}")
    @GET
    @Produces({ MediaType.APPLICATION_JSON })
    @TypeHint(OpenDovePolicy.class)
    @StatusCodes({
            @ResponseCode(code = 200, condition = "Operation successful"),
            @ResponseCode(code = 204, condition = "No content"),
            @ResponseCode(code = 401, condition = "Unauthorized"),
            @ResponseCode(code = 404, condition = "Not Found"),
            @ResponseCode(code = 500, condition = "Internal Error") })
    public Response showPolicyForDomainBynumber(
            @PathParam("domainID") String domainID,
            @PathParam("policyUUID") String policyUUID
            ) {
        IfOpenDoveDomainCRUD sbDomainInterface = OpenDoveCRUDInterfaces.getIfDoveDomainCRU(this);
        if (sbDomainInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        IfSBDovePolicyCRUD sbPolicyInterface = OpenDoveCRUDInterfaces.getIfDovePolicyCRUD(this);
        if (sbPolicyInterface == null) {
            throw new ServiceUnavailableException("OpenDove SB Interface "
                    + RestMessages.SERVICEUNAVAILABLE.toString());
        }
        if (!sbDomainInterface.domainExistsByNumber(domainID)) {
            throw new ResourceNotFoundException("Domain not found");
        }
        if (!sbPolicyInterface.policyExists(policyUUID)) {
            throw new ResourceNotFoundException("Policy not found");
        }
        OpenDovePolicy oDP = sbPolicyInterface.getPolicy(policyUUID);
        return Response.status(200).entity(oDP).build();
    }
}

