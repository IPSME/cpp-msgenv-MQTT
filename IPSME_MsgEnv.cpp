

#include <assert.h>
#include <string.h>
#include <memory>
#include <mutex>
// #include <iostream>
#include <unordered_map>

#include "IPSME_MsgEnv.h"
using IPSME_MsgEnv::RET_TYPE;
using IPSME_MsgEnv::tp_callback;

const char* psz_channel_pattern_ = "IPSME";

const char* psz_server_address_ = "localhost";
const int i_server_port = 1883;

// https://mosquitto.org/api/files/mosquitto-h.html

//----------------------------------------------------------------------------------------------------------------
// singleton

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

//----------------------------------------------------------------------------------------------------------------

std::vector<std::pair<void*, void*>> vec_;

void message_callback_(struct mosquitto* mosq, void* UNUSED, const struct mosquitto_message* p_message)
{
    // printf("%s: \n", __func__); fflush(stdout);

    for (const auto& pair : vec_) {
        tp_callback p_callback= (tp_callback)pair.first;
        void* p_void= pair.second;

        try {
            p_callback(static_cast<char*>(p_message->payload), p_void);
        }
        catch (...) {
            assert(false);
        }
    }
}

RET_TYPE IPSME_MsgEnv::subscribe(tp_callback p_callback, void* p_void)
{
    if (!p_callback) {
        assert(false);
        return 1;
    }

    // printf("%s: \n", __func__); fflush(stdout);

    vec_.emplace_back(p_callback, p_void);

    Singleton& s = Singleton::getInstance();
    int ret= mosquitto_subscribe(s._uptr_mosq.get(), NULL, psz_channel_pattern_, 0);
    if (ret)
        return ret;

    return 0;
}

RET_TYPE IPSME_MsgEnv::unsubscribe(tp_callback p_callback)
{
    Singleton& s = Singleton::getInstance();
    int ret= mosquitto_unsubscribe(s._uptr_mosq.get(), NULL, psz_channel_pattern_);
    if (ret)
        return ret;

    vec_.erase(std::remove_if(
            vec_.begin(), 
            vec_.end(), 
            [p_callback](const std::pair<void*, void*>& pair) {
                return pair.first == p_callback;
            }),
            vec_.end()
        );

    return 0;
}

RET_TYPE IPSME_MsgEnv::publish(const char* psz_msg)
{
    // printf("%s: \n", __func__); fflush(stdout);

    Singleton& s = Singleton::getInstance();

    int ret = mosquitto_publish(s._uptr_mosq.get(), NULL, psz_channel_pattern_, (int) strlen(psz_msg), psz_msg, 0, false);
    if (ret) {
        // std::cerr << "Can't publish to topic\n";
        return ret;
    }

    return 0;
}

RET_TYPE IPSME_MsgEnv::process_requests(int i_timeout)
{
    Singleton& s = Singleton::getInstance();
    mosquitto_loop(s._uptr_mosq.get(), i_timeout, 1);

    return 0;
}