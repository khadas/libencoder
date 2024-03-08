//#define LOG_NDEBUG 0
#define LOG_TAG "AMLVencInstIntf"
#include <utils/Log.h>

#include <stdio.h>
#include <stdlib.h>
#include "AmlVencInstIntf.h"
#include "AmlVencInst.h"
#include "AmlVencInstWave5.h"
#include "AmlVencInstVcCodec.h"
#include "AmlVencComm.h"



namespace android {



IAmlVencInst *VencGetInstance() {
    AmlVencComm VencComm;
    if (VencComm.SeletcHwEnc() == MULTI_WAVE) {
        ALOGD("interface load wave521...");
        return new AmlVencInstWave5;
    }
    else if (VencComm.SeletcHwEnc() == MULTI_VCCODEC) {
        ALOGD("interface load vccodec...");
        return new AmlVencInstVcCodec;
    }
    ALOGE("cannot find support driver,interface load nothing!!");
    return NULL;
}

void VencDelInstance(IAmlVencInst *pInstance) {
    if ( NULL != pInstance ) {
        delete pInstance;
        pInstance = NULL;
    }
}


}

