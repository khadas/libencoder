//--=========================================================================--
//  This file is a part of VPU Reference API project
//-----------------------------------------------------------------------------
//
//       This confidential and proprietary software may be used only
//     as authorized by a licensing agreement from Chips&Media Inc.
//     In the event of publication, the following notice is applicable:
//
//            (C) COPYRIGHT 2006 - 2011  CHIPS&MEDIA INC.
//                      ALL RIGHTS RESERVED
//
//       The entire notice above must be reproduced on all authorized
//       copies.
//
//--=========================================================================--

#include "vpuapifunc.h"
#include "product.h"
#include "common_regdefine.h"
#include "wave4_regdefine.h"


#ifndef MIN
#define MIN(a, b)       (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b)       (((a) > (b)) ? (a) : (b))
#endif
#define MAX_LAVEL_IDX    16
static const int g_anLevel[MAX_LAVEL_IDX] =
{
    10, 11, 11, 12, 13,
    //10, 16, 11, 12, 13,
    20, 21, 22,
    30, 31, 32,
    40, 41, 42,
    50, 51
};

static const int g_anLevelMaxMBPS[MAX_LAVEL_IDX] =
{
    1485,   1485,   3000,   6000, 11880,
    11880,  19800,  20250,
    40500,  108000, 216000,
    245760, 245760, 522240,
    589824, 983040
};

static const int g_anLevelMaxFS[MAX_LAVEL_IDX] =
{
    99,    99,   396, 396, 396,
    396,   792,  1620,
    1620,  3600, 5120,
    8192,  8192, 8704,
    22080, 36864
};

static const int g_anLevelMaxBR[MAX_LAVEL_IDX] =
{
    64,     64,   192,  384, 768,
    2000,   4000,  4000,
    10000,  14000, 20000,
    20000,  50000, 50000,
    135000, 240000
};

static const int g_anLevelSliceRate[MAX_LAVEL_IDX] =
{
    0,  0,  0,  0,  0,
    0,  0,  0,
    22, 60, 60,
    60, 24, 24,
    24, 24
};

static const int g_anLevelMaxMbs[MAX_LAVEL_IDX] =
{
    28,   28,  56, 56, 56,
    56,   79, 113,
    113, 169, 202,
    256, 256, 263,
    420, 543
};

/******************************************************************************
    define value
******************************************************************************/

/******************************************************************************
    Codec Instance Slot Management
******************************************************************************/


RetCode InitCodecInstancePool(Uint32 coreIdx)
{
    int i;
    CodecInst * pCodecInst;
    vpu_instance_pool_t *vip;

    vip = (vpu_instance_pool_t *)vdi_get_instance_pool(coreIdx);
    if (!vip)
        return RETCODE_INSUFFICIENT_RESOURCE;

    if (vip->instance_pool_inited == 0)
    {
        for ( i = 0; i < MAX_NUM_INSTANCE; i++)
        {
            pCodecInst = (CodecInst *)vip->codecInstPool[i];
            pCodecInst->instIndex = i;
            pCodecInst->inUse = 0;
        }
        vip->instance_pool_inited = 1;
    }
    return RETCODE_SUCCESS;
}

/*
 * GetCodecInstance() obtains a instance.
 * It stores a pointer to the allocated instance in *ppInst
 * and returns RETCODE_SUCCESS on success.
 * Failure results in 0(null pointer) in *ppInst and RETCODE_FAILURE.
 */

RetCode GetCodecInstance(Uint32 coreIdx, CodecInst ** ppInst)
{
    int                     i;
    CodecInst*              pCodecInst = 0;
    vpu_instance_pool_t*    vip;
    Uint32                  handleSize;

    vip = (vpu_instance_pool_t *)vdi_get_instance_pool(coreIdx);
    if (!vip)
        return RETCODE_INSUFFICIENT_RESOURCE;

    for (i = 0; i < MAX_NUM_INSTANCE; i++) {
        pCodecInst = (CodecInst *)vip->codecInstPool[i];

        if (!pCodecInst) {
            return RETCODE_FAILURE;
        }

        if (!pCodecInst->inUse) {
            break;
        }
    }

    if (i == MAX_NUM_INSTANCE) {
        *ppInst = 0;
        return RETCODE_FAILURE;
    }

    pCodecInst->inUse         = 1;
    pCodecInst->coreIdx       = coreIdx;
    pCodecInst->codecMode     = -1;
    pCodecInst->codecModeAux  = -1;
    pCodecInst->loggingEnable = 0;
    pCodecInst->isDecoder     = TRUE;
    pCodecInst->productId     = ProductVpuGetId(coreIdx);
    osal_memset((void*)&pCodecInst->CodecInfo, 0x00, sizeof(pCodecInst->CodecInfo));

    handleSize = sizeof(DecInfo);
    if (handleSize < sizeof(EncInfo)) {
        handleSize = sizeof(EncInfo);
    }
    if ((pCodecInst->CodecInfo=(void*)osal_malloc(handleSize)) == NULL) {
        return RETCODE_INSUFFICIENT_RESOURCE;
    }
    osal_memset(pCodecInst->CodecInfo, 0x00, sizeof(handleSize));

    *ppInst = pCodecInst;

    if (vdi_open_instance(pCodecInst->coreIdx, pCodecInst->instIndex) < 0) {
        return RETCODE_FAILURE;
    }

    return RETCODE_SUCCESS;
}

void FreeCodecInstance(CodecInst * pCodecInst)
{
    pCodecInst->inUse = 0;
    pCodecInst->codecMode    = -1;
    pCodecInst->codecModeAux = -1;

    vdi_close_instance(pCodecInst->coreIdx, pCodecInst->instIndex);

    osal_free(pCodecInst->CodecInfo);
    pCodecInst->CodecInfo = NULL;
}

RetCode CheckInstanceValidity(CodecInst * pCodecInst)
{
    int i;
    vpu_instance_pool_t *vip;

    vip = (vpu_instance_pool_t *)vdi_get_instance_pool(pCodecInst->coreIdx);
    if (!vip)
        return RETCODE_INSUFFICIENT_RESOURCE;

    for (i = 0; i < MAX_NUM_INSTANCE; i++) {
        if ((CodecInst *)vip->codecInstPool[i] == pCodecInst)
            return RETCODE_SUCCESS;
    }

    return RETCODE_INVALID_HANDLE;
}



/******************************************************************************
    API Subroutines
******************************************************************************/
Uint64 GetTimestamp(
    EncHandle handle
    )
{
    CodecInst*  pCodecInst = (CodecInst*)handle;
    EncInfo*    pEncInfo   = NULL;
    Uint64      pts;
    Uint32      fps;

    if (pCodecInst == NULL) {
        return 0;
    }

    pEncInfo   = &pCodecInst->CodecInfo->encInfo;
    fps        = pEncInfo->openParam.frameRateInfo;
    if (fps == 0) {
        fps    = 30;        /* 30 fps */
    }

    pts        = pEncInfo->curPTS;
    pEncInfo->curPTS += 90000/fps; /* 90KHz/fps */

    return pts;
}


#ifdef DRAM_TEST
RetCode ExecDRAMReadWriteTest(Uint32 coreIdx, Uint32 *dram_source_addr, Uint32 *dram_destination_addr, Uint32 *dram_data_size)
{
	VpuAttr* pAttr;
	RetCode  ret;

	if (coreIdx >= MAX_NUM_VPU_CORE)
		return RETCODE_INVALID_PARAM;

	EnterLock(coreIdx);

	if (ProductVpuIsInit(coreIdx) == 0) {
		return RETCODE_NOT_INITIALIZED;
	}

	if (GetPendingInst(coreIdx)) {
		LeaveLock(coreIdx);
		return RETCODE_FRAME_NOT_COMPLETE;
	}

	pAttr = &g_VpuCoreAttributes[coreIdx];

	ret         = ProductVpuDRAMReadWriteTest(coreIdx, dram_source_addr, dram_destination_addr, dram_data_size);

	LeaveLock(coreIdx);

	return ret;
}
#endif

RetCode CheckEncInstanceValidity(EncHandle handle)
{
    CodecInst * pCodecInst;
    RetCode ret;

    if (handle == NULL)
        return RETCODE_INVALID_HANDLE;

    pCodecInst = handle;
    ret = CheckInstanceValidity(pCodecInst);
    if (ret != RETCODE_SUCCESS) {
        return RETCODE_INVALID_HANDLE;
    }
    if (!pCodecInst->inUse) {
        return RETCODE_INVALID_HANDLE;
    }

    if (pCodecInst->codecMode != MP4_ENC &&
        pCodecInst->codecMode != HEVC_ENC &&
        pCodecInst->codecMode != AVC_ENC &&
        pCodecInst->codecMode != C7_MP4_ENC &&
        pCodecInst->codecMode != C7_AVC_ENC) {
        return RETCODE_INVALID_HANDLE;
    }
    return RETCODE_SUCCESS;
}


RetCode CheckEncParam(EncHandle handle, EncParam * param)
{
    CodecInst *pCodecInst;
    EncInfo *pEncInfo;

    pCodecInst = handle;
    pEncInfo = &pCodecInst->CodecInfo->encInfo;

    if (param == 0) {
        return RETCODE_INVALID_PARAM;
    }
    if (param->skipPicture != 0 && param->skipPicture != 1) {
        return RETCODE_INVALID_PARAM;
    }
    if (param->skipPicture == 0) {
        if (param->sourceFrame == 0) {
            return RETCODE_INVALID_FRAME_BUFFER;
        }
        if (param->forceIPicture != 0 && param->forceIPicture != 1) {
            return RETCODE_INVALID_PARAM;
        }
    }
    if (pEncInfo->openParam.bitRate == 0) { // no rate control
        if (pCodecInst->codecMode == MP4_ENC || pCodecInst->codecMode == C7_MP4_ENC) {
            if (param->quantParam < 1 || param->quantParam > 31) {
                return RETCODE_INVALID_PARAM;
            }
        }
        else if (pCodecInst->codecMode == HEVC_ENC) {
            if (param->forcePicQpEnable == 1) {
                if (param->forcePicQpI < 0 || param->forcePicQpI > 51)
                    return RETCODE_INVALID_PARAM;

                if (param->forcePicQpP < 0 || param->forcePicQpP > 51)
                    return RETCODE_INVALID_PARAM;

                if (param->forcePicQpB < 0 || param->forcePicQpB > 51)
                    return RETCODE_INVALID_PARAM;
            }
            if (pEncInfo->ringBufferEnable == 0) {
                if (param->picStreamBufferAddr % 16 || param->picStreamBufferSize == 0)
                    return RETCODE_INVALID_PARAM;
            }
        }
        else { // AVC_ENC
            if (param->quantParam < 0 || param->quantParam > 51) {
                return RETCODE_INVALID_PARAM;
            }
        }
    }
    if (pEncInfo->ringBufferEnable == 0) {
        if (param->picStreamBufferAddr % 8 || param->picStreamBufferSize == 0) {
            return RETCODE_INVALID_PARAM;
        }
    }

    if (param->ctuOptParam.roiEnable && param->ctuOptParam.ctuQpEnable)
        return RETCODE_INVALID_PARAM;
    return RETCODE_SUCCESS;
}

void EncSetHostParaAddr(Uint32 coreIdx, PhysicalAddress baseAddr, PhysicalAddress paraAddr)
{
    BYTE tempBuf[8]={0,};                    // 64bit bus & endian
    Uint32 val;

    val =  paraAddr;
    tempBuf[0] = 0;
    tempBuf[1] = 0;
    tempBuf[2] = 0;
    tempBuf[3] = 0;
    tempBuf[4] = (val >> 24) & 0xff;
    tempBuf[5] = (val >> 16) & 0xff;
    tempBuf[6] = (val >> 8) & 0xff;
    tempBuf[7] = (val >> 0) & 0xff;
    VpuWriteMem(coreIdx, baseAddr, (BYTE *)tempBuf, 8, VDI_BIG_ENDIAN);
}


RetCode EnterDispFlagLock(Uint32 coreIdx)
{
    if (vdi_disp_lock(coreIdx) != 0)
        return RETCODE_FAILURE;
    return RETCODE_SUCCESS;
}

RetCode LeaveDispFlagLock(Uint32 coreIdx)
{
    vdi_disp_unlock(coreIdx);
    return RETCODE_SUCCESS;
}

RetCode EnterLock(Uint32 coreIdx)
{
    if (vdi_lock(coreIdx) != 0)
        return RETCODE_FAILURE;
    SetClockGate(coreIdx, 1);
    return RETCODE_SUCCESS;
}

RetCode LeaveLock(Uint32 coreIdx)
{
    SetClockGate(coreIdx, 0);
    vdi_unlock(coreIdx);
    return RETCODE_SUCCESS;
}

RetCode SetClockGate(Uint32 coreIdx, Uint32 on)
{
    CodecInst *inst;
    vpu_instance_pool_t *vip;

    vip = (vpu_instance_pool_t *)vdi_get_instance_pool(coreIdx);
    if (!vip) {
        VLOG(ERR, "SetClockGate: RETCODE_INSUFFICIENT_RESOURCE\n");
        return RETCODE_INSUFFICIENT_RESOURCE;
    }

    inst = (CodecInst *)vip->pendingInst;

    if ( !on && (inst || !vdi_lock_check(coreIdx)))
        return RETCODE_SUCCESS;

    vdi_set_clock_gate(coreIdx, on);

    return RETCODE_SUCCESS;
}

void SetPendingInst(Uint32 coreIdx, CodecInst *inst)
{
    vpu_instance_pool_t *vip;

    vip = (vpu_instance_pool_t *)vdi_get_instance_pool(coreIdx);
    if (!vip)
        return;

    vip->pendingInst = inst;
	if (inst)
		vip->pendingInstIdxPlus1 = (inst->instIndex+1);
	else
		vip->pendingInstIdxPlus1 = 0;
}

void ClearPendingInst(Uint32 coreIdx)
{
    vpu_instance_pool_t *vip;

    vip = (vpu_instance_pool_t *)vdi_get_instance_pool(coreIdx);
    if (!vip)
        return;

    if (vip->pendingInst) {
        vip->pendingInst = 0;
		vip->pendingInstIdxPlus1 = 0;
	}
}



CodecInst *GetPendingInst(Uint32 coreIdx)
{
    vpu_instance_pool_t *vip;
	int pendingInstIdx;

    vip = (vpu_instance_pool_t *)vdi_get_instance_pool(coreIdx);
    if (!vip)
        return NULL;

	if (!vip->pendingInst)
		return NULL;

	pendingInstIdx = vip->pendingInstIdxPlus1-1;
	if (pendingInstIdx < 0)
		return NULL;
	if (pendingInstIdx > MAX_NUM_INSTANCE)
		return NULL;

	return  (CodecInst *)vip->codecInstPool[pendingInstIdx];
}

int GetPendingInstIdx(Uint32 coreIdx)
{
	vpu_instance_pool_t *vip;

	vip = (vpu_instance_pool_t *)vdi_get_instance_pool(coreIdx);
	if (!vip)
		return -1;

	return (vip->pendingInstIdxPlus1-1);
}

Int32 MaverickCache2Config(
    MaverickCacheConfig* pCache,
    BOOL                 decoder ,
    BOOL                 interleave,
    Uint32               bypass,
    Uint32               burst,
    Uint32               merge,
    TiledMapType         mapType,
    Uint32               wayshape
    )
{
    unsigned int cacheConfig = 0;

    if (decoder == TRUE) {
        if (mapType == 0) {// LINEAR_FRAME_MAP
            //VC1 opposite field padding is not allowable in UV separated, burst 8 and linear map
            if (!interleave)
                burst = 0;

            wayshape = 15;

            if (merge == 1)
                merge = 3;

            //GDI constraint. Width should not be over 64
            if (( merge== 1) && (burst))
                burst = 0;
        }
        else {
            //horizontal merge constraint in tiled map
            if (merge == 1)
                merge = 3;
        }
    }
    else { // encoder
        if (mapType == LINEAR_FRAME_MAP) {
            wayshape = 15;
            //GDI constraint. Width should not be over 64
            if ((merge == 1) && (burst))
                burst= 0;
        }
        else {
            //horizontal merge constraint in tiled map
            if (merge == 1)
                merge = 3;
        }
    }

    cacheConfig = (merge & 0x3) << 9;
    cacheConfig = cacheConfig | ((wayshape & 0xf) << 5);
    cacheConfig = cacheConfig | ((burst & 0x1) << 3);
    cacheConfig = cacheConfig | (bypass & 0x3);

    if (mapType != 0)//LINEAR_FRAME_MAP
        cacheConfig = cacheConfig | 0x00000004;

    ///{16'b0, 5'b0, merge[1:0], wayshape[3:0], 1'b0, burst[0], map[0], bypass[1:0]};
    pCache->type2.CacheMode = cacheConfig;

    return 1;
}

RetCode AllocateLinearFrameBuffer(
    TiledMapType            mapType,
    FrameBuffer*            fbArr,
    Uint32                  numOfFrameBuffers,
    Uint32                  sizeLuma,
    Uint32                  sizeChroma
    )
{
    Uint32      i;
    BOOL        yuv422Interleave = FALSE;
    BOOL        fieldFrame       = (BOOL)(mapType == LINEAR_FIELD_MAP);
    BOOL        cbcrInterleave   = (BOOL)(mapType == COMPRESSED_FRAME_MAP || fbArr[0].cbcrInterleave == TRUE);
    BOOL        reuseFb          = FALSE;


    if (mapType != COMPRESSED_FRAME_MAP) {
        switch (fbArr[0].format) {
        case FORMAT_YUYV:
        case FORMAT_YUYV_P10_16BIT_MSB:
        case FORMAT_YUYV_P10_16BIT_LSB:
        case FORMAT_YUYV_P10_32BIT_MSB:
        case FORMAT_YUYV_P10_32BIT_LSB:
        case FORMAT_YVYU:
        case FORMAT_YVYU_P10_16BIT_MSB:
        case FORMAT_YVYU_P10_16BIT_LSB:
        case FORMAT_YVYU_P10_32BIT_MSB:
        case FORMAT_YVYU_P10_32BIT_LSB:
        case FORMAT_UYVY:
        case FORMAT_UYVY_P10_16BIT_MSB:
        case FORMAT_UYVY_P10_16BIT_LSB:
        case FORMAT_UYVY_P10_32BIT_MSB:
        case FORMAT_UYVY_P10_32BIT_LSB:
        case FORMAT_VYUY:
        case FORMAT_VYUY_P10_16BIT_MSB:
        case FORMAT_VYUY_P10_16BIT_LSB:
        case FORMAT_VYUY_P10_32BIT_MSB:
        case FORMAT_VYUY_P10_32BIT_LSB:
            yuv422Interleave = TRUE;
            break;
        default:
            yuv422Interleave = FALSE;
            break;
        }
    }

    for (i=0; i<numOfFrameBuffers; i++) {
        reuseFb = (fbArr[i].bufY != (Uint32)-1 && fbArr[i].bufCb != (Uint32)-1 && fbArr[i].bufCr != (Uint32)-1);
        if (reuseFb == FALSE) {
            if (yuv422Interleave == TRUE) {
                fbArr[i].bufCb = (PhysicalAddress)-1;
                fbArr[i].bufCr = (PhysicalAddress)-1;
            }
            else {
                if (fbArr[i].bufCb == (PhysicalAddress)-1) {
                    fbArr[i].bufCb = fbArr[i].bufY + (sizeLuma >> fieldFrame);
                }
                if (fbArr[i].bufCr == (PhysicalAddress)-1) {
                    if (cbcrInterleave == TRUE) {
                        fbArr[i].bufCr = (PhysicalAddress)-1;
                    }
                    else {
                        fbArr[i].bufCr = fbArr[i].bufCb + (sizeChroma >> fieldFrame);
                    }
                }
            }
        }
    }

    return RETCODE_SUCCESS;
}


/* \brief   Allocate tiled framebuffer on GDI version 1.0 H/W.
 */
RetCode AllocateTiledFrameBufferGdiV1(
    TiledMapType            mapType,
    PhysicalAddress         tiledBaseAddr,
    FrameBuffer*            fbArr,
    Uint32                  numOfFrameBuffers,
    Uint32                  sizeLuma,
    Uint32                  sizeChroma,
    DRAMConfig*             pDramCfg
    )
{
    Uint32          rasLowBitsForHor;
    Uint32          i;
    Uint32          cas, ras, bank, bus;
    Uint32          lumRasTop, lumRasBot, chrRasTop, chrRasBot;
    Uint32          lumFrameRasSize, lumFieldRasSize, chrFieldRasSize;
    PhysicalAddress addrY, addrYRas;

    if (mapType == TILED_FRAME_MB_RASTER_MAP || mapType == TILED_FIELD_MB_RASTER_MAP) {
        for (i=0; i<numOfFrameBuffers; i++) {
            int  lum_top_base;
            int  lum_bot_base;
            int  chr_top_base;
            int  chr_bot_base;

            addrY = ((fbArr[i].bufY+(16384-1))&~(16384-1));

            lum_top_base = addrY;
            lum_bot_base = addrY + sizeLuma/2;
            chr_top_base = addrY + sizeLuma;
            chr_bot_base = addrY + sizeLuma + sizeChroma; // cbcr is interleaved

            lum_top_base = (lum_top_base>>12) & 0xfffff;
            lum_bot_base = (lum_bot_base>>12) & 0xfffff;
            chr_top_base = (chr_top_base>>12) & 0xfffff;
            chr_bot_base = (chr_bot_base>>12) & 0xfffff;

            fbArr[i].bufY  = ( lum_top_base           << 12) | (chr_top_base >> 8);
            fbArr[i].bufCb = ((chr_top_base & 0xff  ) << 24) | (lum_bot_base << 4) | (chr_bot_base >> 16);
            fbArr[i].bufCr = ((chr_bot_base & 0xffff) << 16) ;
            fbArr[i].bufYBot  = (PhysicalAddress)-1;
            fbArr[i].bufCbBot = (PhysicalAddress)-1;
            fbArr[i].bufCrBot = (PhysicalAddress)-1;
        }
    }
    else {
        cas  = pDramCfg->casBit;
        ras  = pDramCfg->rasBit;
        bank = pDramCfg->bankBit;
        bus  = pDramCfg->busBit;
        if (cas == 9 && bank == 2 && ras == 13) {
            rasLowBitsForHor = 3;
        }
        else if(cas == 10 && bank == 3 && ras == 13) {
            rasLowBitsForHor = 2;
        }
        else {
            return RETCODE_INVALID_PARAM;
        }

        for (i=0; i<numOfFrameBuffers; i++) {
            addrY = fbArr[i].bufY - tiledBaseAddr;
            // align base_addr to RAS boundary
            addrYRas  = (addrY + ((1<<(bank+cas+bus))-1)) >> (bank+cas+bus);
            // round up RAS lower 3(or 4) bits
            addrYRas  = ((addrYRas + ((1<<(rasLowBitsForHor))-1)) >> rasLowBitsForHor) << rasLowBitsForHor;

            chrFieldRasSize = sizeChroma>>(pDramCfg->bankBit+pDramCfg->casBit+pDramCfg->busBit);
            lumFieldRasSize = (sizeLuma>>1)>>(pDramCfg->bankBit+pDramCfg->casBit+pDramCfg->busBit);
            lumFrameRasSize = lumFieldRasSize * 2;
            lumRasTop       = addrYRas;
            lumRasBot       = lumRasTop  + lumFieldRasSize;
            chrRasTop       = lumRasTop  + lumFrameRasSize;
            chrRasBot       = chrRasTop  + chrFieldRasSize;

            fbArr[i].bufY    = (lumRasBot << 16) + lumRasTop;
            fbArr[i].bufCb   = (chrRasBot << 16) + chrRasTop;
            if (rasLowBitsForHor == 4) {
                fbArr[i].bufCr  = ((((chrRasBot>>4)<<4) + 8) << 16) + (((chrRasTop>>4)<<4) + 8);
            }
            else if (rasLowBitsForHor == 3) {
                fbArr[i].bufCr  = ((((chrRasBot>>3)<<3) + 4) << 16) + (((chrRasTop>>3)<<3) + 4);
            }
            else if (rasLowBitsForHor == 2) {
                fbArr[i].bufCr  = ((((chrRasBot>>2)<<2) + 2) << 16) + (((chrRasTop>>2)<<2) + 2);
            }
            else if (rasLowBitsForHor == 1) {
                fbArr[i].bufCr  = ((((chrRasBot>>1)<<1) + 1) << 16) + (((chrRasTop>>1)<<1) + 1);
            }
            else {
                return RETCODE_INSUFFICIENT_RESOURCE; // Invalid RasLowBit value
            }
        }
    }

    return RETCODE_SUCCESS;
}

/* \brief   Allocate tiled framebuffer on GDI version 2.0 H/W
 */
RetCode AllocateTiledFrameBufferGdiV2(
    TiledMapType            mapType,
    FrameBuffer*            fbArr,
    Uint32                  numOfFrameBuffers,
    Uint32                  sizeLuma,
    Uint32                  sizeChroma
    )
{
    Uint32          i;
    Uint32          fieldFrame;
    Uint32          sizeFb;
    BOOL            cbcrInterleave;

    sizeFb     = sizeLuma + sizeChroma * 2;
    fieldFrame = (mapType == TILED_FIELD_V_MAP       ||
                  mapType == TILED_FIELD_NO_BANK_MAP ||
                  mapType == LINEAR_FIELD_MAP);

    for (i=0; i<numOfFrameBuffers; i++) {
        cbcrInterleave = fbArr[0].cbcrInterleave;
        fbArr[i].bufCb = fbArr[i].bufY + (sizeLuma >> fieldFrame);
        fbArr[i].bufCr = fbArr[i].bufCb + (sizeChroma >> fieldFrame);

        switch (mapType) {
        case TILED_FIELD_V_MAP:
        case TILED_FIELD_NO_BANK_MAP:
            fbArr[i].bufYBot  = fbArr[i].bufY + (sizeFb>>fieldFrame);
            fbArr[i].bufCbBot = fbArr[i].bufYBot + (sizeLuma>>fieldFrame);
            if (cbcrInterleave == FALSE) {
                fbArr[i].bufCrBot = fbArr[i].bufCbBot + (sizeChroma>>fieldFrame);
            }
            break;
        case TILED_FRAME_V_MAP:
        case TILED_FRAME_H_MAP:
        case TILED_MIXED_V_MAP:
        case TILED_FRAME_NO_BANK_MAP:
            fbArr[i].bufYBot  = fbArr[i].bufY;
            fbArr[i].bufCbBot = fbArr[i].bufCb;
            if (cbcrInterleave == FALSE) {
                fbArr[i].bufCrBot = fbArr[i].bufCr;
            }
            break;
        case TILED_FIELD_MB_RASTER_MAP:
            fbArr[i].bufYBot  = fbArr[i].bufY + (sizeLuma>>1);
            fbArr[i].bufCbBot = fbArr[i].bufCb + sizeChroma;
            break;
        default:
            fbArr[i].bufYBot  = 0;
            fbArr[i].bufCbBot = 0;
            fbArr[i].bufCrBot = 0;
            break;
        }
    }

    return RETCODE_SUCCESS;
}

Int32 ConfigSecAXICoda9(Uint32 coreIdx, Int32 codecMode, SecAxiInfo *sa, Uint32 width, Uint32 height, Uint32 profile)
{
    vpu_buffer_t vb;
    int offset;
    Uint32 MbNumX = ((width & 0xFFFF) + 15) / 16;
    Uint32 MbNumY = ((height & 0xFFFF) + 15) / 16;
    Uint32 productId;


    if (vdi_get_sram_memory(coreIdx, &vb) < 0) {
        return 0;
    }

    productId = ProductVpuGetId(coreIdx);

    if (!vb.size) {
        sa->bufSize = 0;
        sa->u.coda9.useBitEnable = 0;
        sa->u.coda9.useIpEnable = 0;
        sa->u.coda9.useDbkYEnable = 0;
        sa->u.coda9.useDbkCEnable = 0;
        sa->u.coda9.useOvlEnable = 0;
        sa->u.coda9.useBtpEnable = 0;
        return 0;
    }

    sa->bufBase = vb.phys_addr;
    offset      = 0;
    //BIT
    if (sa->u.coda9.useBitEnable) {
        sa->u.coda9.useBitEnable = 1;
        sa->u.coda9.bufBitUse = vb.phys_addr + offset;

        switch (codecMode)
        {
        case AVC_DEC:
            offset = offset + MbNumX * 144;
            break; // AVC
        case RV_DEC:
            offset = offset + MbNumX * 128;
            break;
        case VC1_DEC:
            offset = offset + MbNumX *  64;
            break;
        case AVS_DEC:
            offset = offset + ((MbNumX + 3)&~3) *  32;
            break;
        case MP2_DEC:
            offset = offset + MbNumX * 0;
            break;
        case VPX_DEC:
            offset = offset + MbNumX * 0;
            break;
        case AVC_ENC:
            {
                if (productId == PRODUCT_ID_960)
                    offset = offset + MbNumX * 128;
                else
                    offset = offset + MbNumX * 16;
            }
            break;
        case MP4_ENC:
            offset = offset + MbNumX * 16;
            break;
        default:
            offset = offset + MbNumX *  16;
            break; // MPEG-4, Divx3
        }

        if (offset > vb.size)
        {
            sa->bufSize = 0;
            return 0;
        }
    }

    //Intra Prediction, ACDC
    if (sa->u.coda9.useIpEnable)
    {
        sa->u.coda9.bufIpAcDcUse = vb.phys_addr + offset;
        sa->u.coda9.useIpEnable = 1;

        switch (codecMode)
        {
        case AVC_DEC:
            offset = offset + MbNumX * 64;
            break; // AVC
        case RV_DEC:
            offset = offset + MbNumX * 64;
            break;
        case VC1_DEC:
            offset = offset + MbNumX * 128;
            break;
        case AVS_DEC:
            offset = offset + MbNumX * 64;
            break;
        case MP2_DEC:
            offset = offset + MbNumX * 0;
            break;
        case VPX_DEC:
            offset = offset + MbNumX * 64;
            break;
        case AVC_ENC:
            offset = offset + MbNumX * 64;
            break;
        case MP4_ENC:
            offset = offset + MbNumX * 128;
            break;
        default:
            offset = offset + MbNumX * 128;
            break; // MPEG-4, Divx3
        }

        if (offset > vb.size)
        {
            sa->bufSize = 0;
            return 0;
        }
    }

    //Deblock Chroma
    if (sa->u.coda9.useDbkCEnable)
    {
        sa->u.coda9.bufDbkCUse = vb.phys_addr + offset;
        sa->u.coda9.useDbkCEnable = 1;
        switch (codecMode)
        {
        case AVC_DEC:
            offset = (profile==66/*AVC BP decoder*/) ? offset + (MbNumX * 64) : offset + (MbNumX * 128);
            break; // AVC
        case RV_DEC:
            offset = offset + MbNumX * 128;
            break;
        case VC1_DEC:
            offset = profile==2 ? offset + MbNumX * 256 : offset + MbNumX * 128;
            break;
        case AVS_DEC:
            offset = offset + MbNumX * 64;
            break;
        case MP2_DEC:
            offset = offset + MbNumX * 64;
            break;
        case VPX_DEC:
            offset = offset + MbNumX * 128;
            break;
        case MP4_DEC:
            offset = offset + MbNumX * 64;
            break;
        case AVC_ENC:
            offset = offset + MbNumX * 64;
            break;
        case MP4_ENC:
            offset = offset + MbNumX * 64;
            break;
        default:
            offset = offset + MbNumX * 64;
            break;
        }
        if (offset > vb.size)
        {
            sa->bufSize = 0;
            return 0;
        }
    }

    //Deblock Luma
    if (sa->u.coda9.useDbkYEnable)
    {
        sa->u.coda9.bufDbkYUse = vb.phys_addr + offset;
        sa->u.coda9.useDbkYEnable = 1;

        switch (codecMode)
        {
        case AVC_DEC:
            offset = (profile==66/*AVC BP decoder*/)? offset + (MbNumX * 64) : offset + (MbNumX * 128);
            break; // AVC
        case RV_DEC:
            offset = offset + MbNumX * 128;
            break;
        case VC1_DEC:
            offset = profile==2 ? offset + MbNumX * 256 : offset + MbNumX * 128;
            break;
        case AVS_DEC:
            offset = offset + MbNumX * 64;
            break;
        case MP2_DEC:
            offset = offset + MbNumX * 128;
            break;
        case VPX_DEC:
            offset = offset + MbNumX * 128;
            break;
        case MP4_DEC:
            offset = offset + MbNumX * 64;
            break;
        case AVC_ENC:
            offset = offset + MbNumX * 64;
            break;
        case MP4_ENC:
            offset = offset + MbNumX * 64;
            break;
        default:
            offset = offset + MbNumX * 128;
            break;
        }

        if (offset > vb.size)
        {
            sa->bufSize = 0;
            return 0;
        }
    }

    // check the buffer address which is 256 byte is available.
    if (((offset + 255) & (~255)) > vb.size) {
        VLOG(ERR, "%s:%d NOT ENOUGH SRAM: required(%d), sram(%d)\n", __FUNCTION__, __LINE__, offset, vb.size);
        sa->bufSize = 0;
        return 0;
    }

    //VC1 Bit-plane
    if (sa->u.coda9.useBtpEnable)
    {
        if (codecMode != VC1_DEC)
        {
            sa->u.coda9.useBtpEnable = 0;
        }
        else
        {
            int oneBTP;

            offset = ((offset+255)&~255);
            sa->u.coda9.bufBtpUse = vb.phys_addr + offset;
            sa->u.coda9.useBtpEnable = 1;

            oneBTP  = (((MbNumX+15)/16) * MbNumY + 1) * 2;
            oneBTP  = (oneBTP%256) ? ((oneBTP/256)+1)*256 : oneBTP;

            offset = offset + oneBTP * 3;

            if (offset > vb.size)
            {
                sa->bufSize = 0;
                return 0;
            }
        }
    }

    //VC1 Overlap
    if (sa->u.coda9.useOvlEnable)
    {
        if (codecMode != VC1_DEC)
        {
            sa->u.coda9.useOvlEnable = 0;
        }
        else
        {
            sa->u.coda9.bufOvlUse = vb.phys_addr + offset;
            sa->u.coda9.useOvlEnable = 1;

            offset = offset + MbNumX *  80;

            if (offset > vb.size)
            {
                sa->bufSize = 0;
                return 0;
            }
        }
    }

    sa->bufSize = offset;

    return 1;
}

Int32 ConfigSecAXIWave(Uint32 coreIdx, Int32 codecMode, SecAxiInfo *sa, Uint32 width, Uint32 height, Uint32 profile, Uint32 levelIdc)
{
    vpu_buffer_t vb;
    int offset;
    Uint32 size;
    Uint32 lumaSize;
    Uint32 chromaSize;
    Uint32 productId;

    UNREFERENCED_PARAMETER(codecMode);
    UNREFERENCED_PARAMETER(height);

    if (vdi_get_sram_memory(coreIdx, &vb) < 0)
        return 0;

    productId = ProductVpuGetId(coreIdx);

    if (!vb.size) {
        sa->bufSize                = 0;
        sa->u.wave4.useIpEnable    = 0;
        sa->u.wave4.useLfRowEnable = 0;
        sa->u.wave4.useBitEnable   = 0;
        sa->u.wave4.useEncImdEnable   = 0;
        sa->u.wave4.useEncLfEnable    = 0;
        sa->u.wave4.useEncRdoEnable   = 0;
        return 0;
    }

    sa->bufBase = vb.phys_addr;
    offset      = 0;
    /* Intra Prediction */
    if (sa->u.wave4.useIpEnable == TRUE) {
        sa->u.wave4.bufIp = sa->bufBase + offset;

        switch (productId) {
        case PRODUCT_ID_410:
            lumaSize   = VPU_ALIGN16(width);
            chromaSize = VPU_ALIGN16(width);
            break;
        case PRODUCT_ID_412:
        case PRODUCT_ID_512:
        case PRODUCT_ID_515:
            if ( codecMode == C7_VP9_DEC ) {
                lumaSize   = VPU_ALIGN128(width) * 10/8;
                chromaSize = VPU_ALIGN128(width) * 10/8;
            }
            else {
                lumaSize   = (VPU_ALIGN16(width)*10+127)/128*128/8;
                chromaSize = (VPU_ALIGN16(width)*10+127)/128*128/8;
            }
            break;
        case PRODUCT_ID_4102:
        case PRODUCT_ID_420:
        case PRODUCT_ID_420L:
        case PRODUCT_ID_510:
            lumaSize   = (VPU_ALIGN16(width)*10+127)/128*128/8;
            chromaSize = (VPU_ALIGN16(width)*10+127)/128*128/8;
            break;
        case PRODUCT_ID_520:
            // [fix me]
            break;
        default:
            return 0;
        }

        offset     = lumaSize + chromaSize;
        if (offset > vb.size) {
            sa->bufSize = 0;
            return 0;
        }
    }

    /* Loopfilter row */
    if (sa->u.wave4.useLfRowEnable == TRUE) {
        sa->u.wave4.bufLfRow = sa->bufBase + offset;
        if ( codecMode == C7_VP9_DEC ) {
            if ( profile == VP9_PROFILE_2)
            {
                lumaSize   = VPU_ALIGN64(width) * 8 * 10/8; /* lumaLIne   : 8 */
                chromaSize = VPU_ALIGN64(width) * 8 * 10/8; /* chromaLine : 8 */
            }
            else
            {
                lumaSize   = VPU_ALIGN64(width) * 8; /* lumaLIne   : 8 */
                chromaSize = VPU_ALIGN64(width) * 8; /* chromaLine : 8 */
            }
            offset += lumaSize+chromaSize;
        }
        else {
            Uint32 level = levelIdc/30;
            if (level >= 5) {
                size = VPU_ALIGN32(width)/2 * 13 + VPU_ALIGN64(width)*4;
            }
            else {
                size = VPU_ALIGN64(width)*13;
            }
            offset += size;
        }
        if (offset > vb.size) {
            sa->bufSize = 0;
            return 0;
        }
    }

    if (sa->u.wave4.useBitEnable == TRUE) {
        sa->u.wave4.bufBit = sa->bufBase + offset;
        if (codecMode == C7_VP9_DEC) {
            size = VPU_ALIGN64(width)/64 * (70*8);
        }
        else {
            size = 34*1024; /* Fixed size */
        }
        offset += size;
        if (offset > vb.size) {
            sa->bufSize = 0;
            return 0;
        }
    }

    if (sa->u.wave4.useEncImdEnable == TRUE) {
         /* Main   profile(8bit) : Align32(picWidth)
          * Main10 profile(10bit): Align32(picWidth)
          */
        sa->u.wave4.bufImd = sa->bufBase + offset;
        offset    += VPU_ALIGN32(width);
        if (offset > vb.size) {
            sa->bufSize = 0;
            return 0;
        }
    }

    if (sa->u.wave4.useEncLfEnable == TRUE) {
        /* Main   profile(8bit) :
         *              Luma   = Align64(picWidth) * 5
         *              Chroma = Align64(picWidth) * 3
         * Main10 profile(10bit) :
         *              Luma   = Align64(picWidth) * 7
         *              Chroma = Align64(picWidth) * 5
         */
        Uint32 luma   = (profile == HEVC_PROFILE_MAIN10 ? 7 : 5);
        Uint32 chroma = (profile == HEVC_PROFILE_MAIN10 ? 5 : 3);

        sa->u.wave4.bufLf = sa->bufBase + offset;

        lumaSize   = VPU_ALIGN64(width) * luma;
        chromaSize = VPU_ALIGN64(width) * chroma;
        offset    += lumaSize + chromaSize;

        if (offset > vb.size) {
            sa->bufSize = 0;
            return 0;
        }
    }

    if (sa->u.wave4.useEncRdoEnable == TRUE) {
         /* Main   profile(8bit) : (Align64(picWidth)/32) * 22*16
         * Main10 profile(10bit): (Align64(picWidth)/32) * 22*16
         */
        sa->u.wave4.bufRdo = sa->bufBase + offset;
        offset    += (VPU_ALIGN64(width)/32) * 22*16;

        if (offset > vb.size) {
            sa->bufSize = 0;
            return 0;
        }
    }

    sa->bufSize = offset;

    return 1;
}

Int32 ConfigSecAXICoda7(Uint32 coreIdx, CodStd codStd, SecAxiInfo *sa, Uint32 width, Uint32 height, Uint32 profile)
{
    vpu_buffer_t vb;
    int offset;
    Uint32 size;
    Uint32 lumaSize;
    Uint32 chromaSize;
    Uint32 MbNumX = ((width & 0xFFFF) + 15) / 16;
    Uint32 MbNumY = ((height & 0xFFFF) + 15) / 16;

    if (vdi_get_sram_memory(coreIdx, &vb) < 0)
        return 0;

    if (!vb.size) {
        sa->bufSize                = 0;
        sa->u.wave4.useIpEnable    = 0;
        sa->u.wave4.useLfRowEnable = 0;
        sa->u.wave4.useBitEnable   = 0;
        sa->u.coda9.useBitEnable    = 0;
        sa->u.coda9.useIpEnable     = 0;
        sa->u.coda9.useDbkYEnable   = 0;
        sa->u.coda9.useDbkCEnable   = 0;
        sa->u.coda9.useOvlEnable    = 0;
        sa->u.coda9.useMeEnable     = 0;
        return 0;
    }

    sa->bufBase = vb.phys_addr;
    offset      = 0;

    if (codStd == STD_HEVC) {
        /* Intra Prediction */
        if (sa->u.wave4.useIpEnable == TRUE) {
            sa->u.wave4.bufIp = sa->bufBase + offset;
            lumaSize  = (VPU_ALIGN16(width)*10+127)/128*128/8;
            chromaSize = (VPU_ALIGN16(width)*10+127)/128*128/8;

            offset     = lumaSize + chromaSize;
            if (offset > vb.size) {
                sa->bufSize = 0;
                return 0;
            }
        }

        /* Loopfilter row */
        if (sa->u.wave4.useLfRowEnable == TRUE) {
            /* Main   profile: Luma 5 lines + Chroma 3 lines
                Main10 profile: Luma 7 lines + Chroma 4 lines
            */
            Uint32 lumaLines   = (profile == HEVC_PROFILE_MAIN10 ? 7 : 5);
            Uint32 chromaLines = (profile == HEVC_PROFILE_MAIN10 ? 4 : 3);

            sa->u.wave4.bufLfRow = sa->bufBase + offset;
            lumaSize   = VPU_ALIGN64(width) * lumaLines;
            chromaSize = VPU_ALIGN64(width) * chromaLines;

            offset    += lumaSize + chromaSize;
            if (offset > vb.size) {
                sa->bufSize = 0;
                return 0;
            }
        }

        if (sa->u.wave4.useBitEnable == TRUE) {
            sa->u.wave4.bufBit = sa->bufBase + offset;
            size = VPU_ALIGN64(width) * 4;

            offset += size;
            if (offset > vb.size) {
                sa->bufSize = 0;
                return 0;
            }
        }
    }
    else { // for legacy codec
        /* BIT, VC1 Bit-plane */
        /* Coda7Q use same register for BIT and Bit-plane */
        if (sa->u.coda9.useBitEnable == TRUE) {
            sa->u.coda9.bufBitUse = sa->bufBase + offset;
            sa->u.coda9.useBitEnable = 1;

            switch (codStd)
            {
            case STD_AVC:
                offset = offset + MbNumX * 128;
                break; // AVC
            case STD_RV:
                offset = offset + MbNumX * 128;
                break;
            case STD_VC1:
                {
                    int oneBTP;

                    oneBTP  = (((MbNumX+15)/16) * MbNumY + 1) * 2;
                    oneBTP  = (oneBTP%256) ? ((oneBTP/256)+1)*256 : oneBTP;

                    offset = offset + oneBTP * 3;
                    offset = offset + MbNumX *  64;
                }
                break;
            case STD_AVS:
                offset = offset + ((MbNumX + 3)&~3) *  32;
                break;
            case STD_MPEG2:
                offset = offset + MbNumX * 0;
                break;
            case STD_VP8:
                offset = offset + MbNumX * 128;
                break;
            default:
                offset = offset + MbNumX *  16;
                break; // MPEG-4, Divx3
            }

            if (offset > vb.size) {
                sa->bufSize = 0;
                return 0;
            }
        }

        /* Intra Prediction, ACDC */
        if (sa->u.coda9.useIpEnable == TRUE) {
            sa->u.coda9.bufIpAcDcUse = sa->bufBase + offset;
            sa->u.coda9.useIpEnable = 1;

            switch (codStd)
            {
            case STD_AVC:
                offset = offset + MbNumX * 64 * 2; // MBAFF needs double buffer;
                break; // AVC
            case STD_RV:
                offset = offset + MbNumX * 64;
                break;
            case STD_VC1:
                offset = offset + MbNumX * 128;
                break;
            case STD_AVS:
                offset = offset + MbNumX * 64;
                break;
            case STD_MPEG2:
                offset = offset + MbNumX * 0;
                break;
            case STD_VP8:
                offset = offset + MbNumX * 64;
                break;
            default:
                offset = offset + MbNumX * 128;
                break; // MPEG-4, Divx3
            }

            if (offset > vb.size) {
                sa->bufSize = 0;
                return 0;
            }
        }

        /* Deblock Chroma */
        if (sa->u.coda9.useDbkCEnable == TRUE) {
            sa->u.coda9.bufDbkCUse = sa->bufBase + offset;
            sa->u.coda9.useDbkCEnable = 1;
            switch (codStd)
            {
            case STD_AVC:
                offset = (profile==66 || profile==0) ? (offset + (MbNumX * 64)) : (offset + (MbNumX * 128));
                break; // AVC
            case STD_RV:
                offset = offset + MbNumX * 128;
                break;
            case STD_VC1:
                offset = profile==2 ? offset + MbNumX * 256 : offset + MbNumX * 128;
                offset = (offset + 255) & (~255);
                break;
            case STD_AVS:
                offset = offset + MbNumX * 128;
                break;
            case STD_MPEG2:
                offset = offset + MbNumX * 64;
                break;
            case STD_VP8:
                offset = offset + MbNumX * 128;
                break;
            default:
                offset = offset + MbNumX * 64;
                break;
            }

            if (offset > vb.size) {
                sa->bufSize = 0;
                return 0;
            }
        }

        /* Deblock Luma */
        if (sa->u.coda9.useDbkYEnable == TRUE) {
            sa->u.coda9.bufDbkYUse = sa->bufBase + offset;
            sa->u.coda9.useDbkYEnable = 1;

            switch (codStd)
            {
            case STD_AVC:
                offset = (profile==66/*AVC BP decoder*/ || profile==0/* AVC encoder */) ? (offset + (MbNumX * 64)) : (offset + (MbNumX * 128));
                break; // AVC
            case STD_RV:
                offset = offset + MbNumX * 128;
                break;
            case STD_VC1:
                offset = profile==2 ? offset + MbNumX * 256 : offset + MbNumX * 128;
                offset = (offset + 255) & (~255);
                break;
            case STD_AVS:
                offset = offset + MbNumX * 128;
                break;
            case STD_MPEG2:
                offset = offset + MbNumX * 128;
                break;
            case STD_VP8:
                offset = offset + MbNumX * 128;
                break;
            default:
                offset = offset + MbNumX * 128;
                break;
            }

            if (offset > vb.size) {
                sa->bufSize = 0;
                return 0;
            }
        }

        // check the buffer address which is 256 byte is available.
        if (((offset + 255) & (~255)) > vb.size) {
            sa->bufSize = 0;
            return 0;
        }

        /* Motion Estimation */
        if (sa->u.coda9.useMeEnable == TRUE) {
            offset = (offset + 255) & (~255);
            sa->u.coda9.bufMeUse    = sa->bufBase + offset;
            sa->u.coda9.useMeEnable = 1;

            offset = offset + MbNumX*16*36 + 2048;

            if (offset > vb.size)
            {
                sa->bufSize = 0;
                return 0;
            }
        }

        /* VC1 Overlap */
        if (sa->u.coda9.useOvlEnable == TRUE) {
            if (codStd != STD_VC1) {
                sa->u.coda9.useOvlEnable = 0;
            }
            else {
                sa->u.coda9.bufOvlUse = sa->bufBase + offset;
                sa->u.coda9.useOvlEnable = 1;

                offset = offset + MbNumX *  80;

                if (offset > vb.size)
                {
                    sa->bufSize = 0;
                    return 0;
                }
            }
        }
    }

    sa->bufSize = offset;

    return 1;
}

static int GetXY2AXILogic(int map_val , int xpos, int ypos, int tb)
{
    int invert;
    int assign_zero;
    int tbxor;
    int xysel;
    int bitsel;

    int xypos,xybit,xybit_st1,xybit_st2,xybit_st3;

    invert      = map_val >> 7;
    assign_zero = (map_val & 0x78) >> 6;
    tbxor       = (map_val & 0x3C) >> 5;
    xysel       = (map_val & 0x1E) >> 4;
    bitsel      = map_val & 0x0f;

    xypos     = (xysel) ? ypos : xpos;
    xybit     = (xypos >> bitsel) & 0x01;
    xybit_st1 = (tbxor)       ? xybit^tb : xybit;
    xybit_st2 = (assign_zero) ? 0 : xybit_st1;
    xybit_st3 = (invert)      ? !xybit_st2 : xybit_st2;

    return xybit_st3;
}

static int GetXY2AXIAddr20(TiledMapConfig *pMapCfg, int ycbcr, int posY, int posX, int stride, FrameBuffer *fb)
{
    int tbSeparateMap;
    int use_linear_field;
    int ypos_field;
    int tb;
    int chr_flag;
    int ypos_mod;
    int i;
    int mbx, mby;
    int mbx_num;
    int mb_addr;
    int xy2axiLumMap;
    int xy2axiChrMap;
    int xy2axi_map_sel;
    int temp_bit;
    int tmp_addr;
    int axi_conv;

    int y_top_base;
    int cb_top_base;
    int cr_top_base;
    int y_bot_base;
    int cb_bot_base;
    int cr_bot_base;
    int top_base_addr;
    int bot_base_addr;
    int base_addr;
    int pix_addr;
    int mapType;
    mapType = fb->mapType;

    if (!pMapCfg)
        return -1;

    tbSeparateMap = pMapCfg->tbSeparateMap;
    use_linear_field = (mapType == 9);
    ypos_field = posY/2;
    tb = posY & 1;
    ypos_mod = (tbSeparateMap | use_linear_field) ? ypos_field : posY;
    chr_flag = (ycbcr >> 1) & 0x1;

    mbx_num = stride/16;

    y_top_base = fb->bufY;
    cb_top_base = fb->bufCb;
    cr_top_base = fb->bufCr;
    y_bot_base = fb->bufYBot;
    cb_bot_base = fb->bufCbBot;
    cr_bot_base = fb->bufCrBot;

    if (mapType == LINEAR_FRAME_MAP)
    {
        base_addr = (ycbcr==0) ? y_top_base  : (ycbcr==2) ? cb_top_base : cr_top_base;
        pix_addr = ((posY * stride) + posX) + base_addr;
    }
    else if (mapType == LINEAR_FIELD_MAP)
    {
        top_base_addr = (ycbcr==0) ? y_top_base  : (ycbcr==2) ? cb_top_base : cr_top_base;
        bot_base_addr = (ycbcr==0) ? y_bot_base  : (ycbcr==2) ? cb_bot_base : cr_bot_base;
        base_addr = tb ? bot_base_addr : top_base_addr;

        pix_addr = ((ypos_mod * stride) + posX) + base_addr;
    }
    else if (mapType == TILED_FRAME_MB_RASTER_MAP || mapType == TILED_FIELD_MB_RASTER_MAP)
    {
        top_base_addr = (ycbcr==0) ? y_top_base  : (ycbcr==2) ? cb_top_base : cr_top_base;
        bot_base_addr = (ycbcr==0) ? y_bot_base  : (ycbcr==2) ? cb_bot_base : cr_bot_base;

        if (tbSeparateMap & tb)
            base_addr = bot_base_addr;
        else
            base_addr = top_base_addr;

        if (ycbcr == 0)
        {
            mbx = posX/16;
            mby = posY/16;
        }
        else
        { //always interleave
            mbx = posX/16;
            mby = posY/8;
        }

        mb_addr = mbx_num * mby + mbx;

        // axi_conv[7:0]
        axi_conv = 0;
        for (i=0 ; i<8; i++)
        {
            xy2axiLumMap = pMapCfg->xy2axiLumMap[i];
            xy2axiChrMap = pMapCfg->xy2axiChrMap[i];
            xy2axi_map_sel = (chr_flag) ? xy2axiChrMap : xy2axiLumMap;
            temp_bit = GetXY2AXILogic(xy2axi_map_sel,posX,ypos_mod,tb);
            axi_conv = axi_conv + (temp_bit << i);
        }

        if (mapType == TILED_FRAME_MB_RASTER_MAP)
        {
            if (chr_flag == 0)
                tmp_addr = (mb_addr << 8) + axi_conv;
            else // chroma, interleaved only
                tmp_addr = (mb_addr << 7) + axi_conv;
        }
        else
        { // TILED_FIELD_MB_RASTER_MAP
            if (chr_flag == 0)
                tmp_addr = (mb_addr << 7) + axi_conv;
            else // chroma, interleaved only
                tmp_addr = (mb_addr << 6) + axi_conv;
        }

        pix_addr = tmp_addr + base_addr;
    }
    else
    {

        top_base_addr = (ycbcr==0) ? y_top_base  : (ycbcr==2) ? cb_top_base : cr_top_base;
        bot_base_addr = (ycbcr==0) ? y_bot_base  : (ycbcr==2) ? cb_bot_base : cr_bot_base;
        if (tbSeparateMap & tb)
            base_addr = bot_base_addr;
        else
            base_addr = top_base_addr;

        // axi_conv[31:0]
        axi_conv = 0;
        for (i=0 ; i<32; i++)
        {
            xy2axiLumMap = pMapCfg->xy2axiLumMap[i];
            xy2axiChrMap = pMapCfg->xy2axiChrMap[i];
            xy2axi_map_sel = (chr_flag) ? xy2axiChrMap : xy2axiLumMap;
            temp_bit = GetXY2AXILogic(xy2axi_map_sel,posX,ypos_mod,tb);
            axi_conv = axi_conv + (temp_bit << i);
        }

        pix_addr = axi_conv + base_addr;
    }

    return pix_addr;
}

static int GetXY2RBCLogic(int map_val,int xpos,int ypos, int tb)
{
    int invert;
    int assign_zero;
    int tbxor;
    int xysel;
    int bitsel;

    int xypos,xybit,xybit_st1,xybit_st2,xybit_st3;

    invert        = map_val >> 7;
    assign_zero = (map_val & 0x78) >> 6;
    tbxor        = (map_val & 0x3C) >> 5;
    xysel        = (map_val & 0x1E) >> 4;
    bitsel        = map_val & 0x0f;

    xypos     = (xysel) ? ypos : xpos;
    xybit     = (xypos >> bitsel) & 0x01;
    xybit_st1 = (tbxor)       ? xybit^tb : xybit;
    xybit_st2 = (assign_zero) ? 0 : xybit_st1;
    xybit_st3 = (invert)      ? !xybit_st2 : xybit_st2;

    return xybit_st3;
}

static int GetRBC2AXILogic(int map_val , int ra_in, int ba_in, int ca_in)
{
    int rbc;
    int rst_bit ;
    int rbc_sel = map_val >> 4;
    int bit_sel = map_val & 0x0f;


    if (rbc_sel == 0)
        rbc = ca_in;
    else if (rbc_sel == 1)
        rbc = ba_in;
    else if (rbc_sel == 2)
        rbc = ra_in;
    else
        rbc = 0;

    rst_bit = ((rbc >> bit_sel) & 1);

    return rst_bit;
}


static int GetXY2AXIAddrV10(TiledMapConfig *pMapCfg, int ycbcr, int posY, int posX, int stride, FrameBuffer *fb)
{
    int ypos_mod;
    int temp;
    int temp_bit;
    int i;
    int tb;
    int ra_base;
    int ras_base;
    int ra_conv,ba_conv,ca_conv;

    int pix_addr;

    int lum_top_base,chr_top_base;
    int lum_bot_base,chr_bot_base;

    int mbx,mby,mb_addr;
    int temp_val12bit, temp_val6bit;
    int Addr;
    int mb_raster_base;

    if (!pMapCfg)
        return -1;

    pix_addr       = 0;
    mb_raster_base = 0;
    ra_conv        = 0;
    ba_conv        = 0;
    ca_conv        = 0;

    tb = posY & 0x1;

    ypos_mod =  pMapCfg->tbSeparateMap ? posY >> 1 : posY;

    Addr = ycbcr == 0 ? fb->bufY  :
        ycbcr == 2 ? fb->bufCb : fb->bufCr;

    if (fb->mapType == LINEAR_FRAME_MAP)
        return ((posY * stride) + posX) + Addr;

    // 20bit = AddrY [31:12]
    lum_top_base =   fb->bufY >> 12;

    // 20bit = AddrY [11: 0], AddrCb[31:24]
    chr_top_base = ((fb->bufY  & 0xfff) << 8) | ((fb->bufCb >> 24) & 0xff);  //12bit +  (32-24) bit

    // 20bit = AddrCb[23: 4]
    lum_bot_base =  (fb->bufCb >> 4) & 0xfffff;

    // 20bit = AddrCb[ 3: 0], AddrCr[31:16]
    chr_bot_base =  ((fb->bufCb & 0xf) << 16) | ((fb->bufCr >> 16) & 0xffff);


    if (fb->mapType == TILED_FRAME_MB_RASTER_MAP || fb->mapType == TILED_FIELD_MB_RASTER_MAP)
    {
        if (ycbcr == 0)
        {
            mbx = posX/16;
            mby = posY/16;
        }
        else //always interleave
        {
            mbx = posX/16;
            mby = posY/8;
        }

        mb_addr = (stride/16) * mby + mbx;

        // ca[7:0]
        for (i=0 ; i<8; i++)
        {
            if (ycbcr == 2 || ycbcr == 3)
                temp = pMapCfg->xy2caMap[i] & 0xff;
            else
                temp = pMapCfg->xy2caMap[i] >> 8;
            temp_bit = GetXY2RBCLogic(temp,posX,ypos_mod,tb);
            ca_conv  = ca_conv + (temp_bit << i);
        }

        // ca[15:8]
        ca_conv      = ca_conv + ((mb_addr & 0xff) << 8);

        // ra[15:0]
        ra_conv      = mb_addr >> 8;


        // ra,ba,ca -> axi
        for (i=0; i<32; i++) {

            temp_val12bit = pMapCfg->rbc2axiMap[i];
            temp_val6bit  = (ycbcr == 0 ) ? (temp_val12bit >> 6) : (temp_val12bit & 0x3f);

            temp_bit = GetRBC2AXILogic(temp_val6bit,ra_conv,ba_conv,ca_conv);

            pix_addr =  pix_addr + (temp_bit<<i);
        }

        if (pMapCfg->tbSeparateMap ==1 && tb ==1)
            mb_raster_base = ycbcr == 0 ? lum_bot_base : chr_bot_base;
        else
            mb_raster_base = ycbcr == 0 ? lum_top_base : chr_top_base;

        pix_addr = pix_addr + (mb_raster_base << 12);
    }
    else
    {
        // ca
        for (i=0 ; i<16; i++)
        {
            if (ycbcr == 0 || ycbcr == 1)  // clair : there are no case ycbcr = 1
                temp = pMapCfg->xy2caMap[i] >> 8;
            else
                temp = pMapCfg->xy2caMap[i] & 0xff;

            temp_bit = GetXY2RBCLogic(temp,posX,ypos_mod,tb);
            ca_conv  = ca_conv + (temp_bit << i);
        }

        // ba
        for (i=0 ; i<4; i++)
        {
            if (ycbcr == 2 || ycbcr == 3)
                temp = pMapCfg->xy2baMap[i] & 0xff;
            else
                temp = pMapCfg->xy2baMap[i] >> 8;

            temp_bit = GetXY2RBCLogic(temp,posX,ypos_mod,tb);
            ba_conv  = ba_conv + (temp_bit << i);
        }

        // ras
        for (i=0 ; i<16; i++)
        {
            if (ycbcr == 2 || ycbcr == 3)
                temp = pMapCfg->xy2raMap[i] & 0xff;
            else
                temp = pMapCfg->xy2raMap[i] >> 8;

            temp_bit = GetXY2RBCLogic(temp,posX,ypos_mod,tb);
            ra_conv  = ra_conv + (temp_bit << i);
        }

        if (pMapCfg->tbSeparateMap == 1 && tb == 1)
            ras_base = Addr >> 16;
        else
            ras_base = Addr & 0xffff;

        ra_base  = ra_conv + ras_base;
        pix_addr = 0;

        // ra,ba,ca -> axi
        for (i=0; i<32; i++) {

            temp_val12bit = pMapCfg->rbc2axiMap[i];
            temp_val6bit  = (ycbcr == 0 ) ? (temp_val12bit >> 6) : (temp_val12bit & 0x3f);

            temp_bit = GetRBC2AXILogic(temp_val6bit,ra_base,ba_conv,ca_conv);

            pix_addr = pix_addr + (temp_bit<<i);
        }
        pix_addr += pMapCfg->tiledBaseAddr;
    }

    return pix_addr;
}

Int32 CalcStride(
    Uint32              width,
    Uint32              height,
    FrameBufferFormat   format,
    BOOL                cbcrInterleave,
    TiledMapType        mapType,
    BOOL                isVP9
    )
{
    Uint32  lumaStride   = 0;
    Uint32  chromaStride = 0;

    lumaStride = VPU_ALIGN32(width);

    if (height > width) {
        if ((mapType >= TILED_FRAME_V_MAP && mapType <= TILED_MIXED_V_MAP) ||
            mapType == TILED_FRAME_NO_BANK_MAP ||
            mapType == TILED_FIELD_NO_BANK_MAP)
            width = VPU_ALIGN16(height);	// TiledMap constraints
    }

    if (mapType == LINEAR_FRAME_MAP || mapType == LINEAR_FIELD_MAP) {
        Uint32 twice = 0;

        twice = cbcrInterleave == TRUE ? 2 : 1;
        switch (format) {
        case FORMAT_420:
            /* nothing to do */
            break;
        case FORMAT_420_P10_16BIT_LSB:
        case FORMAT_420_P10_16BIT_MSB:
        case FORMAT_422_P10_16BIT_MSB:
        case FORMAT_422_P10_16BIT_LSB:
            lumaStride = VPU_ALIGN32(width)*2;
            break;
        case FORMAT_420_P10_32BIT_LSB:
        case FORMAT_420_P10_32BIT_MSB:
        case FORMAT_422_P10_32BIT_MSB:
        case FORMAT_422_P10_32BIT_LSB:
            if ( isVP9 == TRUE ) {
                lumaStride   = VPU_ALIGN32(((width+11)/12)*16);
                chromaStride = (((width/2)+11)*twice/12)*16;
            }
            else {
                width = VPU_ALIGN32(width);
                lumaStride   = ((VPU_ALIGN16(width)+11)/12)*16;
                chromaStride = ((VPU_ALIGN16(width/2)+11)*twice/12)*16;
            //if ( isWAVE410 == TRUE ) {
                if ( (chromaStride*2) > lumaStride)
                {
                    lumaStride = chromaStride * 2;
                    VLOG(ERR, "double chromaStride size is bigger than lumaStride\n");
                }
            //}
            }
            if (cbcrInterleave == TRUE) {
                lumaStride = MAX(lumaStride, chromaStride);
            }
            break;
        case FORMAT_422:
            /* nothing to do */
            break;
        case FORMAT_YUYV:       // 4:2:2 8bit packed
        case FORMAT_YVYU:
        case FORMAT_UYVY:
        case FORMAT_VYUY:
            lumaStride = VPU_ALIGN32(width) * 2;
            break;
        case FORMAT_YUYV_P10_16BIT_MSB:   // 4:2:2 10bit packed
        case FORMAT_YUYV_P10_16BIT_LSB:
        case FORMAT_YVYU_P10_16BIT_MSB:
        case FORMAT_YVYU_P10_16BIT_LSB:
        case FORMAT_UYVY_P10_16BIT_MSB:
        case FORMAT_UYVY_P10_16BIT_LSB:
        case FORMAT_VYUY_P10_16BIT_MSB:
        case FORMAT_VYUY_P10_16BIT_LSB:
            lumaStride = VPU_ALIGN32(width) * 4;
            break;
        case FORMAT_YUYV_P10_32BIT_MSB:
        case FORMAT_YUYV_P10_32BIT_LSB:
        case FORMAT_YVYU_P10_32BIT_MSB:
        case FORMAT_YVYU_P10_32BIT_LSB:
        case FORMAT_UYVY_P10_32BIT_MSB:
        case FORMAT_UYVY_P10_32BIT_LSB:
        case FORMAT_VYUY_P10_32BIT_MSB:
        case FORMAT_VYUY_P10_32BIT_LSB:
            lumaStride = VPU_ALIGN32(width*2)*2;
            break;
        default:
            break;
        }
    }
    else if (mapType == COMPRESSED_FRAME_MAP) {
        switch (format) {
        case FORMAT_420:
        case FORMAT_422:
        case FORMAT_YUYV:
        case FORMAT_YVYU:
        case FORMAT_UYVY:
        case FORMAT_VYUY:
            break;
        case FORMAT_420_P10_16BIT_LSB:
        case FORMAT_420_P10_16BIT_MSB:
        case FORMAT_420_P10_32BIT_LSB:
        case FORMAT_420_P10_32BIT_MSB:
        case FORMAT_422_P10_16BIT_MSB:
        case FORMAT_422_P10_16BIT_LSB:
        case FORMAT_422_P10_32BIT_MSB:
        case FORMAT_422_P10_32BIT_LSB:
        case FORMAT_YUYV_P10_16BIT_MSB:
        case FORMAT_YUYV_P10_16BIT_LSB:
        case FORMAT_YVYU_P10_16BIT_MSB:
        case FORMAT_YVYU_P10_16BIT_LSB:
        case FORMAT_YVYU_P10_32BIT_MSB:
        case FORMAT_YVYU_P10_32BIT_LSB:
        case FORMAT_UYVY_P10_16BIT_MSB:
        case FORMAT_UYVY_P10_16BIT_LSB:
        case FORMAT_VYUY_P10_16BIT_MSB:
        case FORMAT_VYUY_P10_16BIT_LSB:
        case FORMAT_YUYV_P10_32BIT_MSB:
        case FORMAT_YUYV_P10_32BIT_LSB:
        case FORMAT_UYVY_P10_32BIT_MSB:
        case FORMAT_UYVY_P10_32BIT_LSB:
        case FORMAT_VYUY_P10_32BIT_MSB:
        case FORMAT_VYUY_P10_32BIT_LSB:
            lumaStride = VPU_ALIGN32(VPU_ALIGN16(width)*5)/4;
            lumaStride = VPU_ALIGN32(lumaStride);
            break;
        default:
            return -1;
        }
    }
    else if (mapType == TILED_FRAME_NO_BANK_MAP || mapType == TILED_FIELD_NO_BANK_MAP) {
        lumaStride = (width > 4096) ? 8192 :
                     (width > 2048) ? 4096 :
                     (width > 1024) ? 2048 :
                     (width >  512) ? 1024 : 512;
    }
    else if (mapType == TILED_FRAME_MB_RASTER_MAP || mapType == TILED_FIELD_MB_RASTER_MAP) {
        lumaStride = VPU_ALIGN32(width);
    }
    else if (mapType == TILED_SUB_CTU_MAP) {
        lumaStride = VPU_ALIGN32(width);
    }
    else {
        width = (width < height) ? height : width;

        lumaStride = (width > 4096) ? 8192 :
                     (width > 2048) ? 4096 :
                     (width > 1024) ? 2048 :
                     (width >  512) ? 1024 : 512;
    }

    return lumaStride;
}

int LevelCalculation(int MbNumX, int MbNumY, int frameRateInfo, int interlaceFlag, int BitRate, int SliceNum)
{
    int mbps;
    int frameRateDiv, frameRateRes, frameRate;
    int mbPicNum = (MbNumX*MbNumY);
    int mbFrmNum;
    int MaxSliceNum;

    int LevelIdc = 0;
    int i, maxMbs;

    if (interlaceFlag)    {
        mbFrmNum = mbPicNum*2;
        MbNumY   *=2;
    }
    else                mbFrmNum = mbPicNum;

    frameRateDiv = (frameRateInfo >> 16) + 1;
    frameRateRes  = frameRateInfo & 0xFFFF;
    frameRate = math_div(frameRateRes, frameRateDiv);
    mbps = mbFrmNum*frameRate;

    for (i=0; i<MAX_LAVEL_IDX; i++)
    {
        maxMbs = g_anLevelMaxMbs[i];
        if ( mbps <= g_anLevelMaxMBPS[i]
        && mbFrmNum <= g_anLevelMaxFS[i]
        && MbNumX   <= maxMbs
            && MbNumY   <= maxMbs
            && BitRate  <= g_anLevelMaxBR[i] )
        {
            LevelIdc = g_anLevel[i];
            break;
        }
    }

    if (i == MAX_LAVEL_IDX)
        i = MAX_LAVEL_IDX-1;

    if (SliceNum)
    {
        SliceNum =  math_div(mbPicNum, SliceNum);

    if (g_anLevelSliceRate[i])
    {
        MaxSliceNum = math_div( MAX( mbPicNum, g_anLevelMaxMBPS[i]/( 172/( 1+interlaceFlag ) )), g_anLevelSliceRate[i] );

        if ( SliceNum> MaxSliceNum)
            return -1;
    }
    }

    return LevelIdc;
}

Int32 CalcLumaSize(
    Int32           productId,
    Int32           stride,
    Int32           height,
    FrameBufferFormat format,
    BOOL            cbcrIntl,
    TiledMapType    mapType,
    DRAMConfig      *pDramCfg
    )
{
    Int32 unit_size_hor_lum, unit_size_ver_lum, size_dpb_lum, field_map, size_dpb_lum_4k;
    UNREFERENCED_PARAMETER(cbcrIntl);

    if (mapType == TILED_FIELD_V_MAP || mapType == TILED_FIELD_NO_BANK_MAP || mapType == LINEAR_FIELD_MAP) {
        field_map = 1;
    }
    else {
        field_map = 0;
    }

    if (mapType == LINEAR_FRAME_MAP || mapType == LINEAR_FIELD_MAP) {
        size_dpb_lum = stride * height;
    }
    else if (mapType == COMPRESSED_FRAME_MAP) {
        size_dpb_lum = stride * height;
    }
    else if (mapType == TILED_FRAME_NO_BANK_MAP || mapType == TILED_FIELD_NO_BANK_MAP) {
        unit_size_hor_lum = stride;
        unit_size_ver_lum = (((height>>field_map)+127)/128) * 128; // unit vertical size is 128 pixel (8MB)
        size_dpb_lum      = unit_size_hor_lum * (unit_size_ver_lum<<field_map);
    }
    else if (mapType == TILED_FRAME_MB_RASTER_MAP || mapType == TILED_FIELD_MB_RASTER_MAP) {
        if (productId == PRODUCT_ID_960) {
            size_dpb_lum   = stride * height;

            // aligned to 8192*2 (0x4000) for top/bot field
            // use upper 20bit address only
            size_dpb_lum_4k =  ((size_dpb_lum + 16383)/16384)*16384;

            if (mapType == TILED_FIELD_MB_RASTER_MAP) {
                size_dpb_lum_4k = ((size_dpb_lum_4k+(0x8000-1))&~(0x8000-1));
            }

            size_dpb_lum = size_dpb_lum_4k;
        }
        else {
            size_dpb_lum    = stride * (height>>field_map);
            size_dpb_lum_4k = ((size_dpb_lum+(16384-1))/16384)*16384;
            size_dpb_lum    = size_dpb_lum_4k<<field_map;
        }
    }
    else if (mapType == TILED_SUB_CTU_MAP) {
        size_dpb_lum    = VPU_ALIGN32(stride) * VPU_ALIGN32(height);    // under sub_ctu_map mode, stride is the same as width.
    }
    else {
        if (productId == PRODUCT_ID_960) {
            Int32    VerSizePerRas,Ras1DBit;
            Int32    ChrSizeYField;
            Int32    ChrFieldRasSize,ChrFrameRasSize,LumFieldRasSize,LumFrameRasSize;

            ChrSizeYField = ((height/2)+1)>>1;

            if (mapType == TILED_FRAME_V_MAP) {
                if (pDramCfg->casBit == 9 && pDramCfg->bankBit == 2 && pDramCfg->rasBit == 13) {	// CNN setting
                    VerSizePerRas = 64;
                    Ras1DBit = 3;
                }
                else if(pDramCfg->casBit == 10 && pDramCfg->bankBit == 3 && pDramCfg->rasBit == 13) {
                    VerSizePerRas = 64;
                    Ras1DBit = 2;
                }
                else
                    return 0;

            }
            else if (mapType == TILED_FRAME_H_MAP) {
                if (pDramCfg->casBit == 9 && pDramCfg->bankBit == 2 && pDramCfg->rasBit == 13) {	// CNN setting
                    VerSizePerRas = 64;
                    Ras1DBit = 3;
                }
                else if(pDramCfg->casBit == 10 && pDramCfg->bankBit == 3 && pDramCfg->rasBit == 13) {
                    VerSizePerRas = 64;
                    Ras1DBit = 2;
                }
                else
                    return 0;

            }
            else if (mapType == TILED_FIELD_V_MAP) {
                if (pDramCfg->casBit == 9 && pDramCfg->bankBit == 2 && pDramCfg->rasBit == 13) {	// CNN setting
                    VerSizePerRas = 64;
                    Ras1DBit = 3;
                }
                else if(pDramCfg->casBit == 10 && pDramCfg->bankBit == 3 && pDramCfg->rasBit == 13) {
                    VerSizePerRas = 64;
                    Ras1DBit = 2;
                }
                else
                    return 0;
            }
            else {         // TILED_FIELD_H_MAP
                if (pDramCfg->casBit == 9 && pDramCfg->bankBit == 2 && pDramCfg->rasBit == 13) {	// CNN setting
                    VerSizePerRas = 64;
                    Ras1DBit = 3;
                }
                else if(pDramCfg->casBit == 10 && pDramCfg->bankBit == 3 && pDramCfg->rasBit == 13) {
                    VerSizePerRas = 64;
                    Ras1DBit = 2;
                }
                else
                    return 0;
            }
            ChrFieldRasSize = ((ChrSizeYField + (VerSizePerRas-1))/VerSizePerRas) << Ras1DBit;
            ChrFrameRasSize = ChrFieldRasSize *2;
            LumFieldRasSize = ChrFrameRasSize;
            LumFrameRasSize = LumFieldRasSize *2;
            size_dpb_lum    = LumFrameRasSize << (pDramCfg->bankBit+pDramCfg->casBit+pDramCfg->busBit);
        }
        else {  // productId != 960
            unit_size_hor_lum = stride;
            unit_size_ver_lum = (((height>>field_map)+63)/64) * 64; // unit vertical size is 64 pixel (4MB)
            size_dpb_lum      = unit_size_hor_lum * (unit_size_ver_lum<<field_map);
        }
    }

    return size_dpb_lum;
}

Int32 CalcChromaSize(
    Int32               productId,
    Int32               stride,
    Int32               height,
    FrameBufferFormat   format,
    BOOL                cbcrIntl,
    TiledMapType        mapType,
    DRAMConfig*         pDramCfg
    )
{
    Int32 chr_size_y, chr_size_x;
    Int32 chr_vscale, chr_hscale;
    Int32 unit_size_hor_chr, unit_size_ver_chr;
    Int32 size_dpb_chr, size_dpb_chr_4k;
    Int32 field_map;

    unit_size_hor_chr = 0;
    unit_size_ver_chr = 0;

    chr_hscale = 1;
    chr_vscale = 1;

    switch (format) {
    case FORMAT_420_P10_16BIT_LSB:
    case FORMAT_420_P10_16BIT_MSB:
    case FORMAT_420_P10_32BIT_LSB:
    case FORMAT_420_P10_32BIT_MSB:
    case FORMAT_420:
        chr_hscale = 2;
        chr_vscale = 2;
        break;
    case FORMAT_224:
        chr_vscale = 2;
        break;
    case FORMAT_422:
    case FORMAT_422_P10_16BIT_LSB:
    case FORMAT_422_P10_16BIT_MSB:
    case FORMAT_422_P10_32BIT_LSB:
    case FORMAT_422_P10_32BIT_MSB:
        chr_hscale = 2;
        break;
    case FORMAT_444:
    case FORMAT_400:
    case FORMAT_YUYV:
    case FORMAT_YVYU:
    case FORMAT_UYVY:
    case FORMAT_VYUY:
    case FORMAT_YUYV_P10_16BIT_MSB:   // 4:2:2 10bit packed
    case FORMAT_YUYV_P10_16BIT_LSB:
    case FORMAT_YVYU_P10_16BIT_MSB:
    case FORMAT_YVYU_P10_16BIT_LSB:
    case FORMAT_UYVY_P10_16BIT_MSB:
    case FORMAT_UYVY_P10_16BIT_LSB:
    case FORMAT_VYUY_P10_16BIT_MSB:
    case FORMAT_VYUY_P10_16BIT_LSB:
    case FORMAT_YUYV_P10_32BIT_MSB:
    case FORMAT_YUYV_P10_32BIT_LSB:
    case FORMAT_YVYU_P10_32BIT_MSB:
    case FORMAT_YVYU_P10_32BIT_LSB:
    case FORMAT_UYVY_P10_32BIT_MSB:
    case FORMAT_UYVY_P10_32BIT_LSB:
    case FORMAT_VYUY_P10_32BIT_MSB:
    case FORMAT_VYUY_P10_32BIT_LSB:
        break;
    default:
        return 0;
    }

    if (mapType == TILED_FIELD_V_MAP || mapType == TILED_FIELD_NO_BANK_MAP || mapType == LINEAR_FIELD_MAP) {
        field_map = 1;
    }
    else {
        field_map = 0;
    }

    if (mapType == LINEAR_FRAME_MAP || mapType == LINEAR_FIELD_MAP) {
        switch (format) {
        case FORMAT_420:
            unit_size_hor_chr = stride/2;
            unit_size_ver_chr = height/2;
            break;
        case FORMAT_420_P10_16BIT_LSB:
        case FORMAT_420_P10_16BIT_MSB:
            unit_size_hor_chr = stride/2;
            unit_size_ver_chr = height/2;
            break;
        case FORMAT_420_P10_32BIT_LSB:
        case FORMAT_420_P10_32BIT_MSB:
            unit_size_hor_chr = VPU_ALIGN16(stride/2);
            unit_size_ver_chr = height/2;
            break;
        case FORMAT_422:
        case FORMAT_422_P10_16BIT_LSB:
        case FORMAT_422_P10_16BIT_MSB:
        case FORMAT_422_P10_32BIT_LSB:
        case FORMAT_422_P10_32BIT_MSB:
            unit_size_hor_chr = VPU_ALIGN32(stride/2);
            unit_size_ver_chr = height;
            break;
        case FORMAT_YUYV:
        case FORMAT_YVYU:
        case FORMAT_UYVY:
        case FORMAT_VYUY:
        case FORMAT_YUYV_P10_16BIT_MSB:   // 4:2:2 10bit packed
        case FORMAT_YUYV_P10_16BIT_LSB:
        case FORMAT_YVYU_P10_16BIT_MSB:
        case FORMAT_YVYU_P10_16BIT_LSB:
        case FORMAT_UYVY_P10_16BIT_MSB:
        case FORMAT_UYVY_P10_16BIT_LSB:
        case FORMAT_VYUY_P10_16BIT_MSB:
        case FORMAT_VYUY_P10_16BIT_LSB:
        case FORMAT_YUYV_P10_32BIT_MSB:
        case FORMAT_YUYV_P10_32BIT_LSB:
        case FORMAT_YVYU_P10_32BIT_MSB:
        case FORMAT_YVYU_P10_32BIT_LSB:
        case FORMAT_UYVY_P10_32BIT_MSB:
        case FORMAT_UYVY_P10_32BIT_LSB:
        case FORMAT_VYUY_P10_32BIT_MSB:
        case FORMAT_VYUY_P10_32BIT_LSB:
            unit_size_hor_chr = 0;
            unit_size_ver_chr = 0;
            break;
        default:
            break;
        }
        size_dpb_chr = (format == FORMAT_400) ? 0 : unit_size_ver_chr * unit_size_hor_chr;
    }
    else if (mapType == COMPRESSED_FRAME_MAP) {
        switch (format) {
        case FORMAT_420:
        case FORMAT_YUYV:       // 4:2:2 8bit packed
        case FORMAT_YVYU:
        case FORMAT_UYVY:
        case FORMAT_VYUY:
            size_dpb_chr = VPU_ALIGN16(stride/2)*height;
            break;
        default:
            /* 10bit */
            stride = VPU_ALIGN64(stride/2)+12; /* FIXME: need width information */
            size_dpb_chr = VPU_ALIGN32(stride)*VPU_ALIGN4(height);
            break;
        }
        size_dpb_chr = size_dpb_chr / 2;
    }
    else if (mapType == TILED_FRAME_NO_BANK_MAP || mapType == TILED_FIELD_NO_BANK_MAP) {
        chr_size_y = (height>>field_map)/chr_hscale;
        chr_size_x = stride/chr_vscale;

        unit_size_hor_chr = (chr_size_x > 4096) ? 8192:
                            (chr_size_x > 2048) ? 4096 :
                            (chr_size_x > 1024) ? 2048 :
                            (chr_size_x >  512) ? 1024 : 512;
        unit_size_ver_chr = ((chr_size_y+127)/128) * 128; // unit vertical size is 128 pixel (8MB)

        size_dpb_chr = (format==FORMAT_400) ? 0 : (unit_size_hor_chr * (unit_size_ver_chr<<field_map));
    }
    else if (mapType == TILED_FRAME_MB_RASTER_MAP || mapType == TILED_FIELD_MB_RASTER_MAP) {
        if (productId == PRODUCT_ID_960) {
            chr_size_x = stride/chr_hscale;
            chr_size_y = height/chr_hscale;
            size_dpb_chr   = chr_size_y * chr_size_x;

            // aligned to 8192*2 (0x4000) for top/bot field
            // use upper 20bit address only
            size_dpb_chr_4k	=  ((size_dpb_chr + 16383)/16384)*16384;

            if (mapType == TILED_FIELD_MB_RASTER_MAP) {
                size_dpb_chr_4k = ((size_dpb_chr_4k+(0x8000-1))&~(0x8000-1));
            }

            size_dpb_chr = size_dpb_chr_4k;
        }
        else {
            size_dpb_chr    = (format==FORMAT_400) ? 0 : ((stride * (height>>field_map))/(chr_hscale*chr_vscale));
            size_dpb_chr_4k = ((size_dpb_chr+(16384-1))/16384)*16384;
            size_dpb_chr    = size_dpb_chr_4k<<field_map;
        }
    }
    else if (mapType == TILED_SUB_CTU_MAP) {
        size_dpb_chr    = VPU_ALIGN16(stride/2) * VPU_ALIGN32(height) / 2;  // under sub_ctu_map mode, stride is the same as width.
    }
    else {
        if (productId == PRODUCT_ID_960) {
            int  VerSizePerRas,Ras1DBit;
            int  ChrSizeYField;
            int  divY;
            int  ChrFieldRasSize,ChrFrameRasSize;

            divY = format == FORMAT_420 || format == FORMAT_224 ? 2 : 1;

            ChrSizeYField = ((height/divY)+1)>>1;

            if (mapType == TILED_FRAME_V_MAP) {
                if (pDramCfg->casBit == 9 && pDramCfg->bankBit == 2 && pDramCfg->rasBit == 13) {	// CNN setting
                    VerSizePerRas = 64;
                    Ras1DBit = 3;
                }
                else if(pDramCfg->casBit == 10 && pDramCfg->bankBit == 3 && pDramCfg->rasBit == 13) {
                    VerSizePerRas = 64;
                    Ras1DBit = 2;
                }
                else
                    return 0;

            }
            else if (mapType == TILED_FRAME_H_MAP) {
                if (pDramCfg->casBit == 9 && pDramCfg->bankBit == 2 && pDramCfg->rasBit == 13) {	// CNN setting
                    VerSizePerRas = 64;
                    Ras1DBit = 3;
                }
                else if(pDramCfg->casBit == 10 && pDramCfg->bankBit == 3 && pDramCfg->rasBit == 13) {
                    VerSizePerRas = 64;
                    Ras1DBit = 2;
                }
                else
                    return 0;

            }
            else if (mapType == TILED_FIELD_V_MAP) {
                if (pDramCfg->casBit == 9 && pDramCfg->bankBit == 2 && pDramCfg->rasBit == 13) {	// CNN setting
                    VerSizePerRas = 64;
                    Ras1DBit = 3;
                }
                else if(pDramCfg->casBit == 10 && pDramCfg->bankBit == 3 && pDramCfg->rasBit == 13) {
                    VerSizePerRas = 64;
                    Ras1DBit = 2;
                }
                else
                    return 0;
            } else {         // TILED_FIELD_H_MAP
                if (pDramCfg->casBit == 9 && pDramCfg->bankBit == 2 && pDramCfg->rasBit == 13) {	// CNN setting
                    VerSizePerRas = 64;
                    Ras1DBit = 3;
                }
                else if(pDramCfg->casBit == 10 && pDramCfg->bankBit == 3 && pDramCfg->rasBit == 13) {
                    VerSizePerRas = 64;
                    Ras1DBit = 2;
                }
                else
                    return 0;
            }
            ChrFieldRasSize = ((ChrSizeYField + (VerSizePerRas-1))/VerSizePerRas) << Ras1DBit;
            ChrFrameRasSize = ChrFieldRasSize *2;
            size_dpb_chr = (ChrFrameRasSize << (pDramCfg->bankBit+pDramCfg->casBit+pDramCfg->busBit)) / 2;      // divide 2  = to calucate one Cb(or Cr) size;

        }
        else {  // productId != 960
            chr_size_y = (height>>field_map)/chr_hscale;
            chr_size_x = cbcrIntl == TRUE ? stride : stride/chr_vscale;

            unit_size_hor_chr = (chr_size_x> 4096) ? 8192:
                                (chr_size_x> 2048) ? 4096 :
                                (chr_size_x > 1024) ? 2048 :
                                (chr_size_x >  512) ? 1024 : 512;
            unit_size_ver_chr = ((chr_size_y+63)/64) * 64; // unit vertical size is 64 pixel (4MB)

            size_dpb_chr  = (format==FORMAT_400) ? 0 : unit_size_hor_chr * (unit_size_ver_chr<<field_map);
            size_dpb_chr /= (cbcrIntl == TRUE ? 2 : 1);
        }

    }
    return size_dpb_chr;
}


