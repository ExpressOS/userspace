#ifndef _HELLOWORLDSERVICE_H_
#define _HELLOWORLDSERVICE_H_

#include <utils/KeyedVector.h>
#include <utils/RefBase.h>
#include <binder/IInterface.h>
#include <binder/Parcel.h>
#include <utils/String16.h>
#include <utils/threads.h>
#include <string.h>

#include <utils/Log.h>

namespace android {
class IMemoryHeap;
}

class IHelloWorldService: public android::IInterface {
public:

        DECLARE_META_INTERFACE(HelloWorldService);

        void hellothere(const char *str);
        void get_buffer();
};

class BnHelloWorldService : public android::BnInterface<IHelloWorldService>
{
	// not sure.
        // actual dispatch.
};

class HelloWorldService : public BnHelloWorldService
{
	class Client;

public:
    static  void                instantiate();

//    class Client : public BnMediaPlayer {
//
//        // IHelloWorld interface
//    }

                            HelloWorldService();
    virtual                 ~HelloWorldService();

    android::status_t onTransact(uint32_t code,
                                 const android::Parcel &data,
                                 android::Parcel *reply,
                                 uint32_t flags);

    mutable     android::Mutex                       mLock;
    android::SortedVector< android::wp<Client> >     mClients;
    int32_t                     mNextConnId;
    android::sp<android::IMemoryHeap> mSharedMemory; 
};


#endif
