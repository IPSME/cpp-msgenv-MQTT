
//
//  IPSME_MsgEnv.h
//

#ifndef IPSME_MSGENV_H
#define IPSME_MSGENV_H

#include <mosquitto.h>

namespace IPSME_MsgEnv
{
	typedef void (*tp_callback)(const char* psz_msg);
	struct t_handle {
		t_handle() : p_mosq(NULL), p_callback(NULL) {}
		struct mosquitto* p_mosq;
		tp_callback p_callback;
	};
	typedef int RET_TYPE;

	RET_TYPE init(t_handle* p_h);
	RET_TYPE destroy(t_handle* p_h);

	RET_TYPE subscribe(t_handle& h, IPSME_MsgEnv::tp_callback p_callback);
	RET_TYPE unsubscribe(t_handle& h);

	RET_TYPE publish(t_handle& h, const char* psz_msg);

	RET_TYPE process_requests(const t_handle& p_h, int i_timeout= 0);
}

#endif // IPSME_MSGENV_H
