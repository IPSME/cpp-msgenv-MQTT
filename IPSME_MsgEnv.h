
//
//  IPSME_MsgEnv.h
//

#ifndef IPSME_MSGENV_H
#define IPSME_MSGENV_H

#include <mosquitto.h>

// rather than include an init() in the interface
// we assume the dev knows we are using mosquitto and calls mosquitto_lib_init() and mosquitto_lib_cleanup()

namespace IPSME_MsgEnv
{
	typedef void (*tp_callback)(const char* psz_msg, void* p_void);
	typedef int RET_TYPE;

	RET_TYPE subscribe(tp_callback p_callback, void* p_void);
	RET_TYPE unsubscribe(tp_callback p_callback);

	RET_TYPE publish(const char* psz_msg);

	RET_TYPE process_requests(int i_timeout= 0);
}

#endif // IPSME_MSGENV_H
