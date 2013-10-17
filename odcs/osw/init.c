/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
 *  Author:
 *      John He
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <semaphore.h>
#include "inc/osw.h"

/*
 ******************************************************************************
 * osw_init --                                                            *//**
 *
 * \brief This routine initialize all components.
 *
 * \retval OSW_OK        Success
 * \retval OSW_ERROR     Failure
 *
 *****************************************************************************/
int osw_init(void)
{
    if (task_base_init() || sem_base_init() || queue_base_init() || timer_base_init())
        return OSW_ERROR;

    return OSW_OK;
}
