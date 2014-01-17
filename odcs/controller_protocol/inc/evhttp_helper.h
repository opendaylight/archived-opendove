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
 *      evhttp_helper.h
 *
 *  Author:
 *      Open DOVE development team
 *
 */


#ifndef _EVHTTP_HELPER_H_
#define _EVHTTP_HELPER_H_

typedef struct helper_token {
    char token_type;
    char *token_str;
    struct helper_token *next;
} helper_token_t;

typedef struct helper_cb {
    struct helper_token *token_chain;
    int forward_flag;
    void (*call_back)(struct evhttp_request *, void *, int, char **);
    void *arg;
    struct helper_cb *next;
} helper_cb_t;

#define MAGIC_TOKEN_ANY '*'
#define TOKEN_DELIMITER "/"
#define TOKEN_TYPE_STRING 0
#define TOKEN_TYPE_MATCH_ANY 1
#define MAX_ARG_NUMBER 16

int helper_evhttp_set_cb_pattern(const char *uri_pattern, int forward_flag,
    void (*call_back)(struct evhttp_request *, void *, int, char **), void *cbarg);
void helper_evhttp_del_cb_pattern(const char *uri_pattern);
helper_cb_t *helper_evhttp_get_cblist(void);
int helper_evhttp_match_uri(helper_token_t *t, char *uri, int *argc, char *argv[]);

int helper_evhttp_get_id_from_uri(const char *uri, int *domain_id, int *vn_id);
int helper_uri_is_same_pattern(const char *uri_pattern, char *uri);
int helper_uri_is_same(const char *uri_pattern, const char *uri);

int helper_evhttp_request_dispatch(struct evhttp_request *req, const char *uri);


#endif
