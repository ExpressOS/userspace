#define LOG_TAG "HelloWorldService"

#include <utils/Log.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

#include <string.h>
#include <cutils/atomic.h>
#include <utils/Errors.h>
#include <binder/IServiceManager.h>
#include <utils/String16.h>

#include <helloworldservice.h>
#include <helloworld.h>
#include "utils/Log.h"

#include <unistd.h>

#include <binder/MemoryHeapBase.h>

const android::String16 IHelloWorldService::descriptor(HELLOWORLD_NAME);

const android::String16 & IHelloWorldService::getInterfaceDescriptor() const {
        return IHelloWorldService::descriptor;
}

IHelloWorldService::~IHelloWorldService()
{}

IHelloWorldService::IHelloWorldService()
{}

void HelloWorldService::instantiate() {
	android::defaultServiceManager()->addService(
                IHelloWorldService::descriptor, new HelloWorldService());
}

HelloWorldService::HelloWorldService()
{
    LOGE("HelloWorldService created");
    mNextConnId = 1;
    mSharedMemory = new android::MemoryHeapBase(4096);
    unsigned int *base = (unsigned int *) mSharedMemory->getBase();
    *base=0xdeadcafe; //Initiate first value in buffer
}

HelloWorldService::~HelloWorldService()
{
    LOGE("HelloWorldService destroyed");
}

android::status_t HelloWorldService::onTransact(uint32_t code,
                                                const android::Parcel &data,
                                                android::Parcel *reply,
                                                uint32_t flags)
{
        LOGE("OnTransact(%u,%u)", code, flags);

        using android::PERMISSION_DENIED;
        
        switch(code) {
        case HW_HELLOTHERE: {
                CHECK_INTERFACE(IHelloWorldService, data, reply);
                const char *str;
                str = data.readCString();
                /* hellothere(str); */
                LOGE("hello: %s\n", str);
                printf("hello: %s\n", str);
                return android::NO_ERROR;
        } break;
	case HW_GETBUFFER: {
                CHECK_INTERFACE(IHelloWorldService, data, reply);
                printf("Getting buffer, mSharedMemory=%p\n", mSharedMemory.get());
                reply->writeStrongBinder(mSharedMemory->asBinder());
                return android::NO_ERROR;
	}
	break;
        default:
                return BBinder::onTransact(code, data, reply, flags);
        }

        return android::NO_ERROR;
}

