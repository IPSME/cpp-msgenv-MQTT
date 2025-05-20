

#include <assert.h>
#include <string.h>
#include <memory>
// #include <iostream>
#include <unordered_map>

#include "IPSME_MsgEnv.h"
using RET_TYPE = IPSME_MsgEnv::RET_TYPE;
using tp_callback = IPSME_MsgEnv::tp_callback;

const char* psz_channel_pattern_ = "IPSME";

const char* psz_server_address_ = "localhost";
const int i_server_port = 1883;

// https://mosquitto.org/api/files/mosquitto-h.html

//----------------------------------------------------------------------------------------------------------------
// singleton

/*
void message_callback_(struct mosquitto* mosq, void* obj, const struct mosquitto_message* message);

class Singleton {
public:
    static Singleton& getInstance() {
        static std::once_flag onceFlag;
        std::call_once(onceFlag, []() { _instance.reset(new Singleton); });
        return *_instance;
    }

    // if _uptr_mosq is made private, use a getter().
    // std::unique_ptr<struct mosquitto, decltype(&mosquitto_destroy)>& getMosquitto() { return _uptr_mosq; }

private:
    friend struct std::default_delete<Singleton>;

    Singleton() : _uptr_mosq(mosquitto_new(NULL, true, NULL), mosquitto_destroy) 
    {
        mosquitto_message_callback_set(_uptr_mosq.get(), message_callback_);

        int delay = 1;  // Initial delay in seconds
        const int max_delay = 60;  // Maximum delay in seconds

        while (true) {
            int connect_result = mosquitto_connect(_uptr_mosq.get(), "localhost", 1883, 60);
            if (connect_result == MOSQ_ERR_SUCCESS)
                break;

            // std::cerr << "Mosquitto connect failed: " << mosquitto_strerror(connect_result) << std::endl;

            std::this_thread::sleep_for(std::chrono::seconds(delay));
            delay *= 2;

            if (delay > max_delay)
                delay = max_delay;
        }
    }

    ~Singleton() = default;
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;

    static std::unique_ptr<Singleton> _instance;

public: // private: 
    std::unique_ptr<struct mosquitto, decltype(&mosquitto_destroy)> _uptr_mosq;
};

std::unique_ptr<Singleton> Singleton::_instance;
*/

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
            p_callback(static_cast<IPSME_MsgEnv::MSG_TYPE>(p_message->payload), p_void_payload);
        }
        catch (...) {
            assert(false);
        }
    }
}

//----------------------------------------------------------------------------------------------------------------

IPSME_MsgEnv::IPSME_MsgEnv() : _uptr_mosq(mosquitto_new(NULL, true, this), mosquitto_destroy)
{
    std::lock_guard<std::mutex> lock(_mutex_mosq);
    mosquitto_message_callback_set(_uptr_mosq.get(), message_callback_);

    int delay = 1;  // Initial delay in seconds
    const int max_delay = 60;  // Maximum delay in seconds

    while (true) {
        int connect_result = mosquitto_connect(_uptr_mosq.get(), "localhost", 1883, 60);
        if (connect_result == MOSQ_ERR_SUCCESS)
            break;

        // std::cerr << "Mosquitto connect failed: " << mosquitto_strerror(connect_result) << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(delay));
        delay *= 2;

        if (delay > max_delay)
            delay = max_delay;
    }
    
    // use manual loop mode
    // int loop_result = mosquitto_loop_start(_uptr_mosq.get());
    // if (loop_result != MOSQ_ERR_SUCCESS) {
    //     throw std::runtime_error("Failed to start Mosquitto loop: " + std::string(mosquitto_strerror(loop_result)));
    // }
}

IPSME_MsgEnv::~IPSME_MsgEnv()
{
    std::lock_guard<std::mutex> lock(_mutex_mosq);
    mosquitto_disconnect(_uptr_mosq.get());
}

//----------------------------------------------------------------------------------------------------------------

RET_TYPE IPSME_MsgEnv::subscribe(tp_callback p_callback, void* p_void)
{
    if (!p_callback) {
        assert(false);
        return 1;
    }

    // printf("%s: \n", __func__); fflush(stdout);

    {
        std::lock_guard<std::mutex> lock(_mutex_vec);
        _vec.emplace_back(p_callback, p_void);
    }

    std::lock_guard<std::mutex> lock(_mutex_mosq);
    int ret= mosquitto_subscribe(_uptr_mosq.get(), NULL, psz_channel_pattern_, 0);
    if (ret)
        return ret;

    return 0;
}

RET_TYPE IPSME_MsgEnv::unsubscribe(tp_callback p_callback)
{
    std::lock_guard<std::mutex> lock(_mutex_mosq);
    int ret= mosquitto_unsubscribe(_uptr_mosq.get(), NULL, psz_channel_pattern_);
    if (ret)
        return ret;

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

    return 0;
}

RET_TYPE IPSME_MsgEnv::publish(MSG_TYPE msg)
{
    // printf("%s: \n", __func__); fflush(stdout);

    std::lock_guard<std::mutex> lock(_mutex_mosq);
    int ret = mosquitto_publish(_uptr_mosq.get(), NULL, psz_channel_pattern_, (int) strlen(msg), msg, 0, false);
    if (ret) {
        // std::cerr << "Can't publish to topic\n";
        return ret;
    }

    return 0;
}

RET_TYPE IPSME_MsgEnv::process_requests(int i_timeout)
{
    std::lock_guard<std::mutex> lock(_mutex_mosq);
    mosquitto_loop(_uptr_mosq.get(), i_timeout, 1);

    return 0;
}
