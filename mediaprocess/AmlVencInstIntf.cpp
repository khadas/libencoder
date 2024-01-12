#include <stdio.h>
#include <stdlib.h>
#include "AmlVencInstIntf.h"
#include "AmlVencInst.h"
#include "AmlVencInstWave5.h"

namespace android {



IAmlVencInst *VencGetInstance() {
    return new AmlVencInstWave5;
}

void VencDelInstance(IAmlVencInst *pInstance) {
    if ( NULL != pInstance ) {
        delete pInstance;
        pInstance = NULL;
    }
}


}

