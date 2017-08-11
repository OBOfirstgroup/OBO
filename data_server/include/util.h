#ifndef __HTTPS_DATA_SERVER_UTIL_H_
#define __HTTPS_DATA_SERVER_UTIL_H_

#define MYHTTPD_SIGNATURE   "MoCarHttpd v0.1"
void persistent_cb (struct evhttp_request *req, void *arg);
void cache_cb (struct evhttp_request *req, void *arg);

#endif
