#ifndef __AML_VENC_PARAM_H_
#define __AML_VENC_PARAM_H_

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "AmlVencParamIntf.h"


namespace android {


class AmlVencParam : public IAmlVencParam {

public:
    AmlVencParam();
    virtual ~AmlVencParam();

public:
    virtual bool RegisterChangeNotify(ParamChangeNotify Notifier,void *Param);
    virtual int GetMaxSupportInstance();
    virtual bool GetBitrateLimit(int &pMin,int &pMax);
    virtual bool GetSizeLimit(stPicSize &pMin,stPicSize &pMax,stPicAlign &align);
    virtual bool GetIsSupportDMAMode();
    virtual bool GetIsSupportCanvasMode();
    virtual bool GetSVCLimit(stSVCInfo &SvcInfo);
    virtual bool GetGopLimit(int &pMin,int &pMax);
    virtual bool GetQpLimit(int &Min,int &Max);
    virtual int GetMaxOutputBufferSize();
    virtual bool GetDefaultProfileLevel(C2Config::profile_t &profile,C2Config::level_t       &level);
    virtual void GetVencParam(stVencParam &Param) { memcpy(&Param,&mVencParam,sizeof(mVencParam)); };
    virtual void SetSize(int width,int height){ mVencParam.Width = width; mVencParam.Height = height; };
    virtual void SetBitrate(int bitrate){ mVencParam.Bitrate = bitrate; };
    virtual void SetSyncPeriodSize(int Period) { mVencParam.SyncFramePeriod = Period; };
    virtual void SetFrameRate(int FrameRate) { mVencParam.FrameRate = FrameRate; };
    virtual void SetQpInfo(stQpInfo QpInfo) { memcpy(&mVencParam.QpInfo,&QpInfo,sizeof(QpInfo));};
    virtual void SetLayerCnt(int LayerCount) { mVencParam.LayerCnt = LayerCount;};
    virtual void SetPixFormat(int Format) { mVencParam.PixFormat = Format;};
    virtual void SetSyncFrameRequest(bool Request) { mVencParam.SyncFrameRequest = Request;};
    virtual void SetPrependFlag(bool Flag) { mVencParam.prepend_header_flag = Flag;};
    virtual void SetCanvasMode(bool Canvas) { mVencParam.canvas_mode = Canvas;};
    virtual void SetColorAspect(stColorAspect ColorAspect) { memcpy(&mVencParam.ColorAspect,&ColorAspect,sizeof(ColorAspect)); };
    virtual void SetProfileLevel(stProfileLevelInfo Info) { memcpy(&mVencParam.ProfileLevel,&Info,sizeof(Info));};
    virtual void SetCodecType(eCodecType CodecType){mCodecType = CodecType;};
    virtual eCodecType GetCodecType(){return mCodecType;};
    virtual bool UpdateAvgQp(int AvgQp);
    virtual bool UpdateFrameType(ePicType FrameType);
    virtual bool UpdateSyncFrameReq(int Request);
    virtual bool UpdatePicSize(int Width,int Height);

protected:
    ParamChangeNotify mNotifier;
    void *mParam;
    eCodecType mCodecType;
    stVencParam mVencParam;
};


}



#endif
