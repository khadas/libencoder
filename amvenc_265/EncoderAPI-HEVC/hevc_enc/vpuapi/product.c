//--=========================================================================--
//  This file is a part of VPU Reference API project
//-----------------------------------------------------------------------------
//
//  This confidential and proprietary software may be used only
//  as authorized by a licensing agreement from Chips&Media Inc.
//  In the event of publication, the following notice is applicable:
//
//            (C) COPYRIGHT 2006 - 2013  CHIPS&MEDIA INC.
//                      ALL RIGHTS RESERVED
//
//   The entire notice above must be reproduced on all authorized copies.
//
//--=========================================================================--
#include "product.h"
#include "common.h"
#include "wave4.h"

VpuAttr g_VpuCoreAttributes[MAX_NUM_VPU_CORE];

static Int32 s_ProductIds[MAX_NUM_VPU_CORE] = {
    PRODUCT_ID_NONE,
};

typedef struct FrameBufInfoStruct {
    Uint32 unitSizeHorLuma;
    Uint32 sizeLuma;
    Uint32 sizeChroma;
    BOOL   fieldMap;
} FrameBufInfo;


Uint32 ProductVpuScan(Uint32 coreIdx)
{
    Uint32  i, productId;
    Uint32 foundProducts = 0;

    /* Already scanned */
    if (s_ProductIds[coreIdx] != PRODUCT_ID_NONE)
        return 1;

    for (i=0; i<MAX_NUM_VPU_CORE; i++) {
        productId = WaveVpuGetProductId(i);
        if (productId != PRODUCT_ID_NONE) {
            s_ProductIds[i] = productId;
            foundProducts++;
        }
    }

    return (foundProducts == MAX_NUM_VPU_CORE);
}


Int32 ProductVpuGetId(Uint32 coreIdx)
{
    return s_ProductIds[coreIdx];
}

RetCode ProductVpuGetVersion(
    Uint32  coreIdx,
    Uint32* versionInfo,
    Uint32* revision
    )
{
    Int32   productId = s_ProductIds[coreIdx];
    RetCode ret = RETCODE_SUCCESS;

    switch (productId) {
    case PRODUCT_ID_410:
    case PRODUCT_ID_4102:
    case PRODUCT_ID_420:
    case PRODUCT_ID_412:
    case PRODUCT_ID_7Q:
    case PRODUCT_ID_420L:
        ret = Wave4VpuGetVersion(coreIdx, versionInfo, revision);
        break;
    default:
        ret = RETCODE_NOT_FOUND_VPU_DEVICE;
    }

    return ret;
}
#ifdef DRAM_TEST
RetCode ProductVpuDRAMReadWriteTest(
    Uint32  coreIdx,
    Uint32* dram_source_addr,
    Uint32* dram_destination_addr,
    Uint32* dram_data_size
    )
{
    Int32   productId = s_ProductIds[coreIdx];
    RetCode ret = RETCODE_SUCCESS;

    switch (productId) {
    case PRODUCT_ID_410:
    case PRODUCT_ID_4102:
    case PRODUCT_ID_510:
        ret = Wave4VpuDRAMReadWriteTest(coreIdx, dram_source_addr, dram_destination_addr, dram_data_size);
        break;
    default:
        ret = RETCODE_NOT_FOUND_VPU_DEVICE;
    }

    return ret;
}
#endif

RetCode ProductVpuInit(Uint32 coreIdx, void* firmware, Uint32 size)
{
    RetCode ret = RETCODE_SUCCESS;
    int     productId;

    productId  = s_ProductIds[coreIdx];

    switch (productId) {
    case PRODUCT_ID_410:
    case PRODUCT_ID_4102:
    case PRODUCT_ID_420:
    case PRODUCT_ID_412:
    case PRODUCT_ID_7Q:
    case PRODUCT_ID_420L:
        ret = Wave4VpuInit(coreIdx, firmware, size);
        break;
    default:
        ret = RETCODE_NOT_FOUND_VPU_DEVICE;
    }

    return ret;
}


RetCode ProductVpuReInit(Uint32 coreIdx, void* firmware, Uint32 size)
{
    RetCode ret = RETCODE_SUCCESS;
    int     productId;

    productId  = s_ProductIds[coreIdx];

    switch (productId) {
    case PRODUCT_ID_410:
    case PRODUCT_ID_4102:
    case PRODUCT_ID_420:
    case PRODUCT_ID_412:
    case PRODUCT_ID_7Q:
    case PRODUCT_ID_420L:
        ret = Wave4VpuReInit(coreIdx, firmware, size);
        break;
    default:
        ret = RETCODE_NOT_FOUND_VPU_DEVICE;
    }

    return ret;
}

Uint32 ProductVpuIsInit(Uint32 coreIdx)
{
    Uint32  pc = 0;
    int     productId;

    productId  = s_ProductIds[coreIdx];

    if (productId == PRODUCT_ID_NONE) {
        ProductVpuScan(coreIdx);
        productId  = s_ProductIds[coreIdx];
    }

    switch (productId) {
    case PRODUCT_ID_410:
    case PRODUCT_ID_4102:
    case PRODUCT_ID_420:
    case PRODUCT_ID_412:
    case PRODUCT_ID_7Q:
    case PRODUCT_ID_420L:
        pc = Wave4VpuIsInit(coreIdx);
        break;
    }

    return pc;
}

Int32 ProductVpuIsBusy(Uint32 coreIdx)
{
    Int32  busy;
    int    productId;

    productId = s_ProductIds[coreIdx];

    switch (productId) {
    case PRODUCT_ID_410:
    case PRODUCT_ID_4102:
    case PRODUCT_ID_420:
    case PRODUCT_ID_412:
    case PRODUCT_ID_7Q:
    case PRODUCT_ID_420L:
        busy = Wave4VpuIsBusy(coreIdx);
        break;
    default:
        busy = 0;
        break;
    }

    return busy;
}

Int32 ProductVpuWaitInterrupt(CodecInst *instance, Int32 timeout)
{
    int     productId;
    int     flag = -1;

    productId = s_ProductIds[instance->coreIdx];

    switch (productId) {
    case PRODUCT_ID_410:
    case PRODUCT_ID_4102:
    case PRODUCT_ID_420:
    case PRODUCT_ID_412:
    case PRODUCT_ID_7Q:
    case PRODUCT_ID_420L:
        flag = Wave4VpuWaitInterrupt(instance, timeout);
        break;
    default:
        flag = -1;
        break;
    }

    return flag;
}

RetCode ProductVpuReset(Uint32 coreIdx, SWResetMode resetMode)
{
    int     productId;
    RetCode ret = RETCODE_SUCCESS;

    productId = s_ProductIds[coreIdx];

    switch (productId) {
    case PRODUCT_ID_410:
    case PRODUCT_ID_4102:
    case PRODUCT_ID_420:
    case PRODUCT_ID_412:
    case PRODUCT_ID_7Q:
    case PRODUCT_ID_420L:
        ret = Wave4VpuReset(coreIdx, resetMode);
        break;
    default:
        ret = RETCODE_NOT_FOUND_VPU_DEVICE;
        break;
    }

    return ret;
}

RetCode ProductVpuSleepWake(Uint32 coreIdx, int iSleepWake, const Uint16* code, Uint32 size)
{
    int     productId;
    RetCode ret = RETCODE_NOT_FOUND_VPU_DEVICE;

    productId = s_ProductIds[coreIdx];

    switch (productId) {
    case PRODUCT_ID_410:
    case PRODUCT_ID_4102:
    case PRODUCT_ID_420:
    case PRODUCT_ID_412:
    case PRODUCT_ID_7Q:
    case PRODUCT_ID_420L:
        ret = Wave4VpuSleepWake(coreIdx, iSleepWake, (void*)code, size);
        break;
    }

    return ret;
}
RetCode ProductVpuClearInterrupt(Uint32 coreIdx, Uint32 flags)
{
    int     productId;
    RetCode ret = RETCODE_NOT_FOUND_VPU_DEVICE;

    productId = s_ProductIds[coreIdx];

    switch (productId) {
    case PRODUCT_ID_410:
    case PRODUCT_ID_4102:
    case PRODUCT_ID_420:
    case PRODUCT_ID_412:
    case PRODUCT_ID_7Q:
    case PRODUCT_ID_420L:
        ret = Wave4VpuClearInterrupt(coreIdx, 0xffff);
        break;
    }

    return ret;
}


RetCode ProductVpuEncBuildUpOpenParam(CodecInst* pCodec, EncOpenParam* param)
{
    Int32   productId;
    Uint32  coreIdx;
    RetCode ret = RETCODE_NOT_SUPPORTED_FEATURE;

    coreIdx   = pCodec->coreIdx;
    productId = s_ProductIds[coreIdx];

    switch (productId) {
    case PRODUCT_ID_420L:
        ret = Wave4VpuBuildUpEncParam(pCodec, param);
        break;
    default:
        ret = RETCODE_NOT_SUPPORTED_FEATURE;
    }

    return ret;
}

/************************************************************************/
/* Encoder                                                    */
/************************************************************************/

/**
 * \param   stride          stride of framebuffer in pixel.
 */
RetCode ProductVpuAllocateFramebuffer(
    CodecInst* inst, FrameBuffer* fbArr, TiledMapType mapType, Int32 num,
    Int32 stride, Int32 height, FrameBufferFormat format,
    BOOL cbcrInterleave, BOOL nv21, Int32 endian,
    vpu_buffer_t* vb, Int32 gdiIndex,
    FramebufferAllocType fbType)
{
    Int32           i;
    Uint32          coreIdx;
    vpu_buffer_t    vbFrame;
    FrameBufInfo    fbInfo;
    DecInfo*        pDecInfo = &inst->CodecInfo->decInfo;
    EncInfo*        pEncInfo = &inst->CodecInfo->encInfo;
    // Variables for TILED_FRAME/FILED_MB_RASTER
    Uint32          sizeLuma;
    Uint32          sizeChroma;
    ProductId       productId     = (ProductId)inst->productId;
    RetCode         ret           = RETCODE_SUCCESS;

    osal_memset((void*)&vbFrame, 0x00, sizeof(vpu_buffer_t));
    osal_memset((void*)&fbInfo,  0x00, sizeof(FrameBufInfo));

    coreIdx = inst->coreIdx;

    DRAMConfig* dramConfig = NULL;
    if (productId == PRODUCT_ID_960) {
        dramConfig = &pDecInfo->dramCfg;
        dramConfig = (inst->isDecoder == TRUE) ? &pDecInfo->dramCfg : &pEncInfo->dramCfg;
    }
    sizeLuma   = CalcLumaSize(inst->productId, stride, height, format, cbcrInterleave, mapType, dramConfig);
    sizeChroma = CalcChromaSize(inst->productId, stride, height, format, cbcrInterleave, mapType, dramConfig);

    // Framebuffer common informations
    for (i=0; i<num; i++) {
        if (fbArr[i].updateFbInfo == TRUE ) {
            fbArr[i].updateFbInfo = FALSE;
            fbArr[i].myIndex        = i+gdiIndex;
            fbArr[i].stride         = stride;
            fbArr[i].height         = height;
            fbArr[i].mapType        = mapType;
            fbArr[i].format         = format;
            fbArr[i].cbcrInterleave = (mapType == COMPRESSED_FRAME_MAP ? TRUE : cbcrInterleave);
            fbArr[i].nv21           = nv21;
            fbArr[i].endian         = endian;
            fbArr[i].lumaBitDepth   = pDecInfo->initialInfo.lumaBitdepth;
            fbArr[i].chromaBitDepth = pDecInfo->initialInfo.chromaBitdepth;
            fbArr[i].sourceLBurstEn = FALSE;
            if (inst->codecMode == HEVC_ENC) {
                if (gdiIndex != 0) {        // FB_TYPE_PPU
                    fbArr[i].srcBufState = SRC_BUFFER_ALLOCATED;
                }
                fbArr[i].lumaBitDepth   = pEncInfo->openParam.EncStdParam.hevcParam.internalBitDepth;
                fbArr[i].chromaBitDepth = pEncInfo->openParam.EncStdParam.hevcParam.internalBitDepth;
            }
        }
    }

    switch (mapType) {
    case LINEAR_FRAME_MAP:
    case LINEAR_FIELD_MAP:
    case COMPRESSED_FRAME_MAP:
    case TILED_SUB_CTU_MAP:
        ret = AllocateLinearFrameBuffer(mapType, fbArr, num, sizeLuma, sizeChroma);
        break;

    default:
        /* Tiled map */
        VLOG(ERR, "shall not reach Tile map vb %p fbType %d \n",vb, fbType);
        break;
    }
    for (i=0; i<num; i++) {
        if (inst->codecMode == HEVC_ENC) {
            if (gdiIndex != 0) {        // FB_TYPE_PPU
                APIDPRINT("SOURCE FB[%02d] Y(0x%08x), Cb(0x%08x), Cr(0x%08x)\n", i, fbArr[i].bufY, fbArr[i].bufCb, fbArr[i].bufCr);
            }
        }
    }
    return ret;
}

RetCode ProductVpuRegisterFramebuffer(CodecInst* instance)
{
    RetCode         ret = RETCODE_FAILURE;
    FrameBuffer*    fb;
    DecInfo*        pDecInfo = &instance->CodecInfo->decInfo;
    Int32           gdiIndex = 0;
    EncInfo*        pEncInfo = &instance->CodecInfo->encInfo;
    switch (instance->productId) {
    case PRODUCT_ID_410:
    case PRODUCT_ID_4102:
    case PRODUCT_ID_420:
    case PRODUCT_ID_412:
    case PRODUCT_ID_420L:
        if (instance->codecMode == HEVC_ENC) {
        // ********************************************
        //    HEVC_ENC
        //*********************************************
        if (pEncInfo->mapType != COMPRESSED_FRAME_MAP)
            return RETCODE_NOT_SUPPORTED_FEATURE;

        fb = pEncInfo->frameBufPool;

        gdiIndex = 0;
        ret = Wave4VpuEncRegisterFramebuffer(instance, &fb[gdiIndex], COMPRESSED_FRAME_MAP, pEncInfo->numFrameBuffers);

        if (ret != RETCODE_SUCCESS)
            return ret;

        break;
        }
    }
    return ret;
}

Int32 ProductCalculateFrameBufSize(Int32 productId, Int32 stride, Int32 height, TiledMapType mapType, FrameBufferFormat format, BOOL interleave, DRAMConfig* pDramCfg)
{
    Int32 size_dpb_lum, size_dpb_chr, size_dpb_all;
    Int32 size_mvcolbuf=0;

    size_dpb_lum = CalcLumaSize(productId, stride, height, format, interleave, mapType, pDramCfg);
    size_dpb_chr = CalcChromaSize(productId, stride, height, format, interleave, mapType, pDramCfg);
    size_dpb_all = size_dpb_lum + size_dpb_chr*2;

    if (productId == PRODUCT_ID_320) {
        size_mvcolbuf =  VPU_ALIGN32(stride)*VPU_ALIGN32(height);
        size_mvcolbuf = (size_mvcolbuf*3)/2;
        size_mvcolbuf = (size_mvcolbuf+4)/5;
        size_mvcolbuf = VPU_ALIGN8(size_mvcolbuf);
        size_dpb_all += size_mvcolbuf;
    }

    return size_dpb_all;
}


Int32 ProductCalculateAuxBufferSize(AUX_BUF_TYPE type, CodStd codStd, Int32 width, Int32 height)
{
    Int32 size = 0;

    switch (type) {
    case AUX_BUF_TYPE_MVCOL:
        if (codStd == STD_AVC || codStd == STD_VC1 || codStd == STD_MPEG4 || codStd == STD_H263 || codStd == STD_RV || codStd == STD_AVS ) {
            size =  VPU_ALIGN32(width)*VPU_ALIGN32(height);
            size = (size*3)/2;
            size = (size+4)/5;
            size = ((size+7)/8)*8;
        }
        else if (codStd == STD_HEVC) {
            size = WAVE4_DEC_HEVC_MVCOL_BUF_SIZE(width, height);
        }
        else if (codStd == STD_VP9) {
            size = WAVE4_DEC_VP9_MVCOL_BUF_SIZE(width, height);
        }
        else {
            size = 0;
        }
        break;
    case AUX_BUF_TYPE_FBC_Y_OFFSET:
        size  = WAVE4_FBC_LUMA_TABLE_SIZE(width, height);
        break;
    case AUX_BUF_TYPE_FBC_C_OFFSET:
        size  = WAVE4_FBC_CHROMA_TABLE_SIZE(width, height);
        break;
    }

    return size;
}

/************************************************************************/
/* ENCODER                                                              */
/************************************************************************/
RetCode ProductCheckEncOpenParam(EncOpenParam* pop)
{
    Int32       coreIdx;
    Int32       picWidth;
    Int32       picHeight;
    Int32       productId;
    VpuAttr*    pAttr;

    if (pop == 0)
        return RETCODE_INVALID_PARAM;

    if (pop->coreIdx > MAX_NUM_VPU_CORE)
        return RETCODE_INVALID_PARAM;

    coreIdx   = pop->coreIdx;
    picWidth  = pop->picWidth;
    picHeight = pop->picHeight;
    productId = s_ProductIds[coreIdx];
    pAttr     = &g_VpuCoreAttributes[coreIdx];

    if ((pAttr->supportEncoders&(1<<pop->bitstreamFormat)) == 0)
        return RETCODE_NOT_SUPPORTED_FEATURE;

    if (pop->ringBufferEnable == TRUE) {
        if (pop->bitstreamBuffer % 8) {
            return RETCODE_INVALID_PARAM;
        }

        if (productId == PRODUCT_ID_420 || productId == PRODUCT_ID_420L || productId == PRODUCT_ID_520) {
            if (pop->bitstreamBuffer % 16)
                return RETCODE_INVALID_PARAM;
        }

        if (productId == PRODUCT_ID_420 || productId == PRODUCT_ID_420L || productId == PRODUCT_ID_520) {
            if (pop->bitstreamBufferSize < (1024*64)) {
                return RETCODE_INVALID_PARAM;
            }
        }

        if (pop->bitstreamBufferSize % 1024 || pop->bitstreamBufferSize < 1024)
            return RETCODE_INVALID_PARAM;
    }

    if (pop->frameRateInfo == 0)
        return RETCODE_INVALID_PARAM;

    if (pop->bitstreamFormat == STD_AVC) {
        if (productId == PRODUCT_ID_980 || productId == PRODUCT_ID_320) {
            if (pop->bitRate > 524288 || pop->bitRate < 0)
                return RETCODE_INVALID_PARAM;
        }
    }
    else if (pop->bitstreamFormat == STD_HEVC) {
        if (productId == PRODUCT_ID_420 || productId == PRODUCT_ID_420L || productId == PRODUCT_ID_520) {
            if (pop->bitRate > 700000000 || pop->bitRate < 0)
                return RETCODE_INVALID_PARAM;
        }
    }
    else {
        if (pop->bitRate > 32767 || pop->bitRate < 0)
            return RETCODE_INVALID_PARAM;
    }

    if (pop->bitRate !=0 && pop->initialDelay > 32767)
        return RETCODE_INVALID_PARAM;

    if (pop->bitRate !=0 && pop->initialDelay != 0 && pop->vbvBufferSize < 0)
        return RETCODE_INVALID_PARAM;

    if (pop->frameSkipDisable != 0 && pop->frameSkipDisable != 1)
        return RETCODE_INVALID_PARAM;

    if (pop->sliceMode.sliceMode != 0 && pop->sliceMode.sliceMode != 1)
        return RETCODE_INVALID_PARAM;

    if (pop->sliceMode.sliceMode == 1) {
        if (pop->sliceMode.sliceSizeMode != 0 && pop->sliceMode.sliceSizeMode != 1) {
            return RETCODE_INVALID_PARAM;
        }
        if (pop->sliceMode.sliceSizeMode == 1 && pop->sliceMode.sliceSize == 0 ) {
            return RETCODE_INVALID_PARAM;
        }
    }

    if (pop->intraRefresh < 0)
        return RETCODE_INVALID_PARAM;

    if (pop->MEUseZeroPmv != 0 && pop->MEUseZeroPmv != 1)
        return RETCODE_INVALID_PARAM;

    if (pop->intraCostWeight < 0 || pop->intraCostWeight >= 65535)
        return RETCODE_INVALID_PARAM;

    if (productId == PRODUCT_ID_980 || productId == PRODUCT_ID_320) {
        if (pop->MESearchRangeX < 0 || pop->MESearchRangeX > 4) {
            return RETCODE_INVALID_PARAM;
        }
        if (pop->MESearchRangeY < 0 || pop->MESearchRangeY > 3) {
            return RETCODE_INVALID_PARAM;
        }
    }
    else {
        if (pop->MESearchRange < 0 || pop->MESearchRange >= 4)
            return RETCODE_INVALID_PARAM;
    }

    if (pop->bitstreamFormat == STD_MPEG4) {
        EncMp4Param * param = &pop->EncStdParam.mp4Param;
        if (param->mp4DataPartitionEnable != 0 && param->mp4DataPartitionEnable != 1) {
            return RETCODE_INVALID_PARAM;
        }
        if (param->mp4DataPartitionEnable == 1) {
            if (param->mp4ReversibleVlcEnable != 0 && param->mp4ReversibleVlcEnable != 1) {
                return RETCODE_INVALID_PARAM;
            }
        }
        if (param->mp4IntraDcVlcThr < 0 || 7 < param->mp4IntraDcVlcThr) {
            return RETCODE_INVALID_PARAM;
        }

        if (picWidth < MIN_ENC_PIC_WIDTH || picWidth > MAX_ENC_PIC_WIDTH ) {
            return RETCODE_INVALID_PARAM;
        }

        if (picHeight < MIN_ENC_PIC_HEIGHT) {
            return RETCODE_INVALID_PARAM;
        }
    }
    else if (pop->bitstreamFormat == STD_H263) {
        EncH263Param * param = &pop->EncStdParam.h263Param;
        Uint32 frameRateInc, frameRateRes;

        if (param->h263AnnexJEnable != 0 && param->h263AnnexJEnable != 1) {
            return RETCODE_INVALID_PARAM;
        }
        if (param->h263AnnexKEnable != 0 && param->h263AnnexKEnable != 1) {
            return RETCODE_INVALID_PARAM;
        }
        if (param->h263AnnexTEnable != 0 && param->h263AnnexTEnable != 1) {
            return RETCODE_INVALID_PARAM;
        }

        if (picWidth < MIN_ENC_PIC_WIDTH || picWidth > MAX_ENC_PIC_WIDTH ) {
            return RETCODE_INVALID_PARAM;
        }
        if (picHeight < MIN_ENC_PIC_HEIGHT) {
            return RETCODE_INVALID_PARAM;
        }

        frameRateInc = ((pop->frameRateInfo>>16) &0xFFFF) + 1;
        frameRateRes = pop->frameRateInfo & 0xFFFF;

        if ((frameRateRes/frameRateInc) <15) {
            return RETCODE_INVALID_PARAM;
        }
    }
    else if (pop->bitstreamFormat == STD_AVC) {
        EncAvcParam* param     = &pop->EncStdParam.avcParam;


        if (param->constrainedIntraPredFlag != 0 && param->constrainedIntraPredFlag != 1)
            return RETCODE_INVALID_PARAM;
        if (param->disableDeblk != 0 && param->disableDeblk != 1 && param->disableDeblk != 2)
            return RETCODE_INVALID_PARAM;
        if (param->deblkFilterOffsetAlpha < -6 || 6 < param->deblkFilterOffsetAlpha)
            return RETCODE_INVALID_PARAM;
        if (param->deblkFilterOffsetBeta < -6 || 6 < param->deblkFilterOffsetBeta)
            return RETCODE_INVALID_PARAM;
        if (param->chromaQpOffset < -12 || 12 < param->chromaQpOffset)
            return RETCODE_INVALID_PARAM;
        if (param->audEnable != 0 && param->audEnable != 1)
            return RETCODE_INVALID_PARAM;
        if (param->frameCroppingFlag != 0 &&param->frameCroppingFlag != 1)
            return RETCODE_INVALID_PARAM;
        if (param->frameCropLeft & 0x01 || param->frameCropRight & 0x01 ||
            param->frameCropTop & 0x01  || param->frameCropBottom & 0x01) {
            return RETCODE_INVALID_PARAM;
        }

    if (picWidth < MIN_ENC_PIC_WIDTH || picWidth > MAX_ENC_PIC_WIDTH )
        return RETCODE_INVALID_PARAM;
    if (picHeight < MIN_ENC_PIC_HEIGHT)
        return RETCODE_INVALID_PARAM;
    }
    else if (pop->bitstreamFormat == STD_HEVC) {
        EncHevcParam* param     = &pop->EncStdParam.hevcParam;
        if (picWidth < W4_MIN_ENC_PIC_WIDTH || picWidth > W4_MAX_ENC_PIC_WIDTH)
            return RETCODE_INVALID_PARAM;

        if (picHeight < W4_MIN_ENC_PIC_HEIGHT || picHeight > W4_MAX_ENC_PIC_HEIGHT)
            return RETCODE_INVALID_PARAM;

        if (param->profile != HEVC_PROFILE_MAIN && param->profile != HEVC_PROFILE_MAIN10)
            return RETCODE_INVALID_PARAM;

        if (param->internalBitDepth != 8 && param->internalBitDepth != 10)
            return RETCODE_INVALID_PARAM;

        if (param->internalBitDepth > 8 && param->profile == HEVC_PROFILE_MAIN)
            return RETCODE_INVALID_PARAM;

        if (param->chromaFormatIdc < 0 || param->chromaFormatIdc > 3)
            return RETCODE_INVALID_PARAM;

        if (param->decodingRefreshType < 0 || param->decodingRefreshType > 2)
            return RETCODE_INVALID_PARAM;

        if (param->gopParam.useDeriveLambdaWeight != 1 && param->gopParam.useDeriveLambdaWeight != 0)
            return RETCODE_INVALID_PARAM;

        if (param->gopPresetIdx == PRESET_IDX_CUSTOM_GOP) {
            if ( param->gopParam.customGopSize < 1 || param->gopParam.customGopSize > MAX_GOP_NUM)
                return RETCODE_INVALID_PARAM;
        }

        if (param->constIntraPredFlag != 1 && param->constIntraPredFlag != 0)
            return RETCODE_INVALID_PARAM;

        if (param->intraRefreshMode < 0 || param->intraRefreshMode > 3)
            return RETCODE_INVALID_PARAM;

        if (param->independSliceMode < 0 || param->independSliceMode > 1)
            return RETCODE_INVALID_PARAM;

        if (param->independSliceMode != 0) {
            if (param->dependSliceMode < 0 || param->dependSliceMode > 2)
                return RETCODE_INVALID_PARAM;
        }

        if (param->useRecommendEncParam < 0 && param->useRecommendEncParam > 3)
            return RETCODE_INVALID_PARAM;

        if (param->useRecommendEncParam == 0 || param->useRecommendEncParam == 2 || param->useRecommendEncParam == 3) {

            if (param->intraInInterSliceEnable != 1 && param->intraInInterSliceEnable != 0)
                return RETCODE_INVALID_PARAM;

            if (param->intraNxNEnable != 1 && param->intraNxNEnable != 0)
                return RETCODE_INVALID_PARAM;


            if (param->skipIntraTrans != 1 && param->skipIntraTrans != 0)
                return RETCODE_INVALID_PARAM;

            if (param->scalingListEnable != 1 && param->scalingListEnable != 0)
                return RETCODE_INVALID_PARAM;

            if (param->tmvpEnable != 1 && param->tmvpEnable != 0)
                return RETCODE_INVALID_PARAM;

            if (param->wppEnable != 1 && param->wppEnable != 0)
                return RETCODE_INVALID_PARAM;

            if (param->useRecommendEncParam != 3) {     // in FAST mode (recommendEncParam == 3), maxNumMerge value will be decided in FW
                if (param->maxNumMerge < 0 || param->maxNumMerge > 3)
                    return RETCODE_INVALID_PARAM;
            }

            if (param->disableDeblk != 1 && param->disableDeblk != 0)
                return RETCODE_INVALID_PARAM;

            if (param->disableDeblk == 0 || param->saoEnable != 0) {
                if (param->lfCrossSliceBoundaryEnable != 1 && param->lfCrossSliceBoundaryEnable != 0)
                    return RETCODE_INVALID_PARAM;
            }

            if (param->disableDeblk == 0) {
                if (param->betaOffsetDiv2 < -6 || param->betaOffsetDiv2 > 6)
                    return RETCODE_INVALID_PARAM;

                if (param->tcOffsetDiv2 < -6 || param->tcOffsetDiv2 > 6)
                    return RETCODE_INVALID_PARAM;
            }
        }

        if (param->losslessEnable != 1 && param->losslessEnable != 0)
            return RETCODE_INVALID_PARAM;

        if (param->intraQP < 0 || param->intraQP > 51)
            return RETCODE_INVALID_PARAM;

        if (pop->rcEnable != 1 && pop->rcEnable != 0)
            return RETCODE_INVALID_PARAM;


        if (pop->rcEnable == 1) {
            if (param->minQp < 0 || param->minQp > 51)
                return RETCODE_INVALID_PARAM;

            if (param->maxQp < 0 || param->maxQp > 51)
                return RETCODE_INVALID_PARAM;

            if (productId == PRODUCT_ID_420L) {
                if (param->initialRcQp < 0 || param->initialRcQp > 63)
                    return RETCODE_INVALID_PARAM;

                if ( (param->initialRcQp < 52) && ((param->initialRcQp > param->maxQp) || (param->initialRcQp < param->minQp)) )
                    return RETCODE_INVALID_PARAM;
            }
            if (param->ctuOptParam.ctuQpEnable)     // can't enable both rcEnable and ctuQpEnable together.
                return RETCODE_INVALID_PARAM;
            if (param->intraQpOffset < -10 && param->intraQpOffset > 10)
                return RETCODE_INVALID_PARAM;

            if (param->cuLevelRCEnable != 1 && param->cuLevelRCEnable != 0)
                return RETCODE_INVALID_PARAM;

            if (param->cuLevelRCEnable == 1) {
                if (param->hvsQPEnable != 1 && param->hvsQPEnable != 0)
                    return RETCODE_INVALID_PARAM;

                if (param->hvsQPEnable) {
                    if (param->maxDeltaQp < 0 || param->maxDeltaQp > 51)
                        return RETCODE_INVALID_PARAM;

                    if (param->hvsQpScaleEnable != 1 && param->hvsQpScaleEnable != 0)
                        return RETCODE_INVALID_PARAM;

                    if (param->hvsQpScaleEnable == 1) {
                        if (param->hvsQpScale < 0 || param->hvsQpScale > 4)
                            return RETCODE_INVALID_PARAM;
                    }
                }
            }

            if (param->ctuOptParam.roiEnable) {
                if (param->ctuOptParam.roiDeltaQp < 1 || param->ctuOptParam.roiDeltaQp > 51)
                    return RETCODE_INVALID_PARAM;
            }

            if (param->ctuOptParam.roiEnable && param->hvsQPEnable)     // can not use both ROI and hvsQp
                return RETCODE_INVALID_PARAM;

            if (param->bitAllocMode < 0 && param->bitAllocMode > 2)
                return RETCODE_INVALID_PARAM;

            if (param->initBufLevelx8 < 0 || param->initBufLevelx8 > 8)
                return RETCODE_INVALID_PARAM;

            if (pop->initialDelay < 10 || pop->initialDelay > 3000 )
                return RETCODE_INVALID_PARAM;
        }

        // packed format & cbcrInterleave & nv12 can't be set at the same time.
        if (pop->packedFormat == 1 && pop->cbcrInterleave == 1)
            return RETCODE_INVALID_PARAM;

        if (pop->packedFormat == 1 && pop->nv21 == 1)
            return RETCODE_INVALID_PARAM;

        // check valid for common param
        if (CheckEncCommonParamValid(pop) == RETCODE_FAILURE)
            return RETCODE_INVALID_PARAM;

        // check valid for RC param
        if (CheckEncRcParamValid(pop) == RETCODE_FAILURE)
            return RETCODE_INVALID_PARAM;

        if (param->wppEnable && pop->ringBufferEnable)      // WPP can be processed on only linebuffer mode.
            return RETCODE_INVALID_PARAM;

        // check additional features for WAVE420L
        if (param->chromaCbQpOffset < -10 || param->chromaCbQpOffset > 10)
            return RETCODE_INVALID_PARAM;

        if (param->chromaCrQpOffset < -10 || param->chromaCrQpOffset > 10)
            return RETCODE_INVALID_PARAM;

        if (param->nrYEnable != 1 && param->nrYEnable != 0)
            return RETCODE_INVALID_PARAM;

        if (param->nrCbEnable != 1 && param->nrCbEnable != 0)
            return RETCODE_INVALID_PARAM;

        if (param->nrCrEnable != 1 && param->nrCrEnable != 0)
            return RETCODE_INVALID_PARAM;

        if (param->nrNoiseEstEnable != 1 && param->nrNoiseEstEnable != 0)
            return RETCODE_INVALID_PARAM;

        if (param->nrNoiseSigmaY < 0 || param->nrNoiseSigmaY > 255)
            return RETCODE_INVALID_PARAM;

        if (param->nrNoiseSigmaCb < 0 || param->nrNoiseSigmaCb > 255)
            return RETCODE_INVALID_PARAM;

        if (param->nrNoiseSigmaCr < 0 || param->nrNoiseSigmaCr > 255)
            return RETCODE_INVALID_PARAM;

        if (param->nrIntraWeightY < 0 || param->nrIntraWeightY > 31)
            return RETCODE_INVALID_PARAM;

        if (param->nrIntraWeightCb < 0 || param->nrIntraWeightCb > 31)
            return RETCODE_INVALID_PARAM;

        if (param->nrIntraWeightCr < 0 || param->nrIntraWeightCr > 31)
            return RETCODE_INVALID_PARAM;

        if (param->nrInterWeightY < 0 || param->nrInterWeightY > 31)
            return RETCODE_INVALID_PARAM;

        if (param->nrInterWeightCb < 0 || param->nrInterWeightCb > 31)
            return RETCODE_INVALID_PARAM;

        if (param->nrInterWeightCr < 0 || param->nrInterWeightCr > 31)
            return RETCODE_INVALID_PARAM;

        if ((param->nrYEnable == 1 || param->nrCbEnable == 1 || param->nrCrEnable == 1) && (param->losslessEnable == 1))
            return RETCODE_INVALID_PARAM;

        if (param->intraRefreshMode == 3 && param-> intraRefreshArg == 0)
            return RETCODE_INVALID_PARAM;

    }
    if (pop->linear2TiledEnable == TRUE) {
        if (pop->linear2TiledMode != FF_FRAME && pop->linear2TiledMode != FF_FIELD )
            return RETCODE_INVALID_PARAM;
    }

    return RETCODE_SUCCESS;
}

RetCode ProductVpuEncFiniSeq(CodecInst* instance)
{
    RetCode     ret = RETCODE_NOT_FOUND_VPU_DEVICE;

    switch (instance->productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
    case PRODUCT_ID_410:
    case PRODUCT_ID_4102:
    case PRODUCT_ID_510:
    case PRODUCT_ID_512:
    case PRODUCT_ID_515:
        ret = RETCODE_NOT_SUPPORTED_FEATURE;
        break;
    case PRODUCT_ID_420:
    case PRODUCT_ID_7Q:
    case PRODUCT_ID_420L:
        ret = Wave4VpuEncFiniSeq(instance);
        break;
    case PRODUCT_ID_520:
        //[fix me]
        break;
    }
    return ret;
}

RetCode ProductVpuEncSetup(CodecInst* instance)
{
    RetCode     ret = RETCODE_NOT_FOUND_VPU_DEVICE;

    switch (instance->productId) {
    case PRODUCT_ID_420L:
        ret = Wave4VpuEncSetup(instance);
        break;
    default:
        break;
    }

    return ret;
}

RetCode ProductVpuEncode(CodecInst* instance, EncParam* param)
{
    RetCode     ret = RETCODE_NOT_FOUND_VPU_DEVICE;

    switch (instance->productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
    case PRODUCT_ID_410:
    case PRODUCT_ID_4102:
    case PRODUCT_ID_510:
    case PRODUCT_ID_512:
    case PRODUCT_ID_515:
        ret = RETCODE_NOT_SUPPORTED_FEATURE;
        break;
    case PRODUCT_ID_420:
    case PRODUCT_ID_420L:
        ret = Wave4VpuEncode(instance, param);
        break;
    case PRODUCT_ID_7Q:
        ret = RETCODE_NOT_SUPPORTED_FEATURE;
        break;
    case PRODUCT_ID_520:
        // [fix me]
        break;
    }

    return ret;
}

RetCode ProductVpuEncGetResult(CodecInst* instance, EncOutputInfo* result)
{
    RetCode     ret = RETCODE_NOT_FOUND_VPU_DEVICE;

    switch (instance->productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
    case PRODUCT_ID_410:
    case PRODUCT_ID_4102:
    case PRODUCT_ID_412:
    case PRODUCT_ID_510:
    case PRODUCT_ID_512:
    case PRODUCT_ID_515:
        ret = RETCODE_NOT_SUPPORTED_FEATURE;
        break;
    case PRODUCT_ID_420:
    case PRODUCT_ID_420L:
        ret = Wave4VpuEncGetResult(instance, result);
        break;
    case PRODUCT_ID_7Q:
        ret = RETCODE_NOT_SUPPORTED_FEATURE;
        break;
    case PRODUCT_ID_520:
        // [fix me]
        break;
    }

    return ret;
}

RetCode ProductVpuEncGiveCommand(CodecInst* instance, CodecCommand cmd, void* param)
{
    RetCode     ret = RETCODE_NOT_SUPPORTED_FEATURE;

    switch (instance->productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
    case PRODUCT_ID_320:
    case PRODUCT_ID_420:
    case PRODUCT_ID_420L:
        ret = Wave4VpuEncGiveCommand(instance, cmd, param);
        break;
    case PRODUCT_ID_520:
        // [fix me]
        break;
    }

    return ret;
}
