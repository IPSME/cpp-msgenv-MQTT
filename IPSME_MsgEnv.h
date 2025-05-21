
//
//  IPSME_MsgEnv.h
//

#ifndef IPSME_MSGENV_H
#define IPSME_MSGENV_H

#include <mosquitto.h>
#include <mutex>

// rather than include an init() in the interface
// we assume the dev knows we are using mosquitto and calls mosquitto_lib_init() and mosquitto_lib_cleanup()

class IPSME_MsgEnv {
public:
	typedef int RET_TYPE;
	typedef char const * const MSG_TYPE;

	typedef void (*tp_callback)(MSG_TYPE, void* p_void);

public:
	IPSME_MsgEnv();
	~IPSME_MsgEnv();

	RET_TYPE subscribe(tp_callback p_callback, void* p_void);
	RET_TYPE unsubscribe(tp_callback p_callback);

	RET_TYPE publish(MSG_TYPE);

	RET_TYPE process_requests(int i_timeout= 0);

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
