/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */



#ifndef AML_VIDEO_ENCODER_M8_FAST_
#define AML_VIDEO_ENCODER_M8_FAST_

#include <time.h>
#include <sys/time.h>
#include <utils/threads.h>
#include <semaphore.h>
#include "AML_HWEncoder.h"
#include "enc_define.h"

#define FASTENC_AVC_IOC_MAGIC  'E'

#define FASTENC_AVC_IOC_GET_ADDR             _IOW(FASTENC_AVC_IOC_MAGIC, 0x00, unsigned int)
#define FASTENC_AVC_IOC_INPUT_UPDATE         _IOW(FASTENC_AVC_IOC_MAGIC, 0x01, unsigned int)
#define FASTENC_AVC_IOC_NEW_CMD              _IOW(FASTENC_AVC_IOC_MAGIC, 0x02, unsigned int)
#define FASTENC_AVC_IOC_GET_STAGE            _IOW(FASTENC_AVC_IOC_MAGIC, 0x03, unsigned int)
#define FASTENC_AVC_IOC_GET_OUTPUT_SIZE      _IOW(FASTENC_AVC_IOC_MAGIC, 0x04, unsigned int)
#define FASTENC_AVC_IOC_CONFIG_INIT          _IOW(FASTENC_AVC_IOC_MAGIC, 0x05, unsigned int)
#define FASTENC_AVC_IOC_FLUSH_CACHE          _IOW(FASTENC_AVC_IOC_MAGIC, 0x06, unsigned int)
#define FASTENC_AVC_IOC_FLUSH_DMA            _IOW(FASTENC_AVC_IOC_MAGIC, 0x07, unsigned int)
#define FASTENC_AVC_IOC_GET_BUFFINFO         _IOW(FASTENC_AVC_IOC_MAGIC, 0x08, unsigned int)
#define FASTENC_AVC_IOC_SUBMIT_ENCODE_DONE   _IOW(FASTENC_AVC_IOC_MAGIC, 0x09, unsigned int)
#define FASTENC_AVC_IOC_READ_CANVAS          _IOW(FASTENC_AVC_IOC_MAGIC, 0x0a, unsigned int)

#define UCODE_MODE_FULL 0
#define UCODE_MODE_SW_MIX 1

typedef struct {
    int pix_width;
    int pix_height;

    int mb_width;
    int mb_height;
    int mbsize;

    int framesize;
    AMVEncBufferType type;
    AMVEncFrameFmt fmt;
    uint32_t canvas;
    ulong plane[3];
} fast_input_t;

typedef struct {
    unsigned char* addr;
    uint32_t size;
} fast_buff_t;

typedef struct {
    int fd;
    bool IDRframe;
    bool mCancel;

    uint32_t enc_width;
    uint32_t enc_height;
    uint32_t quant;

    int fix_qp;

    bool gotSPS;
    uint32_t sps_len;
    bool gotPPS;
    uint32_t pps_len;

    uint32_t total_encode_frame;
    uint32_t total_encode_time;

    fast_input_t src;

    fast_buff_t mmap_buff;
    fast_buff_t input_buf;
    fast_buff_t ref_buf_y[2];
    fast_buff_t ref_buf_uv[2];
    fast_buff_t output_buf;
    uint32_t reencode_frame;
    bool reencode;
    bool logtime;
    struct timeval start_test;
    struct timeval end_test;
} fast_enc_drv_t;

extern void* InitFastEncode(int fd, amvenc_initpara_t* init_para);
extern AMVEnc_Status FastEncodeInitFrame(void *dev, ulong *yuv, AMVEncBufferType type, AMVEncFrameFmt fmt, bool IDRframe);
extern AMVEnc_Status FastEncodeSPS_PPS(void *dev, unsigned char* outptr, int* datalen);
extern AMVEnc_Status FastEncodeSlice(void *dev, unsigned char* outptr, int* datalen);
extern AMVEnc_Status FastEncodeCommit(void* dev, bool IDR);
extern void UnInitFastEncode(void *dev);

#endif
