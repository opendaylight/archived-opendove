/******************************************************************************
** File Main Owner:   DOVE DPS Development Team
** File Description:  UUID Interface
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
*  $Log: uuid.c $
*  $EndLog$
*
*  PORTING HISTORY
*
}
*/

#include "include.h"

#define UUID_LENGTH 36
char dps_node_uuid[UUID_LENGTH+1];

int PythonUUIDLogLevel = DPS_SERVER_LOGLEVEL_NOTICE;

/**
 * \brief The module location defines for the DPS Cluster Database Handling API
 */
#define PYTHON_MODULE_FILE_UUID "dcs_uuid"

/**
 * \brief The PYTHON Class that handles the DPS Cluster Database Requests
 */
#define PYTHON_MODULE_CLASS_UUIDv0 "UUIDv0"

/**
 * \brief The PYTHON function that handles DPS Node Add (to Cluster)
 */
#define PYTHON_FUNC_DPS_GET_UUID "get_uuid"

typedef struct python_dps_uuid_s{
	/**
	 * \brief The DpsClientHandler Object Instance
	 */
	PyObject *instance;
	/*
	 * \brief The function for get_uuid
	 */
	PyObject *get_uuid;
	
	/*
	 * \brief The function for get_uuid2
	 */
	PyObject *get_uuid2;
} python_dps_uuid_t;

/*
 * \brief The uuid Handler PYTHON Interface (Embed)
 */
static  python_dps_uuid_t UUID_Interface;


/*
 ******************************************************************************
 * DPS UUID Interface                                         *//**
 *
 * \addtogroup PythonInterface
 * @{
 * \defgroup DPSUUIDInterface DCS UUID Interface
 * @{
 * Handles Interaction between the DCS Server and DCS Cluster Database Objects.
 * The Gossip Protocol can use this Cluster Database Handler to store the Node
 * Attribute for each node in the DPS Cluster.
 */

/*
 ******************************************************************************
 * dps_get_uuid                                                   *//**
 *
 * \brief - This function can be used to add a Node to Cluster. Determine if
 *          this function needs to exist with Spidercast or will Spidercast
 *          implement this implicitly.
 *
 * \param fLocal         If this is the Local Node
 * \param node_ip        The IP Address of the Node
 *
 * \retval DOVE_STATUS_OK Cluster Node was added to collection
 * \retval DOVE_STATUS_NO_RESOURCES Cluster Node could not be added
 *
 ******************************************************************************
 */
dove_status dps_get_uuid(char *return_uuid)
{
	char *return_val = NULL;
	int return_size = 0;
	int status = DOVE_STATUS_NO_MEMORY;
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;

	log_debug(PythonUUIDLogLevel, "Enter");

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();

	do
	{

		strargs = Py_BuildValue("()");
		if (strargs == NULL)
		{
			log_notice(PythonUUIDLogLevel, "Py_BuildValue returns NULL");
			break;
		}

		// Invoke the Node_Add call
		strret = PyEval_CallObject( UUID_Interface.get_uuid, strargs);
		Py_DECREF(strargs);

		if(strret == NULL)
		{
			log_warn(PythonUUIDLogLevel,
			         "PyEval_CallObject get_uuid returns NULL");
			break;
		}

		//@return: The status of the operation
		//@rtype: dove_status (defined in include/status.h) Integer
		PyArg_Parse(strret, "z#", &return_val, &return_size);

		//printf("return_val is %s, return_size is %d\r\n", return_val, return_size);

		if(return_size > UUID_LENGTH)
			return_size = UUID_LENGTH;
		strncpy(return_uuid, return_val, return_size);

		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
		status = DOVE_STATUS_OK;
	} while(0);
	
	PyGILState_Release(gstate);
	//printf("uuid: %s\n\r",return_val);
	log_debug(PythonUUIDLogLevel, "Exit");
	return (dove_status)status;
}

void show_uuid(void)
{
//	char uuid[37];
//	memset(uuid,0,37);

//	dps_get_uuid(uuid);
	printf("\n\ruuid is %s\n\r", dps_node_uuid);
}

void dps_init_uuid(void)
{
	log_notice(PythonUUIDLogLevel, "Init DPS node UUID");
	memset(dps_node_uuid,0,UUID_LENGTH+1);
	dps_get_uuid(dps_node_uuid);
	log_notice(PythonUUIDLogLevel,"Current DPS Node UUID is %s\r\n",dps_node_uuid);

}

char  *get_uuid(void)
{
	return dps_node_uuid;
}

/*
 ******************************************************************************
 * python_function_init --                                                *//**
 *
 * \brief This routine gets references to all functions in the PYTHON data
 *        handler code (ClusterDatabase) that are needed for processing
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

	log_info(PythonUUIDLogLevel, "Enter");

	memset(&UUID_Interface, 0, sizeof(python_dps_uuid_t));
	do
	{
		// Get handle to an instance of ClusterDatabase
		pyargs = Py_BuildValue("()");
		if (pyargs == NULL)
		{
			log_emergency(PythonUUIDLogLevel,
			              "ERROR! Py_BuildValue () failed...\n");
			status = DOVE_STATUS_NO_RESOURCES;
			break;
		}
		status = python_lib_get_instance(pythonpath,
		                                 PYTHON_MODULE_FILE_UUID,
		                                 PYTHON_MODULE_CLASS_UUIDv0,
		                                 pyargs,
		                                 &UUID_Interface.instance);
		Py_DECREF(pyargs);
		if (status != DOVE_STATUS_OK)
		{
			break;
		}

		// Get handle to function Node_Add
		UUID_Interface.get_uuid =
			PyObject_GetAttrString(UUID_Interface.instance,
			                       PYTHON_FUNC_DPS_GET_UUID);
		if (UUID_Interface.get_uuid == NULL)
		{
			log_emergency(PythonUUIDLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_DPS_GET_UUID);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		status = DOVE_STATUS_OK;
	}while(0);

	log_info(PythonUUIDLogLevel, "Exit: %s",
	         DOVEStatusToString(status));

	return status;
}


/*
 ******************************************************************************
 * python_init_uuid_interface --                                    *//**
 *
 * \brief This routine initializes the DPS Cluster Database interface to PYTHON
 *        OBJECTS
 *
 * \param pythonpath - Pointer to the PYTHON Path
 *
 * \return dove_status
 *
 *****************************************************************************/

dove_status python_init_UUID_interface(char *pythonpath)
{
	dove_status status = DOVE_STATUS_OK;
	status = python_functions_init(pythonpath);
	return status;
}

/*
 ******************************************************************************
 * dcs_read_svc_app_uuid --                                    *//**
 *
 * \brief This routine reads the UUID from the file and store it in a global
 * 	  variable.
 *
 * \return dove_status
 *
 *****************************************************************************/
dove_status dcs_read_svc_app_uuid()
{
	dove_status status = DOVE_STATUS_OK;
	FILE *fp = NULL;
	char ptr[40];
	int n;
	char path[100] = {'\0'};

	sprintf( path, "%s/%s", getenv("HOME"), SERVICE_APPLIANCE_UUID_FILE);
	if((fp = fopen(path,"r")) == NULL) {
		log_alert(PythonUUIDLogLevel,"[ERROR] in opening UUID file [%s]",
		          path);
		status = DOVE_STATUS_ERROR;
		return status;
	}
	else {
		// read the file and populate the global
		if(fgets(ptr,40,fp)!= NULL){
			if((n = strlen(ptr)) > UUID_LENGTH)
			{
				log_info(PythonUUIDLogLevel,
				         "UUID [len = %d] has length greater than %d",
				         n,UUID_LENGTH);
				n = UUID_LENGTH;
			}
			fclose(fp);
			memset(dps_node_uuid,0,UUID_LENGTH+1);
			strncpy(dps_node_uuid,ptr,n);
		}else{
			status = DOVE_STATUS_ERROR;
			log_alert(PythonUUIDLogLevel,
			          "Can not read from the uuid file, it may be a Bad File!");
			fclose(fp);
			return status;
		}
	}	
	return status;
}


/* Write the generated UUID to the file ~/.flash/svc.uuid */
dove_status write_uuid_to_file(void)
{

	dove_status status = DOVE_STATUS_OK;
	FILE *fp = NULL;
	char path[100] = {'\0'};

	sprintf( path, "%s/%s", getenv("HOME"), SERVICE_APPLIANCE_UUID_FILE);
	if((fp = fopen(path,"w")) == NULL) {
		log_alert(PythonUUIDLogLevel, "[ERROR] in opening(write) UUID file [%s]",
		          path);
		status = DOVE_STATUS_ERROR;
	}else{
		log_alert(PythonUUIDLogLevel, "[Write UUID to file %s",
		          path);
		log_alert(PythonUUIDLogLevel, "UUID is %s", dps_node_uuid);
		fputs(dps_node_uuid,fp);
		fclose(fp);
	}
	return status;
}

