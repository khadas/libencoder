#ifndef _INCLUDED_COM_VIDEOPHONE_CODEC
#define _INCLUDED_COM_VIDEOPHONE_CODEC

#ifdef __cplusplus
extern "C" {
#endif

#define vl_codec_handle_t long

    typedef enum vl_codec_id_e {
        CODEC_ID_NONE,
        CODEC_ID_VP8,
        CODEC_ID_H261,
        CODEC_ID_H263,
        CODEC_ID_H264, /* must support */
        CODEC_ID_H265,

    } vl_codec_id_t;

    typedef enum vl_img_format_e
    {
        IMG_FMT_NV12, /* must support  */
        IMG_FMT_NV21,
        IMG_FMT_YV12,
        IMG_FMT_RGB888,
        IMG_FMT_BGR888
    } vl_img_format_t;

    typedef enum vl_frame_type_e
    {
        FRAME_TYPE_NONE,
        FRAME_TYPE_AUTO, /* encoder self-adaptation(default) */
        FRAME_TYPE_IDR,
        FRAME_TYPE_I,
        FRAME_TYPE_P,

    } vl_frame_type_t;

    typedef struct vl_dma_info {
        int shared_fd[3];
        unsigned int num_planes;//for nv12/nv21, num_planes can be 1 or 2
    } vl_dma_info_t;

    /**
     * Getting version information
     *
     *@return : version information
     */
    const char *vl_get_version();

    /**
     * init encoder
     *
     *@param : codec_id: codec type
     *@param : width: video width
     *@param : height: video height
     *@param : frame_rate: framerate
     *@param : bit_rate: bitrate
     *@param : gop GOP: max I frame interval
     *@param : img_format: image format
     *@param : i_qp_min
     *@param : i_qp_max
     *@param : p_qp_min
     *@param : p_qp_max
     *@return : if success return encoder handle,else return <= 0
     */
    vl_codec_handle_t vl_video_encoder_init(vl_codec_id_t codec_id, int width, int height, int frame_rate, int bit_rate, int gop, vl_img_format_t img_format, int i_qp_min, int i_qp_max, int p_qp_min, int p_qp_max);
    /**
     * init encoder with const qp mode
     *
     *@param : codec_id: codec type
     *@param : width: video width
     *@param : height: video height
     *@param : frame_rate: framerate
     *@param : bit_rate: bitrate
     *@param : gop GOP: max I frame interval
     *@param : img_format: image format
     *@param : fix_qp: fix qp
     *@return : if success return encoder handle,else return <= 0
     */
    vl_codec_handle_t vl_video_encoder_init_fix_qp(vl_codec_id_t codec_id, int width, int height, int frame_rate, int bit_rate, int gop, vl_img_format_t img_format, int fix_qp);


    /**
     * encode video
     *
     *@param : handle
     *@param : type: frame type
     *@param : in: data to be encoded
     *@param : in_size: data size
     *@param : out: data output,H.264 need header(0x00，0x00，0x00，0x01),and format must be I420(apk set param out，through jni,so modify "out" in the function,don't change address point)
     *@return ：if success return encoded data length,else return <= 0
     */
    int vl_video_encoder_encode(vl_codec_handle_t handle, vl_frame_type_t type, unsigned char *in, int in_size, unsigned char *out, int format, int buf_type, vl_dma_info_t *dma_info);

    /**
     * destroy encoder
     *
     *@param ：handle: encoder handle
     *@return ：if success return 1,else return 0
     */
    int vl_video_encoder_destroy(vl_codec_handle_t handle);

    /**
     * init decoder
     *
     *@param : codec_id: decoder type
     *@return : if success return decoder handle,else return <= 0
     */
//    vl_codec_handle_t vl_video_decoder_init(vl_codec_id_t codec_id);

    /**
     * decode video
     *
     *@param : handle: decoder handle
     *@param : in: data to be decoded
     *@param : in_size: data size
     *@param : out: data output, intenal set
     *@return ：if success return decoded data length, else return <= 0
     */
//    int vl_video_decoder_decode(vl_codec_handle_t handle, char *in, int in_size, char **out);

    /**
     * destroy decoder
     *@param : handle: decoderhandle
     *@return ：if success return 1, else return 0
     */
//    int vl_video_decoder_destory(vl_codec_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif /* _INCLUDED_COM_VIDEOPHONE_CODEC */
