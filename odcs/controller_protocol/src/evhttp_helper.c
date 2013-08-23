#include "include.h"
#include <event2/http.h>
#include "../inc/evhttp_helper.h"


static helper_cb_t *gcb;

static int helper_token_comp(helper_token_t *t1, helper_token_t *t2)
{
    if(t1->token_type != t2->token_type)
    {
        return ((t1->token_type > t2->token_type) ? 1:-1);
    }
    if(t1->token_type == TOKEN_TYPE_STRING)
    {
        return strcmp(t1->token_str, t2->token_str);
    }
    return 0;
}
static int helper_tokenchain_comp(helper_token_t *t1, helper_token_t *t2)
{
    int ret;
    while(t1 && t2)
    {
        ret = helper_token_comp(t1, t2);
        if (ret)
        {
            return ret;
        }
        t1 = t1->next;
        t2 = t2->next;
    }
    if(NULL == t1 && NULL == t2)
    {
        return 0;
    }
    if(NULL == t1)
    {
        return -1;
    }
    return 1;
}
static void helper_token_chain_release(helper_token_t *t)
{
    helper_token_t *next;
    while(t)
    {
        next = t->next;
        if (NULL != t->token_str)
        {
            free(t->token_str);
        }
        free(t);
        t = next;
    }
}
static void helper_cb_release(helper_cb_t *cb)
{
    if(NULL != cb)
    {
        helper_token_chain_release(cb->token_chain);
        free(cb);
    }
}
static helper_token_t *helper_string_to_token_chain(const char *string, int regx)
{
    char *str = NULL;
    char *str2 = NULL;
    char *saveptr = NULL;
    helper_token_t *t = NULL;
    helper_token_t *h = NULL;
    helper_token_t *next = NULL;
    char *tk;
    int failed = 0;
    if (NULL == string)
    {
        return NULL;
    }
    str = strdup(string);
    if(NULL == str)
    {
        return NULL;
    }
    str2 = str;
    do
    {
        tk = strtok_r(str2, TOKEN_DELIMITER, &saveptr);
        if(NULL == tk)
        {
            break;
        }
        str2 = NULL;
        t = (helper_token_t *)malloc(sizeof(helper_token_t));
        if(NULL == t)
        {
            failed = 1;
            break;
        }
        memset(t, 0, sizeof(helper_token_t));
        if(regx && tk[0] == MAGIC_TOKEN_ANY && tk[1] == '\0')
        {
            t->token_type = TOKEN_TYPE_MATCH_ANY;
        }
        else
        {
            t->token_str = strdup(tk);
            if(NULL == t->token_str)
            {
                failed = 1;
                free(t);
                t = NULL;
                break;
            }
            t->token_type = TOKEN_TYPE_STRING;
        }
        if(NULL == h)
        {
            h = t;
        }
        else
        {
            next->next = t;
        }
        next = t;
    } while (1);
    if(NULL != str)
    {
        free(str);
    }
    if (failed)
    {
        if(h)
        {
            helper_token_chain_release(h);
        }
        h = NULL;
    }
    return h;
}

int helper_evhttp_match_uri(helper_token_t *t, char *uri, int *argc, char *argv[])
{
    char *saveptr = NULL;
    *argc = 0;
    char *strtk;

    strtk = strtok_r(uri, TOKEN_DELIMITER, &saveptr);
    while (strtk && t)
    {
        if(t->token_type == TOKEN_TYPE_MATCH_ANY)
        {
            if(*argc < MAX_ARG_NUMBER)
            {
                argv[*argc] = strtk;
                *argc += 1;
            }
        }
        else
        {            
            if(strcmp(t->token_str, strtk))
            {
                *argc = 0;
                return -1;
            }
        }
        t = t->next;
        strtk = strtok_r(NULL, TOKEN_DELIMITER, &saveptr);
    }
 
    if(NULL == t && NULL == strtk)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

/* Trevor: Added for uri comparison */
int helper_uri_is_same_pattern(const char *uri_pattern, char *uri)
{
	helper_token_t *t = NULL;
	int match = 0;
	int argc = 0;
	char *argv[MAX_ARG_NUMBER] = {0};

	do
	{

		t = helper_string_to_token_chain(uri_pattern, 1);
		if (t == NULL)
		{
			break;
		}
		if (!helper_evhttp_match_uri(t, (char *)uri, &argc, argv))
		{
			match = 1;
		}
	} while(0);
	if (t != NULL)
	{
		helper_token_chain_release(t);
	}

	return match;
}

int helper_uri_is_same(const char *uri_pattern, const char *uri)
{
	if(strcmp(uri_pattern,uri) == 0)
		return 1;
	return 0;
}

int helper_evhttp_set_cb_pattern(const char *uri_pattern, int forward_flag,
    void (*call_back)(struct evhttp_request *, void *, int, char **), void *cbarg)
{
    helper_token_t *t;
    helper_cb_t *cb = NULL;
    if(NULL == uri_pattern || NULL == call_back)
    {
        return -1;
    }
    t = helper_string_to_token_chain(uri_pattern, 1);
    if(NULL == t)
    {
        return -1;
    }
    for (cb = gcb; cb; cb = cb->next)
    {
        if(!helper_tokenchain_comp(cb->token_chain, t))
        {
            helper_token_chain_release(t);
            return -1;
        }
    }
    cb = (helper_cb_t *)malloc(sizeof(helper_cb_t));
    if(NULL == cb)
    {
        helper_token_chain_release(t);
        return -1;
    }
    memset(cb, 0, sizeof(helper_cb_t));
    cb->token_chain = t;
    cb->forward_flag = forward_flag;
    cb->call_back = call_back;
    cb->arg = cbarg;
    cb->next = gcb;
    gcb = cb;
    return 0;
}

void helper_evhttp_del_cb_pattern(const char *uri_pattern)
{
    helper_cb_t *prev = NULL;
    helper_cb_t *curr = NULL;
    helper_token_t *t;
    
    if(NULL == uri_pattern)
    {
        return;
    }
    t = helper_string_to_token_chain(uri_pattern, 1);
    if(NULL == t)
    {
        return;
    }
    for(curr = gcb; curr; curr = curr->next)
    {
        if(!helper_tokenchain_comp(curr->token_chain, t))
        {
            if(prev == NULL)
            {
                gcb = curr->next;
            }
            else
            {
                prev->next = curr->next;
            }
            helper_cb_release(curr);
            break;
        }
        prev = curr;
    }
    helper_token_chain_release(t);
}

int helper_evhttp_get_id_from_uri(const char *uri, int *domain_id, int *vn_id)
{
    char *saveptr = NULL, *endptr = NULL;
    char *str, *token = NULL, *prev_token = NULL;
    bool match = true;
    int ret = 0;

    str = strdup(uri);
    if (str == NULL)
        return -1;

    prev_token = token = strtok_r(str, TOKEN_DELIMITER, &saveptr);
    while (token)
    {
        match = true;
        if (!strcmp(prev_token, "domains"))
        {
            *domain_id = strtoul(token, &endptr, 10);
        }
        else if (!strcmp(prev_token, "dvgs"))
        {
            *vn_id = strtoul(token, &endptr, 10);
        }
        else if (!strcmp(prev_token, "vns"))
        {
            *vn_id = strtoul(token, &endptr, 10);
        }
        else
        {
            match = false;
        }
        if (match && (*endptr != '\0'))
        {
            ret = -1;
            break;
        }

        prev_token = token;
        token = strtok_r(NULL, TOKEN_DELIMITER, &saveptr);
    }

    free(str);
    return ret;
}

helper_cb_t *helper_evhttp_get_cblist(void)
{
    return gcb;
}

/* Trevor: the following definition is for old simulated Dove Controller httpd */
int helper_evhttp_request_dispatch(struct evhttp_request *req, const char *uri)
{
    int argc = 0;
    char *argv[MAX_ARG_NUMBER] = {0};
    helper_cb_t *cb;
    char *str;
    int ret = -1;

    if(NULL == req || NULL == uri)
    {
        return -1;
    }
    str = strdup(uri);
    if(NULL == str)
    {
        return -1;
    }
    for(cb = gcb; cb; cb = cb->next)
    {
        if(!helper_evhttp_match_uri(cb->token_chain, str, &argc, argv))
        {
            cb->call_back(req, cb->arg, argc, argv);
            ret = 0;
            break;
        }
        strcpy(str, uri);
    }
    if(NULL != str)
    {
        free(str);
    }
    return ret;
}

