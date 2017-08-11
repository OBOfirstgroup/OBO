
#include "util.h"

void reg_cb(struct evhttp_request *req, void *arg)
{
    printf("reg cb ...............\n");
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
    int is_reg_succ = 0; //0 代表succ  -1 fail

    //unpack json
    cJSON* root = cJSON_Parse(request_data_buf);
    cJSON* username = cJSON_GetObjectItem(root, "username");
    cJSON* password = cJSON_GetObjectItem(root, "password");
    cJSON* driver = cJSON_GetObjectItem(root, "driver");
    cJSON* email = cJSON_GetObjectItem(root, "email");
    cJSON* tel = cJSON_GetObjectItem(root, "tel");
    cJSON* id_card = cJSON_GetObjectItem(root, "id_card");

    printf("username = %s\n", username->valuestring);
    printf("password = %s\n", password->valuestring);
    printf("driver = %s\n", driver->valuestring);
    printf("tel = %s\n", tel->valuestring);
    printf("email = %s\n", email->valuestring);
    printf("id_card = %s\n", id_card->valuestring);



    //--------------- 插入数据库 --------------------
    /*
       ====给服务端的协议====
    https://ip:port/persistent [json_data]
        {
            cmd: "insert",
            busi: "reg",
            table: "OBO_TABLE_USER",

            username:  "盖伦",
            password:  "ADSWADSADWQ(MD5加密之后的)",
            tel     :  "13332133313",
            email   :  "danbing_at@163.com",
            id_card :  "21040418331323",
            driver  :  "yes",
        }
     */

    //组装一个远程插入数据库的json 通过curl 发送给data服务器
    cJSON *rqst_data_rt = cJSON_CreateObject();
    cJSON_AddStringToObject(rqst_data_rt, "cmd", "insert");
    cJSON_AddStringToObject(rqst_data_rt, "busi", "reg");
    cJSON_AddStringToObject(rqst_data_rt, "table", "OBO_TABLE_USER");
    cJSON_AddStringToObject(rqst_data_rt, "username", username->valuestring);
    cJSON_AddStringToObject(rqst_data_rt, "password", password->valuestring);
    cJSON_AddStringToObject(rqst_data_rt, "tel", tel->valuestring);
    cJSON_AddStringToObject(rqst_data_rt, "email", email->valuestring);
    cJSON_AddStringToObject(rqst_data_rt, "id_card", id_card->valuestring);
    cJSON_AddStringToObject(rqst_data_rt, "driver", driver->valuestring);

    char *reg_post_data = cJSON_Print(rqst_data_rt);

    CURL *curl = NULL;
    CURLcode res;
    response_data_t response_datasrvr;

    curl = curl_easy_init();
    if (curl == NULL) {
        printf("curl init error");
        //返回错误json 前端
    }

    curl_easy_setopt(curl, CURLOPT_URL, "https://0.0.0.0:7778/persistent");
//curl_easy_setopt(curl, CURLOPT_URL, "https://101.200.190.150:7778/persistent");

    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);

    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, reg_post_data );

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, deal_response_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_datasrvr);

    //提交请求向data服务器
    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        printf("curl to data server perform error res = %d", res);
        //返回错误json 前端
    }
    printf("%s,%s,%d\n",__FILE__,__FUNCTION__,__LINE__);
    printf("response_datasrvr.data=%s\n",response_datasrvr.data);



    //从data服务器的返回结果来判断是否查询成功
    cJSON* reponse_from_data = cJSON_Parse(response_datasrvr.data);
    if(!reponse_from_data)
    {
        perror("cJSON_Parse err.");
        return ;
    }
    cJSON* result = cJSON_GetObjectItem(reponse_from_data, "result");
    if (result && strcmp(result->valuestring, "ok") == 0) {
        //入库成功
        printf("insert OBO_TABLE_USER succ!\n");
        is_reg_succ = 0;
    }
    else {
        printf("insert OBO_TABLE_USER fail!\n");
        is_reg_succ = -1;
    }


    //如果还有其他业务 在此时再处理
    //生成一个sessionid
    char sessionid[SESSIONID_LEN] = {0};


    create_sessionid(driver->valuestring, sessionid);

    printf("sessionid = %s\n", sessionid);

    //向远程存储服务器发送 setHash指令  /cache 指令

    //得到远程存储服务器的结果


    // ----------------  给前端回复---------------
    //packet json
    cJSON* response_jni = cJSON_CreateObject();

    if (is_reg_succ == 0) {
        cJSON_AddStringToObject(response_jni, "result", "ok");
    }
    else if (is_reg_succ == -1) {
        cJSON_AddStringToObject(response_jni, "result", "error");
        cJSON_AddStringToObject(response_jni, "reason", "insert OBO_TABLE_USER ERROR");
    }

    char *response_data = cJSON_Print(response_jni);




    /* This holds the content we're sending. */

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


    printf("[response]:\n");
    printf("%s\n", response_data);

    cJSON_Delete(response_jni);
    cJSON_Delete(rqst_data_rt);
    cJSON_Delete(root);
    free(response_data);
}
