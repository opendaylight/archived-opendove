/*
 * Copyright (c) 2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
 *  Source File:
 *      dove-dmc-client.c
 *      REST client for interaction with ODMC 
 *
 *  Author:
 *      Liran Schour
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jansson.h>
#include <netinet/in.h>
#include <arpa/inet.h> 
#include <curl/curl.h>

#define URL_DMC_USER "admin"
#define URL_DMC_PASSWORD "admin"
#define URL_DMC_SB_GET_SERVICE "http://%s:8080/controller/sb/v2/opendove/odmc/odcs/leader"
#define URL_DMC_SB_SWITCH_REG "http://%s:8080/controller/sb/v2/opendove/odmc/switch"
#define URL_DMC_SB_SWITCH_HB "http://%s:8080/controller/sb/v2/opendove/odmc/switch/%s"
#define HTTP_OK 200

enum request_type {
  GET = 0,
  PUT = 1,
  POST = 2,
};

/* resizable memory buffer to write REST replys to */
struct reply_buffer {
  char *memory;
  size_t size;
};

struct write_buffer {
  const char *readptr;
  int sizeleft;
};

#define MAX_UUID_LEN 256
#define DOVE_SWITCH_UUID_FILE "switch.uuid"

static char switch_uuid[MAX_UUID_LEN] = "null";
static char switch_name[MAX_UUID_LEN] = "";

/* method that will be invoked when REST reply is received */
static size_t
reply_cb(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct reply_buffer *buf = (struct reply_buffer *)userp;
 
  buf->memory = realloc(buf->memory, buf->size + realsize + 1);
  if(buf->memory == NULL) {
    /* out of memory! */ 
    fprintf(stderr, "not enough memory (realloc returned NULL)\n");
    return 0;
  }
 
  memcpy(&(buf->memory[buf->size]), contents, realsize);
  buf->size += realsize;
  buf->memory[buf->size] = 0;
 
  return realsize;
}

static long send_rest_req(const char *url, enum request_type type,
			  json_t *js_root, char **data)
{
  CURL *curl;
  CURLcode res;
  struct reply_buffer buffer; 
  long http_status;
  struct curl_slist *headers=NULL;

  curl_global_init(CURL_GLOBAL_ALL);
  
  /* init the curl session */ 
  curl = curl_easy_init();

  if(!curl) {
    fprintf(stderr, "failed to initialize curl handle, exiting\n");
    return 1;
  }

  buffer.memory = malloc(1);  
  buffer.size = 0;
 
  curl_global_init(CURL_GLOBAL_ALL);  
  /* init the curl session */ 
  curl = curl_easy_init();

  headers = curl_slist_append(headers, "Accept: application/json");
  headers = curl_slist_append(headers, "Content-Type: application/json");
  headers = curl_slist_append(headers, "charsets: utf-8");

  curl_easy_setopt(curl, CURLOPT_URL, url); 	//set URL
  curl_easy_setopt(curl, CURLOPT_USERNAME, URL_DMC_USER);	//set username
  curl_easy_setopt(curl, CURLOPT_PASSWORD, URL_DMC_PASSWORD);	//set password
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  
  switch(type) {
  case PUT:
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS,
		     json_dumps(js_root, JSON_PRESERVE_ORDER));
    break;
  case POST:
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS,
		     json_dumps(js_root, JSON_PRESERVE_ORDER));
    break;
  case GET:
    break;
  }
  
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, reply_cb);	//set callback for the reply
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&buffer);	//parameter to the callback
  /* Perform the request, res will get the return code */ 
  
  res = curl_easy_perform(curl);

  /* Check for errors */ 
  if(res != CURLE_OK) {
    //fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    curl_easy_cleanup(curl); 
    curl_global_cleanup();
    return 1;
  }
  
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_status);
  
  curl_easy_cleanup(curl); 
  curl_global_cleanup();
  
  buffer.memory[buffer.size] = '\0';
  
  *data = buffer.memory;
  
  return http_status;
}

static void read_uuid_from_file()
{
  FILE *fp = NULL;
  char ptr[MAX_UUID_LEN];

  if((fp = fopen(DOVE_SWITCH_UUID_FILE,"r")) != NULL) {
    if(fgets(ptr, MAX_UUID_LEN,fp)!= NULL){
      if(strlen(ptr) > 0) {
	strcpy(switch_uuid, ptr);
	fprintf(stderr, "Read uuid %s from file\n", switch_uuid);
      }
    }
  }
}

static void write_uuid_to_file()
{
  FILE *fp;
  
  if((fp = fopen(DOVE_SWITCH_UUID_FILE,"w")) == NULL) {
    fprintf(stderr, "ERROR in opening UUID file [%s]",
	    DOVE_SWITCH_UUID_FILE);
  }else{
    fputs(switch_uuid,fp);
    fclose(fp);
  }
  return;
}

static int parse_switch_reg(char *data,
			    long http_status)
{
  json_error_t jerror;
  json_t *js_id = NULL;
  json_t *js_root = NULL;
  int ret = -1;

  if(http_status != HTTP_OK) {
    fprintf(stderr, "Switch reg status: %ld\n", http_status);
  }

  js_root = json_loads(data, 0, &jerror);
  if (!js_root) {
    //fprintf(stderr,"Error get DCS leader: %s\n", data);
    return -1;
  }

  js_id = json_object_get(js_root, "id");

  if (js_id && json_is_string(js_id)) {
    strcpy(switch_uuid,json_string_value(js_id));
    printf("Retrieved switch UUID from DCS: %s\n", switch_uuid);
    
    write_uuid_to_file();
    
    js_id = json_object_get(js_root, "name");
  
    if (js_id && json_is_string(js_id)) {
      strcpy(switch_name,json_string_value(js_id));
      printf("Retrieved switch name from DCS: %s\n", switch_name);
      ret = 0;
    } else {
      fprintf(stderr, "Error retrieving switch name: %s", data);
      ret = -1;
    }
  } else {
    fprintf(stderr, "Error retrieving switch ID: %s", data);
    ret = -1;
  }

  return ret;
}

int dove_dmc_client_init(const char *dmcIP, char *switchIP)
{
  char url[128];
  CURL *curl;
  json_t *js_root = NULL;
  char *data = NULL;
  long http_status;
  int ret;

  curl_global_init(CURL_GLOBAL_ALL);
  
  /* init the curl session */ 
  curl = curl_easy_init();

  if(!curl) {
    fprintf(stderr, "failed to initialize curl handle, exiting\n");
    return 1;
  }
  
  read_uuid_from_file();

  sprintf(url, URL_DMC_SB_SWITCH_REG, dmcIP);
  
  js_root = json_pack("{s:s, s:s, s:s, s:s}",
		      "id", "null",
		      "name", switchIP,
		      "tunnelip", switchIP,
		      "managementip", switchIP);
  
  http_status = send_rest_req(url, POST, js_root, &data);
  
  ret = parse_switch_reg(data, http_status);

  if(data) {
    free(data);
  }

  curl_easy_cleanup(curl); 
  curl_global_cleanup();

  return ret;
}

void dove_dmc_client_cleanup()
{
  // nothing to do here
}

static int parse_get_leader(char *data,
			    long http_status,
			    struct in_addr *dcs_ip, 
			    short *dcs_port)
{
  json_error_t jerror;
  json_t *js_ip = NULL;
  json_t *js_root = NULL;
  int ret = -1;

  if(http_status != 200 || !(js_root = json_loads(data, 0, &jerror))) {
    fprintf(stderr,"Error get DCS leader status: %ld\n", http_status);
    return -1;
  }

  //get the port
  json_t *js_port = NULL;
  js_port = json_object_get(js_root, "dcs_raw_service_port");
  if (json_is_null(js_port) || !json_is_integer(js_port)) {
    fprintf(stderr, "dcs_raw_service_port in the request response is not an integer");
  } else {
    *dcs_port = (short) htons(json_integer_value(js_port));
    fprintf(stderr, "dcs_raw_service_port is %hu\n", ntohs(*dcs_port));
  }
  
  js_ip = json_object_get(js_root, "ip");
  if (NULL != js_ip && json_is_string(js_ip)) {
    dcs_ip->s_addr = inet_addr(json_string_value(js_ip));
    fprintf(stderr, "DCS IP is %s\n", json_string_value(js_ip));
    ret = 0;
  } else {
    fprintf(stderr, "Error retrieving DCS IP status %ld: %s",
	    http_status, data);
    ret = -1;
  }
  
  return ret;
}

int dove_dmc_get_leader(const char *dmcIP, struct in_addr *dcs_ip, short *dcs_port)
{
  char url[128];
  CURL *curl;
  char *data = NULL;
  long http_status;
  int ret = -1;
  
  curl_global_init(CURL_GLOBAL_ALL);
  
   /* init the curl session */ 
  curl = curl_easy_init();

  if(!curl) {
    fprintf(stderr, "failed to initialize curl handle, exiting\n");
    return 1;
  }
 
  sprintf(url, URL_DMC_SB_GET_SERVICE, dmcIP);
  
  http_status = send_rest_req(url, GET, NULL, &data);
    
  ret = parse_get_leader(data, http_status, dcs_ip, dcs_port);
  
  if(data) {
    free(data);
  }
  
  curl_easy_cleanup(curl); 
  curl_global_cleanup();

  return ret;
}

int dove_dmc_send_hb(const char *dmcIP)
{
  char url[128];
  CURL *curl;
  long http_status;
  int ret = -1;
  json_t *js_root = NULL;
  char *data;

  if(strlen(switch_uuid) == 0) {
    return 0;
  }

  curl_global_init(CURL_GLOBAL_ALL);
  
  /* init the curl session */ 
  curl = curl_easy_init();

  if(!curl) {
    fprintf(stderr, "failed to initialize curl handle, exiting\n");
    return 1;
  }
 
  
  sprintf(url, URL_DMC_SB_SWITCH_HB, dmcIP, switch_uuid);
  
  js_root = json_pack("{s:s, s:s, s:s, s:s}",
		      "id", switch_uuid,
		      "name", switch_name,
		      "tunnelip", switch_name, // switch name is the IP
		      "managementip", switch_name);
  
  http_status = send_rest_req(url, PUT, js_root, &data);
  
  printf("DMC HB response status: %ld\n", http_status);
  
  if(data) {
    free(data);
  }
  
  curl_easy_cleanup(curl); 
  curl_global_cleanup();

  return ret;
}
