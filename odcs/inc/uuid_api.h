/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
 * File:   uuid_api.h
 * Author: Amitabha Biswas
 *
 * Created on Feb 25, 2012, 12:32 PM
 */


#ifndef _UUID_API_
#define _UUID_API_

/**
 * \brief The Log Level of the UUID
 */
extern int PythonUUIDLogLevel;

extern char dps_node_uuid[37];

/**
 * \brief File which stores the service appliance UUID.
 * 	  This UUID is to be used in all the requests.
 */
#define SERVICE_APPLIANCE_UUID_FILE "/flash/svc.uuid"

/*
 ******************************************************************************
 * python_init_UUID_interface --                                    *//**
 *
 * \brief This routine initializes the DPS UUID interface to PYTHON
 *        OBJECTS
 *
 * \param pythonpath - Pointer to the PYTHON Path
 *
 * \return dove_status
 *
 *****************************************************************************/

dove_status python_init_UUID_interface(char *pythonpath);

void show_uuid(void);

void dps_init_uuid(void);
dove_status write_uuid_to_file(void);


/*
 ******************************************************************************
 * dps_get_uuid                                                   *//**
 *
 * \brief - This function get the uuid and store it in the parameter return_uuid. 
 *
 * \param return_uuid   return result
 *
 * \retval DOVE_STATUS_OK Cluster Node was added to collection
 * \retval DOVE_STATUS_NO_RESOURCES Cluster Node could not be added
 *
 ******************************************************************************
 */
dove_status dps_get_uuid(char *return_uuid);

/*
 ******************************************************************************
 * dps_read_svc_app_uuid --                                    *//**
 *
 * \brief This routine reads the UUID from the file and store it in a global
 * 	  variable.
 *
 * \return dove_status
 *
 *****************************************************************************/
dove_status dps_read_svc_app_uuid();



#endif
