#include "hevc_enc/vp_hevc_codec_1_0.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <pthread.h>
#include <assert.h>
extern "C"{
    #include "test_dma_api.h"
}

#define ENCODER_STRING_LEN_MAX  256
#define CFG_ENCODE_NUM_MAX  6

#define FORCE_PICTURE_TYPE  0x1
#define CHANGE_TARGET_RATE  0x2
#define CHANGE_MIN_MAX_QP   0x4
#define CHANGE_GOP_PERIOD   0x8
#define LONGTERM_REF_SET    0x10
#define CUST_SIZE_SMOOTH    0x20
#define CUST_APP_H264_PLUS  0x40
#define CHANGE_MULTI_SLICE  0x80
#define FORCE_PICTURE_SKIP  0x100
#define CHANGE_STRICT_RC    0x200
#define ENCODER_STRING_LEN_MAX  256

typedef struct {
    int FrameNum; //change frame number
    int enable_option;
    //FORCE_PICTURE_TYPE
    int picType; //vl_frame_type_t
    //CHANGE_TARGET_RATE
    int bitRate;
    //CHANGE_MIN_MAX_QP
    int minQpI;
    int maxQpI;
    int maxDeltaQp;
    int minQpP;
    int maxQpP;
    int minQpB;
    int maxQpB;
    //CHANGE_GOP_PERIOD
    int intraQP;
    int GOPPeriod;
    //LONGTERM_REF_SET
    int LTRFlags; //bit 0: UseCurSrcAsLongtermPic; bit 1: Use LTR frame
    // CUST_SIZE_SMOOTH
    int smooth_level; //0 normal, 50(middle), 100 (high)
    // CUST_APP_H264_PLUS
    int newIDRInterval; // shall be multiply of gop length
    int QPdelta_IDR;     // QP delta apply to IDRs
    int QPdelta_LTR;     // QP delta apply to LTR P frames
    // Change multi-slice
    int Slice_Mode;  //multi slice mode
    int Slice_Para;  //para
    // Change strict bitrate control
    int bitrate_window;  //bitrate cal window
    int pskip_threshold;  //threshold for pskip
} CfgChangeParam;

typedef struct {
    int   seqNum;
    char  srcfile[ENCODER_STRING_LEN_MAX];    // yuv data url in your root fs
    char  outfile[ENCODER_STRING_LEN_MAX];    // stream url in your root fs
    int   width;           // width
    int   height;          // height
    int   gop;             // I frame refresh interval
    int   framerate;       // framerate
    int   bitrate;         // bit rate
    int   num;             // encode frame count
    int   fmt;             // encode input fmt 1:nv12, 2:nv21, 3:420p
    int   buf_type;        // 0:vmalloc, 3:dma buffer
    int   num_planes;      // used for dma buffer case. 2 : nv12/nv21, 3 : yuv420p(yv12/yu12)
    int   cfg_opt;         /* optional, flags to control the encode feature settings
                            bit 0:roi control. 0: disabled (default) 1: enabled
                            bit 1: update control. 0: disabled (default) 1: enabled
                            bit 2 ~ bit 6 encode GOP pattern options:
                                0 default(IP) 1:I only    2:IP_only    3: IBBBP
                                4:IP_SVC1     5:IP_SVC2   6: IP_SVC3   7: IP_SVC4  8: CUSTP
                            bit 7:long term references. 0: disabled (default) 1: enabled
                            bit 8:cust source buffer stride . 0: disabled (default) 1: enabled
                            bit 9: convert normal source file stride to cust source stride. 0: disabled (default) 1: enabled
                            bit 10:enable intraRefresh. 0: disabled (default) 1: enabled
                            bit 11 ~ bit 13: profile 0: auto (default) others as following
                                    1: BASELINE(AVC) MAIN (HEVC);    2 MAIN (AVC) main10 (HEVC)
                                    3: HIGH (AVC) STILLPICTURE (HEVC); 4 HIGH10 (AVC)
                            bit 14 ~ bit 15: frame rotation(counter-clockwise, in degree): 0:none, 1:90, 2:180, 3:270
                            bit 16 ~ bit 17: frame mirroring: 0:none, 1:vertical, 2:horizontal, 3:both V and H
                            bit 18: enable customer bitstream buffer size
                            bit 19 ~ bit 20: enable multi slice mode: 0, one slice(no para), 1 by MB(16x16)/CTU(64x64), 2 by CTU + size(byte)
                            bit 21 cust_qp_delta_en
                            bit 22 strict_rate_ctrl_en (skipP allowed to limit bitrate) default 0
                            bit 23 forcePicQpEnable default 0
                            */
    char  roifile[ENCODER_STRING_LEN_MAX];      // optional, roi info url in root fs, no present if roi is disabled
    char  cfg_file[ENCODER_STRING_LEN_MAX];     // optional, cfg update info url. no present if update is disabled
    int   src_buf_stride;    // optional, source buffer stride
    int   intraRefresh ;     // optional, lower 4 bits for intra-refresh mode; others for refresh parameter, controlled by intraRefresh bit
    int   bitStreamBuf ;     // optional, encoded bitstream buffer size in MB
    int   MultiSlice_parameter;         // optional, multi slice parameter in MB/CTU or CTU and size (HIGH_16bits size + LOW_16bit CTU) combined
    int   cust_qp_delta;                // optional, cust_qp_delta apply to P frames value >0 lower; <0 increase the quality
    int   strict_rate_control_parmeter; // optional, High 16 bit bitrate window (frames max 120). low 16 bit bitrate threshold (percent)
    int   forcePicQpIPB;
} encodecCfgParam;

static int ParseCfgUpdateFile(FILE *fp, CfgChangeParam *cfg_update);
static int ParseEncodeCfgFile(FILE *fp, encodecCfgParam *cfg_encode);
static void *encode_thread(void *param);

int main(int argc, const char *argv[]){
    int i = 0;
    int len = 0;
    int arg_count = 0;

    FILE *fp_cfg_encode = NULL;
    encodecCfgParam cfg_encode[CFG_ENCODE_NUM_MAX];

    if ((argc != 2) && (argc < 12))
    {
        printf("Amlogic AVC Encode API \n");
        printf(" usage: output [srcfile][outfile][width][height][gop][framerate][bitrate][num][fmt][buf_type][num_planes]\n");
        printf("  options  :\n");
        printf("  srcfile  : yuv data url in your root fs\n");
        printf("  outfile  : stream url in your root fs\n");
        printf("  width    : width\n");
        printf("  height   : height\n");
        printf("  gop      : I frame refresh interval\n");
        printf("  framerate: framerate \n ");
        printf("  bitrate  : bit rate \n ");
        printf("  num      : encode frame count \n ");
        printf("  fmt      : encode input fmt 1:nv12, 2:nv21, 3:420p\n ");
        printf("  buf_type \t: 0:vmalloc, 3:dma buffer\n");
        printf("  num_planes \t: used for dma buffer case. 1 package,  2 : nv12/nv21, 3 : yuv420p\n");
        printf("  cfg_opt  \t: optional, flags to control the encode feature settings\n");
        printf("            \t\t  bit 0:roi control. 0: disabled (default) 1: enabled\n");
        printf("            \t\t  bit 1: update control. 0: disabled (default) 1: enabled\n");
        printf("            \t\t  bit 2 ~ bit 6 encode GOP pattern options:\n");
        printf("            \t\t\t 0 default(IP) 1:I only    2:IP_only    3: IBBBP\n");
        printf("            \t\t\t 4:IP_SVC1     5:IP_SVC2   6: IP_SVC3   7: IP_SVC4  8: CUSTP \n");
        printf("            \t\t  bit 7:long term references. 0: disabled (default) 1: enabled\n");
        printf("            \t\t  bit 8:cust source buffer stride . 0: disabled (default) 1: enabled\n");
        printf("            \t\t  bit 9: convert normal source file stride to cust source stride. 0: disabled (default) 1: enabled\n");
        printf("            \t\t  bit 10:enable intraRefresh. 0: disabled (default) 1: enabled\n");
        printf("            \t\t  bit 11 ~ bit 13: profile 0: auto (default) others as following\n");
        printf("            \t\t                   1: BASELINE(AVC) MAIN (HEVC);      2 MAIN (AVC) main10 (HEVC) \n");
        printf("            \t\t                   3: HIGH (AVC) STILLPICTURE (HEVC); 4 HIGH10 (AVC) \n");
        printf("            \t\t  bit 14 ~ bit 15: frame rotation(counter-clockwise, in degree): 0:none, 1:90, 2:180, 3:270\n");
        printf("            \t\t  bit 16 ~ bit 17: frame mirroring: 0:none, 1:vertical, 2:horizontal, 3:both V and H\n");
        printf("            \t\t  bit 18: enable customer bitstream buffer size\n");
        printf("            \t\t  bit 19 ~ bit 20: enable multi slice mode: 0, one slice(no para), 1 by MB(16x16)/CTU(64x64), 2 by CTU + size(byte)\n");
        printf("            \t\t  bit 21 cust_qp_delta_en \n");
        printf("            \t\t  bit 22 strict_rate_ctrl_en (skipP allowed to limit bitrate) default 0 \n");
        printf("            \t\t  bit 23 forcePicQpEnable default 0 \n");
        printf("  cfg_file \t: optional, cfg update info url. no present if update is disabled\n");
        printf("or\n");
        printf(" usage: aml_enc_test "
               "[encode_cfg_file]\n");
        printf("  options  \t:\n");
        printf("  encode_cfg_file  \t: encode cfg file info url\n");
        return -1;
    }
    else if (argc == 2)
    {
        printf("argv[1]:%s\n", argv[1]);
        fp_cfg_encode = fopen((argv[1]), "r");
        if (fp_cfg_encode == NULL) {
            printf("open encode cfg file error!\n");
            return 0;
        }
        memset(cfg_encode, 0, CFG_ENCODE_NUM_MAX * sizeof(encodecCfgParam));
        ParseEncodeCfgFile(fp_cfg_encode, cfg_encode);
        fclose(fp_cfg_encode);
        if (cfg_encode[0].seqNum == 0)
            return 0;
    }
    else
    {
        memset(&cfg_encode[0], 0, sizeof(encodecCfgParam));

        len = strlen(argv[1]) > ENCODER_STRING_LEN_MAX ? ENCODER_STRING_LEN_MAX : strlen(argv[1]);
        strncpy(cfg_encode[0].srcfile, argv[1], len);

        len = strlen(argv[2]) > ENCODER_STRING_LEN_MAX ? ENCODER_STRING_LEN_MAX : strlen(argv[2]);
        strncpy(cfg_encode[0].outfile, argv[2], len);

        cfg_encode[0].width =  atoi(argv[3]);
        cfg_encode[0].height = atoi(argv[4]);
        cfg_encode[0].gop = atoi(argv[5]);
        cfg_encode[0].framerate = atoi(argv[6]);
        cfg_encode[0].bitrate = atoi(argv[7]);
        cfg_encode[0].num = atoi(argv[8]);
        cfg_encode[0].fmt = atoi(argv[9]);
        cfg_encode[0].buf_type = atoi(argv[10]);
        cfg_encode[0].num_planes = atoi(argv[11]);
        if (argc > 12) {
            cfg_encode[0].cfg_opt = atoi(argv[12]);
        }
        if (argc > 13) {
            arg_count = 13;
            if ((cfg_encode[0].cfg_opt & 0x2) == 0x2 && argc > arg_count) {
                len = strlen(argv[arg_count]) > ENCODER_STRING_LEN_MAX ? ENCODER_STRING_LEN_MAX : strlen(argv[arg_count]);
                strncpy(cfg_encode[0].cfg_file, argv[arg_count], len);
                printf("cfg_upd_url is\t: %s ;\n", cfg_encode[0].cfg_file);
                arg_count ++;
            }
            if (arg_count != argc) {
                printf("config no match conf %d argc %d\n",
                    cfg_encode[0].cfg_opt, argc);
                return -1;
            }
        }
        cfg_encode[0].seqNum = 1;

    }

    pthread_t tid[CFG_ENCODE_NUM_MAX];
    for (i = 0; i < cfg_encode[0].seqNum; i ++) {
        if (i >= CFG_ENCODE_NUM_MAX)
            break;
        pthread_create(&tid[i], NULL, encode_thread, &cfg_encode[i]);
        pthread_join(tid[i], NULL);
    }

    return 0;
}
static void *encode_thread(void *param)
{
    int ret = 0;
    int width, height, gop, framerate, bitrate, num;
    int outfd = -1;
    FILE *fp = NULL;
    FILE *fp_cfg = NULL;
//    int datalen = 0;
    vl_img_format_hevc_t fmt = IMG_FMT_NONE;
    int buf_type = 0;
    int num_planes = 1;
    int src_buf_stride = 0;
    int bitstream_buf_size = 0;
    struct usr_ctx_s ctx;
    int cfg_upd_enabled = 0, has_cfg_update = 0;
    CfgChangeParam cfgChange;
    vl_frame_type_hevc_t enc_frame_type;
    int cfg_option = 0;
    int frame_num = 0;
    int qp_mode = 0;
    qp_param_hevc_t qp_tbl;

    unsigned char *vaddr = NULL;
    vl_codec_handle_hevc_t handle_enc;
    vl_encode_info_hevc_t encode_info;
    encoding_metadata_hevc_t encoding_metadata;

    encodecCfgParam *cfg_encode = NULL;
    unsigned int framesize = 0;
    unsigned int ysize = 0;
    unsigned int usize = 0;
    unsigned int vsize = 0;
    unsigned int uvsize = 0;
    unsigned int outputBufferLen = 0;
    unsigned char *inputBuffer = NULL;
    unsigned char *outputBuffer = NULL;

    vl_buffer_info_hevc_t inbuf_info;
    vl_dma_info_hevc_t *dma_info = NULL;
    unsigned char *input[3] = { NULL };

    pthread_detach(pthread_self());
    if (NULL != param)
        cfg_encode = (encodecCfgParam *)param;
    else {
        goto end;
    }

    width =  cfg_encode->width;
    if ((width < 1) || (width > 3840/*1920*/))
    {
        printf("invalid width \n");
        goto end;
    }
    height = cfg_encode->height;
    if ((height < 1) || (height > 2160/*1080*/))
    {
        printf("invalid height \n");
        goto end;
    }
    gop = cfg_encode->gop;
    framerate = cfg_encode->framerate;
    bitrate = cfg_encode->bitrate;
    num = cfg_encode->num;
    fmt = (vl_img_format_hevc_t)cfg_encode->fmt;
    buf_type = cfg_encode->buf_type;
    num_planes = cfg_encode->num_planes;
    if ((framerate < 0) || (framerate > 60))
    {
        printf("invalid framerate \n");
        goto end;
    }
    if (bitrate <= 0)
    {
        printf("invalid bitrate \n");
        goto end;
    }
    if (num < 0)
    {
        printf("invalid num \n");
        goto end;
    }
    if (buf_type == 3 && (num_planes < 1 || num_planes > 3))
    {
        printf("invalid num_planes \n");
        goto end;
    }
    printf("src_url is: %s ;\n", cfg_encode->srcfile);
    printf("out_url is: %s ;\n", cfg_encode->outfile);
    printf("width   is: %d ;\n", width);
    printf("height  is: %d ;\n", height);
    printf("gop     is: %d ;\n", gop);
    printf("frmrate is: %d ;\n", framerate);
    printf("bitrate is: %d ;\n", bitrate);
    printf("frm_num is: %d ;\n", num);
    printf("fmt     is\t: %d ;\n", fmt);
    printf("buf_type is\t: %d ;\n", buf_type);
    printf("num_planes is\t: %d ;\n", num_planes);

    if (cfg_encode->cfg_opt > 0) {
        cfg_option = cfg_encode->cfg_opt;
        if ((cfg_option & 0x2) == 0x2 && (strlen(cfg_encode->cfg_file) > 1)) {
            printf("cfg_upd_url is\t: %s ;\n", cfg_encode->cfg_file);
            cfg_upd_enabled = 1;
        }
    }

    outputBufferLen = 1024 * 1024 * sizeof(char);
    if (bitstream_buf_size > 8)
        outputBufferLen = bitstream_buf_size*1024 * 1024 * sizeof(char);
    outputBuffer = (unsigned char *)malloc(outputBufferLen);

    if (src_buf_stride) {
        framesize = src_buf_stride * height * 3 / 2;
        ysize = src_buf_stride * height;
        usize = src_buf_stride * height / 4;
        vsize = src_buf_stride * height / 4;
        uvsize = src_buf_stride * height / 2;
    } else {
        framesize = width * height * 3 / 2;
        ysize = width * height;
        usize = width * height / 4;
        vsize = width * height / 4;
        uvsize = width * height / 2;
    }

    memset(&inbuf_info, 0, sizeof(vl_buffer_info_hevc_t));
    inbuf_info.buf_type = (vl_buffer_type_hevc_t) buf_type;
    inbuf_info.buf_stride = src_buf_stride;
    inbuf_info.buf_fmt = fmt;

    memset(&qp_tbl, 0, sizeof(qp_param_hevc_t));

    qp_tbl.qp_min = 0;
    qp_tbl.qp_max = 51;
    qp_tbl.qp_base = 30;

    memset(&encode_info, 0, sizeof(vl_encode_info_hevc_t));
    encode_info.width = width;
    encode_info.height = height;
    encode_info.bit_rate = bitrate;
    encode_info.frame_rate = framerate;
    encode_info.gop = gop;
    encode_info.img_format = fmt;
    encode_info.qp_mode = qp_mode;
    encode_info.prepend_spspps_to_idr_frames = false;

    if (cfg_upd_enabled) {
        encode_info.enc_feature_opts |= ENABLE_PARA_UPDATE;
        memset(&cfgChange, 0, sizeof(CfgChangeParam));
    }

    if (inbuf_info.buf_type == DMA_TYPE)
    {
        dma_info = &(inbuf_info.buf_info.dma_info);
        dma_info->num_planes = num_planes;
        dma_info->shared_fd[0] = -1;    // dma buf fd
        dma_info->shared_fd[1] = -1;
        dma_info->shared_fd[2] = -1;
        ret = create_ctx(&ctx);
        if (ret < 0)
        {
            printf("gdc create fail ret=%d\n", ret);
            goto exit;
        }

        if (dma_info->num_planes == 3)
        {
            if (fmt != IMG_FMT_YUV420P)
            {
                printf("error fmt %d\n", fmt);
                goto exit;
            }
            ret = alloc_dma_buffer(&ctx, INPUT_BUFF_TYPE, ysize);
            if (ret < 0)
            {
                printf("alloc fail ret=%d, len=%u\n", ret,
                       ysize);
                goto exit;
            }
            vaddr = (unsigned char *) ctx.i_buff[0];
            if (!vaddr)
            {
                printf("mmap failed,Not enough memory\n");
                goto exit;
            }

            printf("hoan debug canvas >> alloc_dma_buffer y,ret=%d, vaddr=0x%p\n", ret,vaddr);
            dma_info->shared_fd[0] = ret;
            input[0] = vaddr;

            ret = alloc_dma_buffer(&ctx, INPUT_BUFF_TYPE, usize);
            if (ret < 0)
            {
                printf("alloc fail ret=%d, len=%u\n", ret,
                       usize);
                goto exit;
            }
            vaddr = (unsigned char *) ctx.i_buff[1];
            if (!vaddr)
            {
                printf("mmap failed,Not enough memory\n");
                goto exit;
            }
            dma_info->shared_fd[1] = ret;
            input[1] = vaddr;

            ret = alloc_dma_buffer(&ctx, INPUT_BUFF_TYPE, vsize);
            if (ret < 0)
            {
                printf("alloc fail ret=%d, len=%u\n", ret,
                       vsize);
                goto exit;
            }
            vaddr = (unsigned char *) ctx.i_buff[2];
            if (!vaddr)
            {
                printf("mmap failed,Not enough memory\n");
                goto exit;
            }

            dma_info->shared_fd[2] = ret;
            input[2] = vaddr;
            printf("hoan debug canvas >> alloc_dma_buffer v,ret=%d, vaddr=0x%p\n", ret,vaddr);
        } else if (dma_info->num_planes == 2)
        {
            ret = alloc_dma_buffer(&ctx, INPUT_BUFF_TYPE, ysize);
            if (ret < 0)
            {
                printf("alloc fail ret=%d, len=%u\n", ret,
                       ysize);
                goto exit;
            }
            vaddr = (unsigned char *) ctx.i_buff[0];
            if (!vaddr)
            {
                printf("mmap failed,Not enough memory\n");
                goto exit;
            }
            dma_info->shared_fd[0] = ret;
            input[0] = vaddr;
            if (fmt != IMG_FMT_NV12 && fmt != IMG_FMT_NV21)
            {
                printf("error fmt %d\n", fmt);
                goto exit;
            }
            ret = alloc_dma_buffer(&ctx, INPUT_BUFF_TYPE, uvsize);
            if (ret < 0)
            {
                printf("alloc fail ret=%d, len=%u\n", ret,
                       uvsize);
                goto exit;
            }
            vaddr = (unsigned char *) ctx.i_buff[1];
            if (!vaddr)
            {
                printf("mmap failed,Not enough memory\n");
                goto exit;
            }
            dma_info->shared_fd[1] = ret;
            input[1] = vaddr;
        } else if (dma_info->num_planes == 1)
        {
            ret =
                alloc_dma_buffer(&ctx, INPUT_BUFF_TYPE, framesize);
            if (ret < 0)
            {
                printf("alloc fail ret=%d, len=%u\n", ret,
                       framesize);
                goto exit;
            }
            vaddr = (unsigned char *) ctx.i_buff[0];
            if (!vaddr)
            {
                printf("mmap failed,Not enough memory\n");
                goto exit;
            }
            dma_info->shared_fd[0] = ret;
            input[0] = vaddr;
        }

        printf("in[0] %d, in[1] %d, in[2] %d\n", dma_info->shared_fd[0],
               dma_info->shared_fd[1], dma_info->shared_fd[2]);
        printf("input[0] %p, input[1] %p,input[2] %p\n", input[0],
               input[1], input[2]);
    } else {
        inputBuffer = (unsigned char *) malloc(framesize);
        if (inputBuffer == NULL) {
            printf("Can not allocat inputBuffer\n");
            goto exit;
        }
        inbuf_info.buf_info.in_ptr[0] = (ulong) (inputBuffer);
        inbuf_info.buf_info.in_ptr[1] =
            (ulong) (inputBuffer + ysize);
        inbuf_info.buf_info.in_ptr[2] =
            (ulong) (inputBuffer + ysize + usize);
    }

    fp = fopen((char *)cfg_encode->srcfile, "rb");
    if (fp == NULL)
    {
        printf("open src file error!\n");
        goto exit;
    }
    if (cfg_upd_enabled) {
        fp_cfg = fopen((const char*)cfg_encode->cfg_file, "r");
        if (fp_cfg == NULL)
        {
            printf("open cfg file error!\n");
            goto exit;
        }
        has_cfg_update = ParseCfgUpdateFile(fp_cfg, &cfgChange);
    }
    outfd = open((char *)cfg_encode->outfile, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (outfd < 0)
    {
        printf("open dst file error!\n");
        goto exit;
    }
    handle_enc = vl_video_encoder_init_hevc(CODEC_ID_H265, encode_info, &qp_tbl);
    while (num > 0) {
        if (inbuf_info.buf_type == DMA_TYPE) {  // read data to dma buf vaddr
            if (dma_info->num_planes == 1) {
                if (fread(input[0], 1, framesize, fp) != framesize) {
                    printf("read input file error!\n");
                    goto exit;
                }
            } else if (dma_info->num_planes == 2) {
                if (fread(input[0], 1, ysize, fp) != ysize) {
                    printf("read input file error!\n");
                    goto exit;
                }
                if (fread(input[1], 1, uvsize, fp) != uvsize) {
                    printf("read input file error!\n");
                    goto exit;
                }
            } else if (dma_info->num_planes == 3) {
                if (fread(input[0], 1, ysize, fp) != ysize) {
                    printf("read input file error!\n");
                    goto exit;
                }
                if (fread(input[1], 1, usize, fp) != usize) {
                    printf("read input file error!\n");
                    goto exit;
                }
                if (fread(input[2], 1, vsize, fp) != vsize) {
                    printf("read input file error!\n");
                    goto exit;
                }
            }
        } else {        // read data to vmalloc buf vaddr
            if (fread(inputBuffer, 1, framesize, fp) != framesize) {
                printf("read input file error!\n");
                goto exit;
            }
        }
        if (cfg_upd_enabled && has_cfg_update) {
            if (cfgChange.FrameNum == frame_num) { // apply updates
                if (cfgChange.enable_option
                    & FORCE_PICTURE_TYPE)
                {
                    if (cfgChange.picType == FRAME_TYPE_IDR
                        || cfgChange.picType
                        == FRAME_TYPE_I)
                    {
                        enc_frame_type = FRAME_TYPE_I;
                        printf("force I frame on %d \n",
                            frame_num);
                    }
                }
                if (cfgChange.enable_option
                    & CHANGE_TARGET_RATE)
                {
                    vl_video_encoder_change_bitrate_hevc(
                        handle_enc,
                        cfgChange.bitRate);
                    printf("Change bitrate to %d on %d \n",
                        cfgChange.bitRate, frame_num);
                }
            }
            if (cfgChange.FrameNum <= frame_num) {
                has_cfg_update =
                    ParseCfgUpdateFile(fp_cfg, &cfgChange);
            }
        }

        memset(outputBuffer, 0, outputBufferLen);
        if (inbuf_info.buf_type == DMA_TYPE)
        {
            sync_cpu(&ctx);
        }
        encoding_metadata =
            vl_video_encoder_encode_hevc(handle_enc, FRAME_TYPE_AUTO,
                        outputBuffer, &inbuf_info);
        if (encoding_metadata.is_valid)
        {
            write(outfd, (unsigned char *) outputBuffer,
                  encoding_metadata.encoded_data_length_in_bytes);
        } else
        {
            printf("encode error %d!\n",
                   encoding_metadata.encoded_data_length_in_bytes);
        }
        num--;
        frame_num++;
    }
exit:
    if (handle_enc)
        vl_video_encoder_destroy_hevc(handle_enc);
    if (outputBuffer)
        free(outputBuffer);
    if (outfd >= 0)
        close(outfd);
    if (fp)
        fclose(fp);
    if (inbuf_info.buf_type == DMA_TYPE)
    {
        if (dma_info->shared_fd[0] >= 0)
            close(dma_info->shared_fd[0]);

        if (dma_info->shared_fd[1] >= 0)
            close(dma_info->shared_fd[1]);

        if (dma_info->shared_fd[2] >= 0)
            close(dma_info->shared_fd[2]);

        destroy_ctx(&ctx);
    } else
    {
        if (inputBuffer)
            free(inputBuffer);
    }
    printf("Encode End!width:%d\n",width);
end:
    pthread_exit(0);
    return NULL;
}


static char*mystrncpy(char* dest, const char* src, int len)
{
    assert(dest != NULL && src != NULL);
    char* temp = dest;
    int i=0;
    //while (i++ < len  && (*temp++ = *src++) != '\0') {}
    while (i++ < len  && (*temp = *src) != '\n' && (*temp++ = *src++) != '\0') {}
    if (*(temp) != '\0')
        *temp = '\0';
    return dest;
}

static int ParseEncodeCfgFile(FILE *fp, encodecCfgParam *cfg_encode)
{
    char* token = NULL;
    char delim[2] = " ";
    static char lineStr[ENCODER_STRING_LEN_MAX*4] = {0};
    char valueStr[ENCODER_STRING_LEN_MAX] = {0};
    int i = 0;
    int len = 0;
    while (1) {
        if (i >= CFG_ENCODE_NUM_MAX)
            return 0;
        if (lineStr[0] == 0x0) { //need a new line;
            if (fgets(lineStr, ENCODER_STRING_LEN_MAX*4, fp) == NULL) {
                if (feof(fp))
                    return 0;
                continue;
            }
        }
        if (strlen(lineStr) == 1) { // skip null
            lineStr[0] = 0x0;
            continue;
        }

        if ((lineStr[0] == '#')
        ||(lineStr[0] == ';')||(lineStr[0] == ':')) { // skip comments
            lineStr[0] = 0x0;
            continue;
        }

        //printf("[%s][%d]:%s\n",__func__,__LINE__,lineStr);
        // keep the original
        memcpy(valueStr, lineStr, strlen(lineStr));
        //param is separated by ' '
        token = strtok(valueStr, delim);
        if (token == NULL) {
            lineStr[0] = 0x0;
            continue;
        }
        token = strtok(NULL, delim);
        if (token == NULL) {
            lineStr[0] = 0x0;
            continue;
        }
        len = strlen(token) > ENCODER_STRING_LEN_MAX ? ENCODER_STRING_LEN_MAX : strlen(token);
        mystrncpy(cfg_encode[i].srcfile, token, len);

        token = strtok(NULL, delim);
        if (token == NULL) {
            lineStr[0] = 0x0;
            continue;
        }
        len = strlen(token) > ENCODER_STRING_LEN_MAX ? ENCODER_STRING_LEN_MAX : strlen(token);
        mystrncpy(cfg_encode[i].outfile, token, len);

        token = strtok(NULL, delim);
        if (token == NULL) {
            lineStr[0] = 0x0;
            continue;
        }
        cfg_encode[i].width = atoi(token);

        token = strtok(NULL, delim);
        if (token == NULL) {
            lineStr[0] = 0x0;
            continue;
        }
        cfg_encode[i].height = atoi(token);

        token = strtok(NULL, delim);
        if (token == NULL) {
            lineStr[0] = 0x0;
            continue;
        }
        cfg_encode[i].gop = atoi(token);

        token = strtok(NULL, delim);
        if (token == NULL) {
            lineStr[0] = 0x0;
            continue;
        }
        cfg_encode[i].framerate = atoi(token);

        token = strtok(NULL, delim);
        if (token == NULL) {
            lineStr[0] = 0x0;
            continue;
        }
        cfg_encode[i].bitrate = atoi(token);

        token = strtok(NULL, delim);
        if (token == NULL) {
            lineStr[0] = 0x0;
            continue;
        }
        cfg_encode[i].num = atoi(token);

        token = strtok(NULL, delim);
        if (token == NULL) {
            lineStr[0] = 0x0;
            continue;
        }
        cfg_encode[i].fmt = atoi(token);

        token = strtok(NULL, delim);
        if (token == NULL) {
            lineStr[0] = 0x0;
            continue;
        }
        cfg_encode[i].buf_type = atoi(token);

        token = strtok(NULL, delim);
        if (token == NULL) {
            lineStr[0] = 0x0;
            continue;
        }
        cfg_encode[i].num_planes = atoi(token);

        token = strtok(NULL, delim);//cfg_opt optional
        if (token == NULL) {
            i++;
            cfg_encode[0].seqNum++;
            lineStr[0] = 0x0;
            continue;
        }
        cfg_encode[i].cfg_opt = atoi(token);

        token = strtok(NULL, delim);
        if (token == NULL) {
            i++;
            cfg_encode[0].seqNum++;
            lineStr[0] = 0x0;
            continue;
        }

        if ((cfg_encode[i].cfg_opt & 0x2) == 0x2) {
            len = strlen(token) > ENCODER_STRING_LEN_MAX ? ENCODER_STRING_LEN_MAX : strlen(token);
            mystrncpy(cfg_encode[i].cfg_file, token, len);
            printf("cfg_upd_url[%d] is\t: %s ;\n", i, cfg_encode[i].cfg_file);
            token = strtok(NULL, delim);
            if (token == NULL) {
                i++;
                cfg_encode[0].seqNum++;
                lineStr[0] = 0x0;
                continue;
            }
        }

        token = strtok(NULL, delim);
        if (token == NULL) {
            i++;
            cfg_encode[0].seqNum++;
            lineStr[0] = 0x0;
            continue;
        } else { //config no match
            printf("config no match conf %d command: %s\n",
                cfg_encode[i].cfg_opt, lineStr);
            lineStr[0] = 0x0;
            continue;
        }
    }
    return 0;
}
static int ParseCfgUpdateFile(FILE *fp, CfgChangeParam *cfg_update)
{
    int parsed_num = 0, frameIdx = 0;
    char* token = NULL;
    static char lineStr[256] = {0, };
    char valueStr[256] = {0, };
    int has_update = 0;
    while (1) {
        if (lineStr[0] == 0x0) { //need a new line;
            if (fgets(lineStr, 256, fp) == NULL ) {
                if (cfg_update->enable_option && has_update)
                    return 1;
                else {
                    if (feof(fp)) return 0;
                    continue;
                }
            }
        }
        if ((lineStr[0] == '#')
            ||(lineStr[0] == ';')||(lineStr[0] == ':'))
        { // skip comments
            lineStr[0] = 0x0;
            continue;
        }
        // keep the original
        memcpy(valueStr, lineStr, strlen(lineStr));
        //frame number is separated by ' ' or ':'
        token = strtok(valueStr, ": ");
        if (token == NULL) {
            lineStr[0] = 0x0;
            continue;
        }
        frameIdx = atoi(token);
        if (frameIdx <0 || frameIdx < cfg_update->FrameNum) {
            // invid number
            lineStr[0] = 0x0;
            continue;
        }
        else if (frameIdx > cfg_update->FrameNum && has_update)
        {// done the current frame;
            if (cfg_update->enable_option)
                return 1;
            has_update = 0;
            continue;

        } else if (has_update ==0) {// clear the options
            cfg_update->FrameNum = frameIdx;
            cfg_update->enable_option = 0;
            has_update = 1;
        }

        token = strtok(NULL, " :"); // check the operation code
        while (token && strlen(token) == 1
            && strncmp(token, " ", 1) == 0)
            token = strtok(NULL, " :"); // skip spaces
        if (token == NULL) { // no operate code
            lineStr[0] = 0x0;
            continue;
        }
        if (strcasecmp("ForceType",token) == 0) {
            // force picture type
            token = strtok(NULL, ":\r\n");
            while (token && strlen(token) == 1
                && strncmp(token, " ", 1) == 0)
            token = strtok(NULL, ":\r\n"); //check space
            if (token) {
                while ( *token == ' ' ) token++;//skip spaces;
                cfg_update->picType = atoi(token);
                cfg_update->enable_option |=
                    FORCE_PICTURE_TYPE;
            }
        } else if (strcasecmp("ForceSkip",token) == 0) {
            // force picture type
            if (token) {
                cfg_update->enable_option |=
                    FORCE_PICTURE_SKIP;
            }
        } else if (strcasecmp("ChangeBitRate",token) == 0) {
            token = strtok(NULL, ":\r\n");
            while ( token && strlen(token) == 1
                && strncmp(token, " ", 1) == 0)
                token = strtok(NULL, ":\r\n"); //check space
            if (token) {
                while ( *token == ' ' ) token++;//skip spaces;
                cfg_update->bitRate = atoi(token);
                cfg_update->enable_option |=
                    CHANGE_TARGET_RATE;
            }
        } else if (strcasecmp("ChangeMinMaxQP",token) == 0) {
            token = strtok(NULL, ":\r\n");
            while (token && strlen(token) == 1
                && strncmp(token, " ", 1) == 0)
                token = strtok(NULL, ":\r\n"); //check space
            if (token) {
                while ( *token == ' ' ) token++;//skip spaces;
                parsed_num = sscanf(token,
                    "%d %d %d %d %d %d %d",
                    &cfg_update->minQpI, &cfg_update->maxQpI,
                    &cfg_update->maxDeltaQp,
                    &cfg_update->minQpP, &cfg_update->maxQpP,
                    &cfg_update->minQpB, &cfg_update->maxQpB);
                if (parsed_num == 7)
                    cfg_update->enable_option |=
                        CHANGE_MIN_MAX_QP;
            }
        } else if (strcasecmp("ChangePeriodGOP",token) == 0) {
            token = strtok(NULL, ":\r\n");
            while ( token && strlen(token) == 1
                && strncmp(token, " ", 1) == 0)
                token = strtok(NULL, ":\r\n"); //check space
            if (token) {
                while ( *token == ' ' ) token++;//skip spaces;
                parsed_num = sscanf(token, "%d %d",
                    &cfg_update->intraQP,
                    &cfg_update->GOPPeriod);
                if (parsed_num == 2)
                    cfg_update->enable_option |=
                        CHANGE_GOP_PERIOD;
            }
        } else if (strcasecmp("SetLongTermRef",token) == 0) {
            token = strtok(NULL, ":\r\n");
            while ( token && strlen(token) == 1
                && strncmp(token, " ", 1) == 0)
                token = strtok(NULL, ":\r\n"); //check space
            if (token) {
                while ( *token == ' ' ) token++;//skip spaces;
                cfg_update->LTRFlags = atoi(token);
                cfg_update->enable_option |=
                    LONGTERM_REF_SET;
            }
        } else if (strcasecmp("SetCustSmooth",token) == 0) {
            token = strtok(NULL, ":\r\n");
            while ( token && strlen(token) == 1
                && strncmp(token, " ", 1) == 0)
                token = strtok(NULL, ":\r\n"); //check space
            if (token) {
                while ( *token == ' ' ) token++;//skip spaces;
                cfg_update->smooth_level = atoi(token);
                cfg_update->enable_option |=
                    CUST_SIZE_SMOOTH;
            }
        } else if (strcasecmp("SetCustH264P",token) == 0) {
            token = strtok(NULL, ":\r\n");
            while ( token && strlen(token) == 1
                && strncmp(token, " ", 1) == 0)
                token = strtok(NULL, ":\r\n"); //check space
            if (token) {
                while ( *token == ' ' ) token++;//skip spaces;
                parsed_num = sscanf(token,
                    "%d %d %d",
                    &cfg_update->newIDRInterval,
                    &cfg_update->QPdelta_IDR,
                    &cfg_update->QPdelta_LTR);
                if (parsed_num == 3)
                    cfg_update->enable_option |=
                        CUST_APP_H264_PLUS;
            }
        } else if (strcasecmp("ChangeMultiSlice",token) == 0) {
            token = strtok(NULL, ":\r\n");
            while ( token && strlen(token) == 1
                && strncmp(token, " ", 1) == 0)
                token = strtok(NULL, ":\r\n"); //check space
            if (token) {
                while ( *token == ' ' ) token++;//skip spaces;
                parsed_num = sscanf(token, "%d %d",
                    &cfg_update->Slice_Mode,
                    &cfg_update->Slice_Para);
                if (parsed_num == 2)
                    cfg_update->enable_option |=
                        CHANGE_MULTI_SLICE;
            }
        }
        else if (strcasecmp("ChangeStrictRC",token) == 0) {
            token = strtok(NULL, ":\r\n");
            while ( token && strlen(token) == 1
                && strncmp(token, " ", 1) == 0)
                token = strtok(NULL, ":\r\n"); //check space
            if (token) {
                while ( *token == ' ' ) token++;//skip spaces;
                parsed_num = sscanf(token, "%d %d",
                    &cfg_update->bitrate_window,
                    &cfg_update->pskip_threshold);
                if (parsed_num == 2)
                    cfg_update->enable_option |=
                        CHANGE_STRICT_RC;
            }
        }
        lineStr[0] = 0x0;
        continue;
    }
}

