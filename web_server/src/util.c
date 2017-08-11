#include "util.h"

/* -------------------------------------------*/
/**
 * @brief  为curl提供的处理服务器返回数据的回调函数
 *
 * @param ptr
 * @param n
 * @param m
 * @param arg
 *
 * @returns   
 *    
 */
/* -------------------------------------------*/
size_t deal_response_data(void *ptr, size_t n, size_t m, void *arg)
{
    response_data_t *res_data = (response_data_t*)arg;

    int count = m*n;

    memcpy(res_data->data, ptr, count);

    return count;
}


/* -------------------------------------------*/
/**
 * @brief  随机得到一个uuid
 *
 * @param str
 *
 * @returns   
 */
/* -------------------------------------------*/
char* get_random_uuid(char *str)
{
    uuid_t uuid;

    uuid_generate(uuid);
    uuid_unparse(uuid, str);

    return str; 
}


/* -------------------------------------------*/
/**
 * @brief  根据用户类别得到一个sessionid
 *
 * @param isDriver
 * @param sessionid
 *
 * @returns   
 */
/* -------------------------------------------*/
char *create_sessionid(const char *isDriver, char*sessionid)
{
    char uuid[36] = {0};

    if (strcmp(isDriver, "yes") == 0) {
        //司机
        sprintf(sessionid, "online-driver-%s", get_random_uuid(uuid));
    }
    else {
        //乘客
        sprintf(sessionid, "online-user-%s", get_random_uuid(uuid));
    }

    return sessionid;
}

