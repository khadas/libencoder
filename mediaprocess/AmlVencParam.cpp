#include "AmlVencParamIntf.h"
#include "AmlVencParam.h"


namespace android {

constexpr static size_t kLinearBufferSize = 2 * 1024 * 1024;

#define SUPPORT_MIN_WIDTH      176
#define SUPPORT_MAX_WIDTH      1920
#define SUPPORT_MIN_HEIGHT     144
#define SUPPORT_MAX_HEIGHT     1080
#define SUPPORT_WIDTH_ALIGN    2
#define SUPPORT_HEIGHT_ALIGN   2
#define IS_SUPPORT_DMA         0
#define IS_SUPPORT_CANVAS      0
#define DEFAULT_MAX_INSTANCE   3


AmlVencParam::AmlVencParam()
             :mNotifier(NULL),
              mParam(NULL),
              mCodecType(H264){
    memset(&mVencParam,0,sizeof(mVencParam));
}

AmlVencParam::~AmlVencParam() {
}

bool AmlVencParam::RegisterChangeNotify(ParamChangeNotify Notifier,void *Param) {
    if (NULL == Notifier) {
        return false;
    }
    mNotifier = Notifier;
    mParam = Param;
    return true;
}

int AmlVencParam::GetMaxSupportInstance() {
    return DEFAULT_MAX_INSTANCE;
}



bool AmlVencParam::GetBitrateLimit(int &pMin,int &pMax) {
    pMin = 1;
    pMax = 100000000;
    return true;
}

bool AmlVencParam::GetSizeLimit(stPicSize &pMin,stPicSize &pMax,stPicAlign &align) {
    pMin.width = SUPPORT_MIN_WIDTH;
    pMin.height = SUPPORT_MIN_HEIGHT;
    pMax.width = SUPPORT_MAX_WIDTH;
    pMax.height = SUPPORT_MAX_HEIGHT;
    align.width = SUPPORT_WIDTH_ALIGN;
    align.height = SUPPORT_HEIGHT_ALIGN;
    return true;
}


int AmlVencParam::GetMaxOutputBufferSize() {
    return kLinearBufferSize;
}


bool AmlVencParam::GetIsSupportDMAMode() {
    return IS_SUPPORT_DMA;
}

bool AmlVencParam::GetIsSupportCanvasMode() {
    return IS_SUPPORT_CANVAS;
}

bool AmlVencParam::GetGopLimit(int &pMin,int &pMax) {
    pMin = 0;
    pMax = 0xffffffff;
    return true;
}

bool AmlVencParam::GetQpLimit(int &Min,int &Max) {
    Min = 0;
    Max = 51;
    return true;
}

bool AmlVencParam::GetDefaultProfileLevel(C2Config::profile_t &profile,C2Config::level_t       &level) {
    if (H264 == mCodecType) {
        profile = PROFILE_AVC_MAIN;
        level = LEVEL_AVC_4_1;
    }
    else if(H265 == mCodecType) {
        profile = PROFILE_HEVC_MAIN;
        level = LEVEL_HEVC_HIGH_5_1;
    }
    else {
        return false;
    }
    return true;
}



//default not support svc function
bool AmlVencParam::GetSVCLimit(stSVCInfo &SvcInfo) {
    memset(&SvcInfo,0,sizeof(SvcInfo));
    return true;
}


bool AmlVencParam::UpdateAvgQp(int AvgQp) {
    return mNotifier(mParam,E_AVRAGE_QP,&AvgQp);
}

bool AmlVencParam::UpdateFrameType(ePicType FrameType) {
    return mNotifier(mParam,E_FRAME_TYPE,&FrameType);
}

bool AmlVencParam::UpdateSyncFrameReq(int Request) {
    mVencParam.SyncFrameRequest = Request;
    return mNotifier(mParam,E_SYNCFRAME_REQUEST,&Request);
}


bool AmlVencParam::UpdatePicSize(int Width,int Height) {
    stPicSize PicSize;
    mVencParam.Width = Width;
    mVencParam.Height = Height;
    PicSize.width = Width;
    PicSize.height = Height;
    return mNotifier(mParam,E_PIC_SIZE,&PicSize);
}



}



