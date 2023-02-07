#include <stdio.h>
#include <stdlib.h>

#ifdef MAKEANDROID
#include <utils/Log.h>
#define LOGCAT
#endif
#include "vp_hevc_multi_codec_1_0.h"
#include "include/AML_HEVCEncoder.h"
#include "include/enc_define.h"
#include "./vpuapi/include/vdi_osal.h"

const char version[] = "Amlogic libvp_hevc_multi_codec version 1.0";

const char *vl_get_version() {
    return version;
}

typedef struct AMVHEVCEncHandle_s {
    AMVHEVCEncParams mEncParams;

    AMVEncBufferType bufType;
    bool mPrependSPSPPSToIDRFrames;
    bool mSpsPpsHeaderReceived;
    bool mKeyFrameRequested;
    bool mSkipFrameRequested;
    int mLongTermRefRequestFlags;
    int mNumInputFrames;
    AMVEncFrameFmt fmt;
    int mSPSPPSDataSize;
    char *mSPSPPSData;
    amv_enc_handle_hevc_t am_enc_handle;
    int shared_fd[3];
    uint32 mNumPlanes;
#if 0
    uint32 magic_num;
    uint32 instance_id;
    uint32 src_idx;
    uint32 enc_width;
    uint32 enc_height;
    uint32 frame_rate; /* frame rate */
    int32 idrPeriod; /* IDR period in number of frames */
    uint32 op_flag;

    uint32 src_num;
    uint32 fb_num;

    AMVEncFrameFmt fmt;

    AVCNalUnitType nal_unit_type;
    AMVEncBufferType bufType;
    int shared_fd[3];
    uint32 mNumPlanes;

    AMVHEVCEncParams mEncParams;
    amv_enc_handle_hevc_t am_enc_handle;
    EncHandle enchandle; /*VPU encoder handler */
    EncOpenParam encOpenParam; /* api open param */
    bool mSpsPpsHeaderReceived;
    int mSPSPPSDataSize;
    char *mSPSPPSData;
    int32_t mNumInputFrames;
    bool mKeyFrameRequested;
    bool mPrependSPSPPSToIDRFrames;
    uint32 mOutputBufferLen;
    Uint64                      startTimeout;
    Uint32                      frameIdx;
    EncParam                    encParam;
    Int32                       encodedSrcFrmIdxArr[ENC_SRC_BUF_NUM];
    Int32                       encMEMSrcFrmIdxArr[ENC_SRC_BUF_NUM];
    EncInitialInfo              initialInfo;
    vpu_buffer_t                vbVuiRbsp;
    vpu_buffer_t                vbHrdRbsp;
    vpu_buffer_t                bsBuffer[ENC_SRC_BUF_NUM];

    Uint32                      reconFbStride;
    Uint32                      reconFbHeight;
    Uint32                      srcFbStride;
    Uint32                      srcFbSize;
    FrameBufferAllocInfo        srcFbAllocInfo;
    BOOL                        fbAllocated;
    FrameBuffer                 pFbRecon[MAX_REG_FRAME];
    vpu_buffer_t                pFbReconMem[MAX_REG_FRAME];
    FrameBuffer                 pFbSrc[ENC_SRC_BUF_NUM];
    vpu_buffer_t                pFbSrcMem[ENC_SRC_BUF_NUM];
    vpu_buffer_t                vbCustomMap[MAX_REG_FRAME];
#endif
} AMVHEVCEncHandle;


AMVEnc_Status initEncParams(AMVHEVCEncHandle *handle,
                        vl_encode_info_hevc_t encode_info,
                        qp_param_hevc_t* qp_tbl) {
    int width = encode_info.width;
    int height = encode_info.height;
    memset(&(handle->mEncParams), 0, sizeof(AMVHEVCEncParams));
    VLOG(DEBUG, "bit_rate:%d", encode_info.bit_rate);
    if ((width % 16 != 0 || height % 2 != 0)) {
        VLOG(DEBUG, "Video frame size %dx%d must be a multiple of 16", width, height);
        //return -1;
    } else if (height % 16 != 0) {
        VLOG(DEBUG, "Video frame height is not standard:%d", height);
    } else {
        VLOG(DEBUG, "Video frame size is %d x %d", width, height);
    }
    handle->mEncParams.rate_control = HEVC_ON;

    handle->mEncParams.initQP = 0;
    handle->mEncParams.initQP = qp_tbl->qp_base;
    handle->mEncParams.maxDeltaQP = qp_tbl->qp_max - qp_tbl->qp_min;
    handle->mEncParams.maxQP = qp_tbl->qp_max;
    handle->mEncParams.minQP = qp_tbl->qp_min;
    handle->mEncParams.qp_mode = encode_info.qp_mode;

    handle->mEncParams.init_CBP_removal_delay = 1600;
    handle->mEncParams.auto_scd = HEVC_OFF;
    handle->mEncParams.out_of_band_param_set = HEVC_OFF;
    handle->mEncParams.num_ref_frame = 1;
    handle->mEncParams.num_slice_group = 1;
    //handle->mEncParams.nSliceHeaderSpacing = 0;
    handle->mEncParams.fullsearch = HEVC_OFF;
    handle->mEncParams.search_range = 16;
    //handle->mEncParams.sub_pel = HEVC_OFF;
    //handle->mEncParams.submb_pred = HEVC_OFF;
    handle->mEncParams.width = width;
    handle->mEncParams.height = height;
    handle->mEncParams.src_width = encode_info.width;
    handle->mEncParams.src_height = encode_info.height;
    handle->mEncParams.bitrate = encode_info.bit_rate;
    handle->mEncParams.frame_rate = encode_info.frame_rate;
    handle->mEncParams.CPB_size = (uint32)(encode_info.bit_rate >> 1);
    handle->mEncParams.FreeRun = HEVC_OFF;

    if (encode_info.bitstream_buf_sz_kb > 0 && encode_info.bitstream_buf_sz_kb <= (32 * 1024)) {
        handle->mEncParams.es_buf_sz =  encode_info.bitstream_buf_sz_kb*1024;
    } else {
        if (encode_info.bitstream_buf_sz > 0 && encode_info.bitstream_buf_sz <= 32)
            handle->mEncParams.es_buf_sz =  encode_info.bitstream_buf_sz*1024*1024;
        else  //default value
            handle->mEncParams.es_buf_sz = 0; //out of range, use default
    }

    handle->mEncParams.MBsIntraRefresh = 0;
    handle->mEncParams.MBsIntraOverlap = 0;
    handle->mEncParams.encode_once = 1;

    if (encode_info.enc_feature_opts & ENABLE_PARA_UPDATE)
      handle->mEncParams.param_change_enable = 1;

    if (encode_info.img_format == IMG_FMT_NV12) {
      VLOG(INFO, "[%s %d] img_format is IMG_FMT_NV12 \n", __func__, __LINE__);
      handle->fmt = AMVENC_NV12;
    } else if (encode_info.img_format == IMG_FMT_NV21) {
      VLOG(INFO, "[%s %d] img_format is IMG_FMT_NV21 \n", __func__, __LINE__);
      handle->fmt = AMVENC_NV21;
    } else if (encode_info.img_format == IMG_FMT_YUV420P) {
      VLOG(INFO, "[%s %d] img_format is IMG_FMT_YUV420 \n", __func__, __LINE__);
      handle->fmt = AMVENC_YUV420;
    } else if (encode_info.img_format == IMG_FMT_RGB888) {
      handle->fmt = AMVENC_RGB888;
    } else if (encode_info.img_format == IMG_FMT_RGBA8888) {
      VLOG(INFO, "img_format is IMG_FMT_RGBA8888 \n");
      handle->fmt = AMVENC_RGBA8888;
    } else {
      VLOG(ERR, "img_format %d not supprot\n",encode_info.img_format);
      return AMVENC_FAIL;
    }

    handle->mEncParams.fmt = handle->fmt;
    if ((encode_info.img_format == IMG_FMT_RGB888) || (encode_info.img_format == IMG_FMT_RGBA8888)) {
      handle->mEncParams.fmt = AMVENC_NV21; //ge2d dst format PIXEL_FORMAT_YCrCb_420_SP
    }
    // Set IDR frame refresh interval
    if (encode_info.gop <= 0) {
        handle->mEncParams.idr_period = 0;   //an infinite period, only one I frame
    } else {
        handle->mEncParams.idr_period = encode_info.gop; //period of I frame, 1 means all frames are I type.
    }

    VLOG(DEBUG, "idrPeriod: %d, gop %d\n", handle->mEncParams.idr_period, encode_info.gop);
    // Set profile and level
    handle->mEncParams.profile = HEVC_MAIN;
    handle->mEncParams.level = HEVC_LEVEL_NONE; // firmware determines a level.
    handle->mEncParams.tier = HEVC_TIER_MAIN;
    handle->mEncParams.initQP = 30;
    handle->mEncParams.BitrateScale = HEVC_OFF;
    handle->mEncParams.refresh_type = HEVC_CRA;
    handle->mEncParams.vui_info_present = encode_info.vui_info_present;
    handle->mEncParams.video_signal_type = encode_info.video_signal_type;
    handle->mEncParams.color_description = encode_info.color_description;
    handle->mEncParams.primaries = encode_info.primaries;
    handle->mEncParams.transfer = encode_info.transfer;
    handle->mEncParams.matrix = encode_info.matrix;
    handle->mEncParams.range = encode_info.range;
    return AMVENC_SUCCESS;
}

bool check_qp_tbl(const qp_param_hevc_t* qp_tbl) {
  if (qp_tbl == NULL) {
    return false;
  }
  if (qp_tbl->qp_min < 0 || qp_tbl->qp_min > 51 || qp_tbl->qp_max < 0 ||
      qp_tbl->qp_max > 51) {
    VLOG(ERR,"qp_min or qp_max out of range [0, 51]\n");
    return false;
  }
  if (qp_tbl->qp_base < 0 || qp_tbl->qp_base > 51) {
    VLOG(ERR,"qp_base out of range [0, 51]\n");
    return false;
  }
  return true;
}

vl_codec_handle_hevc_t vl_video_encoder_init_hevc(vl_codec_id_hevc_t codec_id,
                                       vl_encode_info_hevc_t encode_info,
                                        qp_param_hevc_t* qp_tbl) {
    AMVEnc_Status ret;
    AMVHEVCEncHandle *mHandle = new AMVHEVCEncHandle;
    bool has_mix = false;
    (void)codec_id;

    if (mHandle == NULL)
        goto exit;
    memset(mHandle, 0, sizeof(AMVHEVCEncHandle));
    if (!check_qp_tbl(qp_tbl)) {
      goto exit;
    }

    vp4_set_log_level(ERR);

    ret = initEncParams(mHandle, encode_info, qp_tbl);
    if (ret < AMVENC_SUCCESS)
        goto exit;
    mHandle->am_enc_handle = AML_HEVCInitialize(&(mHandle->mEncParams));
    if (mHandle->am_enc_handle == 0)
        goto exit;

    mHandle->mPrependSPSPPSToIDRFrames =
                  encode_info.prepend_spspps_to_idr_frames;

    mHandle->mSpsPpsHeaderReceived = false;
    mHandle->mNumInputFrames = -1;  // 1st two buffers contain SPS and PPS

    return (vl_codec_handle_hevc_t) mHandle;
exit:
    if (mHandle != NULL)
        delete mHandle;
    return (vl_codec_handle_hevc_t) NULL;
}

encoding_metadata_hevc_t vl_video_encoder_generate_header(vl_codec_handle_hevc_t codec_handle,
                                                                   unsigned char *pHeader,
                                                                   unsigned int *pLength) {
   int ret;
   AMVHEVCEncHandle *handle = (AMVHEVCEncHandle *)codec_handle;
   encoding_metadata_hevc_t result;
   if (!handle->mSpsPpsHeaderReceived) {
        ret = AML_HEVCEncHeader(handle->am_enc_handle, (unsigned char *)pHeader, (unsigned int *)pLength);
        if (ret == AMVENC_SUCCESS) {
            handle->mSPSPPSDataSize = 0;
            handle->mSPSPPSData = (char *)malloc(*pLength);
            if (handle->mSPSPPSData) {
                handle->mSPSPPSDataSize = *pLength;
                memcpy(handle->mSPSPPSData, (unsigned char *)pHeader, handle->mSPSPPSDataSize);
                VLOG(DEBUG, "get mSPSPPSData size= %d at line %d \n", handle->mSPSPPSDataSize, __LINE__);
            }
            handle->mNumInputFrames = 0;
            handle->mSpsPpsHeaderReceived = true;
            result.is_valid = true;
            result.err_cod = AMVENC_SUCCESS;
        } else {
            *pLength = 0;
            result.is_valid = false;
            VLOG(ERR, "Encode SPS and PPS error, encoderStatus = %d. handle: %p\n", ret, (void *)handle);
            if (ret == AMVENC_FAIL)
               result.err_cod = AMVENC_INVALID_PARAM;
            else
                result.err_cod = ret;
            return result;
        }
    }
    return result;
}

encoding_metadata_hevc_t vl_video_encoder_encode_hevc(vl_codec_handle_hevc_t codec_handle,
                                    vl_frame_type_hevc_t frame_type,
                                    unsigned char* out,
                                    vl_buffer_info_hevc_t* in_buffer_info) {
    AMVEnc_Status ret;
    int i;
    uint32_t dataLength = 0;
    int type = 0;
    AMVHEVCEncHandle *handle = (AMVHEVCEncHandle *)codec_handle;
    encoding_metadata_hevc_t result;
    memset(&result, 0, sizeof(encoding_metadata_hevc_t));
    if (in_buffer_info == NULL) {
        VLOG(ERR, "invalid buffer_info\n");
        result.is_valid = false;
        result.err_cod = AMVENC_UNINITIALIZED;
        return result;
    }

    handle->bufType = (AMVEncBufferType)(in_buffer_info->buf_type);
    if (handle->bufType == DMA_BUFF) {
        if (in_buffer_info->buf_fmt == IMG_FMT_NV12 || in_buffer_info->buf_fmt == IMG_FMT_NV21 || in_buffer_info->buf_fmt == IMG_FMT_YUV420P) {
            if ((handle->mEncParams.width % 16 && in_buffer_info->buf_stride == 0) ||
                in_buffer_info->buf_stride % 16) {
               VLOG(ERR, "dma buffer stride must be multiple of 16!");
               result.is_valid = false;
               result.err_cod = AMVENC_ENCPARAM_MEM_FAIL;
               return result;
            }
        }
    }

    if (!handle->mSpsPpsHeaderReceived) {
        ret = AML_HEVCEncHeader(handle->am_enc_handle, (unsigned char *)out, (unsigned int *)&dataLength);
        if (ret == AMVENC_SUCCESS) {
            handle->mSPSPPSDataSize = 0;
            handle->mSPSPPSData = (char *)malloc(dataLength);
            if (handle->mSPSPPSData) {
                handle->mSPSPPSDataSize = dataLength;
                memcpy(handle->mSPSPPSData, (unsigned char *)out, handle->mSPSPPSDataSize);
                VLOG(DEBUG, "get mSPSPPSData size= %d at line %d \n", handle->mSPSPPSDataSize, __LINE__);
            }
            handle->mNumInputFrames = 0;
            handle->mSpsPpsHeaderReceived = true;
        } else {
            VLOG(ERR, "Encode SPS and PPS error, encoderStatus = %d. handle: %p\n", ret, (void *)handle);
            if (ret == AMVENC_FAIL)
               result.err_cod = AMVENC_INVALID_PARAM;
            else
                result.err_cod = ret;
            return result;
        }
    }

    if (frame_type == FRAME_TYPE_I || frame_type == FRAME_TYPE_IDR)
      handle->mKeyFrameRequested = true;

    if (handle->mNumInputFrames >= 0) {
        AMVHEVCEncFrameIO videoInput;
        memset(&videoInput, 0, sizeof(videoInput));
        videoInput.height = handle->mEncParams.height;
        if (!in_buffer_info->buf_stride)
            videoInput.pitch = handle->mEncParams.width; //((handle->mEncParams.width + 15) >> 4) << 4;
        else
            videoInput.pitch = in_buffer_info->buf_stride;
        /* TODO*/
        videoInput.bitrate = handle->mEncParams.bitrate;
        videoInput.frame_rate = handle->mEncParams.frame_rate / 1000;
        videoInput.coding_timestamp = handle->mNumInputFrames * 1000 / videoInput.frame_rate;  // in ms
        handle->shared_fd[0] = -1;
        handle->shared_fd[1] = -1;
        handle->shared_fd[2] = -1;

        videoInput.fmt = handle->fmt;

        VLOG(INFO, "in_buffer_info->buf_fmt=%d", in_buffer_info->buf_fmt);

        if (in_buffer_info->buf_fmt == IMG_FMT_NV12) {
          VLOG(INFO, "img_format is IMG_FMT_NV12 \n");
          videoInput.fmt = AMVENC_NV12;
        } else if (in_buffer_info->buf_fmt == IMG_FMT_NV21) {
          VLOG(INFO, "img_format is IMG_FMT_NV21 \n");
          videoInput.fmt = AMVENC_NV21;
        } else if (in_buffer_info->buf_fmt == IMG_FMT_YUV420P) {
          VLOG(INFO, "img_format is IMG_FMT_YUV420P \n");
          videoInput.fmt = AMVENC_YUV420;
        } else if (in_buffer_info->buf_fmt == IMG_FMT_RGB888) {
          VLOG(INFO, "img_format is IMG_FMT_RGB888 \n");
          videoInput.fmt = AMVENC_RGB888;
        }

        if (handle->bufType == DMA_BUFF) {
            vl_dma_info_hevc_t *dma_info;
            dma_info = &(in_buffer_info->buf_info.dma_info);
            if (videoInput.fmt == AMVENC_NV21 || videoInput.fmt == AMVENC_NV12) {
                if (dma_info->num_planes != 1
                    && dma_info->num_planes != 2) {
                    VLOG(ERR, "invalid num_planes %d\n", dma_info->num_planes);
                    result.is_valid = false;
                    result.err_cod = AMVENC_ENCPARAM_MEM_FAIL;

                    return result;
                }
            } else if (videoInput.fmt == AMVENC_YUV420) {
                if (dma_info->num_planes != 1
                    && dma_info->num_planes != 3) {
                    VLOG(ERR, "YV12 invalid num_planes %d\n", dma_info->num_planes);
                    result.is_valid = false;
                    result.err_cod = AMVENC_ENCPARAM_MEM_FAIL;

                    return result;
                }
            }

            handle->mNumPlanes = dma_info->num_planes;

            for (uint32 i = 0; i < dma_info->num_planes; i++) {
                if (dma_info->shared_fd[i] < 0) {
                    VLOG(ERR, "invalid dma_fd %d\n", dma_info->shared_fd[i]);
                    result.is_valid = false;
                    result.err_cod = AMVENC_ENCPARAM_MEM_FAIL;
                    return result;
                }
                handle->shared_fd[i] = dma_info->shared_fd[i];
                VLOG(INFO, "**shared_fd %d\n", handle->shared_fd[i]);
                videoInput.shared_fd[i] = dma_info->shared_fd[i];
            }

            videoInput.num_planes = handle->mNumPlanes;
            VLOG(INFO, "videoInput.num_planes=%d", videoInput.num_planes);
        } else {
            unsigned long* in = in_buffer_info->buf_info.in_ptr;
            videoInput.YCbCr[0] = (unsigned long)in[0];
            videoInput.YCbCr[1] = (unsigned long)(videoInput.YCbCr[0] + videoInput.height * videoInput.pitch);
            if (videoInput.fmt == AMVENC_NV21 || videoInput.fmt == AMVENC_NV12) {
                videoInput.YCbCr[2] = 0;
            } else if (videoInput.fmt == AMVENC_RGB888) {
                videoInput.YCbCr[1] = 0;
                videoInput.YCbCr[2] = 0;
            } else if (videoInput.fmt == AMVENC_YUV420) {
                videoInput.YCbCr[2] = (unsigned long)(videoInput.YCbCr[1] + videoInput.height * videoInput.pitch / 4);
            }
        }
        //videoInput.fmt = handle->fmt;
        videoInput.canvas = 0xffffffff;
        videoInput.type = handle->bufType;
        videoInput.disp_order = handle->mNumInputFrames;
        videoInput.op_flag = 0;

        if (handle->mKeyFrameRequested == true) {
            videoInput.op_flag = AMVEncFrameIO_FORCE_IDR_FLAG;
            handle->mKeyFrameRequested = false;
            VLOG(INFO, "Force encode a IDR frame at %d frame", handle->mNumInputFrames);
        }
        //if (handle->idrPeriod == 0) {
            //videoInput.op_flag |= AMVEncFrameIO_FORCE_IDR_FLAG;
        //}
        ret = AML_HEVCSetInput(handle->am_enc_handle, &videoInput);
        ++(handle->mNumInputFrames);

        VLOG(DEBUG, "AML_HEVCSetInput ret %d\n", ret);
        if (ret < AMVENC_SUCCESS) {
            VLOG(ERR, "encoderStatus = %d at line %d, handle: %p", ret, __LINE__, (void *)handle);
            result.is_valid = false;
            if (ret == AMVENC_FAIL)
               result.err_cod = AMVENC_INVALID_PARAM;
            else
                result.err_cod = ret;
            return result;
        }

        ret = AML_HEVCEncNAL(handle->am_enc_handle, out, (unsigned int *)&dataLength, &type);
        if (ret == AMVENC_PICTURE_READY) {
            if ((type == 0) || handle->mNumInputFrames == 1) {
                if ((handle->mPrependSPSPPSToIDRFrames ||
                     handle->mNumInputFrames == 1) &&
                    (handle->mSPSPPSData)) {
                    memmove(out + handle->mSPSPPSDataSize, out, dataLength);
                    memcpy(out, handle->mSPSPPSData, handle->mSPSPPSDataSize);
                    dataLength += handle->mSPSPPSDataSize;
                    VLOG(DEBUG, "copy mSPSPPSData to buffer size= %d at line %d \n", handle->mSPSPPSDataSize, __LINE__);
                }
                result.is_key_frame = true;
            }
        } else if ((ret == AMVENC_SKIPPED_PICTURE) || (ret == AMVENC_TIMEOUT)) {
            dataLength = 0;
            if (ret == AMVENC_TIMEOUT) {
                handle->mKeyFrameRequested = true;
                ret = AMVENC_SKIPPED_PICTURE;
            }
            VLOG(DEBUG, "ret = %d at line %d, handle: %p", ret, __LINE__, (void *)handle);
        } else if (ret != AMVENC_SUCCESS) {
            dataLength = 0;
            VLOG(ERR, "encoderStatus = %d at line %d, handle: %p", ret , __LINE__, (void *)handle);
        }
        if (ret < AMVENC_SUCCESS) {
            VLOG(ERR, "encoderStatus = %d, handle: %p", ret,(void*)handle);
            result.is_valid = false;
            if (ret == AMVENC_FAIL)
                result.err_cod = AMVENC_INVALID_PARAM;
            else
                result.err_cod = ret;
            return result;
        }
    }
    result.is_valid = true;
    result.encoded_data_length_in_bytes = dataLength;
    return result;
}

int vl_video_encoder_change_bitrate_hevc(vl_codec_handle_hevc_t codec_handle,
                            int bitRate)
{
    int ret;
    AMVHEVCEncHandle *handle = (AMVHEVCEncHandle *)codec_handle;

    if (handle->am_enc_handle == 0) //not init the encoder yet
        return -1;
    if (handle->mEncParams.param_change_enable == 0) //no change enabled
        return -2;
    ret = AML_HEVCEncChangeBitRate(handle->am_enc_handle, bitRate);
    if (ret != AMVENC_SUCCESS)
        return -3;
    //hist_reset(handle); //bitrate target change reset the hist
    handle->mEncParams.bitrate = bitRate;
    return 0;
}

int vl_video_encoder_destroy_hevc(vl_codec_handle_hevc_t codec_handle) {
    AMVHEVCEncHandle *handle = (AMVHEVCEncHandle *)codec_handle;
    AML_HEVCRelease(handle->am_enc_handle);
    if (handle->mSPSPPSData)
        free(handle->mSPSPPSData);
    if (handle)
        delete handle;
    return 1;
}
