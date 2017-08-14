
#include "util.h"



//TODO 错误处理，需要进行加强
void callcar_cb(struct evhttp_request *req, void *arg)
{
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
    LOG(module_name,proj_name,"Got a POST request for <%s>\n", uri);

    //判断此URI是否合法
    decoded = evhttp_uri_parse (uri);
    if (! decoded)
    {
        LOG(module_name,proj_name,"It's not a good URI. Sending BADREQUEST\n");
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

    LOG(module_name,proj_name,"[post_data][%d]= %s\n", post_data_len, payload);


    /*
       具体的：可以根据Post的参数执行相应操作，然后将结果输出
       ...
       */
    //unpack json
    cJSON* root = cJSON_Parse(request_data_buf);
    cJSON* username = cJSON_GetObjectItem(root, "username");
    cJSON* latitude = cJSON_GetObjectItem(root, "latitude");
    cJSON* longitude= cJSON_GetObjectItem(root, "longitude");
    char temp_buf[1024]={0};


    sprintf(temp_buf,"callcar:");
    sprintf(temp_buf+strlen(temp_buf),"username=%s:", username->valuestring);
    sprintf(temp_buf+strlen(temp_buf),"latitude = %f:", latitude->valuedouble);
    sprintf(temp_buf+strlen(temp_buf),"longitude = %f", longitude->valuedouble);
	LOG(module_name,proj_name,temp_buf);

    //查询数据库
    //TODO  query data server. get drivername
	//参照reg流程的话，需要先了解redis的详细步骤
 //--------------- 插入数据库 --------------------
    /*
       ====给服务端的协议====
    https://ip:port/cache [json_data]
        {
            cmd: "callcar",
            username:  "username",
            latitude:  12.23,
            longitude:  88.253,
        }
     */
	 ///--------------------------------------
  //assemble data which will be sent 2 data_server.
    cJSON *rqst_data_rt = cJSON_CreateObject();
    cJSON_AddStringToObject(rqst_data_rt, "cmd", "callcar");
    cJSON_AddStringToObject(rqst_data_rt, "username", username->valuestring);
	cJSON *latitude_obj=cJSON_CreateNumber(latitude->valuedouble);
	cJSON_AddItemToObject(rqst_data_rt,"latitude",latitude_obj);
	cJSON *longitude_obj=cJSON_CreateNumber(longitude->valuedouble);
	cJSON_AddItemToObject(rqst_data_rt,"longitude",longitude_obj);
	cJSON_Delete(root);
	root=NULL;
    char *reg_post_data = cJSON_Print(rqst_data_rt);
	cJSON_Delete(rqst_data_rt);
		//send data 2 data_server
    CURL *curl = NULL;
    CURLcode res;
    response_data_t response_datasrvr;

    curl = curl_easy_init();
    if (curl == NULL) {
        printf("curl init error");
        evhttp_send_error (req, HTTP_BADREQUEST, 0);
		curl_easy_cleanup(curl);
		return;
    }

    curl_easy_setopt(curl, CURLOPT_URL, "https://0.0.0.0:7778/cache");

    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);

    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, reg_post_data );
    LOG(module_name,proj_name,"send to data_server data is %s", reg_post_data);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, deal_response_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_datasrvr);
    res = curl_easy_perform(curl);
	free(reg_post_data);
	reg_post_data=NULL;
    if (res != CURLE_OK) {
		    LOG(module_name,proj_name,"curl to data server perform error res = %d", res);
			reply_request(req,"服务器内部错误,请稍后再试");
		return;
    }
	    LOG(module_name,proj_name,"response_datasrvr.data=%s\n",response_datasrvr.data);



    //从data服务器的返回结果来判断是否查询成功
    cJSON* reponse_from_data = cJSON_Parse(response_datasrvr.data);
    if(!reponse_from_data)
    {
		LOG(module_name,proj_name,"cJSON_Parse err.");
		cJSON_Delete(reponse_from_data);
		reply_request(req,"服务器内部错误,请稍后再试");
		return;
    }
    cJSON* result = cJSON_GetObjectItem(reponse_from_data, "result");
	if(!result)
	{
		cJSON_Delete(reponse_from_data);
		reply_request(req,"服务器内部错误,请稍后再试");
		return;
    }


    if (strcmp(result->valuestring, "ok")) {

		LOG(module_name,proj_name,"cannot get a  driver");
		cJSON_Delete(reponse_from_data);
		reply_request(req,"");
		return;
    }
	//get driver name.
	cJSON* driver_obj=cJSON_GetObjectItem(reponse_from_data,"username");
	if(!driver_obj)
	{

		LOG(module_name,proj_name,"cJSON_GetObjectItem error");
		cJSON_Delete(reponse_from_data);
		reply_request(req,"服务器内部错误");
	}

	char drivername[1024]={0};
	strncpy(drivername,driver_obj->valuestring,strlen(driver_obj->valuestring));
	LOG(module_name,proj_name,"get drivername=%s",drivername);
	cJSON_Delete(reponse_from_data);

///----------------------------------------------
    //assemble data which want 2 send to client.
    root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "result", "ok");
    cJSON_AddStringToObject(root, "username", drivername);

    char *response_data = cJSON_Print(root);
    cJSON_Delete(root);
	reply_request(req,response_data);
	free(response_data);

    if (decoded)
        evhttp_uri_free (decoded);

}
