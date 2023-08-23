//#define LOG_NDEBUG 0
#define LOG_TAG "AMLVencWave5"
#include <utils/Log.h>

#include <cutils/properties.h>
#include "AmlVencInstWave5.h"


namespace android {

#define ENC_ENABLE_ROI_FEATURE      0x1
#define ENC_ENABLE_PARA_UPDATE      0x2 //enable dynamic settings
#define ENC_ENABLE_LONG_TERM_REF    0x80


#define ENC_PROFILE_AUTO        0
#define ENC_PROFILE_BASELINE    1
#define ENC_PROFILE_MAIN        2
#define ENC_PROFILE_HIGH        3



#define ENCODER_PROP_SVC_ENABLE        "vendor.media.c2.encoder.svc"


AmlVencInstWave5::AmlVencInstWave5()
                 :mIsInit(false),
                  mActualFormat(AML_IMG_FMT_NONE) {

}


AmlVencInstWave5::~AmlVencInstWave5() {

}


bool AmlVencInstWave5::GetIsSupportSVC() {
    stSVCInfo SVC;
    bool svc_enable = false;
    memset(&SVC,0,sizeof(SVC));

    mVencParamInst->GetSVCLimit(SVC);

    svc_enable = property_get_bool(ENCODER_PROP_SVC_ENABLE, false);
    return (svc_enable && SVC.enable);

}


int AmlVencInstWave5::AVCProfileConvert(C2Config::profile_t profile) {
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
            retProfile = ENC_PROFILE_AUTO;
            break;
    }
    return retProfile;
}


bool AmlVencInstWave5::init() {
    ALOGD("wave521 fake init action!");
    return true;
}



void AmlVencInstWave5::GenQpTable(stVencParam VencParam,amvenc_qp_param_t &QpParam) {
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


void AmlVencInstWave5::ExtraInit(amvenc_info_t &VencInfo,amvenc_qp_param_t &QpParam) {
    stVencParam VencParam;
    int IDRPeriod = 0;
    ALOGE("ExtraInit!!");    memset(&VencParam,0,sizeof(VencParam));
    mVencParamInst->GetVencParam(VencParam);

    VencInfo.qp_mode = VencParam.QpInfo.enable;
    GenQpTable(VencParam,QpParam);

    VencInfo.width = VencParam.Width;
    if (VencParam.Width < 256) {
        VencInfo.width = 256; //cause wave521 not support for width < 256
        ALOGD("actual width:%d is not support,setting width to 256",VencParam.Width);
        mVencParamInst->UpdatePicSize(VencInfo.width,VencInfo.height);
        //add crop info????????
    }

    IDRPeriod = (VencParam.SyncFramePeriod / 1000000) * VencParam.FrameRate;
    if (0 == IDRPeriod) { //GOP limit????
        VencInfo.gop = IDRPeriod + 1;
    }
    else {
        VencInfo.gop = IDRPeriod;
    }
    ALOGD("IDRPeriod:%d",VencParam.SyncFramePeriod);

    if (VencInfo.img_format != mActualFormat) {
        VencInfo.img_format = (amvenc_img_format_t)mActualFormat;
        ALOGE("detect color format change to %d",mActualFormat);
    }

    if (H264 == mVencParamInst->GetCodecType())
        VencInfo.profile = AVCProfileConvert(VencParam.ProfileLevel.profile);

    VencInfo.bitstream_buf_sz_kb = mVencParamInst->GetMaxOutputBufferSize() / 1024;

    VencInfo.enc_feature_opts |= ENC_ENABLE_PARA_UPDATE; //enable dynamic settings

    if (GetIsSupportSVC()) {
        ALOGD("layercount:%d",VencParam.LayerCnt);

        uint a = 0;
        if (VencParam.LayerCnt == 2) {
            ALOGD("config ts layer with nLayerCount = 2");
            a = 4;
            VencInfo.enc_feature_opts |= (a << 2) & 0x7c;
        } else if (VencParam.LayerCnt == 3) {
            ALOGD("config ts layer with nLayerCount = 3");
            a = 10;
            VencInfo.enc_feature_opts |= (a << 2) & 0x7c;
        }
    }

}


void AmlVencInstWave5::ExtraPreProcess(stInputFrameInfo &InputFrameInfo) {
    ALOGE("ExtraPreProcess call init for parent");
    if (!mIsInit) { //for wave521 cannot support colorformat change after init
        mActualFormat = InputFrameInfo.colorFmt;
        InitEncoder();
        mIsInit = true;
    }
}


void AmlVencInstWave5::ExtraProcess() {
    ALOGE("ExtraProcess call init for parent");
}

}

