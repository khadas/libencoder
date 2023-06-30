#include <string.h>
#include <dlfcn.h>
#include <pthread.h>

#include "vp_vc_codec_1_0.h"
#include "amvenc_common.h"
#include "amvenc.h"

#define LIB_ENCODER_API_NAME "libvc_codec.so"

typedef vc_codec_handle_t (*fn_init)(vc_codec_id_t codec_id,
                                        vc_encode_info_t encode_info);
typedef vc_encoding_metadata_t (*fn_generate_header)(vc_codec_handle_t codec_handle,
                                                         unsigned char *buffer,
                                                         unsigned int *header_len);
typedef vc_encoding_metadata_t (*fn_encode)(vc_codec_handle_t handle,
                                               vc_frame_type_t type,
                                               unsigned char* out,
                                               vc_buffer_info_t *in_buffer_info,
                                               vc_buffer_info_t *ret_buffer_info);
typedef int (*fn_change_bitrate)(vc_codec_handle_t handle,         int bitRate);
typedef int (*fn_destroy)(vc_codec_handle_t handle);

typedef struct {
    int load_count;
    void *libHandle;
    fn_init init;
    fn_generate_header generate_header;
    fn_encode encode;
    fn_change_bitrate change_bitrate;
    fn_destroy destroy;
}vl_encoder_ops_s;

static vl_encoder_ops_s encoder_ops = {0};
static pthread_mutex_t encoder_ops_mutex = PTHREAD_MUTEX_INITIALIZER;

int amvenc_vers_load_Module(void)
{
    VLOG(DEBUG, "amvenc_ports vcenc loadModule!, load_count:%d\n", encoder_ops.load_count);
    pthread_mutex_lock(&encoder_ops_mutex);
    encoder_ops.load_count ++;
    if (encoder_ops.load_count == 1) {
        encoder_ops.libHandle = dlopen(LIB_ENCODER_API_NAME, RTLD_NOW | RTLD_NODELETE);
        if (encoder_ops.libHandle != NULL) {
            encoder_ops.init = (fn_init)dlsym(encoder_ops.libHandle, "vc_encoder_init");
            if (encoder_ops.init == NULL) {
                VLOG(ERR, "dlsym for vl_multi_encoder_init failed");
                goto exit;
            }

            encoder_ops.generate_header = (fn_generate_header)dlsym(encoder_ops.libHandle, "vc_encode_header");
            if (encoder_ops.generate_header == NULL) {
                VLOG(ERR, "dlsym for vl_multi_encoder_generate_header failed");
                goto exit;
            }

            /*encoder_ops.change_bitrate = (fn_change_bitrate)dlsym(encoder_ops.libHandle, "vc_encoder_change_bitrate");
            if (encoder_ops.change_bitrate == NULL) {
                VLOG(ERR, "dlsym for vl_video_encoder_change_bitrate failed");
                goto exit;
            }*/

            encoder_ops.encode = (fn_encode)dlsym(encoder_ops.libHandle, "vc_encoder_encode");
            if (encoder_ops.encode == NULL) {
                VLOG(ERR, "dlsym for vl_multi_encoder_encode failed");
                goto exit;
            }

            encoder_ops.destroy = (fn_destroy)dlsym(encoder_ops.libHandle,"vc_encoder_destroy");
            if (encoder_ops.destroy == NULL) {
                VLOG(ERR, "dlsym for vl_multi_encoder_destroy failed");
                goto exit;
            }
        } else {
            VLOG(ERR, "dlopen for %s failed,err:%s", LIB_ENCODER_API_NAME, dlerror());
            encoder_ops.load_count --;
            pthread_mutex_unlock(&encoder_ops_mutex);
            return -1;
        }
    }
    pthread_mutex_unlock(&encoder_ops_mutex);
    return 0;
exit:
    dlclose(encoder_ops.libHandle);
    encoder_ops.load_count --;
    pthread_mutex_unlock(&encoder_ops_mutex);
    return -1;
}

amvenc_handle_t amvenc_vers_init(amvenc_codec_id_t vcodec_id,
                                       amvenc_info_t vcodec_info,
                                        amvenc_qp_param_t* qp_param)
{
    vc_codec_id_t codec_id = VC_CODEC_ID_H264;
    vc_encode_info_t encode_info;
    vc_qp_param_t qp_tbl;
    vc_codec_handle_t handle_enc = 0;

    if (qp_param == NULL) {
        VLOG(ERR, "invalid param, qp_param is NULL!\n");
        return 0;
    }

    memset(&encode_info, 0, sizeof(encode_info));
    memset(&qp_tbl, 0, sizeof(qp_tbl));

    switch (vcodec_id)
    {
        case AML_CODEC_ID_H264:
            codec_id = VC_CODEC_ID_H264;
            break;
        case AML_CODEC_ID_H265:
            codec_id = VC_CODEC_ID_H265;
            break;
        default:
            break;
    }

    if (codec_id != VC_CODEC_ID_H264 && codec_id != VC_CODEC_ID_H265) {
        VLOG(ERR, "unsupported codec id %d \n", codec_id);
        return 0;
    }
    encode_info.width = vcodec_info.width;
    encode_info.height = vcodec_info.height;
    encode_info.frame_rate = vcodec_info.frame_rate;
    encode_info.bit_rate = vcodec_info.bit_rate;
    encode_info.gop = vcodec_info.gop;
    encode_info.prepend_spspps_to_idr_frames = vcodec_info.prepend_spspps_to_idr_frames;


    switch (vcodec_info.img_format)
    {
        case AML_IMG_FMT_NV12:
            encode_info.img_format = VC_IMG_FMT_NV12;
            break;
        case AML_IMG_FMT_NV21:
            encode_info.img_format = VC_IMG_FMT_NV21;
            break;
        case AML_IMG_FMT_YUV420P:
            encode_info.img_format = VC_IMG_FMT_YUV420P;
            break;
        case AML_IMG_FMT_YV12:
            encode_info.img_format = VC_IMG_FMT_YV12;
            break;
        case AML_IMG_FMT_RGB888:
            encode_info.img_format = VC_IMG_FMT_RGB888;
            break;
        case AML_IMG_FMT_RGBA8888:
            encode_info.img_format = VC_IMG_FMT_RGBA8888;
            break;
        default:
            break;
    }
    encode_info.qp_mode = vcodec_info.qp_mode;

    encode_info.forcePicQpEnable = vcodec_info.forcePicQpEnable;
    encode_info.forcePicQpI = vcodec_info.forcePicQpI;
    encode_info.forcePicQpP = vcodec_info.forcePicQpP;
    encode_info.forcePicQpB = vcodec_info.forcePicQpB;

    encode_info.enc_feature_opts = vcodec_info.enc_feature_opts;
    encode_info.intra_refresh_mode = vcodec_info.intra_refresh_mode;
    encode_info.intra_refresh_arg = vcodec_info.intra_refresh_arg;

    encode_info.profile = vcodec_info.profile;

    encode_info.frame_rotation = vcodec_info.frame_rotation;
    encode_info.frame_mirroring = vcodec_info.frame_mirroring;

    encode_info.bitstream_buf_sz = vcodec_info.bitstream_buf_sz;
    encode_info.multi_slice_mode = vcodec_info.multi_slice_mode;
    encode_info.multi_slice_arg = vcodec_info.multi_slice_arg;
    encode_info.cust_gop_qp_delta = vcodec_info.cust_gop_qp_delta;
    encode_info.strict_rc_window = vcodec_info.strict_rc_window;
    encode_info.strict_rc_skip_thresh = vcodec_info.strict_rc_skip_thresh;
    encode_info.bitstream_buf_sz_kb = vcodec_info.bitstream_buf_sz_kb;

    //encode_info.vui_parameters_present_flag = vcodec_info.vui_parameters_present_flag;
    //encode_info.video_full_range_flag = vcodec_info.video_full_range_flag;
    //encode_info.video_signal_type_present_flag = vcodec_info.video_signal_type_present_flag;
    //encode_info.colour_description_present_flag = vcodec_info.colour_description_present_flag;
    //encode_info.colour_primaries = vcodec_info.colour_primaries;
    //encode_info.transfer_characteristics = vcodec_info.transfer_characteristics;
    ///encode_info.matrix_coefficients = vcodec_info.matrix_coefficients;

    //qp_tbl.qp_min = qp_param->qp_min;
    //qp_tbl.qp_max = qp_param->qp_max;
    //qp_tbl.qp_I_base = qp_param->qp_I_base;
    //qp_tbl.qp_P_base = qp_param->qp_P_base;
    //qp_tbl.qp_B_base = qp_param->qp_B_base;
    //qp_tbl.qp_I_min = qp_param->qp_I_min;
    //qp_tbl.qp_I_max = qp_param->qp_I_max;
    //qp_tbl.qp_P_min = qp_param->qp_P_min;
    //qp_tbl.qp_P_max = qp_param->qp_P_max;
    //qp_tbl.qp_B_min = qp_param->qp_B_min;
    //qp_tbl.qp_B_max = qp_param->qp_B_max;

    if (encoder_ops.init)
        handle_enc = encoder_ops.init(codec_id, encode_info);

    return (amvenc_handle_t)handle_enc;
}

amvenc_metadata_t amvenc_vers_generate_header(amvenc_handle_t handle,
                                                              unsigned char *pHeader,
                                                              unsigned int *pLength)
{
    vc_encoding_metadata_t encoding_metadata;
    amvenc_metadata_t result;

    memset(&encoding_metadata, 0, sizeof(encoding_metadata));
    memset(&result, 0, sizeof(result));

    if (encoder_ops.generate_header)
        encoding_metadata = encoder_ops.generate_header((vc_codec_handle_t)handle, pHeader, pLength);

    result.encoded_data_length_in_bytes = encoding_metadata.encoded_data_length_in_bytes;
    result.is_key_frame = encoding_metadata.is_key_frame;
    result.timestamp_us = encoding_metadata.timestamp_us;
    result.is_valid = encoding_metadata.is_valid;
    result.extra.frame_type = encoding_metadata.extra.frame_type;
    result.extra.average_qp_value = encoding_metadata.extra.average_qp_value;
    result.extra.intra_blocks = encoding_metadata.extra.intra_blocks;
    result.extra.merged_blocks = encoding_metadata.extra.merged_blocks;
    result.extra.skipped_blocks = encoding_metadata.extra.skipped_blocks;
    result.err_cod = encoding_metadata.err_cod;

    return result;
}

amvenc_metadata_t amvenc_vers_encode(amvenc_handle_t handle,
                                           amvenc_frame_info_t frame_info,
                                           amvenc_frame_type_t type,
                                           unsigned char* out,
                                           amvenc_buffer_info_t *in_buffer_info,
                                           amvenc_buffer_info_t *ret_buffer_info)
{
    vc_encoding_metadata_t encoding_metadata;
    vc_frame_type_t enc_frame_type;
    vc_buffer_info_t inbuf_info;
    vc_buffer_info_t ret_buf;

    amvenc_metadata_t result;

    memset(&result, 0, sizeof(result));

    if ((in_buffer_info == NULL) || (ret_buffer_info == NULL)) {
      VLOG(ERR, "invalid input buffer_info\n");
      result.is_valid = false;
      return result;
    }

    memset(&encoding_metadata, 0, sizeof(encoding_metadata));
    memset(&inbuf_info, 0, sizeof(inbuf_info));
    memset(&ret_buf, 0, sizeof(ret_buf));


    enc_frame_type = VC_FRAME_TYPE_AUTO;
    switch (type)
    {
        case AML_FRAME_TYPE_AUTO:
            enc_frame_type = VC_FRAME_TYPE_AUTO;
            break;
        case AML_FRAME_TYPE_I:
            enc_frame_type = VC_FRAME_TYPE_I;
            break;
        default:
            break;
    }
    inbuf_info.buf_type = in_buffer_info->buf_type;

    inbuf_info.buf_info.dma_info.shared_fd[0] = in_buffer_info->buf_info.dma_info.shared_fd[0];
    inbuf_info.buf_info.dma_info.shared_fd[1] = in_buffer_info->buf_info.dma_info.shared_fd[1];
    inbuf_info.buf_info.dma_info.shared_fd[2] = in_buffer_info->buf_info.dma_info.shared_fd[2];
    inbuf_info.buf_info.dma_info.num_planes = in_buffer_info->buf_info.dma_info.num_planes;

    inbuf_info.buf_info.in_ptr[0] = in_buffer_info->buf_info.in_ptr[0];
    inbuf_info.buf_info.in_ptr[1] = in_buffer_info->buf_info.in_ptr[1];
    inbuf_info.buf_info.in_ptr[2] = in_buffer_info->buf_info.in_ptr[2];
    inbuf_info.buf_info.canvas = in_buffer_info->buf_info.canvas;

    inbuf_info.buf_stride = in_buffer_info->buf_stride;

    inbuf_info.buf_fmt = VC_IMG_FMT_NONE;
    switch (in_buffer_info->buf_fmt)
    {
        case AML_IMG_FMT_NV12:
            inbuf_info.buf_fmt = VC_IMG_FMT_NV12;
            break;
        case AML_IMG_FMT_NV21:
            inbuf_info.buf_fmt = VC_IMG_FMT_NV21;
            break;
        case AML_IMG_FMT_YUV420P:
            inbuf_info.buf_fmt = VC_IMG_FMT_YUV420P;
            break;
        case AML_IMG_FMT_YV12:
            inbuf_info.buf_fmt = VC_IMG_FMT_YV12;
            break;
        case AML_IMG_FMT_RGB888:
            inbuf_info.buf_fmt = VC_IMG_FMT_RGB888;
            break;
        case AML_IMG_FMT_RGBA8888:
            inbuf_info.buf_fmt = VC_IMG_FMT_RGBA8888;
            break;
        default:
            break;
    }

    if (encoder_ops.encode)
        encoding_metadata = encoder_ops.encode((vc_codec_handle_t)handle, enc_frame_type, out, &inbuf_info, &ret_buf);

    ret_buffer_info->buf_type = ret_buf.buf_type;

    ret_buffer_info->buf_info.dma_info.shared_fd[0] = ret_buf.buf_info.dma_info.shared_fd[0];
    ret_buffer_info->buf_info.dma_info.shared_fd[1] = ret_buf.buf_info.dma_info.shared_fd[1];
    ret_buffer_info->buf_info.dma_info.shared_fd[2] = ret_buf.buf_info.dma_info.shared_fd[2];
    ret_buffer_info->buf_info.dma_info.num_planes = ret_buf.buf_info.dma_info.num_planes;

    ret_buffer_info->buf_info.in_ptr[0] = ret_buf.buf_info.in_ptr[0];
    ret_buffer_info->buf_info.in_ptr[1] = ret_buf.buf_info.in_ptr[1];
    ret_buffer_info->buf_info.in_ptr[2] = ret_buf.buf_info.in_ptr[2];
    ret_buffer_info->buf_info.canvas = ret_buf.buf_info.canvas;

    ret_buffer_info->buf_stride = ret_buf.buf_stride;

    ret_buffer_info->buf_fmt = AML_IMG_FMT_NONE;
    switch (ret_buf.buf_fmt)
    {
        case VC_IMG_FMT_NV12:
            ret_buffer_info->buf_fmt = AML_IMG_FMT_NV12;
            break;
        case VC_IMG_FMT_NV21:
            ret_buffer_info->buf_fmt = AML_IMG_FMT_NV21;
            break;
        case VC_IMG_FMT_YUV420P:
            ret_buffer_info->buf_fmt = AML_IMG_FMT_YUV420P;
            break;
        case VC_IMG_FMT_YV12:
            ret_buffer_info->buf_fmt = AML_IMG_FMT_YV12;
            break;
        case VC_IMG_FMT_RGB888:
            ret_buffer_info->buf_fmt = AML_IMG_FMT_RGB888;
            break;
        case VC_IMG_FMT_RGBA8888:
            ret_buffer_info->buf_fmt = AML_IMG_FMT_RGBA8888;
            break;
        default:
            break;
    }

    result.encoded_data_length_in_bytes = encoding_metadata.encoded_data_length_in_bytes;
    result.is_key_frame = encoding_metadata.is_key_frame;
    result.timestamp_us = encoding_metadata.timestamp_us;
    result.is_valid = encoding_metadata.is_valid;
    result.extra.frame_type = encoding_metadata.extra.frame_type;
    result.extra.average_qp_value = encoding_metadata.extra.average_qp_value;
    result.extra.intra_blocks = encoding_metadata.extra.intra_blocks;
    result.extra.merged_blocks = encoding_metadata.extra.merged_blocks;
    result.extra.skipped_blocks = encoding_metadata.extra.skipped_blocks;
    result.err_cod = encoding_metadata.err_cod;

    return result;
}

int amvenc_vers_change_bitrate(amvenc_handle_t handle,          int bitRate)
{
    if (encoder_ops.change_bitrate)
        return encoder_ops.change_bitrate((vc_codec_handle_t)handle, bitRate);
    return 0;
}

int amvenc_vers_destroy(amvenc_handle_t handle)
{
    if (encoder_ops.destroy)
        return encoder_ops.destroy((vc_codec_handle_t)handle);
    return 0;
}

int amvenc_vers_unload_Module(void)
{
    pthread_mutex_lock(&encoder_ops_mutex);
    encoder_ops.load_count --;
    if (encoder_ops.load_count == 0) {
        if (encoder_ops.libHandle) {
            VLOG(DEBUG, "Unloading encoder api lib\n");
            dlclose(encoder_ops.libHandle);
        }
    }
    pthread_mutex_unlock(&encoder_ops_mutex);
    return 0;
}

