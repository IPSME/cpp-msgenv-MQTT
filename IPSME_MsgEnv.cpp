

#include <assert.h>
#include <string.h>
#include <memory>
//#include <iostream>
#include <unordered_map>

#include "../g_.hpp"

#include "IPSME_MsgEnv.h"
using tp_callback = IPSME_MsgEnv::tp_callback;

constexpr char const * const psz_channel_pattern_ = "IPSME";

constexpr char const * const psz_server_address_ = "localhost";
constexpr const int i_server_port = 1883;

// https://mosquitto.org/api/files/mosquitto-h.html

//----------------------------------------------------------------------------------------------------------------

void message_callback_(struct mosquitto* mosq, void* p_void_env, const struct mosquitto_message* p_message)
{
     IPSME_MsgEnv* env = static_cast<IPSME_MsgEnv*>(p_void_env);

    // printf("%s: \n", __func__); fflush(stdout);

    std::vector< std::pair<void*, void*> > vec_local;
    {
        std::lock_guard<std::mutex> lock(env->_mutex_vec);
        vec_local = env->_vec;
    }

    for (const auto& pair : vec_local) {
        tp_callback p_callback= (tp_callback)pair.first;
        void* p_void_payload= pair.second;

        try {
            if (p_message->payloadlen > 0 && p_message->payload) {
                // MQTT message payloads are not guaranteed to be null-terminated in the Mosquitto library
                std::string str_payload(static_cast<IPSME_MsgEnv::t_MSG>(p_message->payload), p_message->payloadlen);
                p_callback(str_payload.c_str(), p_void_payload);
            }
            else
                DebugPrint("Invalid or empty payload\n");
        }
        catch (...) {
            assert(false);
        }
    }
}

//----------------------------------------------------------------------------------------------------------------

IPSME_MsgEnv::IPSME_MsgEnv() : 
    _uptr_mosq_pub(mosquitto_new(NULL, true, NULL), mosquitto_destroy),
    _uptr_mosq_sub(mosquitto_new(NULL, true, this), mosquitto_destroy) // NOTE: this ptr
{
    // Initialize subscriber connection
    {
        std::lock_guard<std::mutex> lock(_mutex_mosq_sub);
        mosquitto_message_callback_set(_uptr_mosq_sub.get(), message_callback_);

        int delay = 1;  // Initial delay in seconds
        const int max_delay = 60;  // Maximum delay in seconds

        while (true) {
            int connect_result = mosquitto_connect(_uptr_mosq_sub.get(), psz_server_address_, i_server_port, 60);
            if (connect_result == MOSQ_ERR_SUCCESS)
                break;

#pragma message("TODO: failed connection shouldn't hang silently e.g., if c'tor called before mosq init")
            DebugPrint("Mosquitto connect failed: [%d] \n", mosquitto_strerror(connect_result));

            std::this_thread::sleep_for(std::chrono::seconds(delay));
            delay *= 2;

            if (delay > max_delay)
                delay = max_delay;
        }
    }

    // Initialize publisher connection
    {
        std::lock_guard<std::mutex> lock(_mutex_mosq_pub);

        int delay = 1;  // Initial delay in seconds
        const int max_delay = 60;  // Maximum delay in seconds

        while (true) {
            int connect_result = mosquitto_connect(_uptr_mosq_pub.get(), psz_server_address_, i_server_port, 60);
            if (connect_result == MOSQ_ERR_SUCCESS)
                break;

            std::this_thread::sleep_for(std::chrono::seconds(delay));
            delay *= 2;
            if (delay > max_delay)
                delay = max_delay;
        }
    }
    
    // use manual loop mode: Failed to start Mosquitto loop : This feature is not supported.
    // int loop_result = mosquitto_loop_start(_uptr_mosq.get());
    // if (loop_result != MOSQ_ERR_SUCCESS)
    //     throw std::runtime_error("Failed to start Mosquitto loop: " + std::string(mosquitto_strerror(loop_result)));
}

IPSME_MsgEnv::~IPSME_MsgEnv()
{
    {
        std::lock_guard<std::mutex> lock(_mutex_mosq_pub);
        mosquitto_disconnect(_uptr_mosq_pub.get());
    }
    {
        std::lock_guard<std::mutex> lock(_mutex_mosq_sub);
        mosquitto_disconnect(_uptr_mosq_sub.get());
    }
}

//----------------------------------------------------------------------------------------------------------------

/* Grok:

Issue: The subscribe function subscribes to the same topic(psz_channel_pattern_) every time a new callback is added, which is inefficient and may fail if the broker rejects duplicate subscriptions(depending on the broker’s implementation).Similarly, unsubscribe removes the subscription for the topic even if other callbacks in _vec still need it, potentially causing subsequent messages to be missed.

- Track the subscription state to avoid redundant subscriptions.For example, maintain a counter for active subscriptions to psz_channel_pattern_.
- Only unsubscribe when the last callback is removed :

bool IPSME_MsgEnv::subscribe(tp_callback p_callback, void* p_void) {
    if (!p_callback) return false;
    {
        std::lock_guard<std::mutex> lock(_mutex_vec);
        _vec.emplace_back(p_callback, p_void);
        if (_vec.size() == 1) { // First callback
            std::lock_guard<std::mutex> lock(_mutex_mosq_sub);
            int ret = mosquitto_subscribe(_uptr_mosq_sub.get(), NULL, psz_channel_pattern_, 0);
            if (ret) return false;
        }
    }
    return true;
}
bool IPSME_MsgEnv::unsubscribe(tp_callback p_callback) {
    std::lock_guard<std::mutex> lock(_mutex_vec);
    _vec.erase(std::remove_if(
        _vec.begin(), _vec.end(),
        [p_callback](const std::pair<void*, void*>& pair) { return pair.first == p_callback; }),
        _vec.end());
    if (_vec.empty()) {
        std::lock_guard<std::mutex> lock(_mutex_mosq_sub);
        int ret = mosquitto_unsubscribe(_uptr_mosq_sub.get(), NULL, psz_channel_pattern_);
        if (ret) return false;
    }
    return true;
}
*/

bool IPSME_MsgEnv::subscribe(tp_callback p_callback, void* p_void)
{
    if (!p_callback) {
        assert(false);
        return false;
    }

    // printf("%s: \n", __func__); fflush(stdout);

    {
        std::lock_guard<std::mutex> lock(_mutex_vec);
        _vec.emplace_back(p_callback, p_void);
    }

    std::lock_guard<std::mutex> lock(_mutex_mosq_sub);
    int ret= mosquitto_subscribe(_uptr_mosq_sub.get(), NULL, psz_channel_pattern_, 0);
    if (ret)
        return false;

    return true;
}

bool IPSME_MsgEnv::unsubscribe(tp_callback p_callback)
{
    std::lock_guard<std::mutex> lock(_mutex_mosq_sub);
    int ret= mosquitto_unsubscribe(_uptr_mosq_sub.get(), NULL, psz_channel_pattern_);
    if (ret)
        return false;

    {
        std::lock_guard<std::mutex> lock(_mutex_vec);
        _vec.erase(std::remove_if(
                _vec.begin(), 
                _vec.end(), 
                [p_callback](const std::pair<void*, void*>& pair) {
                    return pair.first == p_callback;
                }),
                _vec.end()
            );
    }

    return true;
}

bool IPSME_MsgEnv::publish(t_MSG msg)
{
    // printf("%s: \n", __func__); fflush(stdout);

    std::lock_guard<std::mutex> lock(_mutex_mosq_pub);
    int ret = mosquitto_publish(_uptr_mosq_pub.get(), NULL, psz_channel_pattern_, (int) strlen(msg), msg, 0, false);
    if (ret) {
        DebugPrint("Can't publish to topic\n");
        return false;
    }

    return true;
}

void IPSME_MsgEnv::process_msgs(int i_timeout)
{
    {
        std::lock_guard<std::mutex> lock(_mutex_mosq_pub);
        int ret = mosquitto_loop(_uptr_mosq_pub.get(), i_timeout, 1);
        if (ret != MOSQ_ERR_SUCCESS)
        {
            DebugPrint("mosquitto_loop(pub) failed: %d\n", mosquitto_strerror(ret));
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep for 100 milliseconds

            if (ret == MOSQ_ERR_CONN_LOST || ret == MOSQ_ERR_NO_CONN) {
                //int delay = 1;
                //const int max_delay = 60;
                //while (true) {
                //    int connect_result = mosquitto_connect(_uptr_mosq_sub.get(), psz_server_address_, i_server_port, 60);
                //    if (connect_result == MOSQ_ERR_SUCCESS) break;
                //    std::this_thread::sleep_for(std::chrono::seconds(delay));
                //    delay = std::min(delay * 2, max_delay);
            }
        }
    }
    {
        std::lock_guard<std::mutex> lock(_mutex_mosq_sub);
        int ret = mosquitto_loop(_uptr_mosq_sub.get(), i_timeout, 1);
        if (ret != MOSQ_ERR_SUCCESS)
        {
            DebugPrint("mosquitto_loop(sub) failed: %d\n", mosquitto_strerror(ret));
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep for 100 milliseconds

            if (ret == MOSQ_ERR_CONN_LOST || ret == MOSQ_ERR_NO_CONN) {
            }
        }
    }
}
