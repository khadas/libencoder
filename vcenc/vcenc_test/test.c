#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "vp_vc_codec_1_0.h"
#include "test_dma_api.h"
#include <sys/time.h>
#ifdef __ANDROID__
#define LOG_NDEBUG 0
#define LOG_TAG "MultiDriver_bench"
#include <utils/Log.h>

#define MULTI_TRACE_E(fmt, ...)         ALOGE(fmt,##__VA_ARGS__);
#else
#define MULTI_TRACE_E(fmt, ...)         printf(fmt,##__VA_ARGS__);
#endif

static int encode_main(void *param);
static struct usr_ctx_s ctx;
/*-----------
  * main
  *-----------*/

#define WIDTH 3840
#define HEIGHT 3840    // consider rotated frame
#define FRAMERATE 60

#define ENCODER_STRING_LEN_MAX	256

#define ENCODER_STRING_LEN_MAX	256

typedef struct {
	int   nstream;
	struct timeval timeFrameStart;
	struct timeval timeFrameEnd;
	char  srcfile[ENCODER_STRING_LEN_MAX];    // yuv data url in your root fs
	char  outfile[ENCODER_STRING_LEN_MAX];    // stream url in your root fs
	int   width;           // width
	int   height;          // height
	int   gop;             // I frame refresh interval
	int   framerate;       // framerate
	int   bitrate;   	   // bit rate
	int   num;             // encode frame count
	int   fmt;             // encode input fmt 1 : nv12, 2 : nv21, 3 : yuv420p(yv12/yu12)
	int   buf_type;        // 0:vmalloc, 3:dma buffer
	int   num_planes;      // used for dma buffer case. 2 : nv12/nv21, 3 : yuv420p(yv12/yu12)
	int   codec_id;        // 4 : h.264, 5 : h.265
} encodecCfgParam;

// Helper function to calculate time diffs.
static unsigned int uTimeDiff(struct timeval end, struct timeval start)
{
	return (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
}

//return timeDiff in u64
static unsigned long long uTimeDiffLL(struct timeval end, struct timeval start)
{
	return (unsigned long long )(end.tv_sec - start.tv_sec) * 1000000 + (unsigned long long )(end.tv_usec - start.tv_usec);
}

int main(int argc, char **argv)
{
	encodecCfgParam cfg_encode;
	if (argc < 13) {
        printf("Amlogic Encoder API \n");
        printf(" usage: vc_enc_test"
               "[srcfile][outfile][width][height][gop][framerate][bitrate][num][fmt]["
               "buf_type][num_planes][codec_id]\n");
        printf("  options  \t:\n");
        printf("  srcfile  \t: yuv data url in your root fs\n");
        printf("  outfile  \t: stream url in your root fs\n");
        printf("  width    \t: width\n");
        printf("  height   \t: height\n");
        printf("  gop      \t: I frame refresh interval\n");
        printf("  framerate\t: framerate \n");
        printf("  bitrate  \t: bit rate \n");
        printf("  num      \t: encode frame count \n");
        printf("  fmt      \t: encode input fmt 2 : nv12/nv21, 3 : yuv420p(yv12/yu12)\n");
        printf("  buf_type \t: 0:vmalloc, 3:dma buffer\n");
        printf("  num_planes \t: used for dma buffer case. 2 : nv12/nv21, 3 : yuv420p(yv12/yu12)\n");
        printf("  codec_id \t: 4 : h.264, 5 : h.265\n");
        return -1;
        exit(0);
    } else {
		memset(&cfg_encode, 0, sizeof(encodecCfgParam));

		strncpy(cfg_encode.srcfile, argv[1], ENCODER_STRING_LEN_MAX-1);

		strncpy(cfg_encode.outfile, argv[2], ENCODER_STRING_LEN_MAX-1);

		cfg_encode.width = atoi(argv[3]);
		cfg_encode.height = atoi(argv[4]);

		cfg_encode.gop = atoi(argv[5]);
		cfg_encode.framerate = atoi(argv[6]);
		cfg_encode.bitrate = atoi(argv[7]);
		cfg_encode.num = atoi(argv[8]);
		cfg_encode.fmt = atoi(argv[9]);
		cfg_encode.buf_type = atoi(argv[10]);
		cfg_encode.num_planes = atoi(argv[11]);
		cfg_encode.codec_id = atoi(argv[12]);

		cfg_encode.nstream = 1;
	}
    return encode_main((void *)(&cfg_encode));
}

int encode_main(void *param)
{
    int encRet = -1;
    bool forceEnd = false;
	unsigned int outbuffer_len = 0;
	unsigned char *outbuffer = NULL;
	unsigned int framesize = 0;
	unsigned char *inputBuffer = NULL;
	enum vc_frame_type_e frame_type = VC_FRAME_TYPE_AUTO;
	vc_buffer_info_t inbuf_info;
	vc_buffer_info_t ret_buf;
	int width, height, gop, framerate, bitrate, num;
	encodecCfgParam *cfg_encode = NULL;
	vc_img_format_t fmt = VC_IMG_FMT_NONE;
	int buf_type = 0;
	int num_planes = 1;
	vc_codec_id_t codec_id = VC_CODEC_ID_H264;
	FILE *fp = NULL;
	FILE *outfp = NULL;
	char srcfile[ENCODER_STRING_LEN_MAX] = {0};
	char outfile[ENCODER_STRING_LEN_MAX] = {0};
	vc_encode_info_t encode_info;
	vc_codec_handle_t handle_enc = 0;
	vc_encoding_metadata_t encoding_metadata;
	unsigned int frameCntTotal = 0;
	unsigned int frameWriteTime = 0;
	unsigned long long totalTime = 0, ioTime = 0;
	struct timeval timeStart, timeEnd, readStart, readEnd, writeStart, writeEnd;

	if (NULL != param)
		cfg_encode = (encodecCfgParam *)param;
	else {
        return -1;
	}

	width = cfg_encode->width;
	if ((width < 1) || (width > WIDTH))
	{
		printf("invalid width \n");
        return -1;
	}

	height = cfg_encode->height;
	if ((height < 1) || (height > HEIGHT))
	{
		printf("invalid height \n");
        return -1;
	}

	gop = cfg_encode->gop;
	framerate = cfg_encode->framerate;
	bitrate = cfg_encode->bitrate;
	num = cfg_encode->num;
	fmt = cfg_encode->fmt;
	buf_type = cfg_encode->buf_type;
	num_planes = cfg_encode->num_planes;

	switch (cfg_encode->codec_id)
	{
		case 4:
			codec_id = VC_CODEC_ID_H264;
			break;
		case 5:
			codec_id = VC_CODEC_ID_H265;
			break;
		default:
			break;
	}

	if (codec_id != VC_CODEC_ID_H264 && codec_id != VC_CODEC_ID_H265) {
		printf("unsupported codec id %d \n", codec_id);
        return -1;
	}

	if ((framerate < 0) || (framerate > FRAMERATE))
	{
		printf("invalid framerate %d \n", framerate);
        return -1;
	}
	if (bitrate <= 0)
	{
		printf("invalid bitrate \n");
        return -1;
	}
	if (num < 0)
	{
		printf("invalid num \n");
        return -1;
	}
	if (buf_type == 3 && (num_planes < 1 || num_planes > 3))
	{
		printf("invalid num_planes \n");
        return -1;
	}
	printf("src_url is\t: %s ;\n", cfg_encode->srcfile);
	printf("out_url is\t: %s ;\n", cfg_encode->outfile);
	printf("width   is\t: %d ;\n", width);
	printf("height  is\t: %d ;\n", height);
	printf("gop     is\t: %d ;\n", gop);
	printf("frmrate is\t: %d ;\n", framerate);
	printf("bitrate is\t: %d ;\n", bitrate);
	printf("frm_num is\t: %d ;\n", num);
	printf("fmt     is\t: %d ;\n", fmt);
	printf("buf_type is\t: %d ;\n", buf_type);
	printf("num_planes is\t: %d ;\n", num_planes);
	printf("codec is\t: %d ;\n", codec_id);
	if (codec_id == VC_CODEC_ID_H265)
		printf("codec is H265\n");
	else
		printf("codec is H264\n");


	strncpy(srcfile, cfg_encode->srcfile, ENCODER_STRING_LEN_MAX);

	strncpy(outfile, cfg_encode->outfile, ENCODER_STRING_LEN_MAX);

	outbuffer_len = 8*1024 * 1024 * sizeof(char);
	outbuffer = (unsigned char *) malloc(outbuffer_len);
	if (outbuffer == NULL) {
		printf("Can not malloc outbuffer\n");
		goto exit;
	}


	framesize = width * height * 3 / 2;

	memset(&inbuf_info, 0, sizeof(vc_buffer_info_t));
	inbuf_info.buf_type = (vc_buffer_type_t) buf_type;
	inbuf_info.buf_stride = 0;
	inbuf_info.buf_fmt = fmt;

	memset(&encode_info, 0, sizeof(vc_encode_info_t));
	encode_info.width = width;
	encode_info.height = height;
	encode_info.bit_rate = bitrate;
	encode_info.frame_rate = framerate;
	encode_info.gop = gop;
	encode_info.img_format = fmt;

	int ret = 0;
	unsigned ysize;
	unsigned usize;
	unsigned vsize;
	unsigned uvsize;
	unsigned char *vaddr = NULL;
	unsigned char *input[3] = { NULL };

	if (inbuf_info.buf_stride) {
		if (fmt == VC_IMG_FMT_RGB888)
		{
			framesize = inbuf_info.buf_stride * height * 3;
		}
		else if (fmt == VC_IMG_FMT_RGBA8888)
		{
			framesize = inbuf_info.buf_stride * height * 4;
		}
		else
		{
			framesize = inbuf_info.buf_stride * height * 3 / 2;
		}
		ysize = inbuf_info.buf_stride * height;
		usize = inbuf_info.buf_stride * height / 4;
		vsize = inbuf_info.buf_stride * height / 4;
		uvsize = inbuf_info.buf_stride * height / 2;
	} else {
		if (fmt == VC_IMG_FMT_RGB888)
		{
			framesize = width * height * 3;
		}
		else if (fmt == VC_IMG_FMT_RGBA8888)
		{
			framesize = width * height * 4;
		}
		else
		{
			framesize = width * height * 3 / 2;
		}
		ysize = width * height;
		usize = width * height / 4;
		vsize = width * height / 4;
		uvsize = width * height / 2;
	}
	vc_dma_info_t *dma_info = &(inbuf_info.buf_info.dma_info);
	if (inbuf_info.buf_type == VC_DMA_TYPE)
	{
		dma_info = &(inbuf_info.buf_info.dma_info);
		dma_info->num_planes = num_planes;
		dma_info->shared_fd[0] = -1;	// dma buf fd
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
			if (fmt != VC_IMG_FMT_YUV420P)
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
			printf("hoan debug canvas >> alloc_dma_buffer u,ret=%d, vaddr=0x%p\n", ret,vaddr);

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
			if (fmt != VC_IMG_FMT_NV12 && fmt != VC_IMG_FMT_NV21)
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

		inputBuffer =  input[0];
		printf("in[0] %d, in[1] %d, in[2] %d\n", dma_info->shared_fd[0],
		       dma_info->shared_fd[1], dma_info->shared_fd[2]);
		printf("input[0] %p, input[1] %p,input[2] %p\n", input[0],
		       input[1], input[2]);
	} else
    {
		inputBuffer = (unsigned char *) malloc(framesize);
		if (inputBuffer == NULL) {
			printf("Can not allocat inputBuffer\n");
			goto exit;
		}
		inbuf_info.buf_info.in_ptr[0] = (unsigned long) (inputBuffer);
	}

	fp = fopen((const char*)srcfile, "rb");
	if (fp == NULL)
	{
		printf("open src file error!\n");
		goto exit;
	}

	outfp = fopen((const char*)outfile, "wb");
	if (outfp == NULL)
	{
		printf("open dest file error!\n");
		goto exit;
	}

	handle_enc = vc_encoder_init(codec_id, encode_info);
	if (handle_enc == 0) {
		printf("vc_multi_encoder_init() fails\n");
        goto exit;
	}

	gettimeofday(&timeStart, NULL);
	while (num > 0) {
        /* Read YUV to input buffer */
        gettimeofday(&readStart, NULL);
        if (fread(inputBuffer, 1, framesize, fp) != framesize)
        {
            printf("read input file error!\n");
            goto exit;
        }
        gettimeofday(&readEnd, NULL);
        ioTime += uTimeDiff(readEnd, readStart);
        gettimeofday(&cfg_encode->timeFrameStart, NULL);
		encoding_metadata =
		    vc_encoder_encode(handle_enc, frame_type, outbuffer, &inbuf_info, &ret_buf);
		gettimeofday(&writeStart, NULL);

		if (encoding_metadata.is_valid)
		{
            fwrite(outbuffer, 1, encoding_metadata.encoded_data_length_in_bytes, outfp);
		} else
		{
			printf("encode error %d!\n",
			       encoding_metadata.encoded_data_length_in_bytes);
		}
		gettimeofday(&writeEnd, NULL);
		frameWriteTime = uTimeDiff(writeEnd, writeStart);
		ioTime += frameWriteTime;
		gettimeofday(&cfg_encode->timeFrameEnd, NULL);
		num --;
		frameCntTotal ++;
	}
	encRet = 0;
	gettimeofday(&timeEnd, NULL);

	totalTime = uTimeDiffLL(timeEnd, timeStart);
	MULTI_TRACE_E("[Total time: %llu us], [IO time: %llu us]\n", totalTime, ioTime);
	if (frameCntTotal > 1) {
		MULTI_TRACE_E("[Encode Total time: %llu us], [avgTime: %llu us], [avgfps: %d]\n", totalTime - ioTime,
			   (totalTime - ioTime) / frameCntTotal, 1*1000*1000/((totalTime - ioTime) / frameCntTotal));
	}
exit:
	if (handle_enc)
		vc_encoder_destroy(handle_enc);
	if (fp)
		fclose(fp);
	if (outfp)
		fclose(outfp);
	if (outbuffer != NULL)
		free(outbuffer);
	if (inbuf_info.buf_type == VC_DMA_TYPE) {
		if (dma_info->shared_fd[0] >= 0)
			close(dma_info->shared_fd[0]);

		if (dma_info->shared_fd[1] >= 0)
			close(dma_info->shared_fd[1]);

		if (dma_info->shared_fd[2] >= 0)
			close(dma_info->shared_fd[2]);

		destroy_ctx(&ctx);
	} else {
	    if (inputBuffer != NULL)
	        free(inputBuffer);
	}
	return encRet;
}

