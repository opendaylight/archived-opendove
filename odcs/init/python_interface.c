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
 * \brief The Log Level
 */
int PythonLibLogLevel = DPS_SERVER_LOGLEVEL_WARNING;

/**
 * \brief The module location defines for the CLI Handling API
 */

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

/** @} */
/** @} */
