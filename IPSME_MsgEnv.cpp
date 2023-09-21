

#include <string.h>
// #include <iostream>

#include "IPSME_MsgEnv.h"
using IPSME_MsgEnv::RET_TYPE;
using IPSME_MsgEnv::tp_callback;

const char* psz_channel_pattern_ = "IPSME";

const char* psz_server_address_ = "localhost";
const int i_server_port = 1883;

void messageCallback(struct mosquitto* mosq, void* obj, const struct mosquitto_message* message)
{
    // printf("%s: \n", __func__); fflush(stdout);

    IPSME_MsgEnv::t_handle* p_h = static_cast<IPSME_MsgEnv::t_handle*>(obj);

    if (p_h->p_callback)
        p_h->p_callback(static_cast<char*>(message->payload));
}

RET_TYPE IPSME_MsgEnv::init(t_handle* p_h)
{
	mosquitto_lib_init();

    p_h->p_mosq = mosquitto_new(NULL, true, p_h);

    mosquitto_message_callback_set(p_h->p_mosq, messageCallback);

    int ret = mosquitto_connect(p_h->p_mosq, psz_server_address_, i_server_port, 60);
    if (ret) {
        // std::cerr << "Can't connect to broker\n"; // did init() get called?
        return ret;
    }

    return 0;
}

RET_TYPE IPSME_MsgEnv::destroy(t_handle* p_h)
{
    mosquitto_disconnect(p_h->p_mosq);
    mosquitto_destroy(p_h->p_mosq);
    p_h->p_mosq = NULL;
    p_h->p_callback = NULL;

    mosquitto_lib_cleanup();
    return 0;
}

RET_TYPE IPSME_MsgEnv::subscribe(t_handle& h, tp_callback t_handle)
{
    // printf("%s: \n", __func__); fflush(stdout);

    h.p_callback = t_handle;
    int ret= mosquitto_subscribe(h.p_mosq, NULL, psz_channel_pattern_, 0);
    if (ret)
        return ret;

    return 0;
}

RET_TYPE IPSME_MsgEnv::unsubscribe(t_handle& h)
{
    int ret= mosquitto_unsubscribe(h.p_mosq, NULL, psz_channel_pattern_);
    if (ret)
        return ret;

    h.p_callback = NULL;
    return 0;
}

RET_TYPE IPSME_MsgEnv::publish(t_handle& h, const char* psz_msg)
{
    // printf("%s: \n", __func__); fflush(stdout);

    int ret = mosquitto_publish(h.p_mosq, NULL, psz_channel_pattern_, (int) strlen(psz_msg), psz_msg, 0, false);
    if (ret) {
        // std::cerr << "Can't publish to topic\n";
        return ret;
    }

    return 0;
}

RET_TYPE IPSME_MsgEnv::process_requests(const t_handle& h, int i_timeout)
{
    mosquitto_loop(h.p_mosq, i_timeout, 1);

    return 0;
}