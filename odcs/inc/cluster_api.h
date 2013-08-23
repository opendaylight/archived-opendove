/*
 *  Copyright (c) IBM, Inc.  2011 -
 *  All rights reserved
 *
 *  Header File:
 *      cluster_api.h
 *
 *  Abstract:
 *      This header file defines the APIs that are needed from the Spidercast
 *      module.
 *
 *  Author:
 *      DOVE Development Team
 *
 *  Environment:
 *      User World
 *
 *  Revision History:
 *
 */

#ifndef _CLUSTER_DOMAIN_MAPPING_API_
#define _CLUSTER_DOMAIN_MAPPING_API_

/**
 * \ingroup DPSClusterDBInterface
 * @{
 */

/**
 * \brief The Log Level of the Cluster Database
 */
extern int PythonClusterDataLogLevel;

/**
 * \brief Maximum no. of DPS nodes in the cluster
 */
#define MAX_NODES_IN_CLUSTER 20

/**
 * \brief Maximum no of DCS node in stack
 */
#define MAX_NODES_ON_STACK 8

/**
 * \brief Minimum Replication Factor
 */
#define MIN_REPLICATION_FACTOR 2

/**
 * \brief Shared Domain Replication Factor
 */

#define SHARED_REPLICATION_FACTOR 3

/**
 * \brief Maximum Replication Factor (< MAX_NODES_ON_STACK)
 */
#define MAX_REPLICATION_FACTOR 4

/**
 * \brief Maximum no of DCS nodes per domain
 */
#define MAX_NODES_PER_DOMAIN MAX_REPLICATION_FACTOR

/**
 * \brief Domain Id used internally by DCS for shared addr space
 */
#define SHARED_ADDR_SPACE_DOMAIN_ID 0

/**
 * \brief Domain Id used internally by DCS for shared addr space
 */
#define SHARED_ADDR_SPACE_DOMAIN_NAME "domain-shared-addr-space"

/**
 * \brief Domain Id used internally by DCS for shared addr space
 */
#define SHARED_ADDR_SPACE_VNID 0

/**
 * \brief Domain Id used internally by DCS for shared addr space
 */
#define SHARED_ADDR_SPACE_VN_NAME "vn-shared-addr-space"

/*
 ******************************************************************************
 * python_init_cluster_db_interface --                                    *//**
 *
 * \brief This routine initializes the DPS Cluster Database interface to PYTHON
 *        OBJECTS
 *
 * \param pythonpath - Pointer to the PYTHON Path
 *
 * \return dove_status
 *
 *****************************************************************************/

dove_status python_init_cluster_db_interface(char *pythonpath);

/*
 ******************************************************************************
 * dps_get_domain_node_mapping                                            *//**
 *
 * \brief - This function will provide a list of DPS nodes handling a particular
 *          DOVE Domain. This routine will be called by modules such as the
 *          Replicator.
 *
 * \param domain_id        DOVE Domain ID
 * \param node_list_count  The number of allocated elements in the node_list
 * \param node_list        The list of IP Address to be filled by the Cluster
 *                         module. The calling module must allocate this
 *                         structure
 * \param node_list_needed This will be number of IP Address/Node that handle
 *                         that domain. This will be filled in by the Cluster
 *                         Module
 *
 * \retval DOVE_STATUS_OK node_list filled with node_list_needed entries
 * \retval DOVE_STATUS_INVALID_DOMAIN Invalid Domain
 * \retval DOVE_STATUS_RETRY Caller should retry with node_list_needed
 *                                 entries in node_list
 *
 ******************************************************************************
 */

dove_status dps_get_domain_node_mapping(uint32_t domain_id,
                                              uint32_t node_list_count,
                                              ip_addr_t *node_list,
                                              uint32_t *node_list_needed);

/*
 ******************************************************************************
 * dps_get_domain_random_remote_node                                      *//**
 *
 * \brief - This function will return a random node that handles a particular
 *          domain
 *
 * \param domain_id        DOVE Domain ID
 * \param node             A DPS Node that handles that domain. Caller
 *                         allocated structure
 *
 * \retval DOVE_STATUS_OK node will contain a valid entry
 * \retval DOVE_STATUS_INVALID_DOMAIN Invalid Domain
 * \retval DOVE_STATUS_EMPTY No DPS Node handles this domain
 *
 ******************************************************************************
 */

dove_status dps_get_domain_random_remote_node(uint32_t domain_id,
                                              ip_addr_t *node);

/*
 ******************************************************************************
 * dps_get_all_cluster_nodes                                            *//**
 *
 * \brief - This function will provide a list of all DPS nodes in the cluster.
 *
 * \param node_list_count  The number of allocated elements in the node_list
 * \param node_list        The list of IP Address to be filled by the Cluster
 *                         module. The calling module must allocate this
 *                         structure
 * \param node_list_needed This will be number of IP Address/Node that handle
 *                         that domain. This will be filled in by the Cluster
 *                         Module
 *
 * \retval DOVE_STATUS_OK node_list filled with node_list_needed entries
 * \retval DOVE_STATUS_RETRY Caller should retry with node_list_needed
 *                                 entries in node_list
 *
 ******************************************************************************
 */

dove_status dps_get_all_cluster_nodes(uint32_t node_list_count,
                                              ip_addr_t *node_list,
                                              uint32_t *node_list_needed);

/*
 ******************************************************************************
 * dps_get_lowest_loaded_nodes                                            *//**
 *
 * \brief - This function will provide a list of all DPS nodes in the cluster.
 *          This function will be used to communicate local domain mapping to
 *          all the nodes in the cluster.
 *
 * \param num_nodes  The number of lowest loaded nodes needed
 * \param nodes      The list of IP Address to be filled by the this routine.
 *                   The calling module must allocate this structure with at
 *                   least num_nodes entries
 * \param num_nodes_returned The number of nodes actually returned
 *
 * \retval DOVE_STATUS_OK
 * \retval DOVE_STATUS_EXCEEDS_CAP number of nodes actually returned is less
 *                                 than number of nodes needed.
 * \retval DOVE_STATUS_NO_MEMORY No memory
 *
 ******************************************************************************
 */
dove_status dps_get_lowest_loaded_nodes(uint32_t num_nodes,
                                        ip_addr_t *nodes,
                                        uint32_t *num_nodes_returned);

/*
 ******************************************************************************
 * dps_get_lowest_loaded_nodes_not_handling                                            *//**
 *
 * \brief - This function will return a list of least loaded nodes not handling
 * 			a particular domain.
 *
 * \param domain	 Domain Id.
 * \param num_nodes  The number of lowest loaded nodes needed
 * \param nodes      The list of IP Address to be filled by the this routine.
 *                   The calling module must allocate this structure with at
 *                   least num_nodes entries
 * \param num_nodes_returned The number of nodes actually returned
 *
 * \retval DOVE_STATUS_OK
 * \retval DOVE_STATUS_EXCEEDS_CAP number of nodes actually returned is less
 *                                 than number of nodes needed.
 * \retval DOVE_STATUS_NO_MEMORY No memory
 *
 ******************************************************************************
 */
dove_status dps_get_lowest_loaded_nodes_not_handling(uint32_t domain,uint32_t num_nodes,
                                                     ip_addr_t *nodes,
                                                     uint32_t *num_nodes_returned);

/*
 ******************************************************************************
 * dps_get_highest_loaded_nodes_hosting                                            *//**
 *
 * \brief - This function will return a list of heavily loaded nodes handling
 *     	    a particular domain.
 *
 * \param domain	 Domain Id.
 * \param num_nodes  The number of lowest loaded nodes needed
 * \param nodes      The list of IP Address to be filled by the this routine.
 *                   The calling module must allocate this structure with at
 *                   least num_nodes entries
 * \param num_nodes_returned The number of nodes actually returned
 *
 * \retval DOVE_STATUS_OK
 * \retval DOVE_STATUS_EXCEEDS_CAP number of nodes actually returned is less
 *                                 than number of nodes needed.
 * \retval DOVE_STATUS_NO_MEMORY No memory
 *
 ******************************************************************************
 */
dove_status dps_get_highest_loaded_nodes_hosting(uint32_t domain,
                                                     uint32_t num_nodes,
                                                     ip_addr_t *nodes,
                                                     uint32_t *num_nodes_returned);

/*
 ******************************************************************************
 * dps_cluster_node_add                                                   *//**
 *
 * \brief - This function can be used to add a Node to Cluster. Determine if
 *          this function needs to exist with Spidercast or will Spidercast
 *          implement this implicitly.
 *
 * \param node_ip        The IP Address of the Node
 *
 * \retval DOVE_STATUS_OK Cluster Node was added to collection
 * \retval DOVE_STATUS_NO_RESOURCES Cluster Node could not be added
 *
 ******************************************************************************
 */
dove_status dps_cluster_node_add(ip_addr_t *node_ip);

/*
 ******************************************************************************
 * dps_cluster_node_local_update                                          *//**
 *
 * \brief - This function can be used to update Local Node to cluster database
 *
 * \param node_ip        The IP Address of the Node
 *
 * \retval DOVE_STATUS_OK Cluster Node was added to collection
 * \retval DOVE_STATUS_NO_RESOURCES Cluster Node could not be added
 *
 ******************************************************************************
 */
dove_status dps_cluster_node_local_update(ip_addr_t *node_ip);

/*
 ******************************************************************************
 * dps_cluster_node_local_activate                                        *//**
 *
 * \brief - This function can be used to update set if the local node is active
 *
 * \param factive: Whether Local Node is active DCS node in cluster
 *
 ******************************************************************************
 */
dove_status dps_cluster_node_local_activate(int factive);

/*
 ******************************************************************************
 * dps_cluster_node_get_port                                              *//**
 *
 * \brief - This function is used to get the DPS Client Protocol port for a
 *          DPS Node.
 *
 * \param node_ip   The IP Address of the Node. Note that the port must
 *                  be set to zero if the caller doesn't know the port.
 *
 * \retval DOVE_STATUS_OK Cluster Node was added to collection
 * \retval DOVE_STATUS_NO_RESOURCES Cluster Node could not be added
 *
 ******************************************************************************
 */
dove_status dps_cluster_node_get_port(ip_addr_t *node_ip);

/*
 ******************************************************************************
 * dps_cluster_node_delete                                                *//**
 *
 * \brief - This function can be used to delete a Node from the Cluster.
 *          Determine if this function needs to exist with Spidercast or will
 *          Spidercast implement this implicitly.
 *
 * \param node_ip        The IP Address of the Node
 * *
 ******************************************************************************
 */
void dps_cluster_node_delete(ip_addr_t *node_ip);

/*
 ******************************************************************************
 * dps_cluster_node_add_domain                                            *//**
 *
 * \brief - This function is used to indicate a dps node handles a domain.
 *
 * \param node_ip            The IP Address of the Node
 * \param domain_id          The Domain ID
 * \param replication_factor The Replication Factor of the Domain
 *
 * \retval DOVE_STATUS_OK Cluster Node was added to collection
 * \retval DOVE_STATUS_NO_RESOURCES Cluster Node could not be added
 *
 ******************************************************************************
 */
dove_status dps_cluster_node_add_domain(ip_addr_t *node_ip,
                                        uint32_t domain_id,
                                        uint32_t replication_factor);

/*
 ******************************************************************************
 * dps_cluster_node_delete_domain                                         *//**
 *
 * \brief - This function is used to indicate a dps node no longer handles a
 *          domain.
 *
 * \param node_ip        The IP Address of the Node
 * \param domain_id      The Domain ID
 *
 * \retval DOVE_STATUS_OK Cluster Node was added to collection
 * \retval DOVE_STATUS_NO_RESOURCES Cluster Node could not be added
 *
 ******************************************************************************
 */
dove_status dps_cluster_node_delete_domain(ip_addr_t *node_ip,
                                                 uint32_t domain_id);

/*
 ******************************************************************************
 * dps_cluster_node_delete_all_domains                                    *//**
 *
 * \brief - This function is used to clear the domains handled by the node.
 *
 * \param node_ip        The IP Address of the Node
 *
 * \retval DOVE_STATUS_OK Cluster Node was added to collection
 * \retval DOVE_STATUS_NO_RESOURCES Cluster Node could not be added
 *
 ******************************************************************************
 */
dove_status dps_cluster_node_delete_all_domains(ip_addr_t *node_ip);

/*
 ******************************************************************************
 * dps_cluster_show                                                       *//**
 *
 * \brief - This function is used to clear the domains handled by the node.
 *
 * \retval DOVE_STATUS_OK Success
 * \retval DOVE_STATUS_EMPTY No Cluster - Error
 *
 ******************************************************************************
 */
dove_status dps_cluster_show();

/*
 ******************************************************************************
 * dps_node_get_domains                                         *//**
 *
 * \brief - This function is used to retrieve the list of domains handled by a
 * 	    node. The list is returned in a string format.
 *
 * \param node_ip        The IP Address of the Node
 * \param domains        The list of domains handled by the node
 *
 * \retval DOVE_STATUS_OK Cluster Node was added to collection
 *
 ******************************************************************************
 */
dove_status dps_node_get_domains(ip_addr_t node_ip,char ** domains);

/*
 ******************************************************************************
 * dps_cluster_set_node_domain_mapping                                        *//**
 *
 * \brief - This function is used to record the domain mapping of a node.
 *
 *
 * \param node_ip        The IP Address of the Node in string format
 * \param domain_list    List of Domains handled by the node.
 *
 * \retval DOVE_STATUS_OK Cluster Node was added to collection
 *
 ******************************************************************************
 */

dove_status dps_cluster_set_node_domain_mapping(char *node_ip, unsigned int port,
                                                char *domain_list);

/*
 ******************************************************************************
 * dps_domain_get_replication_factor --                                  *//**
 *
 * \brief This routine returns the number of nodes handling a domain.
 *
 * \param domain_id[in]		The Domain ID
 * \param rep_factor[out]	Current Replication Factor
 *
 * \retval DOVE_STATUS_OK, rep_factor
 *
 *****************************************************************************/
dove_status dps_domain_get_replication_factor(unsigned int domain_id,
                                              unsigned int *rep_factor);

/*
 ******************************************************************************
 * dps_cluster_is_local_node_leader                                       *//**
 *
 * \brief - This routine check if the current node is the cluster leader
 *
 * \retval 1 Yes
 * \retval 0 No
 *
 ******************************************************************************
 */
int dps_cluster_is_local_node_leader(void);

/*
 ******************************************************************************
 * dps_cluster_is_local_node_active                                       *//**
 *
 * \brief - This routine check if the current node is the cluster leader
 *
 * \retval 1 Yes
 * \retval 0 No
 *
 ******************************************************************************
 */
int dps_cluster_is_local_node_active(void);

/*
 ******************************************************************************
 * dps_cluster_get_domainid_from_vnid --                                  *//**
 *
 * \brief This routine returns a domain ID which the virtual network (VNID)
 *        is associated with.
 *
 * \param vnid[in]		The Virtual Network ID
 * \param domain_id[out]	The Domain ID
 *
 * \retval DOVE_STATUS_OK, domain_id
 * \retval DOVE_STATUS_INVALID_DVG, 0
 *
 *****************************************************************************/
dove_status dps_cluster_get_domainid_from_vnid(int vnid, int *domain_id);

/*
 ******************************************************************************
 * dps_cluster_write_log --                                               *//**
 *
 * \brief This routine writes logs sent by the Python Cluster Handler to the
 *        Logs
 *
 * \return PyObject
 *
 *****************************************************************************/
PyObject *dps_cluster_write_log(PyObject *self, PyObject *args);

/*
 ******************************************************************************
 * dps_cluster_create_shared_domain                                       *//**
 *
 * \brief This routine create Domain 0 and VNID 0
 *
 * \return PyObject
 *
 *****************************************************************************/
PyObject *dps_cluster_create_shared_domain(PyObject *self, PyObject *args);

/*
 ******************************************************************************
 * dps_cluster_send_local_mapping_to --                                   *//**
 *
 * \brief This routine sends local domain mapping to specific nodes in the
 *        DCS Cluster
 *
 * \return PyObject
 *
 *****************************************************************************/
PyObject *dps_cluster_send_local_mapping_to(PyObject *self, PyObject *args);

/*
 ******************************************************************************
 * dps_cluster_send_heartbeat_to --                                       *//**
 *
 * \brief This routine sends heartbeat message to the remote node
 *
 * \return PyObject
 *
 *****************************************************************************/
PyObject *dps_cluster_send_heartbeat_to(PyObject *self, PyObject *args);

/*
 ******************************************************************************
 * dps_cluster_send_heartbeat_request_to --                               *//**
 *
 * \brief This routine sends heartbeat request message to the remote node
 *
 * \return PyObject
 *
 *****************************************************************************/
PyObject *dps_cluster_send_heartbeat_request_to(PyObject *self, PyObject *args);

/*
 ******************************************************************************
 * dps_cluster_set_leader --                                              *//**
 *
 * \brief This routine sets the cluster_config_version variable
 *
 * \return PyObject
 *
 *****************************************************************************/
PyObject *dps_cluster_set_cluster_config_version(PyObject *self, PyObject *args);

/*
 ******************************************************************************
 * dps_cluster_set_leader --                                              *//**
 *
 * \brief This routine sets the Leader in the C code
 *
 * \return PyObject
 *
 *****************************************************************************/
PyObject *dps_cluster_set_leader(PyObject *self, PyObject *args);

/*
 ******************************************************************************
 * dps_cluster_send_nodes_status_to --                                    *//**
 *
 * \brief This routine sends node list to the specific remote node
 *
 * \return PyObject
 *
 *****************************************************************************/
PyObject *dps_cluster_send_nodes_status_to(PyObject *self, PyObject *args);

/*
 ******************************************************************************
 * dps_domain_activate_on_node --                                    *//**
 *
 * \brief This routine activates a domain on a node
 * 	  Typically used after Mass transfer is complete
 *
 * \return PyObject
 *
 *****************************************************************************/
PyObject *dps_domain_activate_on_node(PyObject *self, PyObject *args);

/*
 ******************************************************************************
 * dps_domain_recover_on_node --                                          *//**
 *
 * \brief This routine asks a node to recover a domain.
 *
 * \return PyObject
 *
 *****************************************************************************/
PyObject *dps_domain_recover_on_node(PyObject *self, PyObject *args);

/*
 ******************************************************************************
 * dps_domain_deactivate_on_node --                                    *//**
 *
 * \brief This routine deactivates a domain on a node. Typically used after
 *        Mass transfer is complete
 *
 * \return PyObject
 *
 *****************************************************************************/
PyObject *dps_domain_deactivate_on_node(PyObject *self, PyObject *args);

/*
 ******************************************************************************
 * dps_cluster_initiate_mass_transfer --                                  *//**
 *
 * \brief This route is used to initiate domain mass transfer from Python code.
 *
 * \return PyObject
 *
 *****************************************************************************/

PyObject *dps_cluster_initiate_mass_transfer(PyObject *self, PyObject *args);

/*
 ******************************************************************************
 * dps_cluster_exchange_domain_mapping --                                 *//**
 *
 * \brief This routine sends local domain mapping to all the nodes the cluster
 *
 *
 * \return void
 *
 *****************************************************************************/
void dps_cluster_exchange_domain_mapping();

/*
 ******************************************************************************
 * dps_cluster_exchange_domain_mapping_activate --                        *//**
 *
 * \brief This routine sends local domain mapping to all the nodes the cluster
 *        after domain activate. The remote node that activated the local node
 *        is sent last.
 *
 * \return void
 *
 *****************************************************************************/
void dps_cluster_exchange_domain_mapping_activate(ip_addr_t *remote_node);

/*
 ******************************************************************************
 * dps_cluster_node_touch --                                              *//**
 *
 * \brief This routine should be called whenever a REST request/reply is
 *        received from in the REST interface. This routine will invoke the
 *        Python callback that deals with cluster interaction and mark the
 *        sending node as "up" if it belongs in the cluster.
 *
 * \param node_ip The IP Address of the Node
 *
 * \return void
 *
 *****************************************************************************/
void dps_cluster_node_touch(ip_addr_t *node_ip);

/*
 ******************************************************************************
 * dps_cluster_node_validate --                                           *//**
 *
 * \brief This routine should be called to valid an IP address as a DCS Node
 *        IP Address
 *
 * \param node_ip The IP Address of the Node
 *
 * \return 1 if Valid, 0 if not valid
 *
 * \return void
 *
 *****************************************************************************/
int dps_cluster_node_validate_remote(char *node_ip_str);

/*
 ******************************************************************************
 * dps_cluster_node_show_load --                                          *//**
 *
 * \brief This routine will show the load on a dps node in the cluster
 *
 * \param node_ip The IP Address of the Node
 *
 * \return void
 *
 *****************************************************************************/
void dps_cluster_node_show_load(ip_addr_t *node_ip);

/*
 ******************************************************************************
 * dps_cluster_node_heartbeat --                                          *//**
 *
 * \brief This routine should be called whenever a Heartbeat message is
 *        received from a remote node
 *
 * \param node_ip The IP Address of the Node
 * \param factive If the node is active
 * \param dmc_config_version The DMC Config Version Number of the node
 *
 * \return void
 *
 *****************************************************************************/
void dps_cluster_node_heartbeat(ip_addr_t *node_ip, int factive,
                                long long dmc_config_version);

/*
 ******************************************************************************
 * dps_cluster_node_heartbeat_request --                                  *//**
 *
 * \brief This routine should be called whenever a Heartbeat Request message is
 *        received from a remote node
 *
 * \param node_ip The IP Address of the Node
 *
 * \return void
 *
 *****************************************************************************/
void dps_cluster_node_heartbeat_request(ip_addr_t *node_ip);

/*
 ******************************************************************************
 * dps_cluster_node_heartbeat_process --                                  *//**
 *
 * \brief Whether to process heartbeat messages
 *
 * \param dps_heartbeat_process - 1/Process, 0/Don't Process
 *
 * \return None
 *
 *****************************************************************************/
void dps_cluster_node_heartbeat_process(uint32_t dps_heartbeat_process);

/*
 ******************************************************************************
 * dps_cluster_nodes_from_dmc_update --                                   *//**
 *
 * \brief This routine updates the (remote) nodes of the cluster with the
 *        list provided in this list. The Local node may be included in this
 *        list too. But if the Local node is not included it doesn't mean
 *        that the local node is deleted.
 *
 * \param nodes - An array of ip addresse
 * \return void
 *
 *****************************************************************************/
void dps_cluster_nodes_from_dmc_update(ip_addr_t *nodes, int num_nodes);

/*
 ******************************************************************************
 * dps_cluster_nodes_status_from_leader --                                *//**
 *
 * \brief This routine should be called when the Leader send the List of Nodes
 *        and their status to the other nodes.
 *
 * \param leader The IP Address of the node sending this information.
 * \param config_version: The configuration version as seen by this node
 * \param nodes - An array of ip addresses of the nodes. The status is
 *                indicated by setting the ip_addr_t.status field according to
 *                the following specification.
 *                ip_addr_t.status = 1 Indicates node is up
 *                ip_addr_t.status = 0 Indicates node is down
 * \param num_nodes - The number of nodes in nodes (array)
 *
 * \return void
 *
 *****************************************************************************/
void dps_cluster_nodes_status_from_leader(ip_addr_t *leader,
                                          long long config_version,
                                          ip_addr_t *nodes,
                                          int num_nodes);

/*
 ******************************************************************************
 * dps_cluster_update_statistics                                            *//**
 *
 * \brief - This function allow updating of statistics of a node
 *
 * \param domain_id    The Domain ID
 * \param dps_node     The IP Address of the DPS Node
 *
 * \retval None
 *
 ******************************************************************************
 */
void dps_cluster_update_statistics(uint32_t domain_id,
                                   ip_addr_t *dps_node,
                                   uint32_t endpoints,
                                   uint32_t tunnels,
                                   uint32_t endpoint_updates,
                                   uint32_t endpoint_lookups,
                                   uint32_t policy_lookups);

/*
 ******************************************************************************
 * dps_cluster_send_domain_delete_to_all_nodes --                         *//**
 *
 * \brief This routine sends the domain delete REST message to all nodes in
 *        the cluster
 *
 * \param pythonpath - Pointer to the PYTHON Path. If this value is NULL, it
 *                     assumes the PYTHON path to be in the same directory.
 *
 * \return dove_status
 *
 *****************************************************************************/
void dps_cluster_send_domain_delete_to_all_nodes(uint32_t domain_id);

/*
 ******************************************************************************
 * dps_cluster_send_vnid_existence_to_all_nodes --                        *//**
 *
 * \brief This routine sends the vnid add/ delete REST message to all nodes in
 *        the cluster
 *
 * \param vnid - The VNI
 * \param fAdd - Whether it's add (1) or delete (0)
 *
 * \return None
 *
 *****************************************************************************/
void dps_cluster_send_vnid_existence_to_all_nodes(uint32_t vnid, int fAdd);

/*
 ******************************************************************************
 * dps_cluster_domain_delete --                                           *//**
 *
 * \brief This routine deletes the Domain from the cluster
 *
 * \param domain_id - Domain ID
 *
 * \return dove_status
 *
 *****************************************************************************/
void dps_cluster_domain_delete(uint32_t domain_id);

/*
 ******************************************************************************
 * dps_cluster_domain_exists --                                           *//**
 *
 * \brief This routine checks existence of domain
 *
 * \param domain_id - Domain ID
 *
 * \retval 0 Domain doesn't exist
 * \retval 1 Domain exists
 *
 *****************************************************************************/
int dps_cluster_domain_exists(uint32_t domain_id);

/*
 ******************************************************************************
 * dps_cluster_heavy_load_threshold --                                    *//**
 *
 * \brief This routine sets the heavy load threshold
 *
 * \param domain_id - Domain ID
 *
 * \return dove_status
 *
 *****************************************************************************/
void dps_cluster_heavy_load_threshold_set(uint32_t threshold);

/*
 ******************************************************************************
 * dps_cluster_reregister_endpoints --                                    *//**
 *
 * \brief This routine asks the DMC to request re-registration of all endpoints
 *        in a VNID.
 *
 * \return PyObject
 *
 *****************************************************************************/
PyObject *dps_cluster_reregister_endpoints(PyObject *self, PyObject *args);

/** @} */

#endif // _CLUSTER_DOMAIN_MAPPING_API_
