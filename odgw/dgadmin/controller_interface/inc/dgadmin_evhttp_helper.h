/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
*/

#ifndef _EVHTTP_HELPER_H_
#define _EVHTTP_HELPER_H_
int helper_evhttp_set_cb_pattern(const char *uri_pattern,
    void (*call_back)(struct evhttp_request *, void *, int, char **), void *cbarg);
void helper_evhttp_del_cb_pattern(const char *uri_pattern);
int helper_evhttp_request_dispatch(struct evhttp_request *req, const char *uri);
int helper_evhttp_request_and_syncprocess (
	const char *address, unsigned short port,
	enum evhttp_cmd_type type, const char *uri,
	struct evkeyvalq *headers, const char *body,
	void (*cb)(struct evhttp_request *, void *), void *arg);
#endif
