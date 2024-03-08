//#define LOG_NDEBUG 0
#define LOG_TAG "AMLVencParamIntf"
#include <utils/Log.h>

#include <stdio.h>
#include "AmlVencParamIntf.h"
#include "AmlVencParam.h"
#include "AmlVencParamWave5.h"
#include "AmlVencParamVcCodec.h"
#include "AmlVencComm.h"


namespace android {


IAmlVencParam *VencParamGetInstance() {
    AmlVencComm VencComm;
    if (VencComm.SeletcHwEnc() == MULTI_WAVE) {
        ALOGD("Param interface load wave521...");
        return new AmlVencParamWave5;
    }
    else if (VencComm.SeletcHwEnc() == MULTI_VCCODEC) {
        ALOGD("Param interface load vccodec...");
        return new AmlVencParamVcCodec;
    }
    ALOGE("cannot find support driver,Param interface load nothing!!");
    return NULL;
}

void VencParamDelInstance(IAmlVencParam *pInstance) {
    if (NULL != pInstance) {
        delete pInstance;
        pInstance = NULL;
    }

}


}



