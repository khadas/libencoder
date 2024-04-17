//#define LOG_NDEBUG 0
#define LOG_TAG "AMLVencVcCodec"
#include <utils/Log.h>

#include <cutils/properties.h>
#include "AmlVencInstVcCodec.h"


namespace android {

#define ENC_PROFILE_AUTO        0
#define ENC_PROFILE_BASELINE    9
#define ENC_PROFILE_MAIN        10
#define ENC_PROFILE_HIGH        11




#define ENCODER_PROP_SVC_ENABLE        "vendor.media.c2.encoder.svc"


AmlVencInstVcCodec::AmlVencInstVcCodec()
                 :mIsInit(false),
                  mActualFormat(AML_IMG_FMT_NONE) {

}


AmlVencInstVcCodec::~AmlVencInstVcCodec() {

}


bool AmlVencInstVcCodec::GetIsSupportSVC() {
    stSVCInfo SVC;
    bool svc_enable = false;
    memset(&SVC,0,sizeof(SVC));

    mVencParamInst->GetSVCLimit(SVC);

    svc_enable = property_get_bool(ENCODER_PROP_SVC_ENABLE, false);
    if (SVC.enable && !svc_enable) {
        ALOGD("hw encoder support svc feuture,use vendor.media.c2.encoder.svc to open it!");
    }
    return (svc_enable && SVC.enable);

}


int AmlVencInstVcCodec::AVCProfileConvert(C2Config::profile_t profile) {
//only for 264
    int retProfile = 0;
    switch (profile) {
        case PROFILE_AVC_BASELINE:
            retProfile = ENC_PROFILE_BASELINE;
            break;
        case PROFILE_AVC_MAIN:
            retProfile = ENC_PROFILE_MAIN;
            break;
        case PROFILE_AVC_HIGH:
            retProfile = ENC_PROFILE_HIGH;
            break;
        default:
            retProfile = ENC_PROFILE_BASELINE;
            break;
    }
    return retProfile;
}

int AmlVencInstVcCodec::AVCLevelConvert(C2Config::level_t Level) {
//only for 264
    ALOGE("AVCLevelConvert level:%x",Level);
    int retLevel = 0;
    switch (Level) {
        case LEVEL_AVC_1:
            retLevel = MEDIA_AVC_LEVEL1;
            break;
        case LEVEL_AVC_1B:
            retLevel = MEDIA_AVC_LEVEL1_B;
            break;
        case LEVEL_AVC_1_1:
            retLevel = MEDIA_AVC_LEVEL1_1;
            break;
        case LEVEL_AVC_1_2:
            retLevel = MEDIA_AVC_LEVEL1_2;
            break;
        case LEVEL_AVC_1_3:
            retLevel = MEDIA_AVC_LEVEL1_3;
            break;
        case LEVEL_AVC_2:
            retLevel = MEDIA_AVC_LEVEL2;
            break;
        case LEVEL_AVC_2_1:
            retLevel = MEDIA_AVC_LEVEL2_1;
            break;
        case LEVEL_AVC_2_2:
            retLevel = MEDIA_AVC_LEVEL2_2;
            break;
        case LEVEL_AVC_3:
            retLevel = MEDIA_AVC_LEVEL3;
            break;
        case LEVEL_AVC_3_1:
            retLevel = MEDIA_AVC_LEVEL3_1;
            break;
        case LEVEL_AVC_3_2:
            retLevel = MEDIA_AVC_LEVEL3_2;
            break;
        case LEVEL_AVC_4:
            retLevel = MEDIA_AVC_LEVEL4;
            break;
        case LEVEL_AVC_4_1:
            retLevel = MEDIA_AVC_LEVEL4_1;
            break;
        case LEVEL_AVC_4_2:
            retLevel = MEDIA_AVC_LEVEL4_2;
            break;
        case LEVEL_AVC_5:
            retLevel = MEDIA_AVC_LEVEL5;
            break;
        case LEVEL_AVC_5_1:
            retLevel = MEDIA_AVC_LEVEL5_1;
            break;
        default:
            retLevel = MEDIA_AVC_LEVEL3;
            break;
    }
    return retLevel;
}

int AmlVencInstVcCodec::HEVCLevelConvert(C2Config::level_t Level) {
//only for 265
    ALOGE("HEVCLevelConvert Level:%x",Level);
    int retLevel = 0;
    switch (Level) {
        case LEVEL_HEVC_MAIN_1:
            retLevel = MEDIA_HEVC_LEVEL1;
            break;
        case LEVEL_HEVC_MAIN_2:
            retLevel = MEDIA_HEVC_LEVEL2;
            break;
        case LEVEL_HEVC_MAIN_2_1:
            retLevel = MEDIA_HEVC_LEVEL2_1;
            break;
        case LEVEL_HEVC_MAIN_3:
            retLevel = MEDIA_HEVC_LEVEL3;
            break;
        case LEVEL_HEVC_MAIN_3_1:
            retLevel = MEDIA_HEVC_LEVEL3_1;
            break;
        case LEVEL_HEVC_MAIN_4:
            retLevel = MEDIA_HEVC_LEVEL4;
            break;
        case LEVEL_HEVC_MAIN_4_1:
            retLevel = MEDIA_HEVC_LEVEL4_1;
            break;
        case LEVEL_HEVC_MAIN_5:
            retLevel = MEDIA_HEVC_LEVEL5;
            break;
        case LEVEL_HEVC_MAIN_5_1:
            retLevel = MEDIA_HEVC_LEVEL5_1;
            break;
        case LEVEL_HEVC_MAIN_5_2:
            retLevel = MEDIA_HEVC_LEVEL5_2;
            break;
        case LEVEL_HEVC_MAIN_6:
            retLevel = MEDIA_HEVC_LEVEL6;
            break;
        case LEVEL_HEVC_MAIN_6_1:
            retLevel = MEDIA_HEVC_LEVEL6_1;
            break;
        case LEVEL_HEVC_MAIN_6_2:
            retLevel = MEDIA_HEVC_LEVEL6_2;
            break;
        default:
            retLevel = MEDIA_HEVC_LEVEL3;
            break;
    }
    return retLevel;
}


bool AmlVencInstVcCodec::init() {
    stVencParam VencParam;
    ALOGD("vccodec init action!");

    memset(&VencParam,0,sizeof(VencParam));
    mVencParamInst->GetVencParam(VencParam);

    mActualFormat = PixelFormatConvert(VencParam.PixFormat);
    if (!mIsInit) {
        InitEncoder();
        mIsInit = true;
    }
    ALOGD("init finish");
    return true;
}



void AmlVencInstVcCodec::GenQpTable(stVencParam VencParam,amvenc_qp_param_t &QpParam) {
    QpParam.qp_min = VencParam.QpInfo.QpMin;
    QpParam.qp_max = VencParam.QpInfo.QpMax;
    QpParam.qp_I_min = VencParam.QpInfo.iMin;
    QpParam.qp_I_max = VencParam.QpInfo.iMax;
    QpParam.qp_P_min = VencParam.QpInfo.pMin;
    QpParam.qp_P_max = VencParam.QpInfo.pMax;
    QpParam.qp_B_min = VencParam.QpInfo.bMin;
    QpParam.qp_B_max = VencParam.QpInfo.bMax;
    QpParam.qp_I_base = 30;
    QpParam.qp_P_base = 30;
    QpParam.qp_B_base = 30;
}


void AmlVencInstVcCodec::ExtraInit(amvenc_info_t &VencInfo,amvenc_qp_param_t &QpParam) {
    stVencParam VencParam;
    int IDRPeriod = 0;
    memset(&VencParam,0,sizeof(VencParam));
    mVencParamInst->GetVencParam(VencParam);

    VencInfo.qp_mode = VencParam.QpInfo.enable;
    GenQpTable(VencParam,QpParam);

    VencInfo.width = VencParam.Width;
    VencInfo.height = VencParam.Height;

    IDRPeriod = (VencParam.SyncFramePeriod / 1000000) * VencParam.FrameRate;
    if (0 == IDRPeriod) {
        VencInfo.gop = IDRPeriod + 1;
    }
    else {
        VencInfo.gop = IDRPeriod;
    }
    ALOGD("mSyncFramePeriod:%" PRId64",gop:%d",VencParam.SyncFramePeriod,VencInfo.gop);

    if (VencInfo.img_format != mActualFormat) {
        VencInfo.img_format = (amvenc_img_format_t)mActualFormat;
        ALOGE("detect color format change to %d",mActualFormat);
    }

    if (H264 == mVencParamInst->GetCodecType()) {
        VencInfo.profile = AVCProfileConvert(VencParam.ProfileLevel.profile);
        VencInfo.level = AVCLevelConvert(VencParam.ProfileLevel.level);
    }
    else if (H265 == mVencParamInst->GetCodecType()) {
        VencInfo.level = HEVCLevelConvert(VencParam.ProfileLevel.level);
    }
    ALOGE("ExtraInit profile:%d,level:%d",VencInfo.profile,VencInfo.level);
    VencInfo.bitstream_buf_sz_kb = mVencParamInst->GetMaxOutputBufferSize() / 1024;

}


void AmlVencInstVcCodec::ExtraPreProcess(stInputFrameInfo &InputFrameInfo) {
}


void AmlVencInstVcCodec::ExtraProcess() {
}

}


