#ifndef __AML_VENC_INST_H_
#define __AML_VENC_INST_H_

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "AmlVencInstIntf.h"
#include "amvenc.h"
#include "AmlVencInstHelper.h"
#include <am_gralloc_ext.h>


namespace android {


typedef struct DataModeInfo {
    unsigned long type;
    unsigned long pAddr;
    unsigned long canvas;
}DataModeInfo_t;


class AmlVencInst : public IAmlVencInst {

public:
    AmlVencInst();
    virtual ~AmlVencInst();

public:
    virtual bool init();
    virtual bool GenerateHeader(char *pHeader,unsigned int &Length);
    virtual eResult PreProcess(std::shared_ptr<C2Buffer> inputBuffer,stInputFrameInfo &InputFrameInfo);
    virtual c2_status_t ProcessOneFrame(stInputFrameInfo InputFrameInfo,stOutputFrame &OutFrame);
    virtual bool Destroy();
    virtual void SetVencParamInst(IAmlVencParam *pVencParamInst);

public:
    virtual void ExtraInit(amvenc_info_t &VencInfo,amvenc_qp_param_t &QpParam);
    virtual void ExtraProcess();
    virtual void ExtraPreProcess(stInputFrameInfo &InputFrameInfo);
private:
    amvenc_img_format_t PixelFormatConvert(int FormatIn);
    bool IsYUV420(const C2GraphicView &view);
    bool IsNV12(const C2GraphicView &view);
    bool IsNV21(const C2GraphicView &view);
    bool IsI420(const C2GraphicView &view);
    c2_status_t GraphicDataProc(std::shared_ptr<C2Buffer> inputBuffer,stInputFrameInfo *pFrameInfo);
    c2_status_t LinearDataProc(std::shared_ptr<const C2ReadView> view,stInputFrameInfo *pFrameInfo);
    c2_status_t DMAProc(const native_handle_t *priv_handle,stInputFrameInfo *pFrameInfo,uint32_t *dumpFileSize);
    c2_status_t ViewDataProc(std::shared_ptr<const C2GraphicView> view,stInputFrameInfo *pFrameInfo,uint32_t *dumpFileSize);
    c2_status_t CanvasDataProc(DataModeInfo_t *pDataMode,stInputFrameInfo *pFrameInfo);
    c2_status_t CheckPicSize(std::shared_ptr<const C2GraphicView> view,uint64_t frameIndex);
    c2_status_t genVuiParam(stVencParam VencParam,int32_t *primaries,int32_t *transfer,int32_t *matrixCoeffs,bool *range);
protected:
    bool InitEncoder();

protected:
    IAmlVencParam *mVencParamInst;
    AmlVencInstHelper *mHelper;
    amvenc_handle_t mVencHandle;
    bool mRequestSync;
    //int mFrameRate;
    int mBitRate;
};



}







#endif

