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

