/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
*/

#ifndef __DGWY_EXTERN_H_
#define __DGWY_EXTERN_H_

extern void* dgwy_cb_func;
extern int dgwy_nl_send_message(struct nl_msg *msg, nl_recvmsg_msg_cb_t func, void *arg);
extern int dgwy_getinfo(void);
extern void dgwy_ctrl_task_main (UINT1 *pDummy);
extern void dgwy_ha_task_main (UINT1 *pDummy);
extern void dgwy_ha_listen_task (UINT1 *pDummy);

extern int64_t      gDgwySemId; 
extern int64_t      gDgwyTaskId;
extern int64_t      gDgwyMsgId;
extern int64_t      gDgwyTimerListId;

extern int64_t      gDgwyHASemId; 
extern int64_t      gDgwyHATaskId;
extern int64_t      gDgwyHAMsgId;
extern int64_t      gDgwyHATimerListId;

extern int64_t      gDgwyHASrvTaskId;
extern int64_t      gDgwyHASrvMsgId;
extern int64_t      gDgwyHASrvSemId; 
extern int64_t      gDgwyHASrvTimerListId;

extern dgwy_service_list_t GLB_SVC_TABLE[MAX_SERVICE_TABLES];
extern pthread_mutex_t GLB_SVC_TABLE_LOCK;
#if 1
#define GLB_SVC_LOCK_INIT(lock) pthread_mutex_init(&(lock),0)
#define GLB_SVC_LOCK(lock) pthread_mutex_lock(&(lock))
#define GLB_SVC_UNLOCK(lock) pthread_mutex_unlock(&(lock))
#else
#define GLB_SVC_LOCK_INIT(lock) printf("%d: LCOK INIT\n",__LINE__);pthread_mutex_init(&(lock),0)
#define GLB_SVC_LOCK(lock)  printf("%d: SVCLOCK\n",__LINE__);pthread_mutex_lock(&(lock))
#define GLB_SVC_UNLOCK(lock) printf("%d: SVC UNLOCK\n",__LINE__);pthread_mutex_unlock(&(lock))
#endif

#define UUID_LENGTH 36

#endif /* __DGWY_EXTERN_H_*/

