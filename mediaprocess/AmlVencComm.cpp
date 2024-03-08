//#define LOG_NDEBUG 0
#define LOG_TAG "AMLVencCommon"
#include <utils/Log.h>

#include "AmlVencComm.h"


namespace android {

#define DRIVER_MULTI_WAVE   "/dev/amvenc_multi"
#define DRIVER_MULTI_VCCODEC     "/dev/vc8000"



AmlVencComm::AmlVencComm() {

}

AmlVencComm::~AmlVencComm() {

}


E_HW_ENC AmlVencComm::SeletcHwEnc () {
    if (access(DRIVER_MULTI_WAVE, F_OK ) != -1) {
        ALOGD("interface load wave521...");
        return MULTI_WAVE;
    }
    else if (access(DRIVER_MULTI_VCCODEC, F_OK ) != -1) {
        ALOGD("interface load vccodec...");
        return MULTI_VCCODEC;
    }
    ALOGE("cannot find support driver,interface load nothing!!");
    return HWENC_NONE;
}


}



