#ifndef __HTTPS_DATA_SERVER_UTIL_H_
#define __HTTPS_DATA_SERVER_UTIL_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include <event2/bufferevent.h>
#include <event2/bufferevent_ssl.h>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/keyvalq_struct.h>
#include "make_log.h"
#include <hiredis/hiredis.h>

#include <cJSON.h>
#define MYHTTPD_SIGNATURE   "MoCarHttpd v0.1"
extern char* module_name;
extern char* proj_name;
void persistent_cb (struct evhttp_request *req, void *arg);
void cache_cb (struct evhttp_request *req, void *arg);

#endif
