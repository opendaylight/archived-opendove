/******************************************************************************
** File Main Owner:   DOVE Development Team
** File Description:  The Initialization Code for the DCS Server
**/
/*
{
 *
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
*  HISTORY
*
*  $Log: init.c $
*  $EndLog$
*
*  PORTING HISTORY
*
}
*/

#include "include.h"

/**
 * \brief The Name of C Library that will be imported by Python
 */
#define DPS_LIB_NAME "dcslib"

/*
 ******************************************************************************
 * DPSServer                                                                   *//**
 *
 * \defgroup DPSServer DCS Server Functionality (User World)
 *
 * The DOVE DCS Server Functionality
 * @{
 */

#define SVA_INTERFACE_NAME "APBR"

#define DCS_ROLE_FILE ".flash/dcs.role"
#define DSA_VERSION_FILE "/dove/dsa_version"

#define DSA_VERSION_MAX_LENGTH 128

/**
 * \brief Contains the DSA version string
 */
char dsa_version_string_storage[DSA_VERSION_MAX_LENGTH];
char *dsa_version_string = dsa_version_string_storage;

/**
 * \brief Whether the Debug CLI was initialized
 */

static int fDebugCLIInitialized = 0;

/**
 * \brief Contains the Local IP address of the DPS Node
 */
ip_addr_t dcs_local_ip;

/* Indicate if the rest server is init well */
int dps_rest_server_init_ok = 0;

/**
 * \brief The Socket which monitors local IP change
 */
int dps_monitor_socket;

/*
 ******************************************************************************
 * set_initial_dps_cluster_leader --                                      *//**
 *
 * \brief Initializes self as the Initial Leader
 *
 ******************************************************************************/
static dove_status set_initial_dps_cluster_leader(void)
{

	dove_status dps_status = DOVE_STATUS_EMPTY;

	memset(&dps_cluster_leader, 0, sizeof(dps_cluster_leader));
	memcpy(&dps_cluster_leader, &dcs_local_ip, sizeof(dcs_local_ip));
	dps_cluster_leader.port_http = DPS_REST_HTTPD_PORT;
	inet_ntop(dps_cluster_leader.family,
	          dps_cluster_leader.ip6,
	          dps_cluster_leader_ip_string,
	          INET6_ADDRSTRLEN);

	dps_status = DOVE_STATUS_OK;
	return dps_status;
}

/*
 ******************************************************************************
 * set_initial_controller_location --                                     *//**
 *
 * \brief Initializes self as the Controller
 *
 ******************************************************************************/
static void set_initial_controller_location(void)
{

	memset(&controller_location, 0, sizeof(controller_location));
	memcpy(&controller_location, &dcs_local_ip, sizeof(controller_location));
	controller_location.port_http = DOVE_CONTROLLER_REST_HTTPD_PORT;
	inet_ntop(controller_location.family,
	          controller_location.ip6,
	          controller_location_ip_string,
	          INET6_ADDRSTRLEN);
	return;
}

/*
 ******************************************************************************
 * get_sva_interface_ip --                                                *//**
 *
 * \brief This routines gets IP address of the Service Appliance Management
 *        Interface.
 *
 * \retval DOVE_STATUS_OK SVA interface IP found
 * \retval DOVE_STATUS_EMPTY SVA interface IP not found
 *
 ******************************************************************************/

static dove_status get_sva_interface_ip(void)
{
	int fd, ret;
	struct ifreq ifr;
	dove_status status = DOVE_STATUS_EMPTY;

	do
	{
		fd = socket(AF_INET, SOCK_DGRAM, 0);
		if (fd <= 0)
		{
			break;
		}
		memset(&ifr,0,sizeof(ifr));
		ifr.ifr_addr.sa_family = AF_INET;
		strncpy(ifr.ifr_name, "APBR", IFNAMSIZ-1);
		ret = ioctl(fd, SIOCGIFADDR, &ifr);
		close(fd);
		if(ret == 0)
		{
			dcs_local_ip.family = AF_INET;
			dcs_local_ip.ip4 = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;
			status = DOVE_STATUS_OK;
		}
	}while(0);

	return status;
}

/*
 ******************************************************************************
 * set_local_ip --                                                        *//**
 *
 * \brief This routine sets the Local IP Address in the dcs_local_ip variable.
 *        That variable can be used by other routines.
 *
 * \remarks: Only supports IPv4 currently
 *
 * \TODO Change this routine to detect IP change dynamically.
 *
 * \retval DOVE_STATUS_OK Success
 * \retval DOVE_STATUS_EMPTY No Local IP detected
 *
 ******************************************************************************/

static dove_status set_local_ip()
{
	struct ifaddrs *myaddrs, *ifa;
	struct sockaddr_in *s4;
	dove_status dps_status = DOVE_STATUS_EMPTY;
	//struct sockaddr_in6 *s6;
	int status;
	char buf[INET6_ADDRSTRLEN];

	myaddrs = NULL;

	do
	{
		dps_status = get_sva_interface_ip();
		if (dps_status == DOVE_STATUS_OK)
		{
			break;
		}
		status = getifaddrs(&myaddrs);
		if (status != 0)
		{
			break;
		}

		for (ifa = myaddrs; ifa != NULL; ifa = ifa->ifa_next)
		{
			if (ifa->ifa_addr == NULL)
			{
				continue;
			}
			if ((ifa->ifa_flags & IFF_UP) == 0)
			{
				continue;
			}
			if (ifa->ifa_addr->sa_family == AF_INET)
			{
				s4 = (struct sockaddr_in *)(ifa->ifa_addr);
				if (ntohl(s4->sin_addr.s_addr) == INADDR_LOOPBACK)
				{
					continue;
				}
				// Check if this was the SVA_INTERFACE_NAME
				if (inet_ntop(AF_INET, (void *)&(s4->sin_addr), buf, sizeof(buf)) != NULL)
				{
					dcs_local_ip.family = AF_INET;
					dcs_local_ip.ip4 = s4->sin_addr.s_addr;
					dps_status = DOVE_STATUS_OK;
					break;
				}
				if (!strcmp(ifa->ifa_name, SVA_INTERFACE_NAME))
				{
					// If we found APBR, then we have our local IP!
					break;
				}
			}
			//Don't show IPv6 Address yet
			//else if (ifa->ifa_addr->sa_family == AF_INET6)
			//{
			//	s6 = (struct sockaddr_in6 *)(ifa->ifa_addr);
			//	if (inet_ntop(AF_INET6, (void *)&(s6->sin6_addr), buf, sizeof(buf)) != NULL)
			//	{
			//		break;
			//	}
			//}
		}
	} while(0);

	if (myaddrs != NULL)
	{
		free(myaddrs);
	}

	dcs_cluster_node_local_update(&dcs_local_ip);

	// Update Leader and Controller information if they are still LOOPBACK
	//if (dps_cluster_leader.ip4 == htonl(INADDR_LOOPBACK))
	//{
	//	set_initial_dps_cluster_leader();
	//}
	if (controller_location.ip4 == htonl(INADDR_LOOPBACK))
	{
		set_initial_controller_location();
	}

	log_notice(PythonDataHandlerLogLevel, "Initializing DCS REST Server");
	if(dps_status == DOVE_STATUS_OK && dps_rest_server_init_ok == 0)
	{
		if(dcs_server_rest_init(DPS_REST_HTTPD_PORT) == DOVE_STATUS_OK)
		{
			dps_rest_server_init_ok = 1;
		}
	}

	return dps_status;
}


/*
 ******************************************************************************
 * dcs_local_ip_monitor_process                                           *//**
 *
 * \brief - Called by the polling thread that gets an indication that the local
 *          IP address has changed. The function sets the Local IP Address in 
 *          the dcs_local_ip variable
 *
 * \param[in] socket - The socket on which the information arrived
 * \param[in] context - NULL
 *
 * \retval DOVE_STATUS_OK
 *
 ******************************************************************************
 */
static int dcs_local_ip_monitor_process(int socket, void *context)
{
	struct nlmsghdr *nlh;
	char buf[4096];
	int len;
	dove_status status = DOVE_STATUS_OK;

	while ((len = recv(dps_monitor_socket, buf, sizeof(buf), 0)) > 0)
	{

		nlh = (struct nlmsghdr *)buf;
		while ((NLMSG_OK(nlh, (uint32_t)len)) && (nlh->nlmsg_type != NLMSG_DONE))
		{
			if (nlh->nlmsg_type == RTM_NEWADDR || nlh->nlmsg_type == RTM_DELADDR)
			{
				struct ifaddrmsg *ifa = (struct ifaddrmsg *)NLMSG_DATA(nlh);
				struct rtattr *rth = IFA_RTA(ifa);
				int rtl = IFA_PAYLOAD(nlh);

				while (rtl && RTA_OK(rth, rtl))
				{
					if (rth->rta_type == IFA_LOCAL)
					{
						char ifname[IFNAMSIZ];

						if_indextoname(ifa->ifa_index, ifname);

						if (!strcmp(ifname, SVA_INTERFACE_NAME) && (nlh->nlmsg_type == RTM_NEWADDR))
						{
							status = set_local_ip();
							if (status != DOVE_STATUS_OK)
							{
								log_warn(PythonDataHandlerLogLevel,
								         "Cannot get IP Address of SVA");
								break;
							}
							// Register the Local DPS Node with the Cluster
							dcs_set_service_role(dcs_role_assigned);
						}
					}
					rth = RTA_NEXT(rth, rtl);
				}
			}
			nlh = NLMSG_NEXT(nlh, len);
		}
	}

	return DOVE_STATUS_OK;
}
/*
 ******************************************************************************
 * dcs_local_ip_monitor_init --                                           *//**
 *
 * \brief This routine creates a NETLINK socket which monitors local IP address
 *        change and addes it into poll array.
 *
 * \remarks: Only supports IPv4 currently
 *
 * \retval DOVE_STATUS_OK Success
 * \retval DOVE_STATUS_NO_RESOURCES No Resources
 * \retval DOVE_STATUS_INVALID_FD Couldn't create RAW Socket
 * \retval DOVE_STATUS_BIND_FAILED Bind failed to the Provided Port
 * \retval DOVE_STATUS_NOT_SUPPORTED Cannot function in Non Blocking Mode
 *
 ******************************************************************************/
static int dcs_local_ip_monitor_init(void)
{
	struct sockaddr_nl addr;
	int ret = DOVE_STATUS_OK;

	do
	{
		if ((dps_monitor_socket = socket(PF_NETLINK, SOCK_RAW, NETLINK_ROUTE)) == -1)
		{
			ret = DOVE_STATUS_INVALID_FD;
			break;
		}

		memset(&addr, 0, sizeof(addr));
		addr.nl_family = AF_NETLINK;
		addr.nl_groups = RTMGRP_IPV4_IFADDR;

		if (bind(dps_monitor_socket, (struct sockaddr *)&addr, sizeof(addr)) == -1)
		{
			ret = DOVE_STATUS_BIND_FAILED;
			break;
		}

		if (fcntl(dps_monitor_socket, F_SETFL, O_NONBLOCK) == -1)
		{
			ret = DOVE_STATUS_NOT_SUPPORTED;
			break;
		}

		ret = fd_process_add_fd(dps_monitor_socket, dcs_local_ip_monitor_process, NULL);
		if (ret != DOVE_STATUS_OK)
		{
			break;
		}
	} while (0);

	return ret;
}

/*
 ******************************************************************************
 * init_dps_lib --                                                        *//**
 *
 * \brief Initializes the DPS Library. This routine must be called from the
 *        PYTHON Script before the DPS Library can be used by PYTHON.
 *
 * \param[in] self  PyObject
 * \param[in] args  The List of Initialization arguments
 *                  (Integer, Integer, String)
 *                  1st input Integer - UDP Port
 *                  2nd input Integer - Whether to exit when CTRL+C is pressed
 *                  3rd input String (Optional) - PYTHON interpreter path
 *
 * \retval 0 Success
 * \retval -1 Failure
 *
 ******************************************************************************/
static PyObject *
init_dps_lib(PyObject *self, PyObject *args)
{
	char *python_path = NULL;
	int udp_port, rest_port, fExitOnCtrlC, fDebugCli;
	if (!PyArg_ParseTuple(args, "iiii|s",
	                      &udp_port,
	                      &rest_port,
	                      &fDebugCli,
	                      &fExitOnCtrlC,
	                      &python_path))
	{
		return Py_BuildValue("i", -1);
	}
	dcs_initialize(udp_port, rest_port, fDebugCli, fExitOnCtrlC, python_path);
	return Py_BuildValue("i", 0);
}

/**
 * \brief The DPS Library Methods
 */
static PyMethodDef dps_lib_methods[] = {
	{"initialize", init_dps_lib, METH_VARARGS, "dcslib doc"},
	{"send_all_connectivity_policies", send_all_connectivity_policies, METH_VARARGS, "dcslib doc"},
	{"send_multicast_tunnels", send_multicast_tunnels, METH_VARARGS, "dcslib doc"},
	{"send_gateways", send_gateways, METH_VARARGS, "dcslib doc"},
	{"send_broadcast_table", send_broadcast_table, METH_VARARGS, "dcslib doc"},
	{"send_address_resolution", send_address_resolution, METH_VARARGS, "dcslib doc"},
	{"send_heartbeat", send_heartbeat, METH_VARARGS, "dcslib doc"},
	{"send_endpoint_reply", send_endpoint_reply, METH_VARARGS, "dcslib doc"},
	{"send_vnid_deletion", send_vnid_deletion, METH_VARARGS, "dcslib doc"},
	{"mass_transfer_endpoint", mass_transfer_endpoint, METH_VARARGS, "dcslib doc"},
	{"mass_transfer_tunnel", mass_transfer_tunnel, METH_VARARGS, "dcslib doc"},
	{"mass_transfer_multicast_sender", mass_transfer_multicast_sender, METH_VARARGS, "dcslib doc"},
	{"mass_transfer_multicast_receiver", mass_transfer_multicast_receiver, METH_VARARGS, "dcslib doc"},
	{"dps_vnids_replicate", dps_vnids_replicate, METH_VARARGS, "dcslib doc"},
	{"dps_policy_bulk_replicate", dps_policy_bulk_replicate, METH_VARARGS, "dcslib doc"},
	{"dps_ipsubnet_bulk_replicate", dps_ipsubnet_bulk_replicate, METH_VARARGS, "dcslib doc"},
	{"report_endpoint_conflict", report_endpoint_conflict, METH_VARARGS, "dcslib doc"},
	{"process_cli_data", process_cli_data, METH_VARARGS, "dcslib doc"},
	{"send_message_and_free", send_message_and_free, METH_VARARGS, "dcslib doc"},
	{"retransmit_data", retransmit_data, METH_VARARGS, "dcslib doc"},
	{"retransmit_timeout", retransmit_timeout, METH_VARARGS, "dcslib doc"},
	{"dps_cluster_write_log", dps_cluster_write_log, METH_VARARGS, "dcslib doc"},
	{"dps_cluster_send_local_mapping_to", dps_cluster_send_local_mapping_to, METH_VARARGS, "dcslib doc"},
	{"dps_cluster_send_heartbeat_to", dps_cluster_send_heartbeat_to, METH_VARARGS, "dcslib doc"},
	{"dps_cluster_send_heartbeat_request_to", dps_cluster_send_heartbeat_request_to, METH_VARARGS, "dcslib doc"},
	{"dps_cluster_set_cluster_config_version", dps_cluster_set_cluster_config_version, METH_VARARGS, "dcslib doc"},
	{"dps_cluster_set_leader", dps_cluster_set_leader, METH_VARARGS, "dcslib doc"},
	{"dps_cluster_send_nodes_status_to", dps_cluster_send_nodes_status_to, METH_VARARGS, "dcslib doc"},
	{"dps_domain_activate_on_node", dps_domain_activate_on_node, METH_VARARGS, "dcslib doc"},
	{"dps_domain_recover_on_node", dps_domain_recover_on_node, METH_VARARGS, "dcslib doc"},
	{"dps_cluster_create_shared_domain", dps_cluster_create_shared_domain, METH_VARARGS, "dcslib doc"},
	{"dps_domain_deactivate_on_node", dps_domain_deactivate_on_node, METH_VARARGS, "dcslib doc"},
	{"dps_cluster_initiate_mass_transfer", dps_cluster_initiate_mass_transfer, METH_VARARGS, "dcslib doc"},
	{"vnid_query_send_to_controller", vnid_query_send_to_controller, METH_VARARGS, "dcslib doc"},
	{"vnid_query_subnets_from_controller", vnid_query_subnets_from_controller, METH_VARARGS, "dcslib doc"},
	{"domain_query_policy_from_controller", domain_query_policy_from_controller, METH_VARARGS, "dcslib doc"},
	{"domain_query_from_controller", domain_query_from_controller, METH_VARARGS, "dcslib doc"},
	{"dps_rest_domain_delete_send_to_dps_node", dps_rest_domain_delete_send_to_dps_node, METH_VARARGS, "dcslib doc"},
	{"dps_rest_vnid_add_send_to_dps_node", dps_rest_vnid_add_send_to_dps_node, METH_VARARGS, "dcslib doc"},
	{"dps_rest_vnid_delete_send_to_dps_node", dps_rest_vnid_delete_send_to_dps_node, METH_VARARGS, "dcslib doc"},
	{"dps_cluster_reregister_endpoints", dps_cluster_reregister_endpoints, METH_VARARGS, "dcslib doc"},
	{"send_all_vm_migration_update", send_all_vm_migration_update, METH_VARARGS, "dcslib doc"},
	{NULL, NULL, 0, NULL}  // end of table marker
};

#if PY_MAJOR_VERSION >= 3
/* module definition structuire */
static struct PyModuleDef dcslibmodule = {
	PyModuleDef_HEAD_INIT,
	DPS_LIB_NAME,
	"dcslib doc", // Maybe NULL
	-1,
	dps_methods // link to methods table
};
#endif

/*
 ******************************************************************************
 * initdcslib --                                                          *//**
 *
 * \brief The DPS Library Module Initializer. The Module Name must be "init"
 *        followed by the DPS Library Name.
 *        For e.g.:
 *        "initdcslib" is the initialization routine for dcslib
 *        OR
 *        "inittemp" is the initialization routine for temp
 *        etc. *
 *
 * \retval None
 *
 ******************************************************************************/

PyMODINIT_FUNC initdcslib(void)
{
#if PY_MAJOR_VERSION >= 3
	return PyModule_Create(&dcslibmodule);
#else
	Py_InitModule3(DPS_LIB_NAME, dps_lib_methods, NULL);
	return;
#endif
}

/*
 ******************************************************************************
 * dcs_write_role_to_file --                                              *//**
 *
 * \brief This routine write the role to the file
 *
 * \param role
 *
 * \return dove_status
 *
 *****************************************************************************/
static void dcs_write_role_to_file(int role)
{
	FILE *fp = NULL;
	char role_string[16];
	char path[100] = {'\0'};

	sprintf(path, "%s/%s", getenv("HOME"), DCS_ROLE_FILE);
	if((fp = fopen(path,"w")) == NULL) {
		log_alert(PythonClusterDataLogLevel,
		          "[ERROR] in opening(write) ROLE file [%s]",
		          path);
	}else{
		sprintf(role_string, "%d", role);
		fputs(role_string, fp);
		log_notice(PythonClusterDataLogLevel,
		           "Wrote Role %d to file %s",role, path);
		fclose(fp);
	}
	return;
}

/*
 ******************************************************************************
 * dcs_set_service_role --                                                *//**
 *
 * \brief This routine starts or stops the DCS service role
 *
 * \param action 1:Activate/Set Local Node, 0:Reset Local Node
 *
 * \return dove_status
 *
 *****************************************************************************/
dove_status dcs_set_service_role(uint32_t action)
{
	dove_status status = DOVE_STATUS_ERROR;
	do
	{
		if (action)
		{
			status = dcs_cluster_node_local_update(&dcs_local_ip);
			if (status != DOVE_STATUS_OK)
			{
				log_alert(RESTHandlerLogLevel,
				          "ALERT! DCS Clustering cannot set local IP");
				break;
			}
			status = dcs_controller_interface_start();
			if (status != DOVE_STATUS_OK)
			{
				log_alert(RESTHandlerLogLevel,
				          "ALERT! DCS Controller Interface cannot be started");
				break;
			}
			status = dcs_protocol_handler_start();
			if (status != DOVE_STATUS_OK)
			{
				log_alert(RESTHandlerLogLevel,
				          "ALERT! DCS Client Server Protocol cannot be started");
				break;
			}
			/* do "cluster node add" via sending "query cluster info" */
			status = dcs_cluster_node_local_activate(1);
			if (status != DOVE_STATUS_OK)
			{
				log_alert(RESTHandlerLogLevel,
				          "ALERT! DCS Clustering cannot activate Local Node");
				break;
			}
			//Query the Controller for Cluster Information
			dcs_role_assigned = 1;
		}
		else
		{
			status = dcs_cluster_node_local_activate(0);
			if (status != DOVE_STATUS_OK)
			{
				log_alert(RESTHandlerLogLevel,
				          "ALERT! DCS Clustering cannot deactivate Local Node");
				break;
			}
			status = dcs_protocol_handler_stop();
			if (status != DOVE_STATUS_OK)
			{
				log_alert(RESTHandlerLogLevel,
				          "ALERT! DCS Client Server Protocol cannot be stopped");
				dcs_cluster_node_local_activate(1);
				break;
			}
			status = dcs_controller_interface_stop();
			if (status != DOVE_STATUS_OK)
			{
				log_alert(RESTHandlerLogLevel,
				          "ALERT! DCS Controller Interface cannot be stopped");
				dcs_cluster_node_local_activate(1);
				dcs_protocol_handler_start();
				break;
			}
			dcs_role_assigned = 0;
			cluster_config_version = 0;
		}
		dcs_write_role_to_file(dcs_role_assigned);
		do
		{
			if (!controller_location_set)
			{
				log_info(RESTHandlerLogLevel, "Controller not set by user");
				break;
			}
			if (dcs_local_ip.ip4 == htonl(INADDR_LOOPBACK))
			{
				log_info(RESTHandlerLogLevel,
				         "Local IP not yet set. Not point in sending query");
				break;
			}
			if(get_dps_appliance_registration_needed()) {
				log_notice(RESTHandlerLogLevel, "DCS: Sending Appliance Registration");
				dps_appliance_registration();
			}
			/*if (action)
			{
				log_notice(RESTHandlerLogLevel,
				           "DCS: Sending Query to DMC for list of Nodes");
				dps_rest_client_query_dove_controller_cluster_info();
			}*/
		}while(0);
	}while(0);

	return status;
}

/*
 ******************************************************************************
 * dps_read_role --                                                       *//**
 *
 * \brief This routine reads the ROLE from the file.
 *
 * \return dove_status
 *
 *****************************************************************************/
static void dcs_read_role()
{
	FILE *fp = NULL;
	char ptr[16];
	int role = 0;
	char path[100] = {'\0'};

	do
	{
		memset(ptr, 0, 16);
		sprintf(path, "%s/%s", getenv("HOME"), DCS_ROLE_FILE);
		fp = fopen(path, "r");
		if(fp == NULL)
		{
			log_notice(PythonDataHandlerLogLevel,
			           "Existing Role File not found [%s]",
			           path);
			break;
		}
		// read the file and populate the global
		if(fgets(ptr, 8, fp) == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "Cannot read ROLE File %s, probably corrupted",
			              path);
			break;
		}
		if(strlen(ptr) > 4)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ROLE length greater than expected",
			              path);
			break;
		}
		role = atoi(ptr);
	}while(0);
	if (fp)
	{
		fclose(fp);
	}
	if (role)
	{
		dcs_set_service_role(1);
		log_notice(PythonDataHandlerLogLevel, "DCS Role: Initialized to Active");
	}
	else
	{
		dcs_set_service_role(0);
		log_notice(PythonDataHandlerLogLevel, "DCS Role: Initialized to Inactive");
	}
	return;
}

/*
 ******************************************************************************
 * dcs_read_version --                                                    *//**
 *
 * \brief This routine reads the version from the flash file
 *
 * \return dove_status
 *
 *****************************************************************************/
static void dcs_read_version()
{
	FILE *fp = NULL;
	int error = -1;

	do
	{
		memset(dsa_version_string, 0, DSA_VERSION_MAX_LENGTH);
		fp = fopen(DSA_VERSION_FILE, "r");
		if(fp == NULL)
		{
			log_alert(PythonDataHandlerLogLevel,
			          "Version file not found [%s]",
			           DSA_VERSION_FILE);
			break;
		}
		// read the file and populate the global
		if(fgets(dsa_version_string, 120, fp) == NULL)
		{
			log_alert(PythonDataHandlerLogLevel,
			          "Cannot read ROLE File %s, probably corrupted",
			          DSA_VERSION_FILE);
			break;
		}
		show_print("DSA Version: %s", dsa_version_string);
		log_notice(PythonDataHandlerLogLevel,
		           "DSA Version: %s", dsa_version_string);
		error = 0;
	}while(0);
	if (fp)
	{
		fclose(fp);
	}
	if (error)
	{
		//Put fake version in
		sprintf(dsa_version_string, "0.0.0 Thur Jan 1 0:0:0 UDT 1970");
		log_emergency(PythonDataHandlerLogLevel,
		              "Generating Fake DSA Version: %s", dsa_version_string);
	}
	return;
}

/*
 ******************************************************************************
 * dcs_initialize                                                         *//**
 *
 * \brief - Initializes the DCS Server
 *
 * \param[in] udp_port - The UDP Port to run the DCS Server on
 * \param[in] rest_port - The port the REST Services should run on
 * \param[in] fDebugCli - Whether the DebugCli should be started.
 * \param[in] fExitOnCtrlC - Whether the Server Process should exit on
 *                           CTRL+C being pressed. In a development environment
 *                           this should be TRUE (1) while in a Production
 *                           Build this should be FALSE (0).
 * \param[in] python_path - The Location of the Python Scripts. This should be
 *                          NULL in most cases since the code will assume the
 *                          scripts are in the "." directory (i.e. the same
 *                          directory) as the dcslib module.
 *
 * \retval -1: Should never happen - This is an infinite loop.
 *
 ******************************************************************************
 */

int dcs_initialize(int udp_port, int rest_port, int fDebugCli, int fExitOnCtrlC, char *python_path)
{
	dove_status status = DOVE_STATUS_OK;
	char buf[INET6_ADDRSTRLEN];
	int ret;

	memset(&dcs_local_ip, 0, sizeof(ip_addr_t));
	dcs_local_ip.family = AF_INET;
	dcs_local_ip.ip4 = htonl(INADDR_LOOPBACK);
	dcs_local_ip.port = udp_port;
	dcs_local_ip.port_http = rest_port;
	dcs_local_ip.xport_type = SOCK_DGRAM;
	set_initial_dps_cluster_leader();
	set_initial_controller_location();

	Py_Initialize();
	PyEval_InitThreads();
	do
	{
		log_console = 0;

		//Read version
		dcs_read_version();
		log_notice(PythonDataHandlerLogLevel, "DCS version %s",dsa_version_string);

		if(OSW_OK != osw_init())
		{
			status = DOVE_STATUS_OSW_INIT_FAILED;
			break;
		}
		log_notice(PythonDataHandlerLogLevel, "Initialized OS Wrapper");

		status = python_init_dcs_protocol_interface(python_path);
		if (status != DOVE_STATUS_OK)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "Cannot initialize the Protocol Interface");
			break;
		}
		log_notice(PythonDataHandlerLogLevel, "Initialized DCS Protocol");

		// Initialize the data handler for the Cluster Database
		status = python_init_cluster_db_interface(python_path);
		if (status != DOVE_STATUS_OK)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "Cannot initialize the Cluster Database");
			break;
		}
		log_notice(PythonDataHandlerLogLevel, "Initialized Cluster Database");

		// Initialize the data handler for the DPS Controller Protocol
		status = python_init_controller_interface(python_path);
		if (status != DOVE_STATUS_OK)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "Cannot initialize the Cluster Database");
			break;
		}
		log_notice(PythonDataHandlerLogLevel, "Initialized Controller Interface");

		// Initialize the debug handler
		status = python_init_debug_interface(python_path);
		if (status != DOVE_STATUS_OK)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "Cannot initialize the Debug Handler");
			break;
		}
		log_notice(PythonDataHandlerLogLevel, "Initialized Debug Interface");

		// Initialize the Retransmit Handler
		status = python_init_retransmit_interface(python_path);
		if (status != DOVE_STATUS_OK)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "Cannot Initialize Retransmit Handler");
			break;
		}
		log_notice(PythonDataHandlerLogLevel, "Initialized Retransmit Interface");

		// Initialize the UUID Interface
		status = python_init_UUID_interface(python_path);
		if (status != DOVE_STATUS_OK)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "Cannot initialize the UUID API");
			break;
		}
		log_notice(PythonDataHandlerLogLevel, "Initialized UUID Interface");

		// Get local IP Address
		set_local_ip();
		log_notice(PythonDataHandlerLogLevel, "Initialized Local IP");

		// Initialize the REST SERVER
		if(dps_rest_server_init_ok == 0)
		{
			status = dcs_server_rest_init((short)DPS_REST_HTTPD_PORT);
			if (status != DOVE_STATUS_OK)
			{
				log_emergency(PythonDataHandlerLogLevel,
				              "Cannot Initialize the REST SERVER");
				break;
			}
			dps_rest_server_init_ok = 1;
		}
		log_notice(PythonDataHandlerLogLevel, "Initialized REST Server");

		dove_status status_tmp = dcs_read_svc_app_uuid();
		if (status_tmp != DOVE_STATUS_OK)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "Could NOT read service appliance UUID from %s/%s",
			              getenv("HOME"),SERVICE_APPLIANCE_UUID_FILE);
			// Create a FAKE UUID interface
			dps_init_uuid();
			write_uuid_to_file();
		}
		log_notice(PythonDataHandlerLogLevel, "READ SVC APP UUID");

		// Register the Local DPS Node with the Cluster
		status = dcs_cluster_node_local_update(&dcs_local_ip);
		if (status != DOVE_STATUS_OK)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "DCS: Cannot add local Node to Cluster");
			break;
		}
		log_notice(PythonDataHandlerLogLevel, "Registered Local Node with Cluster");

		// Initialize the Statistics
		status = dcs_statistics_init(python_path);
		if (status != DOVE_STATUS_OK)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "DCS: Cannot Initialize Statistics Thread");
			break;
		}
		log_notice(PythonDataHandlerLogLevel, "Initialized Statistics Thread");

		// Initialize the Heartbeat
		status = dcs_heartbeat_init(python_path);
		if (status != DOVE_STATUS_OK)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "DCS: Cannot Initialize Heartbeat Thread");
			break;
		}
		log_notice(PythonDataHandlerLogLevel, "Initialized Heartbeat Thread");

		dcs_rest_sync_init();

		log_notice(PythonDataHandlerLogLevel, "Initialized Controller Sync Thread");

		if (fDebugCli)
		{
			// Initialize the CLI Thread PYTHON Interface. This will
			// start the CLI Thread
			status = python_lib_embed_cli_thread_start(fExitOnCtrlC);
			if (status != DOVE_STATUS_OK)
			{
				log_emergency(PythonDataHandlerLogLevel,
				              "DCS: Cannot start the CLI thread");
				break;
			}

			// Initialize the CLI
			status = dcs_server_cli_init();
			if (status != DOVE_STATUS_OK)
			{
				log_emergency(PythonDataHandlerLogLevel,
				              "DCS: Cannot Initialize the CLI");
				break;
			}

			fDebugCLIInitialized = 1;
		}
		else
		{
			//Set net.core.rmem_max to 64M
			ret = system("sysctl -w net.core.rmem_max=67108864 &> /dev/null");
			if (WEXITSTATUS(ret) != 0)
			{
				log_emergency(PythonDataHandlerLogLevel,
				              "DCS: Failed to set net.core.rmem_max to 64M");
			}
			ret = system("sysctl -w net.core.rmem_default=67108864 &> /dev/null");
			if (WEXITSTATUS(ret) != 0)
			{
				log_emergency(PythonDataHandlerLogLevel,
				              "DCS: Failed to set net.core.rmem_default to 64M");
			}
			//Read from file
			dcs_read_role();
		}

		Py_BEGIN_ALLOW_THREADS

		// Initialize the CORE APIs
		status = fd_process_init();
		if (status != DOVE_STATUS_OK)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "DCS: Cannot Initialize CORE processing");
			break;
		}

		// Initialize the DCS Server
		dps_svr_proto_init((uint32_t)udp_port);

		// Print the starting parameters
		inet_ntop(dcs_local_ip.family, dcs_local_ip.ip6, buf, INET6_ADDRSTRLEN);
		log_notice(PythonDataHandlerLogLevel,
		           "DCS Server Started: IP Address <%s>, Port <%d>",
		           buf, dcs_local_ip.port);

		//Initialize local IP monitor
		dcs_local_ip_monitor_init();

		// Start the CORE APIs Communication
		fd_process_start();

		Py_END_ALLOW_THREADS

	}while(0);

	//show_print("Quitting Abnormally: Not cleaning up threads!!!");

	Py_Finalize();

	return -1;
}

/*
 ******************************************************************************
 * print_console --                                                       *//**
 *
 * \brief This routine prints the message to the console. This routine should
 *        be called to print messages to the console.
 *
 * \param output_string - The String To Print
 *
 * \remarks DO NOT log in this routine. Will cause infinite loop :)
 *
 *****************************************************************************/

void print_console(const char *output_string)
{
	fprintf(stderr, "%s\n\r", output_string);
	return;
}



/** @} */
