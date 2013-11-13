/******************************************************************************
** File Main Owner:   DOVE Development Team
** File Description:  The Initialization Code for the DOVE Gateway
**/
/*
{  COPYRIGHT / HISTORY
*
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
*  $Log: init.c $
*  $EndLog$
*
*  PORTING HISTORY
*
}  COPYRIGHT / HISTORY (end)
*/

#include "include.h"
#include "version.h"
#include <uuid/uuid.h>
#include "dgwy_extern.h"
/**
 * \brief The Name of C Library that will be imported by Python
 */
#define DOVE_GATEWAY_LIB_NAME "dgadmin"
#define SERVICE_APPLIANCE_UUID_FILE "/flash/svc.uuid"
#define SERVICE_APPLIANCE_BUILT_VERSION_FILE "/flash/dsa_version"

extern dove_status dps_server_rest_init(void);
extern char g_node_uuid[40];
extern char g_built_version[64];
extern dove_status dgadmin_rest_sync_init(void);


static int appsyscfg_save_tofile(const char *cfgname);


struct mgmt_ip_s 
{
    /**
     * \brief Address family: AF_INET or AF_INET6. 0 means invalid.
     * */
    int  mode; /* 1-dhcp, 0-static*/
    uint16_t family;
    union{
        uint32_t ipv4;
        uint8_t  ipv6[16];
    };
    uint32_t mask;
    uint32_t nexthop;
}__attribute__((packed));
typedef struct mgmt_ip_s mgmt_ip_t;

struct dmc_ip_s 
{
    /**
     * \brief Address family: AF_INET or AF_INET6. 0 means invalid.
     * */
    uint16_t family;
    union{
        uint32_t ipv4;
        uint8_t  ipv6[16];
    };
    uint16_t port;
}__attribute__((packed));
typedef struct dmc_ip_s dmc_ip_t;


struct appsys_cfg
{
    mgmt_ip_t mgmt_ip;
    dmc_ip_t dmc_ip;
}__attribute__((packed));
typedef struct appsys_cfg appsys_cfg_t;


appsys_cfg_t GLB_APPSYS_CFG;
appsys_cfg_t CHK_APPSYS_CFG;

/**
 * \brief Whether the Debug CLI was initialized
 */

static int fDebugCLIInitialized = 0;

/*
 ******************************************************************************
 * DOVEGateway                                                            *//**
 *
 * \defgroup DOVEGateway DPS Server Functionality (User World)
 *
 * The DOVE Gateway Functionality
 * @{
 */

/*
 ******************************************************************************
 * dgwy_ctrl_task_init --                                                 *//**
 *
 * \brief Initializes the DOVE Gateway Control Task. 
 * 		  This routine must be called to setup control task
 * 		  Control TASK is responsible for DPS / DOVE-CONTROLLER interaction.
 *
 * \retval 0 Success
 * \retval !0 Failure
 *
 ******************************************************************************/

static int dgwy_ctrl_task_init(void)
{
    int retval=0;
    
    if (create_sem("CTRL", 0, 0, &gDgwySemId) != OSW_OK)
    {
        printf("Failed to create semaphore \n");
        retval= -1;
        goto init_exit;
    }
            
    /* Create the system task */
    if (create_task("CTRLTASK", 10,
                    OSW_DEFAULT_STACK_SIZE,
                    (task_entry)dgwy_ctrl_task_main,
                    0, &gDgwyTaskId) != OSW_OK) 
    {
        printf("Failed to create task \n");
        goto init_failed;
    }

    if (sem_take(gDgwySemId) != OSW_OK) 
    {
        retval = -1;
        goto init_failed;
    }
    else
    {
        goto init_exit;
    }

init_failed:
    del_sem(gDgwySemId);

init_exit:
    return retval;
}

static int dgwy_ha_listen_task_init(void)
{
    int retval=0;
    
    if (create_sem("HASRV", 0, 0, &gDgwyHASrvSemId) != OSW_OK)
    {
        printf("Failed to create semaphore \n");
        retval= -1;
        goto init_exit;
    }
            
    /* Create the system task */
    if (create_task("HASRV", 10,
                    OSW_DEFAULT_STACK_SIZE,
                    (task_entry)dgwy_ha_listen_task,
                    0, &gDgwyHASrvTaskId) != OSW_OK) 
    {
        printf("Failed to create task \n");
        goto init_failed;
    }

    if (sem_take(gDgwyHASrvSemId) != OSW_OK) 
    {
        retval = -1;
        goto init_failed;
    }
    else
    {
        goto init_exit;
    }

init_failed:
    del_sem(gDgwyHASrvSemId);

init_exit:
    return retval;
}

static int dgwy_ha_task_init(void)
{
    int retval=0;
    
    if (create_sem("HASEM", 0, 0, &gDgwyHASemId) != OSW_OK)
    {
        printf("Failed to create semaphore \n");
        retval= -1;
        goto init_exit;
    }
            
    /* Create the system task */
    if (create_task("HATASK", 10,
                    OSW_DEFAULT_STACK_SIZE,
                    (task_entry)dgwy_ha_task_main,
                    0, &gDgwyHATaskId) != OSW_OK) 
    {
        printf("Failed to create task \n");
        goto init_failed;
    }

    if (sem_take(gDgwyHASemId) != OSW_OK) 
    {
        retval = -1;
        goto init_failed;
    }
    else
    {
        goto init_exit;
    }

init_failed:
    del_sem(gDgwyHASemId);

init_exit:
    return retval;
}




/*
 ******************************************************************************
 * dgwy_ctrl_cli_init --                                                  *//**
 *
 * \brief Initializes the DOVE Gateway Control Netlink channel. 
 * 		  This routine must be called to setup netlink
 *
 *
 * \retval 0 Success
 * \retval !0 Failure
 *
 ******************************************************************************/
static int dgwy_ctrl_init(void)
{
	if(dgwy_ha_listen_task_init())
	{
		printf("Failed to initalize HA LISTENTASK \n");
		return -1;
	}

    if(dgwy_ha_task_init())
	{
		printf("Failed to initalize HA TASK \n");
		return -1;
	}

    if(dgadmin_rest_sync_init())
	{
		printf("Failed to initalize REST SYNC TASK\n");
		return -1;
	}

	if(dgwy_ctrl_task_init())
	{
		printf("Failed to initalize CTRL TASK \n");
		return -1;
	}

    /*
    dgwy_cb_func = dgwy_ctrl_cli_init;

    if(dgwy_nl_send_message(NULL,NULL,NULL)==0)
    {
        return dgwy_getinfo();
    }
    */

    return 0;
}


/*
 ******************************************************************************
 * init_dgadmin --                                                        *//**
 *
 * \brief Initializes the DOVE Gateway Library. This routine must be called
 *        from the PYTHON Script before the DOVE Gateway Library can be used
 *        by PYTHON.
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
init_dgadmin(PyObject *self, PyObject *args)
{
	int fExitOnCtrlC, fDebugCli, ret_val;

	if (!PyArg_ParseTuple(args, "ii", &fExitOnCtrlC, &fDebugCli))
	{
		return Py_BuildValue("i", -1);;
	}

	reset_global_svctable();

    /*
    char buf[1024];
    FILE *fp = NULL;
    memset(buf,0,1024);
    sprintf(buf,"sh /appinit.sh &");
    fp = popen(buf,"r");
    if(fp == NULL)
    {
        log_error(ServiceUtilLogLevel,
                  "APP INIT FAILED\n");
    }
    else
    {
        fclose(fp);
    }
    sleep(10);
    */
    
	ret_val = dgadmin_initialize(fExitOnCtrlC, fDebugCli);
	return Py_BuildValue("i", ret_val);
}

/**
 * \brief The DOVE Gateway Library Methods exposed to Python
 */
static PyMethodDef dove_gateway_lib_methods[] = {
	{"initialize", init_dgadmin, METH_VARARGS, "dpslib doc"},
	{"process_cli_data", process_cli_data, METH_VARARGS, "dpslib doc"},
	{NULL, NULL, 0, NULL}  // end of table marker
};

#if PY_MAJOR_VERSION >= 3
/* module definition structuire */
static struct PyModuleDef dovegatewaylibmodule = {
	PyModuleDef_HEAD_INIT,
	DOVE_GATEWAY_LIB_NAME,
	"dpslib doc", // Maybe NULL
	-1,
	dove_gateway_lib_methods // link to methods table
};
#endif

/*
 ******************************************************************************
 * initdgadmin --                                                         *//**
 *
 * \brief The DOVE Gateway Library Module Initializer. The Module Name must
 *        be "init" followed by the DOVE Gateway Library Name.
 *        For e.g.:
 *        "initdgadmin" is the initialization routine for dgadmin
 *        OR
 *        "inittemp" is the initialization routine for temp
 *        etc. *
 *
 * \retval None
 *
 ******************************************************************************/

PyMODINIT_FUNC initdgadmin(void)
{
#if PY_MAJOR_VERSION >= 3
	return PyModule_Create(&dovegatewaylibmodule);
#else
	Py_InitModule3(DOVE_GATEWAY_LIB_NAME, dove_gateway_lib_methods, NULL);
	return;
#endif
}



int get_appsys_cfg_from_file(char *cfgname, appsys_cfg_t *cfg)
{
    FILE *fp = NULL;
    size_t len = 0;
    char fileName[256];
    int ret = 0;
    int redLen=-1;

    memset(fileName,0,256);
    sprintf(fileName,"/flash/%s",cfgname);

    fp = fopen(fileName,"r");
    if (fp==NULL) 
    {
        log_error(ServiceUtilLogLevel,
                  "Error can't open %s",
                  fileName); 
        ret=-1;
    }
    else
    {
        char *cfgBuffer = (char*)malloc(sizeof(appsys_cfg_t));

        if(cfgBuffer)
        {
            len = sizeof(appsys_cfg_t);
            redLen = fread(cfgBuffer, 1, len, fp);
            if((redLen > 0) && 
               (((size_t)redLen) > sizeof(appsys_cfg_t)))
            {
                log_error(ServiceUtilLogLevel,
                          "%s: Error in config size",
                          __FUNCTION__);
            }
            memcpy(cfg, cfgBuffer, sizeof(appsys_cfg_t));
            free(cfgBuffer);
        }
        else
        {
            log_error(ServiceUtilLogLevel,
                      "Error alloc failed"); 
            ret=-1;
        }

        fclose(fp);
    }
    return ret;
}




/*
 ******************************************************************************
 * dgadm_read_svc_app_uuid --                                    *//**
 *
 * \brief This routine reads the UUID from the file and store it in a global
 * 	  variable.
 *
 * \return dove_status
 *
 *****************************************************************************/
dove_status dgadm_read_svc_app_uuid(void)
{
	dove_status status = DOVE_STATUS_OK;
	FILE *fp = NULL;
	char ptr[40];
	int n;

	if((fp = fopen(SERVICE_APPLIANCE_UUID_FILE,"r")) == NULL) {
		log_alert(ServiceUtilLogLevel,"[ERROR] in opening UUID file [%s]",
		          SERVICE_APPLIANCE_UUID_FILE);
		status = DOVE_STATUS_ERROR;
		return status;
	}
	else 
    {
		// read the file and populate the global
        memset(ptr,0,sizeof(ptr));
		if(fgets(ptr,40,fp)!= NULL){
			if((n = strlen(ptr)) > UUID_LENGTH) {
				log_alert(ServiceUtilLogLevel,
			          "UUID [len = %d] has length greater than %d",
			          n,UUID_LENGTH);
				n = UUID_LENGTH;
			}
			fclose(fp);
            memset(g_node_uuid,0,sizeof(g_node_uuid));
			strncpy(g_node_uuid,ptr,n);
		}
        else
        {
			status = DOVE_STATUS_ERROR;
			log_alert(ServiceUtilLogLevel,
				  "Can not read from the uuid file, it may be a Bad File!");
			fclose(fp);
			return status;
		}
	}	
	return status;
}


/*
 ******************************************************************************
 * dgadm_read_svc_buit_version --                                    *//**
 *
 * \brief This routine reads the UUID from the file and store it in a global
 * 	  variable.
 *
 * \return dove_status
 *
 *****************************************************************************/
dove_status dgadm_read_svc_buit_version(void)
{
	dove_status status = DOVE_STATUS_OK;
	FILE *fp = NULL;
	char ptr[64];
	int n;

	if((fp = fopen(SERVICE_APPLIANCE_BUILT_VERSION_FILE,"r")) == NULL) 
    {
        size_t ln = strlen(odgw_version);
        if ( (ln > 0) && (ln < 60) ) {
            memset(g_built_version, 0, sizeof(g_built_version));
			strncpy(g_built_version, odgw_version, ln);
        } else {		
            log_alert(ServiceUtilLogLevel,"[ERROR] in opening VERSION file [%s]",
                      SERVICE_APPLIANCE_BUILT_VERSION_FILE);
            status = DOVE_STATUS_ERROR;
            return status;
        }
	}
	else 
    {
		// read the file and populate the global
        memset(ptr,0,sizeof(ptr));
		if(fgets(ptr,62,fp)!= NULL){
			if((n = strlen(ptr)) > 60) {
				log_alert(ServiceUtilLogLevel,
			          "Version [len = %d] has length greater than %d",
			          n,60);
				n = 60;
			}
			fclose(fp);
            memset(g_built_version,0,sizeof(g_built_version));
			strncpy(g_built_version,ptr,n);
		}
        else
        {
			status = DOVE_STATUS_ERROR;
			log_alert(ServiceUtilLogLevel,
				  "Can not read from the version file, it may be a Bad File!");
			fclose(fp);
			return status;
		}
	}	
	return status;
}


void dgwy_dmc_monitr_cfg(void)
{
    memset(&CHK_APPSYS_CFG,0,sizeof(CHK_APPSYS_CFG));
    if(0 == get_appsys_cfg_from_file((char*)".appsyscfg",
                                     &CHK_APPSYS_CFG))
    {
        if((GLB_APPSYS_CFG.dmc_ip.ipv4 != CHK_APPSYS_CFG.dmc_ip.ipv4)||
           (GLB_APPSYS_CFG.dmc_ip.port != CHK_APPSYS_CFG.dmc_ip.port))
        {
            get_appsys_cfg_from_file((char*)".appsyscfg",
                                     &GLB_APPSYS_CFG);

            dgwy_ctrl_set_dmc_ipv4(GLB_APPSYS_CFG.dmc_ip.ipv4, 
                                   GLB_APPSYS_CFG.dmc_ip.port);
        }
    }
}


static void check_flash_path(void)
{
    FILE *fp;
    char command[256];

    memset(command,0,256);
    sprintf(command,"mkdir -p /flash/");
   
    fp = popen(command, "r");
    if (fp == NULL) {
        log_error(ServiceUtilLogLevel,"Error popen:%s\n",command);
        printf("/flash not found: check user permission\n"); 
        return;
    }
    pclose(fp);
}

static void generate_uuid(char *strUUID)
{
    uuid_t id;

    uuid_generate(id);
    uuid_unparse(id,strUUID);
}


static void check_appliance_uuid(void)
{
    FILE *fp;
    char uuid[40];

    if (NULL == (fp = fopen(SERVICE_APPLIANCE_UUID_FILE,"r"))) {
        printf("%s NOT FOUND...need to generate UUID \r\n",
               SERVICE_APPLIANCE_UUID_FILE);
        // file not found... generate UUID
        generate_uuid(uuid);
        fp = fopen(SERVICE_APPLIANCE_UUID_FILE,"w");
        if (NULL == fp){
            printf("[ERROR] in creating UUID file : %s\r\n",SERVICE_APPLIANCE_UUID_FILE);
        }
        else {
            fprintf(fp,"%s\r\n",uuid);
    	    fclose(fp);
            chown(SERVICE_APPLIANCE_UUID_FILE, (uid_t) 501, (gid_t) 0);
        }
    }
    else {
        printf("%s exists. No need to generate UUID\r\n",SERVICE_APPLIANCE_UUID_FILE);
        fclose(fp);
    }
}

void set_mgmt_ipv4(appsys_cfg_t *cfg)
{
    if (cfg->mgmt_ip.mode) {
        /* DHCP */
        FILE *fp;
        char command[256];

        memset(command,0,256);
        sprintf(command,"killall -9 dhclient");

        fp = popen(command, "r");
        if (fp == NULL) {
            log_info(ServiceUtilLogLevel,"Error popen:%s\n",command);
        } else {
            pclose(fp);
        }
        
        memset(command,0,256);
        sprintf(command,"/sbin/dhclient APBR&");

        fp = popen(command, "r");
        if (fp == NULL) {
            log_info(ServiceUtilLogLevel,"Error popen:%s\n",command);
        } else {
            pclose(fp);
        }
        return;
    } else if (cfg->mgmt_ip.ipv4 && cfg->mgmt_ip.mask && cfg->mgmt_ip.nexthop) {
        FILE *fp;
        char command[256];
        struct sockaddr_in ipv4;
        struct sockaddr_in maskv4;
        struct sockaddr_in nexthop;
        char ipv4_str[INET_ADDRSTRLEN];
        char ipv4_mask[INET_ADDRSTRLEN];
        char ipv4_nexthop[INET_ADDRSTRLEN];

        memset(command,0,256);
        sprintf(command,"killall -9 dhclient");

        fp = popen(command, "r");
        if (fp == NULL) {
            log_info(ServiceUtilLogLevel,"Error popen:%s\n",command);
        } else {
            pclose(fp);
        }

        ipv4.sin_addr.s_addr = cfg->mgmt_ip.ipv4;
        inet_ntop(AF_INET, &(ipv4.sin_addr), ipv4_str, INET_ADDRSTRLEN);

        maskv4.sin_addr.s_addr = cfg->mgmt_ip.mask;
        inet_ntop(AF_INET, &(maskv4.sin_addr), ipv4_mask, INET_ADDRSTRLEN);

        nexthop.sin_addr.s_addr = cfg->mgmt_ip.nexthop;
        inet_ntop(AF_INET, &(nexthop.sin_addr), ipv4_nexthop, INET_ADDRSTRLEN);

        memset(command,0,256);

        sprintf(command,"/sbin/ifconfig APBR %s netmask %s",ipv4_str,ipv4_mask);

        fp = popen(command, "r");

        if(fp == NULL)
        {
            log_error(ServiceUtilLogLevel,"Error popen:%s\n",command);
            return ;
        }
        pclose(fp);

        sprintf(command,"/sbin/route add default gw %s",ipv4_nexthop);

        fp = popen(command, "r");

        if(fp == NULL)
        {
            log_error(ServiceUtilLogLevel,"Error popen:%s\n",command);
            return ;
        }
        pclose(fp);

    } else {
        /* Handle error */
    }
}

/*
 ******************************************************************************
 * dgadmin_initialize                                                     *//**
 *
 * \brief - Initializes the DOVE Gateway Administrative Service
 *
 * \param[in] fExitOnCtrlC - Whether the Server Process should exit on
 *                           CTRL+C being pressed. In a development environment
 *                           this should be TRUE (1) while in a Production
 *                           Build this should be FALSE (0).
 * \param[in] fDebugCli - Whether the DebugCli should be started.
 *
 * \retval -1: Should never happen - This is an infinite loop.
 *
 ******************************************************************************
 */

int dgadmin_initialize(int fExitOnCtrlC, int fDebugCli)
{
	dove_status status = DOVE_STATUS_OK;
	dove_status dps_srv_status = DOVE_STATUS_OK;
	dps_return_status dps_status = DPS_SUCCESS;

	Py_Initialize();

//	PyEval_InitThreads();

	do
	{
		log_console = 0;

        check_flash_path();
        check_appliance_uuid();

		if (fDebugCli)
		{
			// Initialize the CLI Thread PYTHON Interface. This will
			// start the CLI Thread
			status = python_lib_embed_cli_thread_start(fExitOnCtrlC);
			if (status != DOVE_STATUS_OK)
			{
				fprintf(stderr, "STARTUP_FAILURE: CLI thread cannot be started!!!\n");
				break;
			}

			// Initialize the CLI
			status = dove_gateway_cli_init();
			if (status != DOVE_STATUS_OK)
			{
				fprintf(stderr, "STARTUP_FAILURE: CLI cannot be initialized!!!\n");
				break;
			}

			fDebugCLIInitialized = 1;
		}

		if(OSW_OK != osw_init())
		{
			fprintf(stderr, "STARTUP_FAILURE: OS wrapper cannot be initialized!!!\n");
			status = DOVE_STATUS_OSW_INIT_FAILED;
			break;
		}

		Py_BEGIN_ALLOW_THREADS

		// Initialize the CORE APIs
		dps_srv_status  = fd_process_init();
		if (dps_srv_status != DOVE_STATUS_OK)
		{
			fprintf(stderr, "STARTUP_FAILURE: Communication channel cannot be initialized!!!\n");
			break;
		}

		dps_status = dps_client_init();
		if (dps_status != DPS_SUCCESS)
		{
			fprintf(stderr, "STARTUP_FAILURE: DPS Client cannot be initialized!!!\n");
			break;
		}

        memset(g_node_uuid,0,sizeof(g_node_uuid));
        dgadm_read_svc_app_uuid();
        dgadm_read_svc_buit_version();

		dps_srv_status = dps_server_rest_init();
		if (dps_srv_status != DOVE_STATUS_OK)
		{
			fprintf(stderr, "STARTUP_FAILURE: dps server init failure!!!\n");
			break;
		}

		/* start the ctrl task and nl */
		if (dgwy_ctrl_init() != 0)
		{
			fprintf(stderr,
			        " Error: Ctrl task / Netlink Init error \n");
			//break;
		}
        else
        {
            memset(&GLB_APPSYS_CFG,0,sizeof(GLB_APPSYS_CFG));
            if(0 == get_appsys_cfg_from_file((char*)".appsyscfg",
                                             &GLB_APPSYS_CFG))
            {
                log_info(ServiceUtilLogLevel,
                         "Loaded DMC INFO From /flash/.appsyscfg \n");
                log_info(ServiceUtilLogLevel,
                         "DMC IP 0x%x Port %d \n",GLB_APPSYS_CFG.dmc_ip.ipv4,
                       GLB_APPSYS_CFG.dmc_ip.port);
                dgwy_ctrl_set_dmc_ipv4(GLB_APPSYS_CFG.dmc_ip.ipv4, 
                                       GLB_APPSYS_CFG.dmc_ip.port);

                set_mgmt_ipv4(&GLB_APPSYS_CFG);
            }
        }



		// Start the CORE APIs Communication
		fd_process_start();

		Py_END_ALLOW_THREADS

	}while(0);

	fprintf(stderr, "Quitting Abnormally: Not cleaning up threads!!!\n");

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

static int appsyscfg_save_tofile(const char *cfgname)
{
    FILE *fp = NULL;
    size_t len = 0;
    char fileName[256];
    int ret = 0;

    memset(fileName,0,256);
    sprintf(fileName,"/flash/%s",cfgname);

    fp = fopen(fileName,"w");
    if (fp==NULL) 
    {
        printf("Error can't open %s",
               fileName); 
        ret=-1;
    }
    else
    {
        len = sizeof(GLB_APPSYS_CFG);
        fwrite(&GLB_APPSYS_CFG, len, 1, fp);
        fclose(fp);
    }
    return ret;
}

int
update_dmc(uint32_t ip, int port)
{
    struct sockaddr_in ipv4;
    struct sockaddr_in old_ipv4;
    int old_port;

    /* get current values */
    old_ipv4.sin_addr.s_addr = GLB_APPSYS_CFG.dmc_ip.ipv4;
    old_port =  GLB_APPSYS_CFG.dmc_ip.port;
    GLB_APPSYS_CFG.dmc_ip.ipv4 = 0;
    GLB_APPSYS_CFG.dmc_ip.port = 0;

    printf("%s:%d old:%x mode:%x\n",__FUNCTION__,__LINE__,
           old_port, port);

    //inet_aton(ip_str, &ipv4.sin_addr);
    ipv4.sin_addr.s_addr = ip;

    if((old_port==port) &&
       (ipv4.sin_addr.s_addr==old_ipv4.sin_addr.s_addr))
    {
        /* same input */
        //return 0;
    }

    GLB_APPSYS_CFG.dmc_ip.ipv4=ipv4.sin_addr.s_addr;
    GLB_APPSYS_CFG.dmc_ip.port = port;

    appsyscfg_save_tofile(".appsyscfg");

    return 0;
}

int
update_mgmt_ipv4(int mode, uint32_t ipv4, 
                 uint32_t ipv4_mask, uint32_t nexthop)
{

    GLB_APPSYS_CFG.mgmt_ip.mode = mode;
    GLB_APPSYS_CFG.mgmt_ip.ipv4 = ipv4;
    GLB_APPSYS_CFG.mgmt_ip.mask = ipv4_mask;
    GLB_APPSYS_CFG.mgmt_ip.nexthop = nexthop;

    appsyscfg_save_tofile(".appsyscfg");

    set_mgmt_ipv4(&GLB_APPSYS_CFG);

    return 0;
}

void show_dmc_info(void)
{
    if(GLB_APPSYS_CFG.dmc_ip.ipv4)
    {
        struct sockaddr_in sa;
        sa.sin_addr.s_addr = GLB_APPSYS_CFG.dmc_ip.ipv4;
        show_print("\n\tDMC IPV4    : %s", inet_ntoa(sa.sin_addr));
        show_print("\tDMC PORT    : %u", GLB_APPSYS_CFG.dmc_ip.port);
    }
}

/** @} */
