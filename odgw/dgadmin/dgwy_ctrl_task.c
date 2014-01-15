/******************************************************************************
** File Main Owner:   DOVE Development Team
** File Description:  The code for DOVE Gateway Control Task
**/
/*
{  COPYRIGHT / HISTORY
*
*  COPYRIGHT NOTICE
*  Copyright (c) IBM, Inc.  2012 -
*  All rights reserved
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
*  $Log: dgwy_ctrl_task.c $
*  $EndLog$
*
*  PORTING HISTORY
*
}  COPYRIGHT / HISTORY (end)
*/

#include "include.h"

/*
 ******************************************************************************
 * 																		  *//**
 * \brief GLOBAL VARIABLES    
 *
 ******************************************************************************/
int64_t     gDgwySemId; 
int64_t     gDgwyTaskId;
int64_t     gDgwyMsgId;
int64_t     gDgwyTimerListId;

extern int char_dev_exit(void);
extern int dgwy_device_poll(void);
extern int dgwy_ctrl_nl_init(void);
extern void update_domain_bcast(void);
extern dove_status api_dgadmin_init_appliance(void);
extern int dgwy_ctrl_set_peer_state(uint32_t peer_ipv4, uint8_t state);
extern void dgwy_dmc_monitr_cfg(void);
extern void dgwy_dmc_heartbeat(void);
extern void retry_dmc_register(void);
extern void check_register_tunnel(void);
extern void retry_get_dcs_seed_info(void);
extern uint8_t g_tunnel_reg_status;
extern int dgwy_chardev_fd;
extern int dgwy_getinfo(void);
extern void dgwy_get_vxlan_port(void);

#define DGWY_CTRL_MON_TIMER_INTVL 15


void dgwy_ctrl_task_main (UINT1 *pDummy)
{
    dgwy_ctrl_msg_info_t	*ctrlMsg  		= NULL;
    unsigned int 			listenEvent   = 0;
    unsigned int 			recvEvent     = 0;
    int                     tunnel_reg_delay_cnt=0;
    int                     bcast_update_delay_cnt=0;

    if(dgwy_ctrl_nl_init() != 0)
    {
        printf("%s:error when init nl\n",__FUNCTION__);
        return;
    }

    if (sem_give(gDgwySemId) != OSW_OK) 
	{
		return;
    }
    if (create_queue("CMSG", OSW_MAX_Q_MSG_LEN, 10, &gDgwyMsgId) != OSW_OK) 
    {
        printf("%s:error when create queue\n",__FUNCTION__);
        return;
    }

    if (create_timer("CTRLTASK", DGWYCTRLTASK_TIMER_EVENT, 
                     NULL, 0, &gDgwyTimerListId) != OSW_OK) 
    {
        printf("%s:error when create_timer\n",__FUNCTION__);
        return;
    }

    /* start the timer */
    start_timer(gDgwyTimerListId, DGWY_CTRL_MON_TIMER_INTVL, 0);

    listenEvent = DGWYCTRLTASK_TIMER_EVENT | DGWYCTRLTASK_MSG_EVENT; 

    log_info(ServiceUtilLogLevel,
             "INIT APPLIANCE ");
    api_dgadmin_init_appliance();


	while (1) 
    {
        /* poll for any dps lookup list */
        dgwy_device_poll();

        if(recv_event(gDgwyTaskId, listenEvent, OSW_NO_WAIT, &recvEvent) == OSW_ERROR)
        {
            continue;
        }
    
        if(recvEvent & DGWYCTRLTASK_TIMER_EVENT)
        {
            if(dgwy_chardev_fd==-1)
            {
                log_info(ServiceUtilLogLevel,
                         "dgwy_chardev_fd not opened yet: retry\n");
                dgwy_getinfo();
            }
     
            start_timer(gDgwyTimerListId, DGWY_CTRL_MON_TIMER_INTVL, 0);

            if(bcast_update_delay_cnt++ > 2)
            {
                bcast_update_delay_cnt=0;
                update_domain_bcast();

            }

            dgwy_dmc_heartbeat();
            /* retry dmc register if needed */
            retry_dmc_register();
            dgwy_dmc_monitr_cfg();
            retry_get_dcs_seed_info();
           /*
            * DO tunnel register periodic
            * if(g_tunnel_reg_status==0)*/
            if(tunnel_reg_delay_cnt++ > 4)
            {
                tunnel_reg_delay_cnt=0;
                check_register_tunnel();
                dgwy_get_vxlan_port();
            }
        }
        else if(recvEvent & DGWYCTRLTASK_MSG_EVENT)
        {
            while(queue_receive(gDgwyMsgId, (char *)&ctrlMsg, 
                                sizeof(ctrlMsg), OSW_NO_WAIT) == OSW_OK)
            {
                switch (ctrlMsg->type)
                {   
                    case DGWY_PEER_UP:
                    case DGWY_PEER_DOWN:
                    {
                        dgwy_ctrl_peer_info_t *peer_msg =
                            (dgwy_ctrl_peer_info_t*)ctrlMsg;

                        dgwy_ctrl_set_peer_state(peer_msg->peer_ipv4,
                                                 peer_msg->state);
                        break;
                    }
                    case DGWY_GET_INFO:
                    {
                        /* request for latest if info */
						/*
                        sysrcv_msg_info_t *resp = 
                            (sysrcv_msg_info_t*) malloc (sizeof(sysrcv_msg_info_t));
                        if(resp)
                        {
                           resp->type = DGWY_INFO_RESP;
                           if(queue_send(ctrlMsg->reqMsgId, (char *)&resp, sizeof(resp)) == OSW_OK)
                           {
                               send_event(ctrlMsg->reqTskId, DGWYCTRLTASK_MSG_EVENT);
                           }
                           else
                           {
                               free(resp);
                           }
                        }
						*/

                        break;
                    }
                    default:
                    {

                    }
                }
                if(ctrlMsg)
                {
                    free(ctrlMsg);
                }
            }
        }
        else
        {
            /* unknown event  */
            printf("%s:Unknown event \n",__FUNCTION__);
        }
    } /* while */
}

