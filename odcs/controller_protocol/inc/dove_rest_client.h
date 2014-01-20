/*
 *
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
 *  Source File:
 *      dove_rest_client.h
 *
 *
 *  Author:
 *      John He
 *
 */


#ifndef _DOVE_REST_CLIENT_H_

#define DOVE_REST_REQ_INFO_INIT(_rinfo, _addr, _uri, _port, _type, _request) {\
    (_rinfo)->address = strdup(_addr);\
    (_rinfo)->uri = strdup(_uri);\
    (_rinfo)->port = _port;\
    (_rinfo)->type = _type;\
    (_rinfo)->request = _request;\
}

/*
 ******************************************************************************
 * dove_rest_client_init --                *//**
 *
 * \brief This routine is used to initialize DOVE REST Client infrastructure
 *
 * \retval 0 Success
 * \retval -1 Failure
 *
 ******************************************************************************/
int dove_rest_client_init(void);
#endif

