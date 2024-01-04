#include "AmlVencParamWave5.h"


namespace android {

constexpr static size_t kLinearBufferSize = 5 * 1024 * 1024;

#define IS_SUPPORT_DMA         1
#define IS_SUPPORT_CANVAS      1
#define GOP_MIN   0
#define GOP_MAX   (-1)
#define QP_MIN  0
#define QP_MAX  51


AmlVencParamWave5::AmlVencParamWave5() {
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

AmlVencParamWave5::~AmlVencParamWave5() {

}


bool AmlVencParamWave5::GetSizeLimit(stPicSize &pMin,stPicSize &pMax,stPicAlign &align) {
    pMin.width = 176; //get default min max value?????
    pMin.height = 144;
    pMax.width = 3840;
    pMax.height = 2160;
    align.width = 8;
    align.height = 2;
    return true;
}


bool AmlVencParamWave5::GetBitrateLimit(int &pMin,int &pMax) {
    return true;
}

bool AmlVencParamWave5::GetIsSupportDMAMode() {
    return IS_SUPPORT_DMA;
}
bool AmlVencParamWave5::GetIsSupportCanvasMode() {
    return IS_SUPPORT_CANVAS;
}

int AmlVencParamWave5::GetMaxOutputBufferSize() {
    return kLinearBufferSize;
}

bool AmlVencParamWave5::GetSVCLimit(stSVCInfo &SvcInfo) {
    SvcInfo.enable = true;
    SvcInfo.SupportLayerCntMin = 1;
    SvcInfo.SupportLayerCntMax = 3;
    return true;
}


bool AmlVencParamWave5::GetDefaultProfileLevel(C2Config::profile_t &profile,C2Config::level_t       &level) {
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


bool AmlVencParamWave5::GetGopLimit(int &pMin,int &pMax) {
    pMin = GOP_MIN;
    pMax = GOP_MAX;
    return true;
}

bool AmlVencParamWave5::GetQpLimit(int &Min,int &Max) {
    Min = QP_MIN;
    Max = QP_MAX;
    return true;
}


}

