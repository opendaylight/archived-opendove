/******************************************************************************
** File Main Owner:   DOVE Development Team
** File Description:  This file exposes the dps server as a library that can
**                    be consumed by a Python Script. It provides the APIs
**                    needed to communicate with the Python Scripts using
**                    both Extending and Embedding Techniques.
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
*  $Log: python_library.c $
*  $EndLog$
*
*  PORTING HISTORY
*
}
*/

#include "include.h"

/**
 * \brief Whether the CLI has been initialized
 */

static int fCli_Initialized = 0;

/**
 * \brief The Log Level
 */
int PythonLibLogLevel = DPS_SERVER_LOGLEVEL_INFO;

/**
 * \brief The module location defines for the CLI Handling API
 */
#define PYTHON_MODULE_FILE_CLI "python_cli_interface"
#define PYTHON_MODULE_CLASS_CLI "python_cli_interface"
#define PYTHON_MODULE_FUNCTION_CLI_START "cli_start"

/**
 * \brief The CLI Handling PYTHON API Structure
 */

typedef struct python_dps_cli_s{
	/**
	 * \brief The Object Instance
	 */
	PyObject *instance;
	/*
	 * \brief The function that starts the CLI thread
	 */
	PyObject *cli_start;
}python_dps_cli_t;

/*
 * \brief The CLI Handler PYTHON Interface (Embed)
 */

static python_dps_cli_t Python_Lib_Cli = {NULL, NULL};

/*
 ******************************************************************************
 * Python Library                                                         *//**
 *
 * \addtogroup DPSServer
 * @{
 * \defgroup PythonInterface Python and C (Extend and Embed) Interface
 * @{
 * Python and C interaction using Embedding and Extending Techniques
 */

/*
 ******************************************************************************
 * process_cli_data --                                                    *//**
 *
 * \brief This is the routine that the PYTHON Scripts must call to execute CLI
 *        functions
 *
 * \param[in] self  PyObject
 * \param[in] args  The input MUST of the structure dps_client_data_t and
 *                  represented as a byte array
 *
 * \retval 0 Success
 * \retval -1 Failure
 *
 ******************************************************************************/
PyObject *process_cli_data(PyObject *self, PyObject *args)
{
	char *data;
	int size;
	int ret = DOVE_STATUS_BAD_ADDRESS;
	void *cli_data;
	PyObject *ret_val;

	//Py_BEGIN_ALLOW_THREADS
	log_debug(PythonLibLogLevel, "Enter");

	if (!PyArg_ParseTuple(args, "s#", &data, &size))
	{
		log_debug(PythonLibLogLevel, "NO DATA! - NULL");
	}
	else
	{
		log_debug(PythonLibLogLevel, "Data Present!");
		cli_data = (void *)data;
		// Invoke the CLI callback here
		ret = dps_server_cli_callback(cli_data);
	}
	ret_val = Py_BuildValue("i", ret);
	log_debug(PythonLibLogLevel, "Exit");

	//Py_END_ALLOW_THREADS
	return ret_val;
}

/*
 ******************************************************************************
 * python_lib_get_instance --                                            *//**
 *
 * \brief This routine gets a handle to the instance of the PYTHON Class
 *
 * \param pythonpath - The Location of the PYTHON Module
 * \param module_file - The Module File Name
 * \param class_name - The Class Name
 * \param function_name - The Function Name
 * \param pyargs - The Arguments to Initialize the Instance of the Class With
 * \param ppy_instance - Location where the PYTHON Instance reference
 *                       will be stored
 *
 * \retval 0 DOVE_STATUS_OK
 * \retval Other Failure
 *
 *****************************************************************************/

dove_status python_lib_get_instance(char *pythonpath,
                                    const char *module_file,
                                    const char *class_name,
                                    PyObject *pyargs,
                                    PyObject **ppy_instance)
{
	PyObject *pymodule, *pyclass;
	dove_status ret_val;
	char *my_python_path;
	int n;
	char path[MAX_PYTHON_INTERPRETOR_PATH];
	char complete_path[MAX_PYTHON_INTERPRETOR_PATH];
	PyGILState_STATE gstate;

	log_info(PythonLibLogLevel, "Enter");

	pymodule = pyclass = NULL;
	*ppy_instance = NULL;

	gstate = PyGILState_Ensure();

	do
	{
		ret_val = DOVE_STATUS_NOT_SUPPORTED;

		if (pythonpath == NULL)
		{
			// Assume the path is the pwd
			my_python_path = getcwd(path, MAX_PYTHON_INTERPRETOR_PATH);
			if (my_python_path == NULL)
			{
				log_emergency(PythonLibLogLevel, "ERROR! Cannot get current directory: Error %d\n",
				              errno);
				break;
			}
		}
		else
		{
			my_python_path = pythonpath;
		}
		sprintf(complete_path,"PYTHONPATH=%s", my_python_path);
		log_info(PythonLibLogLevel, "PYTHONPATH set to %s", my_python_path);

		// Add the location of the PYTHON Script to the
		n = putenv(complete_path);
		if (n != 0)
		{
			log_emergency(PythonLibLogLevel, "ERROR! Cannot set PythonPath to %s: Error %d\n",
			              my_python_path, errno);
			break;
		}


		log_info(PythonLibLogLevel, "Importing Module File %s", module_file);
		pymodule = PyImport_ImportModule(module_file);
		if (pymodule == NULL)
		{
			log_emergency(PythonLibLogLevel,
			              "ERROR! PyImport_ImportModule from %s failed...\n",
			              module_file);
			break;
		}
		log_info(PythonLibLogLevel, "Importing Module Class %s", class_name);
		pyclass = PyObject_GetAttrString(pymodule, class_name);
		if (pyclass == NULL)
		{
			log_emergency(PythonLibLogLevel,
			              "ERROR! PyObject_GetAttrString %s failed...\n",
			              class_name);
			break;
		}
		log_info(PythonLibLogLevel, "Creating instance of Module Class %s", class_name);
		*ppy_instance = PyEval_CallObject(pyclass, pyargs);
		if (*ppy_instance == NULL)
		{
			log_emergency(PythonLibLogLevel,
			              "ERROR! PyEval_CallObject() Instance Creation failed...\n");
			break;
		}
		log_info(PythonLibLogLevel, "Created instance of Module Class %s, %p",
		         class_name, *ppy_instance);

		ret_val = DOVE_STATUS_OK;
	} while (0);

	// Cleanup
	if ((ret_val != DOVE_STATUS_OK) && (*ppy_instance != NULL))
	{
		Py_DECREF(*ppy_instance);
		*ppy_instance = NULL;
	}
	if (pyclass)
	{
		Py_DECREF(pyclass);
		pyclass = NULL;
	}
	if (pymodule)
	{
		Py_DECREF(pymodule);
		pymodule = NULL;
	}

	PyGILState_Release(gstate);

	log_info(PythonLibLogLevel, "Exit: %s", DOVEStatusToString(ret_val));

	return ret_val;
}

/*
 ******************************************************************************
 * python_lib_embed_initialize_cli --                                     *//**
 *
 * \brief This routine initializes the DPS Message Handling Python Interface
 *
 * \param pythonpath - Pointer to the Python Path
 * \param fExitOnCtrlC - Whether the CLI Thread should exit on ctrl+c
 *
 * \retval 0 Success
 * \retval -1 Failure
 *
 *****************************************************************************/

static dove_status python_lib_embed_initialize_cli(char *pythonpath,
                                                   int fExitOnCtrlC)
{
	dove_status status;
	PyObject *pyargs;

	log_info(PythonLibLogLevel, "Enter");

	do
	{
		pyargs = Py_BuildValue("(i)", fExitOnCtrlC);
		if (pyargs == NULL)
		{
			log_emergency(PythonLibLogLevel,
			              "ERROR! Py_BuildValue () failed...\n");
			status = DOVE_STATUS_NO_RESOURCES;
			break;
		}
		status = python_lib_get_instance(pythonpath,
		                                 PYTHON_MODULE_FILE_CLI,
		                                 PYTHON_MODULE_CLASS_CLI,
		                                 pyargs,
		                                 &Python_Lib_Cli.instance);
		Py_DECREF(pyargs);
		if (status != DOVE_STATUS_OK)
		{
			break;
		}
		Python_Lib_Cli.cli_start = PyObject_GetAttrString(Python_Lib_Cli.instance,
		                                                  PYTHON_MODULE_FUNCTION_CLI_START);
		if (Python_Lib_Cli.cli_start == NULL)
		{
			log_emergency(PythonLibLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_MODULE_FUNCTION_CLI_START);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}
		status = DOVE_STATUS_OK;
	}while(0);

	log_info(PythonLibLogLevel, "Exit: %s", DOVEStatusToString(status));

	return status;
}

/*
 ******************************************************************************
 * cli_thread --                                                          *//**
 *
 * \brief This is the CLI Thread
 *
 * \param params Input to the Thread (Whether to exit on CTRL+C)
 *
 * \retval 0 Success
 * \retval -1 Failure
 *
 * \todo Determine if this routine is needed.
 *
 *****************************************************************************/

void * cli_thread(void *params)
{
	PyObject *strret, *strargs;
	int intret;
	int fExitOnCtrlC = (int)(size_t)params;
	dove_status status;
	PyGILState_STATE gstate;

	log_debug(PythonLibLogLevel, "Enter");

	gstate = PyGILState_Ensure();
	do
	{
		status = python_lib_embed_initialize_cli(NULL, fExitOnCtrlC);
		if (status != DOVE_STATUS_OK)
		{
			show_print("cli_thread initialize Failure %s",
			           DOVEStatusToString(status));
			break;
		}

		strargs = Py_BuildValue("()");
		if(strargs == NULL)
		{
			show_print("cli_thread Py_BuildValue returns NULL");
			break;
		}
		strret = PyEval_CallObject(Python_Lib_Cli.cli_start, strargs);
		Py_DECREF(strargs);
		if (strret == NULL)
		{
			show_print("cli_thread PyEval_CallObject returns NULL");
			break;
		}
		PyArg_Parse(strret, "i", &intret);
		Py_DECREF(strret);
	}while(0);

	PyGILState_Release(gstate);
	log_debug(PythonLibLogLevel, "Exit");
	return ((void *) NULL);
}

/*
 ******************************************************************************
 * python_lib_embed_cli_thread_start --                                   *//**
 *
 * \brief This routine gets the PYTHON request to send a message.
 *
 * \param fExitOnCtrlC Whether to exist on CTRL+C
 *
 * \return dove_status
 *
 *****************************************************************************/

dove_status python_lib_embed_cli_thread_start(int fExitOnCtrlC)
{
	dove_status status;
	PyObject *strret, *strargs;
	int intret;
	PyGILState_STATE gstate;

	log_debug(PythonLibLogLevel, "Enter");

	gstate = PyGILState_Ensure();
	do
	{
		status = python_lib_embed_initialize_cli(NULL, fExitOnCtrlC);
		if (status != DOVE_STATUS_OK)
		{
			log_emergency(PythonLibLogLevel, "Failure %s",
			              DOVEStatusToString(status));
			break;
		}

		strargs = Py_BuildValue("()");
		if (strargs == NULL)
		{
			log_emergency(PythonLibLogLevel, "Py_BuildValue returns NULL");
			status = DOVE_STATUS_NO_RESOURCES;
		}
		strret = PyEval_CallObject(Python_Lib_Cli.cli_start, strargs);
		Py_DECREF(strargs);

		if (strret == NULL)
		{
			log_emergency(PythonLibLogLevel, "PyEval_CallObject returns NULL");
			status = DOVE_STATUS_NO_RESOURCES;
		}
		PyArg_Parse(strret, "i", &intret);
		Py_DECREF(strret);

		fCli_Initialized = 1;
	}while(0);

	PyGILState_Release(gstate);
	log_debug(PythonLibLogLevel, "Exit :%s", DOVEStatusToString(status));

	return status;

}

/** @} */
/** @} */
