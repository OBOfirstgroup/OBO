#include "util.h"

//TODO 稍后改名，/cache/callcar.
void cache_cb (struct evhttp_request *req, void *arg)
{
    struct evbuffer *evb = NULL;
    const char *uri = evhttp_request_get_uri (req);
    struct evhttp_uri *decoded = NULL;

    /* 判断 req 是否是GET 请求 */
    if (evhttp_request_get_command (req) == EVHTTP_REQ_GET)
    {
        struct evbuffer *buf = evbuffer_new();
        if (buf == NULL) return;
        evbuffer_add_printf(buf, "Requested: %s\n", uri);
        evhttp_send_reply(req, HTTP_OK, "OK", buf);
        return;
    }

    /* 这里只处理Post请求, Get请求，就直接return 200 OK  */
    if (evhttp_request_get_command (req) != EVHTTP_REQ_POST)
    {
        evhttp_send_reply (req, 200, "OK", NULL);
        return;
    }

    printf ("Got a POST request for <%s>\n", uri);

    //判断此URI是否合法
    decoded = evhttp_uri_parse (uri);
    if (! decoded)
    {
        printf ("It's not a good URI. Sending BADREQUEST\n");
        evhttp_send_error (req, HTTP_BADREQUEST, 0);
        return;
    }

    /* Decode the payload */
    struct evbuffer *buf = evhttp_request_get_input_buffer (req);
    evbuffer_add (buf, "", 1);    /* NUL-terminate the buffer */
    char *payload = (char *) evbuffer_pullup (buf, -1);
    int post_data_len = evbuffer_get_length(buf);
    char request_data_buf[4096] = {0};
    memcpy(request_data_buf, payload, post_data_len);

    printf("[post_data][%d]=\n %s\n", post_data_len, payload);


    /*
       具体的：可以根据Post的参数执行相应操作，然后将结果输出
       ...
       */
    //unpack json
    cJSON* root = cJSON_Parse(request_data_buf);
    cJSON* username_obj = cJSON_GetObjectItem(root, "username");
    cJSON* cmd_obj = cJSON_GetObjectItem(root, "cmd");
    cJSON* latitude_obj = cJSON_GetObjectItem(root, "latitude");
    cJSON* longitude_obj = cJSON_GetObjectItem(root, "longitude");


    char temp_buf[1024]={0};


    sprintf(temp_buf+strlen(temp_buf),"username=%s:", username_obj->valuestring);
    sprintf(temp_buf+strlen(temp_buf),"cmd=%s:", cmd_obj->valuestring);
    sprintf(temp_buf+strlen(temp_buf),"latitude = %f:", latitude_obj->valuedouble);
    sprintf(temp_buf+strlen(temp_buf),"longitude = %f", longitude_obj->valuedouble);
    LOG(module_name,proj_name,temp_buf);
    //redis op.
    redisContext *c;
    redisReply *reply;
    const char *hostname = "127.0.0.1";
    int port =6379;

    struct timeval timeout = { 1, 500000 }; // 1.5 seconds
    c = redisConnectWithTimeout(hostname, port, timeout);
    if (c == NULL || c->err) {
        if (c) {
            LOG(module_name,proj_name,"Connection error: %s\n", c->errstr);
            redisFree(c);
        } else {
            LOG(module_name,proj_name,"Connection error: can't allocate redis context\n");
        }
        exit(1);
    }

    reply = redisCommand(c,"GEORADIUS DRIVER_POOL %f  %f 10 km  asc",longitude_obj->valuedouble,latitude_obj->valuedouble);
    char driver_name[64]={0};
    if ((reply->type == REDIS_REPLY_ARRAY)&&(reply->elements>0)) {
        strcpy(driver_name,reply->element[0]->str);
        LOG(module_name,proj_name,"chose driver name is:%s",driver_name);

    }else{
        LOG(module_name,proj_name,"no proper driver can receive order.");
    }
    freeReplyObject(reply);
    redisFree(c);


    cJSON_Delete(root);
    root=NULL;
    //assemble data.which send to web_server
    root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "result", "ok");
    cJSON_AddStringToObject(root, "username", driver_name);

    char *response_data = cJSON_Print(root);
    cJSON_Delete(root);
    root=NULL;

    //send to web_server.
    //HTTP header
    evhttp_add_header(evhttp_request_get_output_headers(req), "Server", MYHTTPD_SIGNATURE);
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "text/plain; charset=UTF-8");
    evhttp_add_header(evhttp_request_get_output_headers(req), "Connection", "close");

    evb = evbuffer_new ();
    evbuffer_add_printf(evb, "%s", response_data);
    //将封装好的evbuffer 发送给客户端
    evhttp_send_reply(req, HTTP_OK, "OK", evb);

    if (decoded)
        evhttp_uri_free (decoded);
    if (evb)
        evbuffer_free (evb);

    LOG(module_name,proj_name,"[response]:%s\n", response_data);

    free(response_data);
}
