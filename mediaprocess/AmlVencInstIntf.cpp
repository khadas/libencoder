#include <stdio.h>
#include <stdlib.h>
#include "AmlVencInstIntf.h"
#include "AmlVencInst.h"
#include "AmlVencInstWave5.h"

namespace android {



IAmlVencInst *IAmlVencInst::GetInstance() {
    return new AmlVencInstWave5;
}

void IAmlVencInst::DelInstance(IAmlVencInst *pInstance) {
    if (NULL != pInstance) {
        delete pInstance;
        pInstance = NULL;
    }
}


}

