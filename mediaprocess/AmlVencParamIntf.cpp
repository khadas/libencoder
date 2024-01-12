#include <stdio.h>
#include "AmlVencParamIntf.h"
#include "AmlVencParam.h"
#include "AmlVencParamWave5.h"

namespace android {


IAmlVencParam *VencParamGetInstance() {
    return new AmlVencParamWave5;
}

void VencParamDelInstance(IAmlVencParam *pInstance) {
    if (NULL != pInstance) {
        delete pInstance;
        pInstance = NULL;
    }

}


}



