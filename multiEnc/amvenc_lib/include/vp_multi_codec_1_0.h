
#ifndef _INCLUDED_COM_VIDEO_MULTI_CODEC
#define _INCLUDED_COM_VIDEO_MULTI_CODEC

#include <stdlib.h>
#include <stdbool.h>

#define vl_codec_handle_t long

typedef struct encoding_metadata_e {
  int encoded_data_length_in_bytes; /* size of the encoded buffer */
  bool is_key_frame; /* if true, the encoded frame is a keyframe */
  int timestamp_us;  /* timestamp in usec of the encode frame */
  bool is_valid;     /* if true, the encoding was successful */
} encoding_metadata_t;

typedef enum vl_codec_id_e {
  CODEC_ID_NONE,
  CODEC_ID_VP8,
  CODEC_ID_H261,
  CODEC_ID_H263,
  CODEC_ID_H264, /* must support */
  CODEC_ID_H265,
} vl_codec_id_t;

typedef enum vl_img_format_e {
  IMG_FMT_NONE,
  IMG_FMT_NV12, /* must support  */
  IMG_FMT_NV21,
  IMG_FMT_YUV420,
  IMG_FMT_YV12
} vl_img_format_t;

typedef enum vl_frame_type_e {
  FRAME_TYPE_NONE,
  FRAME_TYPE_AUTO, /* encoder self-adaptation(default) */
  FRAME_TYPE_IDR,
  FRAME_TYPE_I,
  FRAME_TYPE_P,
  FRAME_TYPE_B,
} vl_frame_type_t;

typedef enum vl_fmt_type_e {
  AML_FMT_ENC = 0,
  AML_FMT_RAW = 1,
} vl_fmt_type_t;
/* buffer type*/
typedef enum {
  VMALLOC_TYPE = 0,
  CANVAS_TYPE = 1,
  PHYSICAL_TYPE = 2,
  DMA_TYPE = 3,
} vl_buffer_type_t;

/* encoder info*/
typedef struct vl_encode_info {
  int width;
  int height;
  int frame_rate;
  int bit_rate;
  int gop;
  bool prepend_spspps_to_idr_frames;
  vl_img_format_t img_format;
  int qp_mode;
} vl_encode_info_t;

/* dma buffer info*/
/*for nv12/nv21, num_planes can be 1 or 2
  for yv12, num_planes can be 1 or 3
 */
typedef struct vl_dma_info {
  int shared_fd[3];
  unsigned int num_planes;//for nv12/nv21, num_planes can be 1 or 2
} vl_dma_info_t;

/*When the memory type is V4L2_MEMORY_DMABUF, dma_info.shared_fd is a
  file descriptor associated with a DMABUF buffer.
  When the memory type is V4L2_MEMORY_USERPTR, in_ptr is a userspace
  pointer to the memory allocated by an application.
*/

typedef union {
  vl_dma_info_t dma_info;
  unsigned long in_ptr[3];
} vl_buf_info_u;

/* input buffer info
 * buf_type = VMALLOC_TYPE correspond to  buf_info.in_ptr
   buf_type = DMA_TYPE correspond to  buf_info.dma_info
 */
typedef struct vl_buffer_info {
  vl_buffer_type_t buf_type;
  vl_buf_info_u buf_info;
} vl_buffer_info_t;

/* noise reduction type*/
typedef enum {
  NR_DISABLE = 0,
  NR_SPATIAL = 1,
  NR_TEMPORAL = 2,
  NR_BOTH = 3,
} nr_mode_type_t;

typedef struct vl_param_runtime {
  int* idr;
  int bitrate;
  int frame_rate;

  bool enable_vfr;
  int min_frame_rate;

  nr_mode_type_t nr_mode;
} vl_param_runtime_t;

typedef struct qp_param_s {
  int qp_min;
  int qp_max;
  int qp_I_base;
  int qp_P_base;
  int qp_B_base;
  int qp_I_min;
  int qp_I_max;
  int qp_P_min;
  int qp_P_max;
  int qp_B_min;
  int qp_B_max;
} qp_param_t;

#ifdef __cplusplus
extern "C" {
#endif
/**
 * Getting version information
 *
 *@return : version information
 */
const char* vl_get_version();

/**
 * init encoder
 *
 *@param : codec_id: codec type
 *@param : vl_encode_info_t: encode info
 *         width:      video width
 *         height:     video height
 *         frame_rate: framerate
 *         bit_rate:   bitrate
 *         gop GOP:    max I frame interval
 *         prepend_spspps_to_idr_frames: if true, adds spspps header
 *         to all idr frames (keyframes).
 *         buf_type:
 *         0: need memcpy from input buf to encoder internal dma buffer.
 *         3: input buf is dma buffer, encoder use input buf without memcopy.
 *img_format: image format
 *@return : if success return encoder handle,else return <= 0
 */
vl_codec_handle_t vl_multi_encoder_init(vl_codec_id_t codec_id,
                                        vl_encode_info_t encode_info,
                                        qp_param_t* qp_tbl);


/**
 * encode video
 *
 *@param : handle
 *@param : type: frame type
 *@param : in: data to be encoded
 *@param : in_size: data size
 *@param : out: data output,need header(0x00，0x00，0x00，0x01),and format
 *         must be I420(apk set param out，through jni,so modify "out" in the
 *         function,don't change address point)
 *@param : in_buffer_info
 *         buf_type:
 *              VMALLOC_TYPE: need memcpy from input buf to encoder internal dma buffer.
 *              DMA_TYPE: input buf is dma buffer, encoder use input buf without memcopy.
 *         buf_info.dma_info: input buf dma info.
 *              num_planes:For nv12/nv21, num_planes can be 1 or 2.
 *                         For YV12, num_planes can be 1 or 3.
 *              shared_fd: DMA buffer fd.
 *         buf_info.in_ptr: input buf ptr.
 *@param : ret_buffer_info
 *         buf_type:
 *              VMALLOC_TYPE: need memcpy from input buf to encoder internal dma buffer.
 *              DMA_TYPE: input buf is dma buffer, encoder use input buf without memcopy.
 *              due to references and reordering, the DMA buffer may not return immedialtly.
 *         buf_info.dma_info: returned buf dma info if any.
 *              num_planes:For nv12/nv21, num_planes can be 1 or 2.
 *                         For YV12, num_planes can be 1 or 3.
 *              shared_fd: DMA buffer fd.
 *         buf_info.in_ptr: returned input buf ptr.
 *@return ：if success return encoded data length,else return <= 0
 */
encoding_metadata_t vl_multi_encoder_encode(vl_codec_handle_t handle,
                                           vl_frame_type_t type,
                                           unsigned char* out,
                                           vl_buffer_info_t *in_buffer_info,
                                           vl_buffer_info_t *ret_buffer_info);

/**
 * destroy encoder
 *
 *@param ：handle: encoder handle
 *@return ：if success return 1,else return 0
 */
int vl_multi_encoder_destroy(vl_codec_handle_t handle);


#ifdef __cplusplus
}
#endif

#endif /* _INCLUDED_COM_VIDEO_MULTI_CODEC */
