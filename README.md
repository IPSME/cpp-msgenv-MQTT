# cpp-msgenv-MQTT
This library contains the wrapper code for sending messages to the macOS messaging environment (ME). The ready available pubsub between processes on macOS is the `NSDistributedNotificationCenter`.

> ### IPSME- Idempotent Publish/Subscribe Messaging Environment
> https://dl.acm.org/doi/abs/10.1145/3458307.3460966

#### Initializiing
```
IPSME_MsgEnv::t_handle h;
IPSME_MsgEnv::init(&h);
```

#### Subscribing
```
void handler_(const char* psz_msg)
{
    std::cout << "Received message: " << psz_msg << std::endl;

}

RET_TYPE ret = IPSME_MsgEnv::subscribe(h, &handler_);
assert(ret == 0);

while (true) {
     IPSME_MsgEnv::process_requests(h);

     // ...
}
```

It is by design that a participant receives the messages it has published itself. If this is not desirable, each message can contain a "referer" (sic) identifier and a clause added in the `ipsme_handler_` to drop those messages containing the participant's own referer id.

#### Publishing
```
RET_TYPE ret = IPSME_MsgEnv::publish(h, "...");
assert(ret == 0);
```


## Discussion

This implementation uses Eclipse Mosquitto as a dependency. A native implementation can be done later.

https://mosquitto.org/
