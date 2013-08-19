/******************************************************************************
** File Main Owner:   DOVE DPS Development Team
** File Description:  This file embedded the Python Module that handles the
**                    DPS Client Server Protocol
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
*  $Log: controller_interface.c $
*  $EndLog$
*
*  PORTING HISTORY
*
}
*/

#include "include.h"

/**
 * \brief Variable indicating that Controller Interface has started
 */
static int started = 0;

/**
 * \brief Contains the IP address of the DPS Cluster Leader
 */
ip_addr_t controller_location;

/**
 * \brief Whether the controller location was configured by user
 */
int controller_location_set = 0;

/**
 * \brief Contains the IP address of DPS Cluster Leader in Readable
 *        string format
 */
char controller_location_ip_string_storage[INET6_ADDRSTRLEN];
char *controller_location_ip_string = controller_location_ip_string_storage;

/**
 * \brief The module location defines for the DPS Controller Message Handling API
 */
#define PYTHON_MODULE_FILE_CONTROLLER_HANDLER "controller_protocol_handler"

/**
 * \brief The PYTHON Class that handles the DPS Controller Message Requests
 */
#define PYTHON_MODULE_CLASS_CONTROLLER_HANDLER "DpsControllerHandler"

/**
 * \brief The PYTHON function that Stops the Controller Handler
 */
#define PYTHON_FUNC_CONTROLLER_FUNCTION_STOP "Stop"

/**
 * \brief The PYTHON function that Starts the Controller Handler
 */
#define PYTHON_FUNC_CONTROLLER_FUNCTION_START "Start"


/**
 * \brief The DPS controller handler function pointers data structure
 */

typedef struct python_dps_controller_s{
	/**
	 * \brief The DpsClientHandler Object Instance
	 */
	PyObject *instance;
	/**
	 * \brief Domain_Show_Address_Resolution
	 */
	PyObject *Stop;
	/**
	 * \brief Domain_Delete_All_Local
	 */
	PyObject *Start;
}python_dps_controller_t;

/*
 * \brief The DPS Controller Message Handler PYTHON Interface (Embed)
 */
static python_dps_controller_t Controller_Interface;

/*
 * \brief Function Handler for each DPS Message Type
 */

typedef dove_status (*dps_controller_func_handler)(dps_controller_data_op_t *data);

/**
 * \brief An Array of Callback for every DPS Controller Object Operation Code
 */

static dps_controller_func_handler function_array[DPS_CONTROLLER_OP_MAX];

/*
 ******************************************************************************
 * DPS Object Interface                                                   *//**
 *
 * \addtogroup PythonInterface
 * @{
 * \defgroup DPSControllerInterface DPS Client Server Protocol Interface
 * @{
 * Handles Interaction between the DPS Client Server Protocol and DPS (PYTHON)
 * Objects
 ******************************************************************************
 */

/*
 ******************************************************************************
 * functions_init --                                              *//**
 *
 * \brief This initializes the C function handler for the various request types
 *
 * \retval DOVE_STATUS_OK
 *
 *****************************************************************************/

static dove_status functions_init(void)
{
	int i;
	for (i = 0; i < DPS_CONTROLLER_OP_MAX; i++)
	{
		function_array[i] = NULL;
	}
	return DOVE_STATUS_OK;
}

/*
 ******************************************************************************
 * python_function_init --                                                *//**
 *
 * \brief This routine gets references to all functions in the PYTHON data
 *        handler code (DpsControllerHandler) that are needed for processing
 *        requests received from the Controller.
 *
 * \param pythonpath - Pointer to the PYTHON Path. If this value is NULL, it
 *                     assumes the PYTHON path to be in the same directory.
 *
 * \return dove_status
 *
 *****************************************************************************/

static dove_status python_functions_init(char *pythonpath)
{
	dove_status status;
	PyObject *pyargs;

	log_info(PythonDataHandlerLogLevel, "Enter");

	memset(&Controller_Interface, 0, sizeof(python_dps_controller_t));
	do
	{
		// Get handle to an instance of DpsControllerHandler
		pyargs = Py_BuildValue("()");
		if (pyargs == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! Py_BuildValue () failed...\n");
			status = DOVE_STATUS_NO_RESOURCES;
			break;
		}
		status = python_lib_get_instance(pythonpath,
		                                 PYTHON_MODULE_FILE_CONTROLLER_HANDLER,
		                                 PYTHON_MODULE_CLASS_CONTROLLER_HANDLER,
		                                 pyargs,
		                                 &Controller_Interface.instance);
		Py_DECREF(pyargs);
		if (status != DOVE_STATUS_OK)
		{
			break;
		}

		// Get handle to function Stop
		Controller_Interface.Stop =
			PyObject_GetAttrString(Controller_Interface.instance,
			                       PYTHON_FUNC_CONTROLLER_FUNCTION_STOP);
		if (Controller_Interface.Stop == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_CONTROLLER_FUNCTION_STOP);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Start
		Controller_Interface.Start =
			PyObject_GetAttrString(Controller_Interface.instance,
			                       PYTHON_FUNC_CONTROLLER_FUNCTION_START);
		if (Controller_Interface.Start == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_CONTROLLER_FUNCTION_START);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		status = DOVE_STATUS_OK;
	}while(0);

	log_info(PythonDataHandlerLogLevel, "Exit: %s",
	         DOVEStatusToString(status));

	return status;
}

/*
 ******************************************************************************
 * python_init_dps_controller_interface --                                *//**
 *
 * \brief This routine initializes the DPS Controller Interface to PYTHON
 *        OBHECTS
 *
 * \param pythonpath - Pointer to the Python Path
 *
 * \return dove_status
 *
 *****************************************************************************/

dove_status python_init_dps_controller_interface(char *pythonpath)
{
	dove_status status = DOVE_STATUS_OK;

	do
	{
		status = python_functions_init(pythonpath);
		if (status != DOVE_STATUS_OK)
		{
			break;
		}
		status = functions_init();
	} while (0);

	return status;
}

/*
 ******************************************************************************
 * dps_controller_interface_stop --                                       *//**
 *
 * \brief This routine stops the DPS Controller Interface
 *
 * \retval DOVE_STATUS_OK Success
 * \retval DOVE_STATUS_NO_RESOURCES No resources
 *
 *****************************************************************************/
dove_status dps_controller_interface_stop()
{
	PyObject *strargs;
	PyGILState_STATE gstate;
	int status = DOVE_STATUS_NO_RESOURCES;

	log_info(PythonDataHandlerLogLevel, "Enter");

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();

	do
	{

		//def Stop():
		strargs = Py_BuildValue("()");
		if (strargs == NULL)
		{
			log_warn(PythonDataHandlerLogLevel, "Py_BuildValue returns NULL");
			break;
		}

		// Invoke the Node_Local_Activate call
		PyEval_CallObject(Controller_Interface.Stop, strargs);
		Py_DECREF(strargs);
		log_notice(PythonDataHandlerLogLevel, "DPS Controller Interface Stopped");
		started = 0;
		status = DOVE_STATUS_OK;
	} while(0);

	PyGILState_Release(gstate);
	log_info(PythonDataHandlerLogLevel, "Exit");

	return (dove_status)status;
}

/*
 ******************************************************************************
 * dps_controller_interface_start --                                       *//**
 *
 * \brief This routine starts the DPS Controller Interface
 *
 * \retval DOVE_STATUS_OK Success
 * \retval DOVE_STATUS_NO_RESOURCES No resources
 *
 *****************************************************************************/
dove_status dps_controller_interface_start()
{
	PyObject *strargs;
	PyGILState_STATE gstate;
	int status = DOVE_STATUS_NO_RESOURCES;

	log_info(PythonDataHandlerLogLevel, "Enter");

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();

	do
	{
		//def Start():
		strargs = Py_BuildValue("()");
		if (strargs == NULL)
		{
			log_warn(PythonDataHandlerLogLevel, "Py_BuildValue returns NULL");
			break;
		}

		// Invoke the Node_Local_Activate call
		PyEval_CallObject(Controller_Interface.Start, strargs);
		Py_DECREF(strargs);
		log_notice(PythonDataHandlerLogLevel, "DPS Controller Interface Started");
		started = 1;
		status = DOVE_STATUS_OK;
	} while(0);

	PyGILState_Release(gstate);
	log_info(PythonDataHandlerLogLevel, "Exit");

	return (dove_status)status;
}

/*
 ******************************************************************************
 * dps_controller_data_msg --                                             *//**
 *
 * \brief This routine initializes handles DPS Controller Data Object Messages
 *
 * \param data - Pointer to DPS Client Server Protocol Message
 *
 * \return dove_status
 *
 *****************************************************************************/

dove_status dps_controller_data_msg(dps_controller_data_op_t *data)
{
	uint32_t msg_type;
	dove_status status = DOVE_STATUS_NOT_FOUND;
	dps_controller_func_handler func;

	msg_type = (uint32_t)data->type;
	do
	{
		if (!started &&
		    (msg_type != DPS_CONTROLLER_LOCATION_SET) &&
		    (msg_type != DPS_CONTROLLER_LOCATION_GET))
		{
			status = DOVE_STATUS_INACTIVE;
			break;
		}
		if (msg_type >= DPS_CONTROLLER_OP_MAX)
		{
			break;
		}
		func = function_array[msg_type];
		if (func == NULL)
		{
			break;
		}
		status = func(data);
	}while(0);

	return status;
}

/** @} */
/** @} */
