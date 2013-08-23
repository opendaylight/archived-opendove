/******************************************************************************
** File Main Owner:   DOVE DPS Development Team
** File Description:  Interaction between the DPS Protocol and DPS Retransmit (PYTHON)
**                    interface.
**/
/*
{
* Copyright (c) 2010-2013 IBM Corporation
* All rights reserved.
*
* This program and the accompanying materials are made available under the
* terms of the Eclipse Public License v1.0 which accompanies this
* distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
*
*
*  HISTORY
*
*  $Log: retransmit_interface.c $
*  $EndLog$
*
*  PORTING HISTORY
*
}
*/

#include "include.h"
#include "raw_proto_timer.h"

/**
 * \brief Variable that holds the logging variable
 */
int PythonRetransmitLogLevel = DPS_SERVER_LOGLEVEL_NOTICE;

/**
 * \brief The module location defines for the DPS Message Handling API
 */
#define PYTHON_MODULE_FILE_RETRANSMIT_HANDLER "retransmit_handler"

/**
 * \brief The PYTHON Class that handles the DPS Retransmit Handler
 */
#define PYTHON_MODULE_CLASS_RETRANSMIT_HANDLER "DPSRetransmitHandler"

/**
 * \brief The PYTHON function that handles Retransmit_Queue
 */
#define PYTHON_FUNC_RETRANSMIT_QUEUE "Queue"

/**
 * \brief The PYTHON function that handles Retransmit_DeQueue
 */
#define PYTHON_FUNC_RETRANSMIT_DEQUEUE "DeQueue"

/**
 * \brief The PYTHON function for generating Query ID
 */
#define PYTHON_FUNC_GENERATE_QUERY_ID "Generate_Query_Id"

/**
 * \brief The PYTHON function for Show the state of Retransmit Timer
 */
#define PYTHON_FUNC_RETRANSMIT_SHOW "Show"

/**
 * \brief The DPS Retransmit handler function pointers data structure
 */
typedef struct python_dps_retransmit_s{
	/**
	 * \brief The DpsClientHandler Object Instance
	 */
	PyObject *instance;
	/*
	 * \brief The function that performs Retransmit_Queue
	 */
	PyObject *Retransmit_Queue;
	/*
	 * \brief The function that performs Retransmit_DeQueue
	 */
	PyObject *Retransmit_DeQueue;
	/**
	 * \brief Generate Query ID
	 */
	PyObject *Generate_Query_Id;
	/**
	 * \brief The function that shows the Retransmit Details
	 */
	PyObject *Retransmit_Show;
}python_dps_retransmit_t;

/*
 * \brief The DPS Retransmit handler PYTHON Interface (Embed)
 */
static python_dps_retransmit_t Retransmit_Interface;

/**
 * \brief The context associated with Address Resolution
 */
typedef struct dps_retransmit_context_s{
	/**
	 * \brief The Family of the Socket
	 */
	sa_family_t family;
	/**
	 * \brief The UDP socket descriptor
	 */
	int sock_fd;
	/**
	 * \brief The address to send the packet to
	 */
	union{
		/**
		 * \brief IPv4 Socket
		 */
		struct sockaddr_in v4;
		/**
		 * \brief IPv6 Socket
		 */
		struct sockaddr_in6 v6;
	}address;
	/**
	 * \brief The query id
	 */
	uint32_t query_id;
	/**
	 * \brief The context associated with the packet
	 */
	void *context;
	/**
	 * \brief The callback associated with the packet
	 */
	rpt_callback_ptr callback_function;
	/**
	 * \brief The retransmit owner type
	 */
	rpt_owner_t owner_type;
	/*
	 * \brief The data len
	 */
	size_t data_len;
	/*
	 * \brief The data packet
	 */
	char data[1];
}dps_retransmit_context_t;

/*
 ******************************************************************************
 * DPS Retransmit Interface                                               *//**
 *
 * \addtogroup PythonInterface
 * @{
 * \defgroup DPSRetransmitInterface DPS RAW Protocol Retransmit Interface
 * @{
 * Handles Interaction between the DPS Protocol and DPS Retransmit (PYTHON)
 *         interface
 */

/*
 ******************************************************************************
 * retransmit_data --                                                       *//**
 *
 * \brief This is the routine that the PYTHON Scripts must call to retransmit
 *        a message.
 *
 * \param[in] self  PyObject
 * \param[in] args  dps_retransmit_context_s as Python Object
 *
 * \retval 0 Success
 * \retval -1 Failure
 *
 ******************************************************************************/
PyObject *retransmit_data(PyObject *self, PyObject *args)
{
	PyObject *ret_val = NULL;
	PyObject *py_retransmit_context;
	dps_retransmit_context_t *retransmit_context;
	socklen_t udp_sock_len;
	struct sockaddr *udp_to;
	int send_result = -1;

	//Py_BEGIN_ALLOW_THREADS
	log_debug(PythonRetransmitLogLevel, "Enter");
	do
	{
		// Parameters are (PyObject)
		if (!PyArg_ParseTuple(args, "O", &py_retransmit_context))
		{
			log_error(PythonRetransmitLogLevel, "NO DATA! - NULL!");
			break;
		}
		retransmit_context = (dps_retransmit_context_t *)PyCObject_AsVoidPtr(py_retransmit_context);
		if (retransmit_context == NULL)
		{
			log_error(PythonRetransmitLogLevel, "retransmit_context is NULL!");
			break;
		}
		// Send the message
		if (retransmit_context->family == AF_INET)
		{
			udp_sock_len = sizeof(retransmit_context->address.v4);
			udp_to = (struct sockaddr *)&retransmit_context->address.v4;
		}
		else if (retransmit_context->family == AF_INET6)
		{
			udp_sock_len = sizeof(retransmit_context->address.v6);
			udp_to = (struct sockaddr *)&retransmit_context->address.v6;
		}
		else
		{
			log_error(PythonRetransmitLogLevel,
			          "INVALID retransmit_context->family!");
			break;
		}
		send_result = sendto(retransmit_context->sock_fd,
		                     retransmit_context->data,
		                     retransmit_context->data_len, 0,
		                     udp_to, udp_sock_len);
		if (send_result < 0)
		{
			log_error(PythonRetransmitLogLevel, "sendto error %d", errno);
			break;
		}
		send_result = 0;
		log_debug(PythonRetransmitLogLevel,
		          "Retransmitted context %p, query_id %d, %d bytes",
		          retransmit_context, retransmit_context->query_id, retransmit_context->data_len);
	} while(0);
	log_debug(PythonRetransmitLogLevel, "Exit");

	//Py_END_ALLOW_THREADS
	ret_val = Py_BuildValue("i", send_result);
	return ret_val;
}

/*
 ******************************************************************************
 * retransmit_timeout --                                                  *//**
 *
 * \brief This is the routine that the PYTHON Scripts must call to indicate
 *        timeout to the DPS Protocol.
 *
 * \param[in] self  PyObject
 * \param[in] args  dps_retransmit_context_t as Python Object
 *
 * \retval 0 Success
 *
 ******************************************************************************/
PyObject *retransmit_timeout(PyObject *self, PyObject *args)
{
	PyObject *ret_val = NULL;
	PyObject *py_retransmit_context;
	dps_retransmit_context_t *retransmit_context;

	//Py_BEGIN_ALLOW_THREADS
	log_debug(PythonRetransmitLogLevel, "Enter");
	do
	{
		// Parameters are (PyObject)
		if (!PyArg_ParseTuple(args, "O", &py_retransmit_context))
		{
			log_error(PythonRetransmitLogLevel, "NO DATA! - NULL!");
			break;
		}
		retransmit_context = (dps_retransmit_context_t *)PyCObject_AsVoidPtr(py_retransmit_context);
		if (retransmit_context == NULL)
		{
			log_error(PythonRetransmitLogLevel, "retransmit_context is NULL!");
			break;
		}
		// Invoke the Callback function
		retransmit_context->callback_function(RAW_PROTO_MAX_NUM_RETRANSMIT_EXCEEDED,
		                                      retransmit_context->data,
		                                      retransmit_context->context,
		                                      retransmit_context->owner_type);
		log_debug(PythonRetransmitLogLevel,
		          "Freeing Message %p, PyObject %p, Ref Count %d",
		          retransmit_context, py_retransmit_context, py_retransmit_context->ob_refcnt);
		// Free the context the message
		free(retransmit_context);
		Py_DECREF(py_retransmit_context);
	} while(0);
	log_debug(PythonRetransmitLogLevel, "Exit");

	//Py_END_ALLOW_THREADS
	ret_val = Py_BuildValue("i", 0);
	return ret_val;
}

/*
 ******************************************************************************
 * retransmit_timer_start --                                              *//**
 *
 * \brief This is the routine that the DPS Protocol Handler must call to start
 *        a retransmit timer on a packet
 *
 * \param[in] data  The Data to send
 * \param[in] data_len  Data Length
 * \param[in] query_id  The Query ID to associated with the packet
 * \param[in] sockFd  The socket file descriptor
 * \param[in] addr  The Socket Address
 * \param[in] context  The Context associated with the packet
 * \param[in] callback  The callback to invoke when retransmission times out
 * \param[in] owner  The owner type
 *
 * \retval 0 Success
 * \retval non-zero Failure
 *
 ******************************************************************************/

int retransmit_timer_start(char *data, int data_len, uint32_t query_id,
                           int sockFd, struct sockaddr *addr,
                           void *context, rpt_callback_ptr callback, rpt_owner_t owner)
{
	PyObject *py_retransmit_context = NULL;
	dps_retransmit_context_t *retransmit_context = NULL;
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;
	int status = -1;

	log_debug(PythonRetransmitLogLevel,
	          "Enter: query_id %d, context %p, data_len %d",
	          query_id, context, data_len);
	gstate = PyGILState_Ensure();
	do
	{
		if (sockFd < 0)
		{
			log_error(PythonRetransmitLogLevel, "Bad socket fd %d", sockFd);
			break;
		}
		if ((addr->sa_family != AF_INET) && (addr->sa_family != AF_INET6))
		{
			log_error(PythonRetransmitLogLevel, "Bad socket family %d", addr->sa_family);
		}
		//Allocate a context
		retransmit_context = (dps_retransmit_context_t *)malloc(dps_offsetof(dps_retransmit_context_t,
		                                                                     data[data_len]));
		if (retransmit_context == NULL)
		{
			log_error(PythonRetransmitLogLevel, "Cannot allocate retransmit context");
			break;
		}
		//Convert to a PyObject
		py_retransmit_context = PyCObject_FromVoidPtr((void *)retransmit_context, NULL);
		if (py_retransmit_context == NULL)
		{
			log_error(PythonDataHandlerLogLevel, "PyCObject_FromVoidPtr returns NULL");
			break;
		}
		//Store data
		retransmit_context->family = addr->sa_family;
		if (retransmit_context->family == AF_INET)
		{
			memcpy(&retransmit_context->address.v4, addr, sizeof(struct sockaddr_in));
		}
		else
		{
			memcpy(&retransmit_context->address.v6, addr, sizeof(struct sockaddr_in6));
		}
		retransmit_context->callback_function = callback;
		retransmit_context->context = context;
		memcpy(retransmit_context->data, data, data_len);
		retransmit_context->data_len = (size_t)data_len;
		retransmit_context->owner_type = owner;
		retransmit_context->sock_fd = sockFd;
		retransmit_context->query_id = query_id;
		//Invoke PYTHON code to insert into table
		strargs = Py_BuildValue("(IOI)", query_id, py_retransmit_context, data_len);
		if (strargs == NULL)
		{
			log_notice(PythonRetransmitLogLevel, "Py_BuildValue returns NULL");
			break;
		}
		// Add the query ID to the dictionary to track the replies
		strret = PyEval_CallObject(Retransmit_Interface.Retransmit_Queue,
		                           strargs);
		Py_DECREF(strargs);
		if(strret == NULL)
		{
			log_alert(PythonRetransmitLogLevel,
			         "PyEval_CallObject Retransmit_Queue returns NULL");
			break;
		}
		PyArg_Parse(strret, "I", &status);
		log_debug(PythonRetransmitLogLevel, "PyEval_CallObject returns status %d", status);
		Py_DECREF(strret);
	}while(0);

	if (status != 0)
	{
		if (py_retransmit_context != NULL)
		{
			Py_DECREF(py_retransmit_context);
		}
		if (retransmit_context != NULL)
		{
			free(retransmit_context);
		}
	}

	PyGILState_Release(gstate);

	log_debug(PythonRetransmitLogLevel, "Exit status %d", status);
	return status;
}

/*
 ******************************************************************************
 * retransmit_timer_show --                                               *//**
 *
 * \brief This is the routine that the DPS Protocol Handler must call to stop
 *        the retransmit timer on a packet. It should be called by the DPS
 *        Protocol Handler when it receives a reply packet.
 *        Once this routine is called a valid context returned, the timer
 *        routine will not longer own any resources associated with the packet.
 *
 * \param[in] query_id  The query
 * \param[in] pcontext  Pointer to a location that will return the context
 *
 * \retval 0 Success
 * \retval non-zero Failure
 *
 ******************************************************************************/
int retransmit_timer_stop(uint32_t query_id, void **pcontext)
{
	int status = -1;
	PyObject *py_retransmit_context = NULL;
	dps_retransmit_context_t *retransmit_context = NULL;
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;

	log_debug(PythonRetransmitLogLevel, "Enter: query id %d", query_id);
	gstate = PyGILState_Ensure();
	do{
		//Invoke PYTHON code to dequeue from table
		strargs = Py_BuildValue("(I)", query_id);
		if (strargs == NULL)
		{
			log_notice(PythonRetransmitLogLevel, "Py_BuildValue returns NULL");
			break;
		}
		// Add the query ID to the dictionary to track the replies
		strret = PyEval_CallObject(Retransmit_Interface.Retransmit_DeQueue,
		                           strargs);
		Py_DECREF(strargs);
		if(strret == NULL)
		{
			log_warn(PythonRetransmitLogLevel,
			         "PyEval_CallObject Retransmit_DeQueue returns NULL");
			break;
		}
		PyArg_ParseTuple(strret, "IO", &status, &py_retransmit_context);
		Py_DECREF(strret);
		if (status != 0)
		{
			log_debug(PythonRetransmitLogLevel, "status %d", status);
			break;
		}
		if (py_retransmit_context == NULL)
		{
			status = -1;
			log_debug(PythonRetransmitLogLevel, "py_retransmit_context is NULL");
			break;
		}
		retransmit_context = (dps_retransmit_context_t *)PyCObject_AsVoidPtr(py_retransmit_context);
		if (retransmit_context == NULL)
		{
			status = -1;
			log_error(PythonRetransmitLogLevel, "retransmit_context is NULL!");
			break;
		}
		*pcontext = retransmit_context->context;
		log_debug(PythonRetransmitLogLevel,
		          "Freeing Message %p, PyObject %p, Ref Count %d",
		          retransmit_context, py_retransmit_context, py_retransmit_context->ob_refcnt);
		// Free the context the message
		free(retransmit_context);
		Py_DECREF(py_retransmit_context);
	}while(0);
	PyGILState_Release(gstate);

	log_debug(PythonRetransmitLogLevel, "Exit: status %d, context %p", status, *pcontext);
	return status;
}

/*
 ******************************************************************************
 * retransmit_timer_query_id_generate --                                  *//**
 *
 * \brief This is the routine that the DPS Protocol Handler must call to
 *        generate a query id. This query id is guaranteed to be unique
 *        system wide.
 *
 * \retval Query ID
 *
 ******************************************************************************/
uint32_t retransmit_timer_query_id_generate(void)
{
	uint32_t query_id = 0;
	PyObject *strargs, *strret;
	PyGILState_STATE gstate;

	log_debug(PythonRetransmitLogLevel, "Enter");
	gstate = PyGILState_Ensure();
	do{
		strargs = Py_BuildValue("()");
		if (strargs == NULL)
		{
			log_warn(PythonRetransmitLogLevel, "Py_BuildValue returns NULL");
			break;
		}
		strret = PyEval_CallObject(Retransmit_Interface.Generate_Query_Id,
		                           strargs);
		Py_DECREF(strargs);
		if(strret == NULL)
		{
			log_alert(PythonRetransmitLogLevel,
			          "PyEval_CallObject Generate_Query_Id returns NULL");
			break;
		}
		PyArg_Parse(strret, "I", &query_id);
		Py_DECREF(strret);
	}while(0);
	PyGILState_Release(gstate);

	log_debug(PythonRetransmitLogLevel, "Exit query_id %d", query_id);
	return query_id;
}

/*
 ******************************************************************************
 * retransmit_timer_show --                                                *//**
 *
 * \brief This is the routine that shows the retransmit timer details
 *
 * \retval None
 *
 ******************************************************************************/
void retransmit_timer_show()
{
	PyGILState_STATE gstate;
	PyObject *strargs;

	log_debug(PythonRetransmitLogLevel, "Enter");
	gstate = PyGILState_Ensure();
	do
	{
		strargs = Py_BuildValue("()");
		if (strargs == NULL)
		{
			log_warn(PythonRetransmitLogLevel, "Py_BuildValue returns NULL");
			break;
		}
		PyEval_CallObject(Retransmit_Interface.Retransmit_Show, strargs);
		Py_DECREF(strargs);
	}while(0);
	PyGILState_Release(gstate);
	log_debug(PythonRetransmitLogLevel, "Exit");
	return;
}

/*
 ******************************************************************************
 * python_function_init --                                                *//**
 *
 * \brief This routine gets references to all functions in the PYTHON data
 *        handler code (DPSRetransmitHandler) that are needed for processing
 *        requests received from DPS Clients.
 *
 * \param pythonpath - Pointer to the PYTHON Path. If this value is NULL, it
 *                     assumes the PYTHON path to be in the same directory.
 *
 * \return dove_status
 *
 *****************************************************************************/

static dove_status python_function_init(char *pythonpath)
{
	dove_status status;
	PyObject *pyargs;

	log_info(PythonRetransmitLogLevel, "Enter");

	memset(&Retransmit_Interface, 0, sizeof(python_dps_retransmit_t));

	do
	{
		// Get handle to an instance of DPSRetransmitHandler
		pyargs = Py_BuildValue("()");
		if (pyargs == NULL)
		{
			log_emergency(PythonRetransmitLogLevel,
			              "ERROR! Py_BuildValue () failed...\n");
			status = DOVE_STATUS_NO_RESOURCES;
			break;
		}
		status = python_lib_get_instance(pythonpath,
		                                 PYTHON_MODULE_FILE_RETRANSMIT_HANDLER,
		                                 PYTHON_MODULE_CLASS_RETRANSMIT_HANDLER,
		                                 pyargs,
		                                 &Retransmit_Interface.instance);
		Py_DECREF(pyargs);
		if (status != DOVE_STATUS_OK)
		{
			break;
		}

		// Get handle to function Retransmit_Queue
		Retransmit_Interface.Retransmit_Queue =
			PyObject_GetAttrString(Retransmit_Interface.instance,
			                       PYTHON_FUNC_RETRANSMIT_QUEUE);
		if (Retransmit_Interface.Retransmit_Queue == NULL)
		{
			log_emergency(PythonRetransmitLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_RETRANSMIT_QUEUE);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Retransmit_DeQueue
		Retransmit_Interface.Retransmit_DeQueue =
			PyObject_GetAttrString(Retransmit_Interface.instance,
			                       PYTHON_FUNC_RETRANSMIT_DEQUEUE);
		if (Retransmit_Interface.Retransmit_DeQueue == NULL)
		{
			log_emergency(PythonRetransmitLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_RETRANSMIT_DEQUEUE);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Generate_Query_Id
		Retransmit_Interface.Generate_Query_Id =
			PyObject_GetAttrString(Retransmit_Interface.instance,
			                       PYTHON_FUNC_GENERATE_QUERY_ID);
		if (Retransmit_Interface.Generate_Query_Id == NULL)
		{
			log_emergency(PythonRetransmitLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_GENERATE_QUERY_ID);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Retransmit_Show
		Retransmit_Interface.Retransmit_Show =
			PyObject_GetAttrString(Retransmit_Interface.instance,
			                       PYTHON_FUNC_RETRANSMIT_SHOW);
		if (Retransmit_Interface.Retransmit_Show == NULL)
		{
			log_emergency(PythonRetransmitLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_RETRANSMIT_SHOW);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		status = DOVE_STATUS_OK;
	}while(0);

	log_info(PythonRetransmitLogLevel, "Exit: %s",
	         DOVEStatusToString(status));

	return status;
}

/*
 ******************************************************************************
 * python_init_retransmit_interface --                                  *//**
 *
 * \brief This routine initializes the functions needed to handle the
 *        Retransmit Timer module
 *
 * \param pythonpath - Pointer to the PYTHON Path. If this value is NULL, it
 *                     assumes the PYTHON path to be in the same directory.
 *
 * \return dove_status
 *
 *****************************************************************************/
dove_status python_init_retransmit_interface(char *pythonpath)
{
	dove_status status = DOVE_STATUS_OK;

	log_info(PythonRetransmitLogLevel, "Enter %s");
	do
	{
		status = python_function_init(pythonpath);
		if (status != DOVE_STATUS_OK)
		{
			break;
		}
	} while (0);
	log_info(PythonRetransmitLogLevel, "Exit: %s",
	         DOVEStatusToString(status));
	return status;
}

/** @} */
/** @} */
