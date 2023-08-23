#include <stdio.h>
#include "AmlVencParamIntf.h"
#include "AmlVencParam.h"
#include "AmlVencParamWave5.h"

namespace android {


IAmlVencParam *IAmlVencParam::GetInstance() {
    return new AmlVencParamWave5;
}

void IAmlVencParam::DelInstance(IAmlVencParam *pInstance) {
    if (NULL != pInstance) {
        delete pInstance;
        pInstance = NULL;
    }

}


}



