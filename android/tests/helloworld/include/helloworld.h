#ifndef _HELLOWORLD_H_
#define _HELLOWORLD_H_

#define HELLOWORLD_NAME "org.credil.helloworldservice.HelloWorldServiceInterface"

enum {
        HW_HELLOTHERE=1,
	HW_GETBUFFER,
};


class IHelloWorldClient: public android::IInterface {
public:
        DECLARE_META_INTERFACE(HelloWorldClient);

        virtual void hellothere(const char *str) = 0;
        virtual void get_buffer() = 0;
};

#endif
