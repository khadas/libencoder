#ifndef AMLOGIC_HEVCENCODER_
#define AMLOGIC_HEVCENCODER_

#include "enc_define.h"

#ifdef SUPPORT_SRC_BUF_CONTROL
#define ENC_SRC_BUF_NUM             2000
#else
#define ENC_SRC_BUF_NUM             16          //!< case of GOPsize = 8 (IBBBBBBBP), max src buffer num  = 13
#endif

#define amv_enc_handle_hevc_t long

typedef struct amvhevcenc_initpara_s {
    uint32 enc_width;
    uint32 enc_height;
    uint32 nSliceHeaderSpacing;
    uint32 MBsIntraRefresh;
    uint32 MBsIntraOverlap;
    int initQP;
    bool rcEnable;
    uint32 bitrate;
    uint32 frame_rate;
    uint32 cpbSize;
    bool bitrate_scale;
    uint8 encode_once;
} amvhevcenc_initpara_t;

typedef struct amvhevcenc_hw_s {
    ENC_DEV_TYPE dev_id;
    int dev_fd;
    void* rc_data;
    void* dev_data;
    amvhevcenc_initpara_t init_para;
} amvhevcenc_hw_t;

typedef struct FrameIOhevc_s {
    ulong YCbCr[3];
    AMVEncBufferType type;
    AMVEncFrameFmt fmt;
    int pitch;
    int height;
    uint32 disp_order;
    uint is_reference;
    uint32 coding_timestamp;
    uint32 op_flag;
    uint32 canvas;
    uint32 bitrate;
    float frame_rate;
    uint32 scale_width;
    uint32 scale_height;
    uint32 crop_left;
    uint32 crop_right;
    uint32 crop_top;
    uint32 crop_bottom;
    uint32 num_planes;
    int shared_fd[3];
} AMVHEVCEncFrameIO;

typedef struct HEVCEncParams_s {
    HEVCProfile profile; /* profile of the bitstream to be compliant with*/
    HEVCLevel level; /* level of the bitstream to be compliant with*/
    uint32 tier; /* tier of the bitstream to be compliant with*/

    uint32 width; /* width of an input frame in pixel */
    uint32 height; /* height of an input frame in pixel */

    int num_ref_frame; /* number of reference frame used */
    int num_slice_group; /* number of slice group */

    uint32 nSliceHeaderSpacing;

    HEVCFlag auto_scd; /* scene change detection on or off */
    int idr_period; /* idr frame refresh rate in number of target encoded frame (no concept of actual time).*/

    bool prepend_spspps_to_idr_frames; /* if true, prepends spspps header into all
                                      idr frames.*/

    HEVCFlag fullsearch; /* enable full-pel full-search mode */
    int search_range; /* search range for motion vector in (-search_range,+search_range) pixels */
    //HEVCFlag sub_pel;    /* enable sub pel prediction */
    //HEVCFlag submb_pred; /* enable sub MB partition mode */

    HEVCFlag rate_control; /* rate control enable, on: RC on, off: constant QP */
    int qp_mode;
    int initQP; /* initial QP */
    int maxQP;
    int minQP;
    int maxDeltaQP;            /* max QP delta*/
    uint32 bitrate; /* target encoding bit rate in bits/second */
    uint32 CPB_size; /* coded picture buffer in number of bits */
    uint32 init_CBP_removal_delay; /* initial CBP removal delay in msec */

    uint32 frame_rate; /* frame rate in the unit of frames per 1000 second */
    /* note, frame rate is only needed by the rate control, AVC is timestamp agnostic. */

    uint32 MBsIntraRefresh;
    uint32 MBsIntraOverlap;

    HEVCFlag out_of_band_param_set; /* flag to set whether param sets are to be retrieved up front or not */
    HEVCFlag FreeRun;
    HEVCFlag BitrateScale;
    uint32 dev_id; /* ID to identify the hardware encoder version */
    uint8 encode_once; /* flag to indicate encode once or twice */

    uint32 src_width;  /*src buffer width before crop and scale */
    uint32 src_height; /*src buffer height before crop and scale */
    HEVCRefreshType refresh_type; /*refresh type of intra picture*/
    AMVEncFrameFmt fmt;
    int es_buf_sz;        /* ES buffer size */
    uint32 param_change_enable; /* enable on the fly change parameters*/

    bool vui_info_present;
    bool video_signal_type; /*video_signal_type_present_flag*/
    bool color_description; /*color_description_present_flag*/
    int primaries; /*color primaries*/
    int transfer; /*color transfer charicstics*/
    int matrix; /* color space matrix coefficients*/
    bool range; /*color range flag, 0:full, 1:limitedd*/
    bool crop_enable;
    int crop_left;
    int crop_top;
    int crop_right;
    int crop_bottom;
} AMVHEVCEncParams;

typedef struct {
    uint32 enc_width;
    uint32 enc_height;
    AMVEnc_State state;
    bool rcEnable; /* rate control enable, on: RC on, off: constant QP */
    int initQP; /* initial QP */
    uint32 bitrate; /* target encoding bit rate in bits/second */
    uint32 cpbSize; /* coded picture buffer in number of bits */
    AVCNalUnitType nal_unit_type;
    AVCSliceType slice_type;

    uint32 modTimeRef; /* Reference modTime update every I-Vop*/
    uint32 wrapModTime; /* Offset to modTime Ref, rarely used */
    uint32 frame_rate; /* frame rate */
    int idrPeriod; /* IDR period in number of frames */
    bool first_frame; /* a flag for the first frame */
    uint prevProcFrameNum; /* previously processed frame number, could be skipped */
    uint prevProcFrameNumOffset;
    uint32 lastTimeRef;
    uint frame_in_gop;
    amvhevcenc_hw_t hw_info;
} hevc_info_t;

extern amv_enc_handle_hevc_t AML_HEVCInitialize(AMVHEVCEncParams *encParam);
extern AMVEnc_Status AML_HEVCSetInput(amv_enc_handle_hevc_t ctx_handle, AMVHEVCEncFrameIO *input);
extern AMVEnc_Status AML_HEVCEncChangeBitRate(amv_enc_handle_hevc_t ctx_handle,
                                int BitRate);
extern AMVEnc_Status AML_HEVCEncChangeFrameRate(amv_enc_handle_hevc_t ctx_handle,
                                int FrameRate,int BitRate);
extern AMVEnc_Status AML_HEVCEncGetAvgQp(amv_enc_handle_hevc_t ctx_handle, int *avgqp);
extern AMVEnc_Status AML_HEVCEncNAL(amv_enc_handle_hevc_t ctx_handle,
                             unsigned char* buffer,
                             unsigned int* buf_nal_size,
                             int *nal_type);
extern AMVEnc_Status AML_HEVCEncHeader(amv_enc_handle_hevc_t ctx_handle,
                                unsigned char* buffer,
                                unsigned int* buf_nal_size);
extern AMVEnc_Status AML_HEVCRelease(amv_enc_handle_hevc_t ctx_handle);

#endif
