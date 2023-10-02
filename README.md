# cpp-msgenv-MQTT
This library contains the wrapper code for sending messages to a Windows messaging environment (ME). For this implementation, MQTT is used as ME, implemented using Eclipse Mosquitto.

> ### IPSME- Idempotent Publish/Subscribe Messaging Environment
> https://dl.acm.org/doi/abs/10.1145/3458307.3460966

#### Initializiing && Cleanup
```
#include "cpp-msgenv-MQTT.git/IPSME_MsgEnv.h"

int main() 
{
    mosquitto_lib_init();

    // ...

    mosquitto_lib_cleanup();

    return 0;
}

```

* include: ```<path>\mosquitto\devel```
* lib: ```<path>\mosquitto\devel```
* dependencies: ```mosquitto.lib```
* runtime: ```PATH=($PATH);<path>\mosquitto```


#### Subscribing
```
void handler_(const char* psz_msg, void* p_void)
{
    std::cout << "Received message: " << psz_msg << std::endl;

}

RET_TYPE ret = IPSME_MsgEnv::subscribe(&handler_, NULL);
assert(ret == 0);

while (true) {
     IPSME_MsgEnv::process_requests();

     // ...
}
```

It is by design that a participant receives the messages it has published itself. If this is not desirable, each message can contain a "referer" (sic) identifier and a clause added in the `ipsme_handler_` to drop those messages containing the participant's own referer id.

#### Publishing
```
RET_TYPE ret = IPSME_MsgEnv::publish("...");
assert(ret == 0);
```


## Discussion

This implementation uses Eclipse Mosquitto as a dependency. A native implementation can be done later.

https://mosquitto.org/
