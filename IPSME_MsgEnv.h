
//
//  IPSME_MsgEnv.h
//

#ifndef IPSME_MSGENV_H
#define IPSME_MSGENV_H

#include <mosquitto.h>
#include <mutex>
#include <vector>

// rather than include an init() in the interface
// we assume the dev knows we are using mosquitto and calls mosquitto_lib_init() and mosquitto_lib_cleanup()

class IPSME_MsgEnv {
public:
	typedef char const * const t_MSG;
	typedef void (*tp_callback)(t_MSG, void* p_void);

	typedef enum : signed int {
		k_ERROR = -1,
		k_NOP = 0,
		k_HANDLED = 1,
	} t_RET;

public:
	IPSME_MsgEnv();
	~IPSME_MsgEnv();

	bool subscribe(tp_callback p_callback, void* p_void);
	bool unsubscribe(tp_callback p_callback);

	bool publish(t_MSG);

	void process_msgs(int i_timeout= 0);

private:
	std::vector< std::pair<void*, void*> > _vec;
	std::mutex _mutex_vec;

	std::unique_ptr<struct mosquitto, decltype(&mosquitto_destroy)> _uptr_mosq_pub;
	std::unique_ptr<struct mosquitto, decltype(&mosquitto_destroy)> _uptr_mosq_sub;
	std::mutex _mutex_mosq_pub;
	std::mutex _mutex_mosq_sub;

	friend void message_callback_(struct mosquitto* mosq, void* p_void, const struct mosquitto_message* p_message);
};

#endif // IPSME_MSGENV_H
