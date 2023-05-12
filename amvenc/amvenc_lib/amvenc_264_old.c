#include <string.h>
#include <dlfcn.h>
#include <pthread.h>

#include "vpcodec_1_0.h"
#include "amvenc_common.h"
#include "amvenc.h"

//=====待区分Android和Linux====
#define LIB_ENCODER_API_NAME "libvpcodec.so"

typedef vl_codec_handle_t (*fn_init)(vl_codec_id_t codec_id,
                                                int width, int height,
                                                int frame_rate, int bit_rate,
                                                int gop, vl_img_format_t img_format,
                                                int i_qp_min, int i_qp_max, int p_qp_min, int p_qp_max);
//_need_modify_待实现
typedef int (*fn_generate_header)(vl_codec_handle_t codec_handle,
                                                         unsigned char *pHeader,
                                                         unsigned int *pLength);
typedef int (*fn_encode)(vl_codec_handle_t handle,
                                    vl_frame_type_t type, unsigned char *in,
                                    int in_size, unsigned char *out, int format,
                                    int buf_type, vl_dma_info_t *dma_info);
//_need_modify_待实现
typedef int (*fn_change_bitrate)(vl_codec_handle_t handle, int bitRate);
typedef int (*fn_destroy)(vl_codec_handle_t handle);

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

int amvenc_264_load_Module(void)
{
    VLOG(DEBUG, "amvenc_ports hcodec initModule!");
    pthread_mutex_lock(&encoder_ops_mutex);
    encoder_ops.load_count ++;
    if (encoder_ops.load_count == 1) {
        encoder_ops.libHandle = dlopen(LIB_ENCODER_API_NAME, RTLD_NOW | RTLD_NODELETE);
        if (encoder_ops.libHandle != NULL) {
            encoder_ops.init = (fn_init)dlsym(encoder_ops.libHandle, "vl_video_encoder_init");
            if (encoder_ops.init == NULL) {
                VLOG(ERR, "dlsym for vl_video_encoder_init failed");
                goto exit;
            }

            /*encoder_ops.generate_header = (fn_generate_header)dlsym(handle, "vl_multi_encoder_generate_header");
            if (encoder_ops.generate_header == NULL) {
                VLOG(ERR, "dlsym for vl_multi_encoder_generate_header failed");
                goto exit;
            }

            encoder_ops.change_bitrate = (fn_change_bitrate)dlsym(handle, "vl_video_encoder_change_bitrate");
            if (encoder_ops.change_bitrate == NULL) {
                VLOG(ERR, "dlsym for vl_video_encoder_change_bitrate failed");
                goto exit;
            }*/

            encoder_ops.encode = (fn_encode)dlsym(encoder_ops.libHandle, "vl_video_encoder_encode");
            if (encoder_ops.encode == NULL) {
                VLOG(ERR, "dlsym for vl_video_encoder_encode failed");
                goto exit;
            }
            encoder_ops.destroy = (fn_destroy)dlsym(encoder_ops.libHandle,"vl_video_encoder_destroy");
            if (encoder_ops.destroy == NULL) {
                VLOG(ERR, "dlsym for vl_video_encoder_destroy failed,err:%s",dlerror());
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

vl_codec_handle_t amvenc_264_init(amvenc_codec_id_t vcodec_id,
                                       amvenc_info_t vcodec_info,
                                        amvenc_qp_param_t* qp_param)
{
    vl_codec_id_t codec_id = CODEC_ID_H264;
    vl_img_format_t img_format = IMG_FMT_NV12;
    vl_codec_handle_t handle_enc = 0;

    if (qp_param == NULL) {
        VLOG(ERR, "invalid param, vqp_tbl is NULL!\n");
        return 0;
    }

    switch (vcodec_info.img_format)
    {
        case AML_IMG_FMT_NV12:
            img_format = IMG_FMT_NV12;
            break;
        case AML_IMG_FMT_NV21:
            img_format = IMG_FMT_NV21;
            break;
        case AML_IMG_FMT_YUV420P:
            img_format = IMG_FMT_YV12;
            break;
        case AML_IMG_FMT_RGB888:
            img_format = IMG_FMT_RGB888;
            break;
        default:
            break;
    }

    if (encoder_ops.init)
        handle_enc = encoder_ops.init(codec_id,
                                vcodec_info.width, vcodec_info.height,
                                vcodec_info.frame_rate, vcodec_info.bit_rate,
                                vcodec_info.gop, img_format,
                                qp_param->qp_I_min, qp_param->qp_I_max, qp_param->qp_P_min, qp_param->qp_P_max);

    return (vl_codec_handle_t)handle_enc;
}

amvenc_metadata_t amvenc_264_generate_header(vl_codec_handle_t handle,
                                                              unsigned char *pHeader,
                                                              unsigned int *pLength)
{
    amvenc_metadata_t result;
    memset(&result, 0, sizeof(result));

    return result;
}

amvenc_metadata_t amvenc_264_encode(vl_codec_handle_t handle,
                                           amvenc_frame_type_t type,
                                           unsigned char* out,
                                           amvenc_buffer_info_t *in_buffer_info,
                                           amvenc_buffer_info_t *ret_buffer_info)
{
    vl_frame_type_t enc_frame_type;
    unsigned char *input_buffer = NULL;
    int in_size = 0;
    int fmt = 0;
    int buf_type = 0;
    vl_dma_info_t dma_info;
    int datalen = 0;

    amvenc_metadata_t result;

    memset(&result, 0, sizeof(result));
    memset(&dma_info, 0, sizeof(dma_info));

    if ((in_buffer_info == NULL) || (ret_buffer_info == NULL)) {
      VLOG(ERR, "invalid input buffer_info\n");
      result.is_valid = false;
      return result;
    }

    enc_frame_type = FRAME_TYPE_AUTO;
    switch (type)
    {
        case AML_FRAME_TYPE_AUTO:
            enc_frame_type = FRAME_TYPE_AUTO;
            break;
        case AML_FRAME_TYPE_I:
            enc_frame_type = FRAME_TYPE_I;
            break;
        default:
            break;
    }

    buf_type = in_buffer_info->buf_type;

    dma_info.shared_fd[0] = in_buffer_info->buf_info.dma_info.shared_fd[0];
    dma_info.shared_fd[1] = in_buffer_info->buf_info.dma_info.shared_fd[1];
    dma_info.shared_fd[2] = in_buffer_info->buf_info.dma_info.shared_fd[2];
    dma_info.num_planes = in_buffer_info->buf_info.dma_info.num_planes;

    input_buffer = (unsigned char *)in_buffer_info->buf_info.in_ptr[0];

    switch (in_buffer_info->buf_fmt)
    {
        case AML_IMG_FMT_NV12:
            fmt = IMG_FMT_NV12;
            break;
        case AML_IMG_FMT_NV21:
            fmt = IMG_FMT_NV21;
            break;
        case AML_IMG_FMT_YUV420P:
            fmt = IMG_FMT_YV12;
            break;
        case AML_IMG_FMT_RGB888:
            fmt = IMG_FMT_RGB888;
            break;
        default:
            break;
    }

    if (encoder_ops.encode)
        datalen = encoder_ops.encode(handle, enc_frame_type, input_buffer, in_size, out, fmt, buf_type, &dma_info);

    if (datalen <= 0) {
        result.is_valid = false;
        return result;
    }
    ret_buffer_info->buf_type = buf_type;

    ret_buffer_info->buf_info.dma_info.shared_fd[0] = dma_info.shared_fd[0];
    ret_buffer_info->buf_info.dma_info.shared_fd[1] = dma_info.shared_fd[1];
    ret_buffer_info->buf_info.dma_info.shared_fd[2] = dma_info.shared_fd[2];
    ret_buffer_info->buf_info.dma_info.num_planes = dma_info.num_planes;

    ret_buffer_info->buf_info.in_ptr[0] = in_buffer_info->buf_info.in_ptr[0];
    ret_buffer_info->buf_info.in_ptr[1] = in_buffer_info->buf_info.in_ptr[1];
    ret_buffer_info->buf_info.in_ptr[2] = in_buffer_info->buf_info.in_ptr[2];
    ret_buffer_info->buf_info.canvas = in_buffer_info->buf_info.canvas;

    ret_buffer_info->buf_stride = in_buffer_info->buf_stride;

    ret_buffer_info->buf_fmt = in_buffer_info->buf_fmt;

    result.encoded_data_length_in_bytes = datalen;
    //result.is_key_frame = encoding_metadata.is_key_frame;
    //result.timestamp_us = encoding_metadata.timestamp_us;
    result.is_valid = true;
    //result.extra.frame_type = encoding_metadata.extra.frame_type;
    //result.extra.average_qp_value = encoding_metadata.extra.average_qp_value;
    //result.extra.intra_blocks = encoding_metadata.extra.intra_blocks;
    //result.extra.merged_blocks = encoding_metadata.extra.merged_blocks;
    //result.extra.skipped_blocks = encoding_metadata.extra.skipped_blocks;
    //result.err_cod = encoding_metadata.err_cod;

    return result;
}

int amvenc_264_change_bitrate(vl_codec_handle_t handle,          int bitRate)
{
    if (encoder_ops.change_bitrate)
        return encoder_ops.change_bitrate(handle, bitRate);
    return 0;
}

int amvenc_264_destroy(vl_codec_handle_t handle)
{
    if (encoder_ops.destroy)
        return encoder_ops.destroy(handle);
    return 0;
}

int amvenc_264_unload_Module(void)
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

