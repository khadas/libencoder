#ifndef __AML_VENC_PARAM_WAVE5_H_
#define __AML_VENC_PARAM_WAVE5_H_

#include "AmlVencParam.h"


namespace android {


class AmlVencParamWave5 : public AmlVencParam {

public:
    AmlVencParamWave5();
    virtual ~AmlVencParamWave5();

public:
    bool GetSizeLimit(stPicSize &pMin,stPicSize &pMax,stPicAlign &align);
    bool GetBitrateLimit(int &pMin,int &pMax);
    bool GetIsSupportDMAMode();
    bool GetIsSupportCanvasMode();
    bool GetDefaultProfileLevel(C2Config::profile_t &profile,C2Config::level_t &level);
    bool GetSVCLimit(stSVCInfo &SvcInfo);
    bool GetGopLimit(int &pMin,int &pMax);
    bool GetQpLimit(int &Min,int &Max);
    int GetMaxOutputBufferSize();
};


}



#endif

