#ifndef __AML_VENC_INST_VCCODEC_H_
#define __AML_VENC_INST_VCCODEC_H_

#include "AmlVencInst.h"


namespace android {


class AmlVencInstVcCodec : public AmlVencInst {

public:
    AmlVencInstVcCodec();
    virtual ~AmlVencInstVcCodec();

public:
    virtual bool init();
    virtual void ExtraInit(amvenc_info_t &VencInfo,amvenc_qp_param_t &QpParam);
    virtual void ExtraProcess();
    virtual void ExtraPreProcess(stInputFrameInfo &InputFrameInfo);
private:
    bool GetIsSupportSVC();
    int AVCProfileConvert(C2Config::profile_t profile);
    int AVCLevelConvert(C2Config::level_t Level);
    int HEVCLevelConvert(C2Config::level_t Level);
    void GenQpTable(stVencParam VencParam,amvenc_qp_param_t &QpParam);
private:
    bool mIsInit;
    int mActualFormat;
};



}


#endif



