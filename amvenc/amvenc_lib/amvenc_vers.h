#ifndef __AMVENC_VERS_H__
#define __AMVENC_VERS_H__

#ifdef __cplusplus
extern "C" {
#endif

int amvenc_vers_load_Module(void);
amvenc_handle_t amvenc_vers_init(amvenc_codec_id_t codec_id,
                                    amvenc_info_t encode_info,
                                    amvenc_qp_param_t* qp_tbl);
amvenc_metadata_t amvenc_vers_generate_header(amvenc_handle_t handle,
                                                          unsigned char *pHeader,
                                                          unsigned int *pLength);
amvenc_metadata_t amvenc_vers_encode(amvenc_handle_t handle,
                                           amvenc_frame_info_t frame_info,
                                           amvenc_frame_type_t type,
                                           unsigned char* out,
                                           amvenc_buffer_info_t *in_buffer_info,
                                           amvenc_buffer_info_t *ret_buf);
int amvenc_vers_change_bitrate(amvenc_handle_t handle,          int bitRate);
int amvenc_vers_destroy(amvenc_handle_t handle);
int amvenc_vers_unload_Module(void);

#ifdef __cplusplus
}
#endif

#endif /* __AMVENC_VERS_H__ */

