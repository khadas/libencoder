#ifndef _JPEGENC_API_HEADER_
#define _JPEGENC_API_HEADER_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#define jpegenc_handle_t long

typedef enum {
	JPEGENC_LOCAL_BUFF = 0,
	JPEGENC_CANVAS_BUFF,
	JPEGENC_PHYSICAL_BUFF,
	JPEGENC_DMA_BUFF,
	JPEGENC_MAX_BUFF_TYPE
}jpegenc_mem_type_e;

typedef enum {
	FMT_YUV422_SINGLE = 0,
	FMT_YUV444_SINGLE,
	FMT_NV21,
	FMT_NV12,
	FMT_YUV420,
	FMT_YUV444_PLANE,
	FMT_RGB888,
	FMT_RGB888_PLANE,
	FMT_RGB565,
	FMT_RGBA8888,
	MAX_FRAME_FMT
}jpegenc_frame_fmt_e;

typedef struct jpegenc_frame_info
{
	int width;
	int height;
	int w_stride;
	int h_stride;
	int quality;
	jpegenc_frame_fmt_e iformat;
	jpegenc_frame_fmt_e oformat;
	jpegenc_mem_type_e mem_type;
	unsigned long YCbCr[3];
	int plane_num;
} jpegenc_frame_info_t;

typedef enum {
	ENC_FAILED = -1,
	ENC_SUCCESS,
}jpegenc_result_e;

/**
 * init jpeg encoder
 *
 *@return : opaque handle for jpeg encoder if success, 0 if failed
 */
jpegenc_handle_t jpegenc_init();

/**
 * encode yuv frame with jpeg encoder
 *
 *@param : handle: opaque handle for jpeg encoder to return
 *@param : width: video width
 *@param : height: video height
 *@param : w_stride: stride for width
 *@param : h_stride: stride for height
 *@param : quality: jpeg quality scale value
 *@param : iformat: input yuv format
 *@param : oformat: output for jpeg
 *@param : mem_type: memory buffer type for input yuv
 *@param : dma_fd: if mem_type is dma, this is the shared dma fd
 *@param : in_buf: if mem_type is vmalloc, this is the memory buffer addr for yuv input
 *@param : out_buf: memory buffer addr for output jpeg file data
 *@return : the length of encoded jpge file data, 0 if encoding failed
 */
jpegenc_result_e jpegenc_encode(jpegenc_handle_t handle, jpegenc_frame_info_t frame_info, unsigned char *out, int *out_size);

/**
 * release jpeg encoder
 *
 *@param : handle: opaque handle for jpeg encoder to return
 *@return : 0 if succes, errcode otherwise
 */
int jpegenc_destroy(jpegenc_handle_t handle);


#ifdef __cplusplus
}
#endif


#endif
