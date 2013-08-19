/******************************************************************************
** File Main Owner:   DOVE Development Team
** File Description:  The Initialization Code for the DPS Server
**/
/*
{
*
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
 * \defgroup DPSServer DPS Server Functionality (User World)
 *
 * The DOVE DPS Server Functionality
 * @{
 */

#define SVA_INTERFACE_NAME "APBR"

#define DCS_ROLE_FILE "/flash/dcs.role"
#define DSA_VERSION_FILE "/dove/dsa_version"

#define DSA_VERSION_MAX_LENGTH 128

/**
 * \brief Contains the DSA version string
 */
char dsa_version_string_storage[DSA_VERSION_MAX_LENGTH];
char *dsa_version_string = dsa_version_string_storage;

/**
 * \brief Contains the Local IP address of the DPS Node
 */
ip_addr_t dps_local_ip;

/* Indicate if the rest server is init well */
int dps_rest_server_init_ok = 0;

/**
 * \brief The Socket which monitors local IP change
 */
int dps_monitor_socket;

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
			dps_local_ip.family = AF_INET;
			dps_local_ip.ip4 = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;
			status = DOVE_STATUS_OK;
		}
	}while(0);

	return status;
}

/*
 ******************************************************************************
 * set_local_ip --                                                        *//**
 *
 * \brief This routine sets the Local IP Address in the dps_local_ip variable.
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
					dps_local_ip.family = AF_INET;
					dps_local_ip.ip4 = s4->sin_addr.s_addr;
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

	return dps_status;
}


/*
 ******************************************************************************
 * dps_local_ip_monitor_process                                           *//**
 *
 * \brief - Called by the polling thread that gets an indication that the local
 *          IP address has changed. The function sets the Local IP Address in 
 *          the dps_local_ip variable
 *
 * \param[in] socket - The socket on which the information arrived
 * \param[in] context - NULL
 *
 * \retval DOVE_STATUS_OK
 *
 ******************************************************************************
 */
static int dps_local_ip_monitor_process(int socket, void *context)
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
 * dps_local_ip_monitor_init --                                           *//**
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
static int dps_local_ip_monitor_init(void)
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

		ret = fd_process_add_fd(dps_monitor_socket, dps_local_ip_monitor_process, NULL);
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
	int udp_port, rest_port;
	if (!PyArg_ParseTuple(args, "ii",
	                      &udp_port,
	                      &rest_port,
	                      &python_path))
	{
		return Py_BuildValue("i", -1);
	}
	dps_initialize(udp_port, rest_port, python_path);
	return Py_BuildValue("i", 0);
}

/**
 * \brief The DPS Library Methods
 */
static PyMethodDef dps_lib_methods[] = {
	{"initialize", init_dps_lib, METH_VARARGS, "dcslib doc"},
	{"send_message_and_free", send_message_and_free, METH_VARARGS, "dcslib doc"},
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
 *        "initdcslib" is the initialization routine for dpslib
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
 * dcs_set_service_role --                                                *//**
 *
 * \brief This routine starts or stops the DCS service role
 *
 * \param action 1:Activate/Set Local Node, 0:Reset Local Node
 *
 * \return dove_status
 *
 *****************************************************************************/
dove_status dps_set_service_role(uint32_t action)
{
	dove_status status = DOVE_STATUS_ERROR;
	do
	{
		if (action)
		{
			status = dps_controller_interface_start();
			if (status != DOVE_STATUS_OK)
			{
				break;
			}
			status = dps_protocol_handler_start();
			if (status != DOVE_STATUS_OK)
			{
				break;
			}
		}
		else
		{
			status = dps_protocol_handler_stop();
			if (status != DOVE_STATUS_OK)
			{
				break;
			}
			status = dps_controller_interface_stop();
			if (status != DOVE_STATUS_OK)
			{
				dps_protocol_handler_start();
				break;
			}
		}
	}while(0);

	return status;
}

/*
 ******************************************************************************
 * dps_initialize                                                         *//**
 *
 * \brief - Initializes the DPS Server
 *
 * \param[in] udp_port - The UDP Port to run the DPS Server on
 * \param[in] rest_port - The port the REST Services should run on
 * \param[in] python_path - The Location of the Python Scripts. This should be
 *                          NULL in most cases since the code will assume the
 *                          scripts are in the "." directory (i.e. the same
 *                          directory) as the dcslib module.
 *
 * \retval -1: Should never happen - This is an infinite loop.
 *
 ******************************************************************************
 */

int dps_initialize(int udp_port, int rest_port, char *python_path)
{
	dove_status status = DOVE_STATUS_OK;
	char buf[INET6_ADDRSTRLEN];

	memset(&dps_local_ip, 0, sizeof(ip_addr_t));
	dps_local_ip.family = AF_INET;
	dps_local_ip.ip4 = htonl(INADDR_LOOPBACK);
	dps_local_ip.port = udp_port;
	dps_local_ip.port_http = rest_port;
	dps_local_ip.xport_type = SOCK_DGRAM;

	Py_Initialize();
	PyEval_InitThreads();
	do
	{
		log_console = 0;

		// Initialize the new data handler for the DPS Client Protocol
		status = python_init_dps_protocol_interface(python_path);
		if (status != DOVE_STATUS_OK)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "Cannot initialize the Protocol Interface");
			break;
		}

		// Initialize the data handler for the DPS Controller Protocol
		status = python_init_dps_controller_interface(python_path);
		if (status != DOVE_STATUS_OK)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "Cannot initialize the Cluster Database");
			break;
		}

		// Get local IP Address
		set_local_ip();

		Py_BEGIN_ALLOW_THREADS

		// Initialize the CORE APIs
		status = fd_process_init();
		if (status != DOVE_STATUS_OK)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "DCS: Cannot Initialize CORE processing");
			break;
		}

		// Initialize the DPS Server
		dps_svr_proto_init((uint32_t)udp_port);

		// Print the starting parameters
		inet_ntop(dps_local_ip.family, dps_local_ip.ip6, buf, INET6_ADDRSTRLEN);
		log_notice(PythonDataHandlerLogLevel,
		           "DCS Server Started: IP Address <%s>, Port <%d>",
		           buf, dps_local_ip.port);

		//Initialize local IP monitor
		dps_local_ip_monitor_init();

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
