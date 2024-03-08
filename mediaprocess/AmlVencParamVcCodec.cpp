#include "AmlVencParamVcCodec.h"


namespace android {

constexpr static size_t kLinearBufferSize = 5 * 1024 * 1024;

#define IS_SUPPORT_DMA         1
#define IS_SUPPORT_CANVAS      0
#define GOP_MIN   0
#define GOP_MAX   (-1)
#define QP_MIN  0
#define QP_MAX  51


AmlVencParamVcCodec::AmlVencParamVcCodec() {
    mVencParam.QpInfo.enable = false;
    mVencParam.QpInfo.QpMin = QP_MIN;
    mVencParam.QpInfo.QpMax = QP_MAX;
    mVencParam.QpInfo.iMin = QP_MIN;
    mVencParam.QpInfo.iMax = QP_MAX;
    mVencParam.QpInfo.pMin = QP_MIN;
    mVencParam.QpInfo.pMax = QP_MAX;
    mVencParam.QpInfo.bMin = QP_MIN;
    mVencParam.QpInfo.bMax = QP_MAX;
}

AmlVencParamVcCodec::~AmlVencParamVcCodec() {

}


bool AmlVencParamVcCodec::GetSizeLimit(stPicSize &pMin,stPicSize &pMax,stPicAlign &align) {
    pMin.width = 176; //get default min max value?????
    pMin.height = 144;
    pMax.width = 3840;
    pMax.height = 2160;
    align.width = 8;
    align.height = 2;
    return true;
}


bool AmlVencParamVcCodec::GetBitrateLimit(int &pMin,int &pMax) {
    return true;
}

bool AmlVencParamVcCodec::GetIsSupportDMAMode() {
    return IS_SUPPORT_DMA;
}
bool AmlVencParamVcCodec::GetIsSupportCanvasMode() {
    return IS_SUPPORT_CANVAS;
}

int AmlVencParamVcCodec::GetMaxOutputBufferSize() {
    return kLinearBufferSize;
}

bool AmlVencParamVcCodec::GetSVCLimit(stSVCInfo &SvcInfo) {
    SvcInfo.enable = false;
    //SvcInfo.SupportLayerCntMin = 1;
    //SvcInfo.SupportLayerCntMax = 3;
    return true;
}


bool AmlVencParamVcCodec::GetDefaultProfileLevel(C2Config::profile_t &profile,C2Config::level_t       &level) {
    if (H264 == mCodecType) {
        profile = PROFILE_AVC_MAIN;
        level = LEVEL_AVC_4_1;
    }
    else if(H265 == mCodecType) {
        profile = PROFILE_HEVC_MAIN;
        level = LEVEL_HEVC_MAIN_5_1;
    }
    else {
        return false;
    }
    return true;
}


bool AmlVencParamVcCodec::GetGopLimit(int &pMin,int &pMax) {
    pMin = GOP_MIN;
    pMax = GOP_MAX;
    return true;
}

bool AmlVencParamVcCodec::GetQpLimit(int &Min,int &Max) {
    Min = QP_MIN;
    Max = QP_MAX;
    return true;
}


}

