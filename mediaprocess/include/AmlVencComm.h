#ifndef __AML_VENC_COMMON_H_
#define __AML_VENC_COMMON_H_

#include<stdio.h>
#include<stdlib.h>
#include<string.h>


namespace android {

typedef enum _E_HW_ENC {
    HWENC_NONE,
    MULTI_WAVE,
    MULTI_VCCODEC
}E_HW_ENC;

class AmlVencComm {

public:
    AmlVencComm();
    virtual ~AmlVencComm();

public:
    E_HW_ENC SeletcHwEnc();
};



}







#endif

