/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 * */


#include "include.h"
#include "dgwy_extern.h"

/*
 ******************************************************************************
 * 																		  *//**
 * \brief GLOBAL VARIABLES    
 *
 ******************************************************************************/
int64_t     gDgwyHASemId; 
int64_t     gDgwyHATaskId;
int64_t     gDgwyHAMsgId;
int64_t     gDgwyHATimerListId;

int64_t     gDgwyHASrvSemId; 
int64_t     gDgwyHASrvTaskId;
int64_t     gDgwyHASrvMsgId;
int64_t     gDgwyHASrvTimerListId;

#define DGWY_HA_MON_TIMER_INTVL     10      /* sec */
#define DGWY_HA_KEEP_ALIVE_TIMER    5       /* sec */
#define DGWY_HA_KEEP_ALIVE_TIMOUT   30      /* sec */
#define DGWY_HA_LISTEN_PORT         51978   
#define MAX_PEERS                   2       
#define MAX_POLLFD 8
   
extern int get_all_mac_addess(char *buf);

struct pollfd ha_fds[MAX_POLLFD];
int idxPoll = 0;
dgwy_ha_peer_table_t ha_peers[MAX_PEERS];

char vlan_ha_local_msg[VLAN_HA_MSG_LEN];
char vlan_ha_peer_msg[VLAN_HA_MSG_LEN];

int is_mac_found_in_peer(char * mac)
{
    int retval=0;
    if(*vlan_ha_peer_msg)
    {
        int i=0;
        char *ptr = vlan_ha_peer_msg+1;
        for(i=0; i<(*vlan_ha_peer_msg); i++)
        {
            if(memcmp(ptr,mac,6)==0)
            {
                retval=1;
                log_debug(ServiceUtilLogLevel,
                          ">>> %x %x %x %x %x %x <<< \r\n",
                          mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
                break;
            }
            ptr = ptr + 6;
        }
    }
    return retval;
}

int add_fds_list(int fd)
{
    int i=0;
    int emptyIdx=-1;
    if(idxPoll>=MAX_POLLFD)
    {
        log_error(ServiceUtilLogLevel,
                  "%s: Poll FD list full \n",
                  __FUNCTION__);
        return -1;
    }

    for(i=0; i<MAX_POLLFD; i++)
    {
        if((emptyIdx==-1) &&
           (ha_fds[i].fd == 0))
        {
            emptyIdx=i;
            break;
        }
    }

    if(emptyIdx!=-1)
    {
        ha_fds[emptyIdx].fd = fd;
        ha_fds[emptyIdx].events = POLLIN;
        idxPoll++;
        log_info(ServiceUtilLogLevel,
                 "Aded accept fd %d idxPoll=%d\n",
                 fd, idxPoll);
    }
    else
    {
        log_error(ServiceUtilLogLevel,
                  "%s: Poll FD list full \n",
                  __FUNCTION__);
    }
    return 0;
}

int rem_fds(int fd)
{
    int i=0;

    if(fd<=0)
    {
        return 0;
    }

    for(i=0; i<MAX_POLLFD; i++)
    {
        if(ha_fds[i].fd == fd)
        {
            ha_fds[i].fd=0;
            idxPoll--;
            break;
        }
    }

    log_info(ServiceUtilLogLevel,"Removed fd %d idxPoll=%d\n",
             fd, idxPoll);

    return 0;
}

int connect_peer(uint32_t peer_ipv4,uint32_t local_ipv4)
{
    int sd = 0;
    struct sockaddr_in peer;
    struct sockaddr_in local;

    sd = socket (AF_INET,SOCK_STREAM,0);
    if(sd < 0)
    {
        return -1;
    }

    /* bind to local devnet ipv4 */
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = (local_ipv4); /*sip*/
    local.sin_port = 0; /* any sport */
    bind(sd, (struct sockaddr*)&local, sizeof(local));

    peer.sin_family = AF_INET;
    peer.sin_port = htons(DGWY_HA_LISTEN_PORT); /*dport*/
    peer.sin_addr.s_addr = (peer_ipv4); /* dip */


    log_debug(ServiceUtilLogLevel,
              "call connect %x\n",peer.sin_addr.s_addr);

    if(connect(sd,(struct sockaddr *)&peer, sizeof(peer)) < 0)
    {
        log_error(ServiceUtilLogLevel,
                  "%s: Connect failed ipv4 0x%x\n",
                  __FUNCTION__,peer_ipv4);
        close(sd);
        return -1;
    }
    return sd;
}

int keep_alive_peers(void)
{
    int i=0;

    log_debug(ServiceUtilLogLevel,
              "keep_alive_peers\n");

    if(*vlan_ha_local_msg < 4)
    {
        get_all_mac_addess(vlan_ha_local_msg);
    }

    for(i=0; i<MAX_PEERS; i++)
    {
        /* does it exist?  */
        if(ha_peers[i].peer_ipv4 == 0)
        {
            continue;
        }
             
        if(ha_peers[i].fdWrite > 0)
        {
            int ret = 0; int wrote=0;
            /*ret = sprintf(vlan_ha_local_msg,"AMEYA-%x-AMEYA",ha_peers[i].local_ipv4);*/
            ret = (*vlan_ha_local_msg * 6)+1;
            if(ret > 0)
            {
                wrote = send(ha_peers[i].fdWrite,vlan_ha_local_msg,ret,0);
                if(wrote != ret)
                {
                    int fd=0;
                    /* close fd and try to reconnect */
                    close(ha_peers[i].fdWrite);
                    ha_peers[i].fdWrite = 0;
                    fd = connect_peer(ha_peers[i].peer_ipv4,
                                      ha_peers[i].local_ipv4);
                    if(fd>0)
                    { 
                        int flags=0;
                        flags = flags|O_NONBLOCK;
                        fcntl(fd, F_SETFL, flags);
                        ha_peers[i].fdWrite = fd;
                        wrote = send(ha_peers[i].fdWrite,vlan_ha_local_msg,ret,0);
                    }
                }
                log_debug(ServiceUtilLogLevel,
                          "Keep alive wrote %d\n",wrote);
            }

            if(ha_peers[i].state == 2)
            {
                clock_t now = clock();
                int timsince = (now - ha_peers[i].last) / CLOCKS_PER_SEC;
                if(timsince > DGWY_HA_KEEP_ALIVE_TIMOUT)
                {
                    dgwy_ctrl_peer_info_t *msg = 
                        (dgwy_ctrl_peer_info_t*)malloc(sizeof(dgwy_ctrl_peer_info_t));
                    if(msg)
                    {

                        log_info(ServiceUtilLogLevel,
                                 "Peer DOWN:%x [time : %d]", 
                                 ha_peers[i].peer_ipv4, timsince);
                        /* missing keep alive from peer */
                        ha_peers[i].state = 1; /* mark inactive */
                        
                        /* notify ctrl task */
                        msg->msg_info.type = DGWY_PEER_DOWN;
                        msg->peer_ipv4 = ha_peers[i].peer_ipv4;
                        msg->state = 1; /* inactive */
                        if (queue_send(gDgwyMsgId, (char *)&msg, sizeof(msg)) != OSW_OK)
                        {
                            log_error(ServiceUtilLogLevel, "Send Message error");
                            free(msg);
                        }
                        else if(send_event(gDgwyTaskId, DGWYCTRLTASK_MSG_EVENT) != OSW_OK)
                        {
                            log_error(ServiceUtilLogLevel, "Send EVENT error");
                        }
                    }
                }
                else
                {
                    log_debug(ServiceUtilLogLevel,
                              "Peer alive %x [time : %d]", 
                              ha_peers[i].peer_ipv4, timsince);
                }
            }
        }
        else
        {
            int fd=0;
            ha_peers[i].fdWrite = 0;
            fd = connect_peer(ha_peers[i].peer_ipv4,
                              ha_peers[i].local_ipv4);
            if(fd>0)
            { 
                int flags=0;
                flags = flags|O_NONBLOCK;
                fcntl(fd, F_SETFL, flags);
                ha_peers[i].fdWrite = fd;
            }
        }
    }

    return 0;
}


void update_peer_clock(uint32_t peer_ipv4)
{
    int i=0;

    log_debug(ServiceUtilLogLevel,
              "peer %x\n",peer_ipv4);

    for(i=0; i<MAX_PEERS; i++)
    {
        /* does it exist?  */
        if(peer_ipv4 == ha_peers[i].peer_ipv4)
        {
            log_debug(ServiceUtilLogLevel,
                      "%x state %d\n",
                      peer_ipv4, ha_peers[i].state);

            ha_peers[i].last = clock();
            if(ha_peers[i].state == 1)
            {
                /* peer was inactive 
                 * Send active notification to ctrl task */
                dgwy_ctrl_peer_info_t *msg = 
                    (dgwy_ctrl_peer_info_t*)malloc(sizeof(dgwy_ctrl_peer_info_t));
                if(msg)
                {
                    msg->msg_info.type = DGWY_PEER_UP;
                    msg->peer_ipv4 = ha_peers[i].peer_ipv4;
                    msg->state = 2; /* active */

                    log_debug(ServiceUtilLogLevel,
                              "%x state %d\n",
                              peer_ipv4, ha_peers[i].state);

                    if (queue_send(gDgwyMsgId, (char *)&msg, sizeof(msg)) != OSW_OK)
                    {
                        log_error(ServiceUtilLogLevel, "Send Message error");
                        free(msg);
                    }
                    else if(send_event(gDgwyTaskId, DGWYCTRLTASK_MSG_EVENT) != OSW_OK)
                    {
                        log_error(ServiceUtilLogLevel, "Send EVENT error");
                    }
                }
            }
            ha_peers[i].state = 2;
        }
    }
}


int process_peer_msg(char *buf, int buflen, int fd)
{
    /* get peer info */
    struct sockaddr_storage peer;
    int ret = 0;
    socklen_t  len ;

    len = sizeof(peer);
    ret = getpeername(fd, (struct sockaddr*)&peer, &len);

    log_debug(ServiceUtilLogLevel,
              "getpeername ret %d errno:%d buflen %d\n",
              ret, errno, buflen);

    if(ret == -1)
    {
        log_error(ServiceUtilLogLevel,
                  "%s:Failed getpeername errno=%d\n",
                  __FUNCTION__, errno);
        return -1;
    }
    if(peer.ss_family == AF_INET)
    {
        struct sockaddr_in *s = (struct sockaddr_in *)&peer;
        uint32_t peer_ipv4 = (s->sin_addr.s_addr);
        update_peer_clock(peer_ipv4);
    }
    else
    {
        /* AF_INET6 */
        /* XXX TODO IPV6 later */
    }
    return 0;
}


int update_peers_table(uint32_t peer_ipv4, uint32_t local_ipv4)
{
    int i=0;
    int emptyIdx=-1;

    log_debug(ServiceUtilLogLevel,
              "peer %x local %x\n",
              peer_ipv4,local_ipv4);

    for(i=0; i<MAX_PEERS; i++)
    {
        /* does it exist?  */
        if(peer_ipv4 == ha_peers[i].peer_ipv4)
        {
            return 0;
        }
        if((emptyIdx==-1) &&(ha_peers[i].state==0))
        {
            emptyIdx=i;
        }
    }

    log_debug(ServiceUtilLogLevel,
              "emptyIdx %d\n",emptyIdx);

    if(emptyIdx>=0)
    {
        int fd=0;
        ha_peers[emptyIdx].peer_ipv4 = peer_ipv4;
        ha_peers[emptyIdx].local_ipv4 = local_ipv4;
        ha_peers[emptyIdx].state = 1; /* inactive */
        ha_peers[emptyIdx].last = clock();
        fd = connect_peer(peer_ipv4, local_ipv4);

        log_debug(ServiceUtilLogLevel,
                  "connect fd %d\n",fd);
        if(fd>0)
        {
            int flags=0;

            flags = flags|O_NONBLOCK;
            fcntl(fd, F_SETFL, flags);
            ha_peers[emptyIdx].fdWrite = fd;
        }
    }
    else
    {
        log_error(ServiceUtilLogLevel,
                  "%s: No empty spot\n",
                  __FUNCTION__);
        return -1;
    }
    return 0;
}

int dgwy_ha_state_poll(void)
{
    int rv =0;
    int i=0;
    struct pollfd fds[MAX_POLLFD];
    struct pollfd *pfds = fds;
    int cnt=0;

    log_debug(ServiceUtilLogLevel,"idxPoll=%d\n",
              idxPoll);

    if(idxPoll<=0)
    {
        return -1;
    }

    for(i=0; i<MAX_POLLFD; i++)
    {
        if(ha_fds[i].fd)
        {
            pfds[cnt].fd = ha_fds[i].fd;
            pfds[cnt].events = POLLIN;
            cnt++;
        }
    }
    
    rv = poll(pfds, cnt, 500 /* 500 ms wait*/);
    if (rv == -1) 
    {
        log_error(ServiceUtilLogLevel,
                  "%s: Poll error \n",
                  __FUNCTION__);
    }
    else if (rv == 0) 
    {
        
    }
    else 
    {
        log_debug(ServiceUtilLogLevel,"GOT POOL IN\n");
        for(i=0; i<cnt; i++)
        {
            int ret = 0;
            char buf[1024];

            log_debug(ServiceUtilLogLevel,
                      "pfd revent %d\n",pfds[i].revents);

            if(pfds[i].revents == 0)
                continue;

            if(pfds[i].revents != POLLIN)
            {
                log_error(ServiceUtilLogLevel,
                          "%s: Event not pollin \n",
                          __FUNCTION__);
                continue;
            }

            ret = read(pfds[i].fd, buf, 1024);
            if(ret>0)
            {
                /* got keep alive */
                if(*vlan_ha_peer_msg < 4)
                {
                    if((ret >= ((*buf)*6)+1)&&
                       (ret < VLAN_HA_MSG_LEN))
                    {
                        /* did not get enough buffer */
                        memcpy(vlan_ha_peer_msg,buf,ret);
                    }
                }
                process_peer_msg(buf, ret, pfds[i].fd);
            }
            else if(errno != EAGAIN)
            {
                log_error(ServiceUtilLogLevel,
                          "Read on fd %d at index %d failed return %d",
                          pfds[i].fd, i, ret);
                rem_fds(pfds[i].fd);
            }
        }
    }
    return 0;
}


void dgwy_ha_task_main (UINT1 *pDummy)
{
    dgwy_ha_task_msg_info_t	*haMsg  	  = NULL;
    unsigned int 			listenEvent   = 0;
    unsigned int 			recvEvent     = 0;
    int                     startFlg      = 0;

    if (sem_give(gDgwyHASemId) != OSW_OK) 
	{
		return;
    }
    if (create_queue("HAMSG", OSW_MAX_Q_MSG_LEN, 10, &gDgwyHAMsgId) != OSW_OK)
    {
        printf("%s:error when create queue\n",__FUNCTION__);
        return;
    }

    if (create_timer("HATASK", DGWY_HA_TASK_TIMER_EVENT, 
                     NULL, 0, &gDgwyHATimerListId) != OSW_OK)
    {
        printf("%s:error when create_timer\n",__FUNCTION__);
        return;
    }

    /* start the timer */
    start_timer(gDgwyHATimerListId, DGWY_HA_KEEP_ALIVE_TIMER, 0);

    listenEvent = DGWY_HA_TASK_TIMER_EVENT | DGWY_HA_TASK_MSG_EVENT; 

    log_info(ServiceUtilLogLevel,
             "INIT HA TASK ");

    memset(ha_peers,0,sizeof(ha_peers));
	while (1) 
    {
        /* poll for any dps lookup list */

        if(startFlg)
        {
            dgwy_ha_state_poll();
        }

        if(recv_event(gDgwyHATaskId, listenEvent, OSW_NO_WAIT, &recvEvent) == OSW_ERROR)
        {
            sleep(8);
            continue;
        }
    
        log_debug(ServiceUtilLogLevel,"recvEvent %d\n",
                  recvEvent);
        if(recvEvent & DGWY_HA_TASK_TIMER_EVENT)
        {
            start_timer(gDgwyHATimerListId, DGWY_HA_KEEP_ALIVE_TIMER, 0);
            /* send heart beat to peers */
            keep_alive_peers();
        }
        if(recvEvent & DGWY_HA_TASK_MSG_EVENT)
        {
            log_debug(ServiceUtilLogLevel,"GOT DGWY_HA_TASK_MSG_EVENT");
            while(queue_receive(gDgwyHAMsgId, (char *)&haMsg, 
                                sizeof(haMsg), OSW_NO_WAIT) == OSW_OK)
            {
                switch(haMsg->type)
                {   
                    case DGWY_HA_START:
                    {
                        dgwy_ha_task_msg_connect_t *lmsg=NULL;
                        log_info(ServiceUtilLogLevel,
                                 "HA TASK Started\n");
                        startFlg = 1;

                        lmsg = (dgwy_ha_task_msg_connect_t*)
                                malloc(sizeof(dgwy_ha_task_msg_connect_t));
                        if(lmsg==NULL)
                        {
                            log_error(ServiceUtilLogLevel,
                                      "Caould not alllocate");
                        }
                        else
                        {
                            lmsg->msg_info.type = DGWY_HA_LISTEN_START;
                            /* connected: send to ha task */
                            if (queue_send(gDgwyHASrvMsgId, (char *)&lmsg, sizeof(lmsg)) != OSW_OK)
                            {
                                log_error(ServiceUtilLogLevel, "Send Message error");
                                free(lmsg);
                            }
                            else if(send_event(gDgwyHASrvTaskId, DGWY_HA_TASK_MSG_EVENT) != OSW_OK)
                            {
                                log_error(ServiceUtilLogLevel, "Send EVENT error");
                            }

                            log_info(ServiceUtilLogLevel,
                                      "Sent event LISTEN START\n");
                        }
                        break;
                    }
                    case DGWY_HA_SKT_ACCEPTED:
                    {
                        dgwy_ha_task_msg_connect_t *cmsg
                            = (dgwy_ha_task_msg_connect_t*)haMsg;
                        add_fds_list(cmsg->fdConnected);
                        log_info(ServiceUtilLogLevel,
                                 "Event: DGWY_HA_SKT_ACCEPTED\n");
                        break;
                    }
                    case DGWY_HA_CTRL_PEER_IPV4:
                    {
                        dgwy_ha_task_msg_peer_t *bmsg
                            = (dgwy_ha_task_msg_peer_t*)haMsg;

                        log_debug(ServiceUtilLogLevel,
                                  "Event:DGWY_HA_CTRL_PEER_IPV4\n");

                        update_peers_table(bmsg->peer_ipv4,bmsg->local_ipv4);
                        break; 
                    }
                    default:
                        break;
                }
                if(haMsg)
                {
                    free(haMsg);
                }
            }
        }
    }
}

void dgwy_ha_listen_task (UINT1 *pDummy)
{
    dgwy_ha_task_msg_info_t	*haMsg  	  = NULL;
    unsigned int 			listenEvent   = 0;
    unsigned int 			recvEvent     = 0;
    uint8_t                 startFlg      = 0;
    int                     fdListen      = 0;
    int                     fdConnect     = 0;
    int                     flags         = 0;
    struct sockaddr_in addrListen; 

    if (sem_give(gDgwyHASrvSemId) != OSW_OK) 
	{
		return;
    }
    if (create_queue("LMSG", OSW_MAX_Q_MSG_LEN, 10, &gDgwyHASrvMsgId) != OSW_OK)
    {
        printf("%s:error when create queue\n",__FUNCTION__);
        return;
    }

    if (create_timer("HASRV", DGWY_HA_LISTEN_TASK_TIMER_EVENT, 
                     NULL, 0, &gDgwyHASrvTimerListId) != OSW_OK)
    {
        printf("%s:error when create_timer\n",__FUNCTION__);
        return;
    }

    /* start the timer */
    start_timer(gDgwyHASrvTimerListId, DGWY_HA_MON_TIMER_INTVL, 0);

    listenEvent = DGWY_HA_LISTEN_TASK_TIMER_EVENT | DGWY_HA_LISTEN_TASK_MSG_EVENT; 

    log_info(ServiceUtilLogLevel,
             "INIT HA TASK ");

    fdListen = socket(AF_INET, SOCK_STREAM, 0);
    memset(&addrListen, '0', sizeof(addrListen));
    
    addrListen.sin_family = AF_INET;
    addrListen.sin_addr.s_addr = htonl(INADDR_ANY);
    addrListen.sin_port = htons(DGWY_HA_LISTEN_PORT); 
    
    bind(fdListen, (struct sockaddr*)&addrListen, sizeof(addrListen)); 
    listen(fdListen, 10); 

    flags = flags|O_NONBLOCK;

    fcntl(fdListen, F_SETFL, flags);

	while (1) 
    {
        if(startFlg)
        {
            fdConnect=0;
            fdConnect = accept(fdListen, (struct sockaddr*)NULL, NULL);
            if(fdConnect > 0)
            {
                dgwy_ha_task_msg_connect_t *msg=NULL;

                log_info(ServiceUtilLogLevel,
                         "Accepted %d\n", fdConnect);

                fcntl(fdConnect, F_SETFL, flags);

                msg = (dgwy_ha_task_msg_connect_t*)malloc(sizeof(dgwy_ha_task_msg_connect_t));
                if(msg==NULL)
                {
                    close(fdConnect);
                }
                else
                {
                    msg->msg_info.type = DGWY_HA_SKT_ACCEPTED;
                    msg->fdConnected = fdConnect;
                    /* connected: send to ha task */
                    if (queue_send(gDgwyHAMsgId, (char *)&msg, sizeof(msg)) != OSW_OK)
                    {
                        log_error(ServiceUtilLogLevel, "Send Message error");
                        free(msg);
                    }
                    if(send_event(gDgwyHATaskId, DGWY_HA_TASK_MSG_EVENT) != OSW_OK)
                    {
                        log_error(ServiceUtilLogLevel, "Send EVENT error");
                    }

                    log_info(ServiceUtilLogLevel,
                              "Accepted sent event\n");
                }
            }
        }
        else
        {
            dgwy_ha_task_msg_connect_t *msg=NULL;

            sleep(10);
            msg = (dgwy_ha_task_msg_connect_t*)malloc(sizeof(dgwy_ha_task_msg_connect_t));
            if(msg)
            {
                msg->msg_info.type = DGWY_HA_START;
                /* connected: send to ha task */
                if (queue_send(gDgwyHAMsgId, (char *)&msg, sizeof(msg)) != OSW_OK)
                {
                    log_error(ServiceUtilLogLevel, "Send HA START Message error");
                    free(msg);
                }
                if(send_event(gDgwyHATaskId, DGWY_HA_TASK_MSG_EVENT) != OSW_OK)
                {
                    log_error(ServiceUtilLogLevel, "Send HA START EVENT error");
                }
                else
                {
                    log_info(ServiceUtilLogLevel,
                             "Sent event HA START\n");
                }

            }
        }
        
        if(recv_event(gDgwyHASrvTaskId, listenEvent, OSW_NO_WAIT, &recvEvent) == OSW_ERROR)
        {
            sleep(10);
            continue;
        }

        log_debug(ServiceUtilLogLevel,
                  "rcvEvnet %d\n", recvEvent);
    
        if(recvEvent & DGWY_HA_TASK_TIMER_EVENT)
        {
            start_timer(gDgwyHASrvTimerListId, DGWY_HA_MON_TIMER_INTVL, 0);

			/* TODO */
        }
        if(recvEvent & DGWY_HA_TASK_MSG_EVENT)
        {
            while(queue_receive(gDgwyHASrvMsgId, (char *)&haMsg, 
                                sizeof(haMsg), OSW_NO_WAIT) == OSW_OK)
            {
                switch(haMsg->type)
                {   
                    case DGWY_HA_LISTEN_START:
                    {
                        log_info(ServiceUtilLogLevel,
                                 "HA LISTEN TASK Started\n");
                        startFlg = 1;
                        break;
                    }
                    default:
                        break;
                }
                if(haMsg)
                {
                    free(haMsg);
                }
            }
        }
    }
}

